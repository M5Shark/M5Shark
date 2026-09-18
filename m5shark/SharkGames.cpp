// SHARK Games module. Copyright (c) 2026 M5SHARK. All rights reserved.
#include "SharkGames.h"
#include "configs.h"

#ifdef HAS_SCREEN

#include "SharkTheme.h"
#include "Display.h"
#include "MenuFunctions.h"
#include <SD.h>
#include <FS.h>

extern Display display_obj;
extern MenuFunctions menu_function_obj;

#define GRID 12          // snake cell size
#define W 240
#define H 320

namespace {
  TFT_eSPI& tft = display_obj.tft;
  #define th() (*shark_theme)

  bool tap(uint16_t* x, uint16_t* y) {
    return tft.getTouch(x, y, 350);
  }

  void waitRelease() {
    uint16_t x, y;
    while (tft.getTouch(&x, &y, 350)) delay(10);
  }

  void frame(const char* title) {
    tft.fillScreen(TFT_BLACK);
    tft.setFreeFont(NULL); tft.setTextWrap(false); tft.setTextSize(1);
    menu_function_obj.drawStatusBar();
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(th().accent, TFT_BLACK);
    tft.drawString(String("// ") + title, 6, 30, 2);
    tft.setTextDatum(TL_DATUM);
    tft.drawFastHLine(0, 40, W, th().edge);
    tft.drawFastHLine(0, 40, 60, th().accent);
  }

  void backKey() {
    tft.fillRect(6, 296, W - 12, 20, th().surface);
    tft.drawRect(6, 296, W - 12, 20, th().grey);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(th().grey, th().surface);
    tft.drawString("BACK", W / 2, 306, 1);
    tft.setTextDatum(TL_DATUM);
  }

  bool backHit(uint16_t ty) { return ty >= 294; }

  // ---- SNAKE ----
  struct Vec2 { int8_t x, y; };

  void drawSnakeCell(int cx, int cy, uint16_t col) {
    tft.fillRect(cx * GRID + 1, cy * GRID + 45, GRID - 2, GRID - 2, col);
  }

  int8_t dirFromTap(uint16_t tx, uint16_t ty) {
    // Screen thirds: top=up, bottom=down, left=left, right=right
    if (ty < 100) return 0;      // up
    if (ty > 220) return 1;      // down
    if (tx < 80) return 2;       // left
    if (tx > 160) return 3;      // right
    return -1;
  }

  // ---- TIC TAC TOE ----
  char board[9];
  const int CELL = 60;
  const int BX = 30, BY = 80;

  void drawX(int cell) {
    int cx = BX + (cell % 3) * CELL + CELL / 2;
    int cy = BY + (cell / 3) * CELL + CELL / 2;
    for (int i = -18; i <= 18; i++) {
      tft.drawPixel(cx + i, cy + i, th().accent);
      tft.drawPixel(cx - i, cy + i, th().accent);
    }
  }

  void drawO(int cell) {
    int cx = BX + (cell % 3) * CELL + CELL / 2;
    int cy = BY + (cell / 3) * CELL + CELL / 2;
    tft.drawCircle(cx, cy, 20, th().warn);
  }

  char checkWin() {
    int lines[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};
    for (int i = 0; i < 8; i++)
      if (board[lines[i][0]] != ' ' &&
          board[lines[i][0]] == board[lines[i][1]] &&
          board[lines[i][1]] == board[lines[i][2]])
        return board[lines[i][0]];
    return ' ';
  }

  int aiMove() {
    // Simple AI: try to win, block, then center, then corner, then any
    int lines[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};
    // Try to win
    for (int i = 0; i < 8; i++)
      for (int j = 0; j < 3; j++) {
        int a = lines[i][j], b = lines[i][(j+1)%3], c = lines[i][(j+2)%3];
        if (board[a] == 'O' && board[b] == 'O' && board[c] == ' ') return c;
      }
    // Block
    for (int i = 0; i < 8; i++)
      for (int j = 0; j < 3; j++) {
        int a = lines[i][j], b = lines[i][(j+1)%3], c = lines[i][(j+2)%3];
        if (board[a] == 'X' && board[b] == 'X' && board[c] == ' ') return c;
      }
    if (board[4] == ' ') return 4;
    int corners[4] = {0, 2, 6, 8};
    for (int i = 0; i < 4; i++)
      if (board[corners[i]] == ' ') return corners[i];
    for (int i = 0; i < 9; i++)
      if (board[i] == ' ') return i;
    return -1;
  }
}

// ===== SNAKE =====
void SharkGames::snake() {
  // Static to avoid 400+ bytes on the loopTask stack (the game also
  // creates lambdas that reference these, growing the frame further).
  static Vec2 snake[200];
  int len = 3;
  int dir = 3; // 0=up 1=down 2=left 3=right
  Vec2 food = {10, 12};
  uint32_t score = 0, best = 0;
  bool dead = false;

  auto reset = [&]() {
    len = 3; dir = 3; score = 0; dead = false;
    for (int i = 0; i < len; i++) { snake[i].x = 5 - i; snake[i].y = 8; }
    // Spawn food on a cell not occupied by the snake.
    bool ok = false;
    while (!ok) {
      food.x = random(0, W / GRID); food.y = random(0, (H - 100) / GRID);
      ok = true;
      for (int i = 0; i < len; i++)
        if (snake[i].x == food.x && snake[i].y == food.y) { ok = false; break; }
    }
  };

  reset();
  frame("SNAKE");

  uint32_t last = 0;
  while (true) {
    if (dead) {
      if (score > best) best = score;
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(th().warn, TFT_BLACK);
      tft.drawString("GAME OVER", W / 2, 150, 4);
      tft.setTextColor(th().content, TFT_BLACK);
      tft.drawString("Score: " + String(score) + "  Best: " + String(best), W / 2, 190, 2);
      tft.setTextColor(th().dim, TFT_BLACK);
      tft.drawString("tap to retry", W / 2, 220, 1);
      tft.setTextDatum(TL_DATUM);
      backKey();
      uint16_t tx, ty;
      while (!tap(&tx, &ty)) delay(20);
      waitRelease();
      if (backHit(ty)) return;
      reset();
      frame("SNAKE");
    }

    if (millis() - last >= 150) {
      last = millis();

      // Move
      Vec2 head = snake[0];
      if (dir == 0) head.y--;
      else if (dir == 1) head.y++;
      else if (dir == 2) head.x--;
      else head.x++;

      // Collision
      if (head.x < 0 || head.x >= W / GRID || head.y < 0 || head.y >= (H - 100) / GRID)
        dead = true;
      for (int i = 0; i < len && !dead; i++)
        if (snake[i].x == head.x && snake[i].y == head.y) dead = true;

      if (dead) continue;

      // Food
      bool ate = (head.x == food.x && head.y == food.y);
      if (ate) {
        score++;
        // Respawn food on a free cell.
        bool free = false;
        while (!free) {
          food.x = random(0, W / GRID);
          food.y = random(0, (H - 100) / GRID);
          free = true;
          for (int i = 0; i < len; i++)
            if (snake[i].x == food.x && snake[i].y == food.y) { free = false; break; }
        }
      }

      // Shift body
      if (!ate) drawSnakeCell(snake[len - 1].x, snake[len - 1].y, TFT_BLACK);
      for (int i = len - 1; i > 0; i--) snake[i] = snake[i - 1];
      snake[0] = head;
      if (ate && len < 200) len++;

      // Draw
      drawSnakeCell(food.x, food.y, th().warn);
      drawSnakeCell(snake[0].x, snake[0].y, th().accent);
      for (int i = 1; i < len; i++)
        drawSnakeCell(snake[i].x, snake[i].y, th().content);

      // HUD
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(th().dim, TFT_BLACK);
      tft.fillRect(0, 270, W, 20, TFT_BLACK);
      tft.drawString("Score: " + String(score), 6, 272, 2);
      tft.setTextDatum(MR_DATUM);
      tft.drawString("Len: " + String(len), W - 6, 272, 2);
      tft.setTextDatum(TL_DATUM);
    }

    // Touch input
    uint16_t tx, ty;
    if (tap(&tx, &ty)) {
      waitRelease();
      if (backHit(ty)) return;
      int8_t nd = dirFromTap(tx, ty);
      if (nd >= 0) {
        // Prevent 180-degree turn
        if (!(nd == 0 && dir == 1) && !(nd == 1 && dir == 0) &&
            !(nd == 2 && dir == 3) && !(nd == 3 && dir == 2))
          dir = nd;
      }
    }
    delay(5);
  }
}

// ===== REACTION =====
void SharkGames::reaction() {
  uint32_t best = 999999;
  uint32_t times[5];
  int round = 0;

  frame("REACTION");
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(th().dim, TFT_BLACK);
  tft.drawString("5 rounds - tap when green", W / 2, 100, 2);
  tft.drawString("tap to start", W / 2, 140, 1);
  tft.setTextDatum(TL_DATUM);
  backKey();
  uint16_t tx, ty;
  while (!tap(&tx, &ty)) delay(20);
  waitRelease();
  if (backHit(ty)) return;

  for (round = 0; round < 5; round++) {
    frame("REACTION");
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(th().dim, TFT_BLACK);
    tft.drawString("Round " + String(round + 1) + "/5", W / 2, 100, 2);
    tft.drawString("wait for green...", W / 2, 130, 1);
    tft.setTextDatum(TL_DATUM);
    backKey();

    // Random delay 1-4 seconds
    delay(random(1000, 4000));

    // Check if user tapped (BACK exits cleanly, anything else = too early)
    if (tft.getTouch(&tx, &ty, 350)) {
      waitRelease();
      if (backHit(ty)) return;
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(th().warn, TFT_BLACK);
      tft.drawString("TOO EARLY!", W / 2, 150, 3);
      tft.setTextDatum(TL_DATUM);
      delay(1500);
      round--;
      continue;
    }

    // Green!
    tft.fillRect(0, 90, W, 160, th().accent);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, th().accent);
    tft.drawString("TAP!", W / 2, 160, 4);
    tft.setTextDatum(TL_DATUM);

    uint32_t start = micros();
    while (!tap(&tx, &ty)) delay(1);
    uint32_t elapsed = (micros() - start) / 1000;
    waitRelease();
    if (backHit(ty)) return;

    times[round] = elapsed;
    if (elapsed < best) best = elapsed;

    tft.fillRect(0, 90, W, 160, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(elapsed < 250 ? th().accent : th().content, TFT_BLACK);
    tft.drawString(String(elapsed) + " ms", W / 2, 140, 4);
    tft.setTextDatum(TL_DATUM);
    delay(1200);
  }

  // Results
  frame("REACTION");
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(th().accent, TFT_BLACK);
  tft.drawString("RESULTS", W / 2, 80, 3);
  tft.setTextColor(th().content, TFT_BLACK);
  for (int i = 0; i < 5; i++)
    tft.drawString("R" + String(i + 1) + ": " + String(times[i]) + " ms", W / 2, 120 + i * 24, 2);
  tft.setTextColor(th().warn, TFT_BLACK);
  tft.drawString("Best: " + String(best) + " ms", W / 2, 250, 3);
  tft.setTextDatum(TL_DATUM);
  backKey();
  while (!tap(&tx, &ty)) delay(20);
  waitRelease();
}

// ===== TIC TAC TOE =====
void SharkGames::tictactoe() {
  while (true) {
    memset(board, ' ', 9);
    frame("TIC TAC TOE");
    // Draw grid
    for (int i = 1; i < 3; i++) {
      tft.drawFastVLine(BX + i * CELL, BY, CELL * 3, th().accent);
      tft.drawFastHLine(BX, BY + i * CELL, CELL * 3, th().accent);
    }
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(th().dim, TFT_BLACK);
    tft.drawString("You are X - tap a cell", 6, 50, 1);
    backKey();

    char turn = 'X';
    char winner = ' ';
    int moves = 0;

    while (winner == ' ' && moves < 9) {
      if (turn == 'X') {
        uint16_t tx, ty;
        if (!tap(&tx, &ty)) { delay(10); continue; }
        waitRelease();
        if (backHit(ty)) return;
        // Map to cell
        if (tx >= BX && tx < BX + CELL * 3 && ty >= BY && ty < BY + CELL * 3) {
          int cell = ((ty - BY) / CELL) * 3 + ((tx - BX) / CELL);
          if (board[cell] == ' ') {
            board[cell] = 'X';
            drawX(cell);
            turn = 'O';
            moves++;
            winner = checkWin();
          }
        }
      } else {
        delay(500);
        int cell = aiMove();
        if (cell >= 0 && board[cell] == ' ') {
          board[cell] = 'O';
          drawO(cell);
          turn = 'X';
          moves++;
          winner = checkWin();
        }
      }
    }

    // Result
    tft.setTextDatum(MC_DATUM);
    if (winner == 'X') {
      tft.setTextColor(th().accent, TFT_BLACK);
      tft.drawString("YOU WIN!", W / 2, 240, 3);
    } else if (winner == 'O') {
      tft.setTextColor(th().warn, TFT_BLACK);
      tft.drawString("AI WINS!", W / 2, 240, 3);
    } else {
      tft.setTextColor(th().grey, TFT_BLACK);
      tft.drawString("DRAW", W / 2, 240, 3);
    }
    tft.setTextColor(th().dim, TFT_BLACK);
    tft.drawString("tap to play again", W / 2, 270, 1);
    tft.setTextDatum(TL_DATUM);
    backKey();
    uint16_t tx, ty;
    while (!tap(&tx, &ty)) delay(20);
    waitRelease();
    if (backHit(ty)) return;
  }
}

// ===== CUSTOM GAMES =====
uint8_t SharkGames::customCount() {
  #ifdef HAS_SD
    if (!SD.exists("/GAMES")) return 0;
    File dir = SD.open("/GAMES");
    if (!dir || !dir.isDirectory()) return 0;
    uint8_t n = 0;
    while (true) {
      File f = dir.openNextFile();
      if (!f) break;
      if (!f.isDirectory() && String(f.name()).endsWith(".gme")) n++;
      f.close();
    }
    return n;
  #else
    return 0;
  #endif
}

String SharkGames::customName(uint8_t i) {
  #ifdef HAS_SD
    if (!SD.exists("/GAMES")) return "";
    File dir = SD.open("/GAMES");
    if (!dir) return "";
    uint8_t n = 0;
    while (true) {
      File f = dir.openNextFile();
      if (!f) break;
      String name = f.name();
      if (!f.isDirectory() && name.endsWith(".gme")) {
        if (n == i) {
          // Read the TITLE line
          String title = name.substring(0, name.length() - 4);
          f.seek(0);
          while (f.available()) {
            String line = f.readStringUntil('\n');
            line.trim();
            if (line.startsWith("TITLE:")) {
              title = line.substring(6);
              break;
            }
          }
          f.close();
          return title;
        }
        n++;
      }
      f.close();
    }
    return "";
  #else
    return "";
  #endif
}

void SharkGames::playCustom(const String& path) {
  #ifdef HAS_SD
    File f = SD.open(path);
    if (!f) return;

    String title = "GAME";
    String type = "QUIZ";
    struct Q { String q; String opts[4]; int ans; };
    Q questions[20];
    int nq = 0;

    // Parse
    while (f.available() && nq < 20) {
      String line = f.readStringUntil('\n');
      line.trim();
      if (line.startsWith("TITLE:")) {
        title = line.substring(6);
      } else if (line.startsWith("TYPE:")) {
        type = line.substring(5);
      } else if (line.startsWith("Q:")) {
        String body = line.substring(2);
        int start = 0;
        int part = 0;
        questions[nq].ans = 0;
        for (int i = 0; i <= body.length() && part < 6; i++) {
          if (i == body.length() || body[i] == '|') {
            String seg = body.substring(start, i);
            seg.trim();
            if (part == 0) questions[nq].q = seg;
            else if (part <= 4) questions[nq].opts[part - 1] = seg;
            else questions[nq].ans = seg.toInt();
            start = i + 1;
            part++;
          }
        }
        if (questions[nq].ans < 0 || questions[nq].ans > 3) questions[nq].ans = 0;
        nq++;
      } else if (line.startsWith("END")) {
        break;
      }
    }
    f.close();

    if (nq == 0) return;

    // Play quiz
    int score = 0;
    for (int q = 0; q < nq; q++) {
      frame(title.c_str());
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(th().dim, TFT_BLACK);
      tft.drawString("Q" + String(q + 1) + "/" + String(nq) + "  Score: " + String(score), 6, 48, 1);

      // Question
      tft.setTextColor(th().content, TFT_BLACK);
      String qtext = questions[q].q;
      while (qtext.length() > 3 && tft.textWidth(qtext, 2) > W - 20) qtext.remove(qtext.length() - 1);
      tft.drawString(qtext, 10, 60, 2);

      // Options (up to 4)
      for (int opt = 0; opt < 4; opt++) {
        if (questions[q].opts[opt].length() == 0) continue;
        int y = 100 + opt * 45;
        tft.fillRect(10, y, W - 20, 40, th().panel);
        tft.drawRect(10, y, W - 20, 40, th().edge);
        tft.setTextColor(th().content, th().panel);
        String otext = String("ABCD"[opt]) + ". " + questions[q].opts[opt];
        while (otext.length() > 3 && tft.textWidth(otext, 2) > W - 30) otext.remove(otext.length() - 1);
        tft.drawString(otext, 18, y + 10, 2);
      }
      backKey();

      // Wait for answer
      int chosen = -1;
      while (chosen < 0) {
        uint16_t tx, ty;
        if (!tap(&tx, &ty)) { delay(15); continue; }
        waitRelease();
        if (backHit(ty)) return;
        for (int opt = 0; opt < 4; opt++) {
          int y = 100 + opt * 45;
          if (ty >= y && ty < y + 40 && questions[q].opts[opt].length() > 0) {
            chosen = opt;
            break;
          }
        }
      }

      // Feedback
      bool correct = (chosen == questions[q].ans);
      if (correct) score++;
      tft.fillRect(10, 100, W - 20, 180, TFT_BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(correct ? th().accent : th().warn, TFT_BLACK);
      tft.drawString(correct ? "CORRECT!" : "WRONG", W / 2, 150, 4);
      if (!correct) {
        tft.setTextColor(th().grey, TFT_BLACK);
        tft.drawString("Answer: " + questions[q].opts[questions[q].ans], W / 2, 190, 2);
      }
      tft.setTextDatum(TL_DATUM);
      delay(1500);
    }

    // Final score
    frame(title.c_str());
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(th().accent, TFT_BLACK);
    tft.drawString("QUIZ COMPLETE", W / 2, 100, 3);
    tft.setTextColor(th().content, TFT_BLACK);
    tft.drawString("Score: " + String(score) + "/" + String(nq), W / 2, 140, 4);
    tft.setTextColor(th().dim, TFT_BLACK);
    tft.drawString("tap to finish", W / 2, 190, 1);
    tft.setTextDatum(TL_DATUM);
    backKey();
    uint16_t tx, ty;
    while (!tap(&tx, &ty)) delay(20);
    waitRelease();
  #endif
}

// ===== MENU =====
void SharkGames::gamesMenu() {
  struct Entry { const char* label; void (*run)(); };
  const Entry builtins[] = {
    {"SNAKE",        SharkGames::snake},
    {"REACTION",     SharkGames::reaction},
    {"TIC TAC TOE",  SharkGames::tictactoe},
  };
  const uint8_t nb = 3;
  uint8_t nc = customCount();

  while (true) {
    frame("GAMES");

    // Built-in games
    uint8_t y = 48;
    for (uint8_t i = 0; i < nb; i++) {
      tft.fillRect(6, y, W - 12, 36, th().panel);
      tft.drawRect(6, y, W - 12, 36, th().edge);
      tft.setTextDatum(ML_DATUM);
      tft.setTextColor(th().content, th().panel);
      tft.drawString(builtins[i].label, 14, y + 18, 2);
      tft.setTextDatum(TL_DATUM);
      y += 40;
    }

    // Custom games from SD
    if (nc > 0) {
      tft.setTextColor(th().dim, TFT_BLACK);
      tft.drawString("CUSTOM (SD /GAMES):", 6, y + 4, 1);
      y += 18;
      for (uint8_t i = 0; i < nc && i < 4; i++) {
        String name = customName(i);
        tft.fillRect(6, y, W - 12, 30, th().surface);
        tft.drawRect(6, y, W - 12, 30, th().accent);
        tft.setTextDatum(ML_DATUM);
        tft.setTextColor(th().accent, th().surface);
        tft.drawString(name, 14, y + 15, 2);
        tft.setTextDatum(TL_DATUM);
        y += 34;
      }
    } else {
      tft.setTextColor(th().dim, TFT_BLACK);
      tft.drawString("No custom games on SD", 6, y + 4, 1);
      tft.drawString("AI can create them via MCP", 6, y + 18, 1);
    }

    backKey();

    uint16_t tx, ty;
    if (!tap(&tx, &ty)) { delay(15); continue; }
    waitRelease();
    if (backHit(ty)) return;

    // Map tap to game
    uint8_t by = 48;
    for (uint8_t i = 0; i < nb; i++) {
      if (ty >= by && ty < by + 36) {
        builtins[i].run();
        break;
      }
      by += 40;
    }
    // Custom games start after "CUSTOM" label
    if (nc > 0) {
      uint8_t cy = 48 + nb * 40 + 18;
      for (uint8_t i = 0; i < nc && i < 4; i++) {
        if (ty >= cy && ty < cy + 30) {
          // Find the path for index i
          #ifdef HAS_SD
            File dir = SD.open("/GAMES");
            if (dir) {
              uint8_t n = 0;
              while (true) {
                File f = dir.openNextFile();
                if (!f) break;
                String name = f.name();
                if (!f.isDirectory() && name.endsWith(".gme")) {
                  if (n == i) {
                    playCustom("/GAMES/" + name);
                    f.close();
                    break;
                  }
                  n++;
                }
                f.close();
              }
            }
          #endif
          break;
        }
        cy += 34;
      }
    }
  }
}

#endif

SharkGames shark_games_obj;
