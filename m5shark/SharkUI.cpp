#include "SharkUI.h"

#if defined(HAS_SCREEN) && defined(MARAUDER_V8)

#include "SharkLogo.h"
#include "SharkDivSplash.h"
#include "SharkSpiderSplash.h"
#include "SharkDedsecLogo.h"
#include "Display.h"
#ifdef HAS_SD
  #include <SD.h>
  #include "SDInterface.h"
  extern SDInterface sd_obj;
#endif
extern Display display_obj;

namespace {
  // The splash is a character-cell image, not a bitmap: 34 x 28 cells of the
  // 6 x 8 px GLCD font fill 204 x 224 px of the 240 x 320 panel. Building the
  // art out of CP437 dither blocks and stray ASCII is what produces the
  // leaked-terminal look, and every mark costs about 1 KB of flash.
  const uint8_t ART_COLS = 34;
  const uint8_t ART_ROWS = 28;
  const int16_t ART_X = 18;
  const int16_t ART_Y = 38;
  const int16_t CELL_W = 6;
  const int16_t CELL_H = 8;
  const uint16_t WD_KEEP_COLOR = 0;

  const char* const skull_mask[ART_ROWS] = {
    "           ############           ",
    "        ##################        ",
    "      ######################      ",
    "    ##########################    ",
    "   ############################   ",
    "  ##############################  ",
    "  ##############################  ",
    " ################################ ",
    " ####        ########        #### ",
    " ###          ######          ### ",
    " ###          ######          ### ",
    " ###          ######          ### ",
    "  ##          ######          ##  ",
    "  ###        ########        ###  ",
    "   ###      ##########      ###   ",
    "    ############  ############    ",
    "     ##########    ##########     ",
    "     #########      #########     ",
    "      ######################      ",
    "      ### ## ## ## ## ## ###      ",
    "      ### ## ## ## ## ## ###      ",
    "       ## ## ## ## ## ## ##       ",
    "       ##                ##       ",
    "       ## ## ## ## ## ## ##       ",
    "        # ## ## ## ## ## #        ",
    "        # ## ## ## ## ## #        ",
    "         ################         ",
    "           ############           "
  };

  const char* const spider_mask[ART_ROWS] = {
    "                                  ",
    "                                  ",
    "    ###                    ###    ",
    "     ###                  ###     ",
    "      ###                ###      ",
    "       ###     ####     ###       ",
    "        ###   ######   ###        ",
    "  ###    ###  ######  ###    ###  ",
    "   ###    ### ###### ###    ###   ",
    "    ###   #### #### ####   ###    ",
    "     ###   ### #### ###   ###     ",
    "      ####  ##########  ####      ",
    "       ##### ######## #####       ",
    "          ##############          ",
    "            ##########            ",
    "            ##########            ",
    "            ##########            ",
    "            ##########            ",
    "          ##############          ",
    "       ####################       ",
    "      ####  ##########  ####      ",
    "     ###   ############   ###     ",
    "    ###   ### ###### ###   ###    ",
    "   ###   ###   ####   ###   ###   ",
    "  ###   ###            ###   ###  ",
    "      ####              ####      ",
    "     ####                ####     ",
    "    ###                    ###    "
  };

  const char* const cyber_mask[ART_ROWS] = {
    "                                  ",
    "                                  ",
    "            ##########            ",
    "         ################         ",
    "        ##################        ",
    "       ####################       ",
    "      ######################      ",
    "      ######################      ",
    "     ########################     ",
    "     ##                    ##     ",
    "     ##                    ##     ",
    "     ##                    ##     ",
    "     ##                    ##     ",
    "      #                    #      ",
    "      ######################      ",
    "      #########    #########      ",
    "      #########    #########      ",
    "      #########    #########      ",
    "      #########    #########      ",
    "      ######################      ",
    "      ######################      ",
    "         ################         ",
    "          ## ## ## ## ##          ",
    "          ## ## ## ## ##          ",
    "          ## ## ## ## ##          ",
    "          ## ## ## ## ##          ",
    "                                  ",
    "                                  "
  };

  const char* const demon_mask[ART_ROWS] = {
    "                                  ",
    "                                  ",
    "  #                            #  ",
    "   #                          #   ",
    "   ##                        ##   ",
    "    #        ########        #    ",
    "    ##     ############     ##    ",
    "     ##   ##############   ##     ",
    "      #  ################  #      ",
    "      ##      ######      ##      ",
    "       ###    ######    ###       ",
    "        ###   ######   ###        ",
    "        ####  ######  ####        ",
    "        ##### ###### #####        ",
    "        ##################        ",
    "         ################         ",
    "         #####      #####         ",
    "         #####      #####         ",
    "         #####      #####         ",
    "         ################         ",
    "         ## ## #### ## ##         ",
    "         ## ## #### ## ##         ",
    "                                  ",
    "                                  ",
    "                                  ",
    "                                  ",
    "                                  ",
    "                                  "
  };

  const char* const alien_mask[ART_ROWS] = {
    "                                  ",
    "                                  ",
    "             ########             ",
    "           ############           ",
    "          ##############          ",
    "         ################         ",
    "         ################         ",
    "        ##################        ",
    "        ##################        ",
    "       ####################       ",
    "       ####################       ",
    "       ####################       ",
    "       ####################       ",
    "       ###  ##########  ###       ",
    "       ##    ########    ##       ",
    "       #      ######      #       ",
    "        ##################        ",
    "        ##################        ",
    "         ################         ",
    "       ####################       ",
    "        ##################        ",
    "          ##############          ",
    "           ############           ",
    "             ########             ",
    "              ######              ",
    "               ####               ",
    "                                  ",
    "                                  "
  };

  const char* const bio_mask[ART_ROWS] = {
    "                                  ",
    "                                  ",
    "                                  ",
    "                                  ",
    "                                  ",
    "                                  ",
    "        ####          ####        ",
    "       ######        ######       ",
    "      ########      ########      ",
    "      ##    ##      ##    ##      ",
    "      ##    ###    ###    ##      ",
    "      ##    ##      ##    ##      ",
    "      ######## #### ########      ",
    "       ###### ##  ## ######       ",
    "        ####  #    #  ####        ",
    "              ##  ##              ",
    "               ####               ",
    "                                  ",
    "               ####               ",
    "              ######              ",
    "             ########             ",
    "             ##    ##             ",
    "            ###    ###            ",
    "             ##    ##             ",
    "             ########             ",
    "              ######              ",
    "               ####               ",
    "                                  "
  };

  const char* const reaper_mask[ART_ROWS] = {
    "                                  ",
    "                                  ",
    "          ##          ##          ",
    "         ##  ########  ##         ",
    "        ##  ##      ##  ##        ",
    "        #  ##        ##  #        ",
    "       ##  #          #  ##       ",
    "      ##  #   ######   #  ##      ",
    "     ##   #   ######   #   ##     ",
    "     ##   #    ####    #   ##     ",
    "    ##    #    ####    #    ##    ",
    "   ##     #    ####    #     ##   ",
    "   ##      #  ######  #      ##   ",
    "  ##       ## ###### ##       ##  ",
    " ##         ##      ##         ## ",
    " #           ########           # ",
    "##                              ##",
    "#                                #",
    "                                  ",
    "                                  ",
    "                                  ",
    "                                  ",
    "                                  ",
    "                                  ",
    "                                  ",
    "                                  ",
    "                                  ",
    "                                  "
  };

  // CP437 shading blocks read as solid from arm's length; the stray glyphs are
  // only used on the silhouette edge so the outline stays ragged.
  const uint8_t dense_glyphs[8] = {0xB1, 0xB2, 0xDB, 0xB2, 0xB1, 0xDB, 0xB2, 0xB1};
  const char edge_glyphs[16] = {'0', 'M', 'g', 'p', 'N', '#', '%', '&',
                                '$', '4', 'X', 'W', '8', 'q', 'R', 'j'};
  const char rain_glyphs[16] = {'0', '1', 'A', 'Z', '7', 'X', '#', '$',
                                'K', '3', 'W', '9', 'F', '5', 'T', '2'};

  const char* const* activeMask() {
    switch (shark_theme->mark) {
      case SHARK_MARK_SPIDER: return spider_mask;
      case SHARK_MARK_CYBER:  return cyber_mask;
      case SHARK_MARK_DEMON:  return demon_mask;
      case SHARK_MARK_ALIEN:  return alien_mask;
      case SHARK_MARK_BIO:    return bio_mask;
      case SHARK_MARK_REAPER: return reaper_mask;
      default:                return skull_mask;
    }
  }

  bool cellFilled(uint8_t col, uint8_t row) {
    if (row >= ART_ROWS || col >= ART_COLS)
      return false;
    return activeMask()[row][col] != ' ';
  }

  bool cellOnEdge(uint8_t col, uint8_t row) {
    return !cellFilled(col - 1, row) || !cellFilled(col + 1, row) ||
           !cellFilled(col, row - 1) || !cellFilled(col, row + 1);
  }

  void drawCell(TFT_eSPI& tft,
                uint8_t col,
                uint8_t row,
                uint16_t salt,
                int16_t dx,
                uint16_t force_color) {
    if (!cellFilled(col, row))
      return;

    const uint16_t noise = wdHash(col, row, salt);
    const int16_t x = ART_X + col * CELL_W + dx;
    const int16_t y = ART_Y + row * CELL_H;
    uint8_t glyph;
    uint16_t color;

    if (cellOnEdge(col, row)) {
      glyph = (uint8_t)edge_glyphs[noise & 0x0F];
      color = ((noise >> 8) & 0x03) ? shark_theme->mark_b : shark_theme->grey;
    }
    else {
      glyph = dense_glyphs[noise & 0x07];
      // A rare accent cell keeps the mark from reading as a flat stencil.
      color = ((noise >> 9) % 23) ? shark_theme->mark_a : shark_theme->mark_c;
    }

    if (force_color != WD_KEEP_COLOR)
      color = force_color;

    tft.drawChar(x, y, glyph, color, TFT_BLACK, 1);
  }

  void drawRows(TFT_eSPI& tft,
                int16_t first_row,
                int16_t last_row,
                uint16_t salt,
                int16_t dx,
                uint16_t force_color) {
    if (first_row < 0)
      first_row = 0;
    if (last_row >= ART_ROWS)
      last_row = ART_ROWS - 1;

    for (int16_t row = first_row; row <= last_row; row++)
      for (uint8_t col = 0; col < ART_COLS; col++)
        drawCell(tft, col, (uint8_t)row, salt, dx, force_color);
  }

  void clearRows(TFT_eSPI& tft, int16_t first_row, int16_t rows) {
    tft.fillRect(ART_X - 6,
                 ART_Y + first_row * CELL_H,
                 ART_COLS * CELL_W + 12,
                 rows * CELL_H,
                 TFT_BLACK);
  }

  // Offset ghost passes in the danger and warn colors. Each ghost spills two
  // pixels outside its own cell, so the main pass drawn on top leaves a colored
  // fringe instead of erasing it: the 2077 chromatic aberration.
  void drawChromaGhost(TFT_eSPI& tft, int16_t first_row, int16_t last_row) {
    drawRows(tft, first_row, last_row, 0, -2, shark_theme->danger);
    drawRows(tft, first_row, last_row, 0, 2, shark_theme->warn);
  }

  // Reprints a scatter of already visible cells so the whole mark keeps
  // flickering while the next lines are still decoding.
  void churnCells(TFT_eSPI& tft, uint8_t revealed_rows, uint16_t salt) {
    for (uint8_t i = 0; i < 26; i++) {
      const uint16_t noise = wdHash(i, salt, 0x5A5A);
      const uint8_t col = noise % ART_COLS;
      const uint8_t row = (noise >> 6) % (revealed_rows ? revealed_rows : 1);
      drawCell(tft, col, row, (uint16_t)(salt + i), 0, WD_KEEP_COLOR);
    }
  }

  void drawCarrierNoise(TFT_eSPI& tft, uint16_t salt) {
    for (uint8_t i = 0; i < 70; i++) {
      const uint16_t noise = wdHash(i, salt, 0xA5A5);
      const uint8_t col = noise % ART_COLS;
      const uint8_t row = (noise >> 6) % ART_ROWS;
      tft.drawChar(ART_X + col * CELL_W,
                   ART_Y + row * CELL_H,
                   (uint8_t)edge_glyphs[(noise >> 11) & 0x0F],
                   ((noise >> 4) & 0x07) ? WD_DIM : WD_CYAN_DIM,
                   TFT_BLACK,
                   1);
    }
  }

  void drawHeader(TFT_eSPI& tft, int16_t width, const String& version) {
    tft.fillRect(0, 0, width, 22, TFT_BLACK);
    tft.fillRect(0, 0, 66, 20, WD_CYAN);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, WD_CYAN);
    tft.drawString(SHARK_UI_NAME, 33, 10, 2);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    // Header carries the active theme's name, so each theme's boot is labelled.
    tft.drawString(String("// ") + shark_theme->name, 72, 10, 1);
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(WD_AMBER, TFT_BLACK);
    tft.drawString(String(SHARK_UI_TAG) + " " + version, width - 6, 10, 1);
    tft.setTextDatum(TL_DATUM);
    tft.drawFastHLine(0, 21, width, WD_EDGE);
    tft.drawFastHLine(0, 21, 66, WD_CYAN);
  }

  void drawStatus(TFT_eSPI& tft, const char* message, uint16_t color) {
    tft.fillRect(ART_X, 268, ART_COLS * CELL_W, 9, TFT_BLACK);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString(">", ART_X, 272, 1);
    tft.setTextColor(color, TFT_BLACK);
    tft.drawString(message, ART_X + 12, 272, 1);
    tft.setTextDatum(TL_DATUM);
  }

  // One bar cell per art column, so the readout lines up with the artwork.
  void drawProgress(TFT_eSPI& tft, uint8_t complete) {
    for (uint8_t i = 0; i < ART_COLS; i++) {
      const bool on = i < complete;
      tft.drawChar(ART_X + i * CELL_W,
                   286,
                   on ? 0xDB : 0xB0,
                   on ? WD_CYAN : WD_EDGE,
                   TFT_BLACK,
                   1);
    }
  }

  void drawFooter(TFT_eSPI& tft, int16_t width) {
    wdRule(tft, ART_X, 300, ART_COLS * CELL_W, WD_EDGE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WD_DIM, TFT_BLACK);
    tft.drawString(String(HARDWARE_NAME) + "  //  " + shark_theme->tag, width / 2, 310, 1);
    tft.setTextDatum(TL_DATUM);
  }

  // The wordmark sits inside the art, punched out of it on a black plate.
  void drawWordMark(TFT_eSPI& tft, int16_t row, bool framed) {
    const int16_t mark_w = (int16_t)strlen(shark_theme->wordmark) * CELL_W;
    const int16_t x = ART_X + ((ART_COLS * CELL_W - mark_w) / 2);
    const int16_t y = ART_Y + row * CELL_H;
    tft.fillRect(x - 6, y - 4, mark_w + 12, CELL_H + 7, TFT_BLACK);
    if (framed) {
      tft.drawRect(x - 6, y - 4, mark_w + 12, CELL_H + 7, WD_EDGE);
      wdCornerTicks(tft, x - 6, y - 4, mark_w + 12, CELL_H + 7, 5, WD_CYAN);
    }
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString(shark_theme->wordmark, x, y, 1);
  }

  // Two sliced bands per frame. Nothing is buffered: a band is blanked and
  // redrawn with a horizontal offset, then put back by restoreGlitch() so the
  // next frame always starts from the clean mark.
  const uint8_t GLITCH_BANDS = 2;
  const int16_t GLITCH_HEIGHT = 2;

  int16_t glitchRow(uint8_t band, uint16_t salt) {
    return (int16_t)(wdHash(band, salt, 0x3C3C) % (ART_ROWS - GLITCH_HEIGHT));
  }

  void drawGlitch(TFT_eSPI& tft, uint16_t salt) {
    for (uint8_t band = 0; band < GLITCH_BANDS; band++) {
      const uint16_t noise = wdHash(band, salt, 0x3C3C);
      const int16_t row = glitchRow(band, salt);
      const int16_t dx = ((noise >> 5) & 0x01) ? 4 : -4;
      const uint16_t color = ((noise >> 6) & 0x01) ? WD_CYAN : WD_AMBER;

      clearRows(tft, row, GLITCH_HEIGHT);
      drawRows(tft, row, row + GLITCH_HEIGHT - 1, salt, dx, color);
    }
  }

  void restoreGlitch(TFT_eSPI& tft, uint16_t salt) {
    for (uint8_t band = 0; band < GLITCH_BANDS; band++) {
      const int16_t row = glitchRow(band, salt);
      clearRows(tft, row, GLITCH_HEIGHT);
      if (shark_theme->splash == SHARK_SPLASH_CHROMA)
        drawChromaGhost(tft, row, row + GLITCH_HEIGHT - 1);
      drawRows(tft, row, row + GLITCH_HEIGHT - 1, 0, 0, WD_KEEP_COLOR);
    }
  }

  // ------------------------------------------------------------- mark boot
  void playMarkBoot(TFT_eSPI& tft) {
    const bool chroma = (shark_theme->splash == SHARK_SPLASH_CHROMA);

    // 1. Carrier noise, before anything resolves.
    for (uint8_t frame = 0; frame < 3; frame++) {
      drawCarrierNoise(tft, frame);
      drawStatus(tft, shark_theme->boot_a, WD_GREY);
      drawProgress(tft, (uint8_t)(frame + 1));
      delay(36);
    }
    clearRows(tft, 0, ART_ROWS);

    // 2. The mark decodes two lines at a time while the visible part flickers.
    for (uint8_t frame = 0; frame < 14; frame++) {
      const uint8_t revealed = (uint8_t)((frame + 1) * 2);
      drawRows(tft, frame * 2, revealed - 1, (uint16_t)(frame + 7), 0, WD_KEEP_COLOR);
      churnCells(tft, revealed, (uint16_t)(frame + 41));
      drawStatus(tft, shark_theme->boot_b, WD_BONE);
      drawProgress(tft, (uint8_t)(3 + frame * 2));
      delay(32);
    }

    // 3. Signal break.
    for (uint8_t frame = 0; frame < 3; frame++) {
      drawGlitch(tft, (uint16_t)(frame + 3));
      drawStatus(tft, shark_theme->boot_c, WD_AMBER);
      drawProgress(tft, (uint8_t)(31 + frame));
      delay(44);
      restoreGlitch(tft, (uint16_t)(frame + 3));
    }

    // 4. Lock. Every cell is redrawn from the same salt, so the mark stops
    // moving and looks identical on every boot.
    clearRows(tft, 0, ART_ROWS);
    if (chroma)
      drawChromaGhost(tft, 0, ART_ROWS - 1);
    drawRows(tft, 0, ART_ROWS - 1, 0, 0, WD_KEEP_COLOR);
    drawWordMark(tft, 23, false);
  }

  // ------------------------------------------------------------- rain boot
  void playRainBoot(TFT_eSPI& tft) {
    int8_t head[ART_COLS];
    uint8_t speed[ART_COLS];
    uint8_t trail[ART_COLS];

    for (uint8_t col = 0; col < ART_COLS; col++) {
      const uint16_t noise = wdHash(col, 0, 0x1234);
      head[col] = -(int8_t)(noise % ART_ROWS);
      speed[col] = 1 + ((noise >> 6) & 0x01);
      trail[col] = 5 + ((noise >> 8) % 9);
    }

    for (uint8_t frame = 0; frame < 34; frame++) {
      for (uint8_t col = 0; col < ART_COLS; col++) {
        const int16_t x = ART_X + col * CELL_W;

        // Erase the tail and dim the cell behind the head.
        const int16_t tail_row = head[col] - trail[col];
        if (tail_row >= 0 && tail_row < ART_ROWS)
          tft.fillRect(x, ART_Y + tail_row * CELL_H, CELL_W, CELL_H, TFT_BLACK);

        const int16_t body_row = head[col] - 1;
        if (body_row >= 0 && body_row < ART_ROWS)
          tft.drawChar(x,
                       ART_Y + body_row * CELL_H,
                       (uint8_t)rain_glyphs[wdHash(col, body_row, 3) & 0x0F],
                       shark_theme->accent,
                       TFT_BLACK,
                       1);

        const int16_t fade_row = head[col] - (trail[col] / 2);
        if (fade_row >= 0 && fade_row < ART_ROWS)
          tft.drawChar(x,
                       ART_Y + fade_row * CELL_H,
                       (uint8_t)rain_glyphs[wdHash(col, fade_row, 5) & 0x0F],
                       shark_theme->accent_dim,
                       TFT_BLACK,
                       1);

        head[col] += speed[col];
        if (head[col] >= 0 && head[col] < ART_ROWS)
          tft.drawChar(x,
                       ART_Y + head[col] * CELL_H,
                       (uint8_t)rain_glyphs[wdHash(col, head[col], frame) & 0x0F],
                       shark_theme->mark_a,
                       TFT_BLACK,
                       1);
        if (head[col] - trail[col] > ART_ROWS)
          head[col] = -(int8_t)(wdHash(col, frame, 0x77) % 10);
      }

      if (frame < 4)
        drawStatus(tft, shark_theme->boot_a, WD_GREY);
      else if (frame < 24)
        drawStatus(tft, shark_theme->boot_b, WD_BONE);
      else
        drawStatus(tft, shark_theme->boot_c, WD_AMBER);
      drawProgress(tft, (uint8_t)(1 + frame));
      delay(28);
    }

    drawWordMark(tft, 13, true);
  }

  void playWebBoot(TFT_eSPI& tft) {
    const int16_t cx = tft.width() / 2;
    const int16_t cy = 148;
    const int8_t dx[12] = {0, 45, 78, 94, 78, 45, 0, -45, -78, -94, -78, -45};
    const int8_t dy[12] = {-108, -94, -62, 0, 62, 94, 108, 94, 62, 0, -62, -94};
    for (uint8_t frame = 0; frame < 18; frame++) {
      tft.fillRect(8, 30, tft.width() - 16, 232, TFT_BLACK);
      for (uint8_t ring = 1; ring <= 4; ring++) {
        int16_t r = 18 + ring * 20 + (frame & 3);
        tft.drawCircle(cx, cy, r, ring == 4 ? WD_CYAN : WD_EDGE);
      }
      for (uint8_t i = 0; i < 12; i++) {
        uint16_t c = (i == frame % 12) ? WD_WHITE : ((i + 1 == frame % 12) ? WD_CYAN : WD_CYAN_DIM);
        tft.drawLine(cx, cy, cx + dx[i], cy + dy[i], c);
      }
      drawStatus(tft, frame < 10 ? shark_theme->boot_b : shark_theme->boot_c,
                 frame < 10 ? WD_BONE : WD_AMBER);
      drawProgress(tft, (uint8_t)(2 + frame * 2));
      delay(38);
    }
    tft.fillCircle(cx, cy, 7, WD_CYAN);
    drawWordMark(tft, 13, true);
  }

  void playAuroraBoot(TFT_eSPI& tft) {
    for (uint8_t frame = 0; frame < 22; frame++) {
      tft.fillRect(8, 30, tft.width() - 16, 232, TFT_BLACK);
      for (uint8_t band = 0; band < 5; band++) {
        uint16_t c = band == 0 ? WD_WHITE : (band & 1 ? WD_CYAN : WD_CYAN_SOFT);
        int16_t last_y = 0;
        for (int16_t x = 10; x < tft.width() - 10; x += 4) {
          int16_t y = 94 + band * 17 + (int16_t)((wdHash(x / 4 + frame, band, 0xA71) % 25) - 12);
          if (x > 10)
            tft.drawLine(x - 4, last_y, x, y, c);
          last_y = y;
        }
      }
      drawStatus(tft, frame < 15 ? shark_theme->boot_b : shark_theme->boot_c,
                 frame < 15 ? WD_BONE : WD_AMBER);
      drawProgress(tft, (uint8_t)(1 + frame * 3 / 2));
      delay(34);
    }
    drawWordMark(tft, 21, true);
  }

  void playEmberBoot(TFT_eSPI& tft) {
    tft.fillRect(8, 30, tft.width() - 16, 232, TFT_BLACK);
    for (uint8_t frame = 0; frame < 24; frame++) {
      tft.fillRect(8, 30, tft.width() - 16, 232, TFT_BLACK);
      for (uint8_t i = 0; i < 52; i++) {
        uint16_t n = wdHash(i, 0, 0xF1E0);
        int16_t x = 14 + (n % (tft.width() - 28));
        int16_t y = 245 - ((frame * (2 + ((n >> 8) & 3)) + (n >> 3)) % 205);
        uint16_t c = ((n + frame) & 3) ? WD_CYAN : WD_AMBER;
        tft.fillRect(x, y, 2, 2 + ((n >> 5) & 3), c);
      }
      tft.drawFastHLine(14, 248, tft.width() - 28, WD_CYAN);
      drawStatus(tft, frame < 17 ? shark_theme->boot_b : shark_theme->boot_c,
                 frame < 17 ? WD_BONE : WD_AMBER);
      drawProgress(tft, (uint8_t)(1 + frame * 4 / 3));
      delay(32);
    }
    drawWordMark(tft, 12, true);
  }

  void playSunsetBoot(TFT_eSPI& tft) {
    const int16_t horizon = 146;
    for (uint8_t frame = 0; frame < 20; frame++) {
      tft.fillRect(8, 30, tft.width() - 16, 232, TFT_BLACK);
      tft.fillCircle(tft.width() / 2, 102, 43, WD_AMBER);
      for (int16_t y = 72; y < 132; y += 8)
        tft.drawFastHLine(tft.width() / 2 - 44, y + (frame & 1), 88, TFT_BLACK);
      tft.drawFastHLine(10, horizon, tft.width() - 20, WD_CYAN);
      for (uint8_t i = 0; i < 9; i++) {
        int16_t y = horizon + ((i * i * 3 + frame * 3) % 105);
        tft.drawFastHLine(10, y, tft.width() - 20, i & 1 ? WD_CYAN_DIM : WD_EDGE);
      }
      for (int16_t x = -100; x <= 340; x += 28)
        tft.drawLine(tft.width() / 2, horizon, x + frame * 2, 252, WD_CYAN_DIM);
      drawStatus(tft, frame < 13 ? shark_theme->boot_b : shark_theme->boot_c,
                 frame < 13 ? WD_BONE : WD_AMBER);
      drawProgress(tft, (uint8_t)(2 + frame * 8 / 5));
      delay(36);
    }
    drawWordMark(tft, 8, true);
  }

  void playRadarBoot(TFT_eSPI& tft) {
    const int16_t cx = tft.width() / 2;
    const int16_t cy = 146;
    const int8_t dx[16] = {0, 35, 64, 84, 92, 84, 64, 35, 0, -35, -64, -84, -92, -84, -64, -35};
    const int8_t dy[16] = {-92, -84, -64, -35, 0, 35, 64, 84, 92, 84, 64, 35, 0, -35, -64, -84};
    for (uint8_t frame = 0; frame < 24; frame++) {
      tft.fillRect(8, 30, tft.width() - 16, 232, TFT_BLACK);
      tft.drawCircle(cx, cy, 30, WD_EDGE);
      tft.drawCircle(cx, cy, 60, WD_EDGE);
      tft.drawCircle(cx, cy, 92, WD_CYAN_DIM);
      tft.drawFastHLine(cx - 92, cy, 184, WD_EDGE);
      tft.drawFastVLine(cx, cy - 92, 184, WD_EDGE);
      uint8_t p = frame & 15;
      tft.drawLine(cx, cy, cx + dx[p], cy + dy[p], WD_CYAN);
      for (uint8_t echo = 0; echo < 4; echo++) {
        uint16_t n = wdHash(echo, frame / 4, 0x6A05);
        tft.fillCircle(cx - 75 + (n % 150), cy - 70 + ((n >> 7) % 140), 2, WD_WHITE);
      }
      drawStatus(tft, frame < 16 ? shark_theme->boot_b : shark_theme->boot_c,
                 frame < 16 ? WD_BONE : WD_AMBER);
      drawProgress(tft, (uint8_t)(1 + frame * 4 / 3));
      delay(34);
    }
    drawWordMark(tft, 13, true);
  }

  void playThemeBoot(TFT_eSPI& tft) {
    switch (shark_theme->splash) {
      case SHARK_SPLASH_RAIN:   playRainBoot(tft); break;
      case SHARK_SPLASH_WEB:    playWebBoot(tft); break;
      case SHARK_SPLASH_AURORA: playAuroraBoot(tft); break;
      case SHARK_SPLASH_EMBER:  playEmberBoot(tft); break;
      case SHARK_SPLASH_SUNSET: playSunsetBoot(tft); break;
      case SHARK_SPLASH_RADAR:  playRadarBoot(tft); break;
      default:                  playMarkBoot(tft); break;
    }
  }
}

namespace {
  // First splash: a live ASCII "liquid density morph" skull. Four 43x17 skull
  // poses -- front, oblique-right, lateral profile, and a jaw-drop bio-anomaly --
  // are stored as character art. Every frame the engine blends between two poses
  // with a smoothstep, drives each cell with a vertical sine wave, a rolling
  // horizontal glitch band and per-pixel flicker, and maps the resulting density
  // onto the " .,:;i1tfLCG08@#$" ramp (with hex-code injection in the glitch
  // bands). The skull is rendered from live text, tinted in the active theme and
  // flipped to red on the anomaly frame. Ported from the reference web engine.
  constexpr int SKULL_COLS = 43;
  constexpr int SKULL_ROWS = 17;

  const char* const skull_frames[4][SKULL_ROWS] = {
    { // FRAME 0: front facing
      R"(                  _.,',._                  )",
      R"(               ,y$$$$SSIi:.                )",
      R"(             .d$$$$$$$$$SSIi:.             )",
      R"(             j$$$$$$$$$$SSIIIi:            )",
      R"(             $$$SSS$$$$$$SIISSii-          )",
      R"(            j$S$$S$$$S$$$?'::iISii:        )",
      R"(            7  '?$7'   `~b.:iISII::        )",
      R"(            j  ,4$$.     $7-:iIIi::-       )",
      R"(            $,j' `$Sp,_.d$L._~?i::         )",
      R"(            ?$$.  `$$IS$$$S?*`,`:'         )",
      R"(             `$dpj$$7::`^` .:i:-'          )",
      R"(              j$S$$$Ii:' jIii7             )",
      R"(              -:`'%:*.  ,$7i?              )",
      R"(               .::\:\.p$S7Ii'              )",
      R"(               $%%$$$$SIi:`                )",
      R"(               ?$$$$$$?~                   )",
      R"(                `^"^j '                    )"
    },
    { // FRAME 1: slight right turn
      R"(                 .,o%S$$$Siu,.             )",
      R"(               ,i$$$Z$$$$$$$$$k.           )",
      R"(              iIS$Z$$$$$$$$$$$Si:          )",
      R"(              :ISSZ?$$$$S$$$$$$$Si.        )",
      R"(              iI$$Si:`?SS$$$$SSd$S:        )",
      R"(              :iS$SIi:-$$$$?$Zd$$Ii        )",
      R"(              -?S?i:- ,7^```~$$?~```?      )",
      R"(               -?,-.:j$     j$b    ,       )",
      R"(               `~4$$?$k-.,s$' ?,d          )",
      R"(                p  `~4$?'J$:  i^?          )",
      R"(                ?i   .:iSI$%iL``           )",
      R"(                i$.  ,^~4SiSi              )",
      R"(                :?Si:/.  `~ ~`             )",
      R"(                 `~4L?$:ui-j'              )",
      R"(                   `~$$$SIi                )",
      R"(                     `~o,-`                )",
      R"(                                           )"
    },
    { // FRAME 2: right profile
      R"(         .ii:-        ,y$$$SIII,           )",
      R"(         jS:- .    ,d$$?~``.;;-`?,         )",
      R"(         pSi: .   dSSp`::.`,`'?k.?-        )",
      R"(         y?k:-   d$$L`:,`      ?:i:        )",
      R"(         ?`,u,_,dp.`~:,       $ii-         )",
      R"(         :iS$$S$$7ii;-.       JI?'         )",
      R"(         .;S$$S$$7di? -:k     dS;'         )",
      R"(         ?S$$Sb?$7~`4k ,7    d7'           )",
      R"(         .SS$?I?    S:7      j?            )",
      R"(         `?S$?jS$,_.d?'      ?             )",
      R"(          d$7jI$7i?``        `.            )",
      R"(          j$7jSI$i:-                       )",
      R"(         -$7 ?$?Sb:;                       )",
      R"(          ~  i?^d?i'                       )",
      R"(          j . S' `"                        )",
      R"(          ?:-j7                            )",
      R"(          S:j7                             )"
    },
    { // FRAME 3: jaw drop / bio-anomaly
      R"(                  _.,',._                  )",
      R"(               ,y$$$$SSIi:.                )",
      R"(             .d$$$$$$$$$SSIi:.             )",
      R"(             j$$$$$$$$$$SSIIIi:            )",
      R"(             $$$SSS$$$$$$SIISSii-          )",
      R"(            j$S$$S$$$S$$$?'::iISii:        )",
      R"(            7  '?$7'   `~b.:iISII::        )",
      R"(            j  ,4$$.     $7-:iIIi::-       )",
      R"(            $,j' `$Sp,_.d$L._~?i::         )",
      R"(            ?$$.  `$$IS$$$S?*`,`:'         )",
      R"(             `$dpj$$7::`^` .:i:-'          )",
      R"(               ..::::::::::..              )",
      R"(              j$$$$$$$$$$$$$$7             )",
      R"(              :$$$$$$$$$$$$$$i             )",
      R"(               $$$$$$$$$$$$$$'             )",
      R"(               ?$$$$$$$$$$$$~              )",
      R"(                `^"^^^^^^" '               )"
    }
  };

  // Base brightness of each art character, matching the reference weight table.
  // '~' stands in for the original degree glyphs (weight 0.4); unknown non-space
  // characters default to 0.5 exactly as the web engine does.
  float skullCharWeight(char c) {
    switch (c) {
      case ' ':  return 0.0f;
      case '.':  return 0.1f;
      case ',':  return 0.15f;
      case ':':  return 0.2f;
      case '-':  return 0.25f;
      case '`':
      case '\'': return 0.3f;
      case '^':  return 0.35f;
      case '~':
      case '_':  return 0.4f;
      case '\\': return 0.45f;
      case '*':  return 0.5f;
      case 'i':
      case 'o':  return 0.6f;
      case 'I':
      case 'u':  return 0.7f;
      case 'j':
      case 'L':  return 0.75f;
      case '?':
      case '7':
      case 'k':
      case 's':
      case 'J':  return 0.8f;
      case 'p':
      case 'y':
      case 'b':  return 0.85f;
      case 'd':
      case 'Z':  return 0.9f;
      case 'S':  return 0.95f;
      case '$':
      case '%':  return 1.0f;
      default:   return 0.5f;
    }
  }

  // Linear blend of two RGB565 colors (t: 0 -> a, 1 -> b).
  inline uint16_t skullMix(uint16_t a, uint16_t b, float t) {
    int ar = (a >> 11) & 0x1F, ag = (a >> 5) & 0x3F, ab = a & 0x1F;
    int br = (b >> 11) & 0x1F, bg = (b >> 5) & 0x3F, bb = b & 0x1F;
    int r = ar + (int)((br - ar) * t);
    int g = ag + (int)((bg - ag) * t);
    int bl = ab + (int)((bb - ab) * t);
    return (uint16_t)((r << 11) | (g << 5) | bl);
  }

  void playSkullMorph(TFT_eSPI& tft, const String& version) {
    const int16_t w = tft.width();
    tft.fillScreen(TFT_BLACK);

    // Boot chrome: header, corner frame, footer, and an identity plate.
    drawHeader(tft, w, version);
    wdCornerTicks(tft, 4, 28, w - 8, 268, 10, WD_CYAN_DIM);
    drawFooter(tft, w);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WD_GREY, TFT_BLACK);
    tft.drawString("SKULL_RENDER.EXE", w / 2, 42, 2);
    tft.setTextColor(WD_DIM, TFT_BLACK);
    tft.drawString(String(shark_theme->name) + " NEURAL SCAN", w / 2, 60, 1);
    tft.setTextDatum(TL_DATUM);

    // Precompute the density maps (0..255) once for the life of the process.
    static uint8_t dens[4][SKULL_ROWS][SKULL_COLS];
    static bool dens_built = false;
    if (!dens_built) {
      for (int f = 0; f < 4; f++)
        for (int y = 0; y < SKULL_ROWS; y++) {
          const char* row = skull_frames[f][y];
          int len = (int)strlen(row);
          for (int x = 0; x < SKULL_COLS; x++) {
            char c = (x < len) ? row[x] : ' ';
            dens[f][y][x] = (uint8_t)(skullCharWeight(c) * 255.0f);
          }
        }
      dens_built = true;
    }

    const char ramp[] = " .,:;i1tfLCG08@#$";   // 17 chars, brightest index = 16
    const int rampMax = 16;
    const char hexc[] = "0123456789ABCDEF";
    const int seq[6] = {0, 1, 2, 1, 0, 3};
    const char* labels[6] = {
      "SCAN: ANTERIOR CRANIUM",
      "SCAN: OBLIQUE RIGHT",
      "SCAN: LATERAL PROFILE",
      "SCAN: OBLIQUE RIGHT",
      "SCAN: ANTERIOR CRANIUM",
      "WARNING: BIO-ANOMALY"
    };

    const int CELL_W = 5, CELL_H = 8;
    const int SW = SKULL_COLS * CELL_W + 1;   // 216
    const int SH = SKULL_ROWS * CELL_H;       // 136
    const int16_t sx = (w - SW) / 2;
    const int16_t sy = 84;

    // Render into an off-screen sprite so the morph never tears or flickers.
    // Falls back to direct drawing if the buffer cannot be allocated.
    TFT_eSprite spr = TFT_eSprite(&tft);
    spr.setColorDepth(16);
    bool use_sprite = (spr.createSprite(SW, SH) != nullptr);
    if (use_sprite) { spr.setTextFont(1); spr.setTextWrap(false); }

    uint16_t rowcol[SKULL_ROWS];
    uint32_t rng = 0x1234abcdu ^ millis();

    const float cycleDuration = 0.75f;                 // seconds per pose morph
    const uint32_t t0 = millis();
    const uint32_t duration = (uint32_t)(6 * cycleDuration * 1000.0f);  // one loop
    int last_label = -1;
    uint32_t last_chrome = 0;

    while (millis() - t0 < duration) {
      float time = (millis() - t0) * 0.001f;
      float totalProgress = time / cycleDuration;
      int curr = ((int)floorf(totalProgress)) % 6;
      int next = (curr + 1) % 6;
      float tt = totalProgress - floorf(totalProgress);
      float smoothT = tt * tt * (3.0f - 2.0f * tt);
      bool glitching = (seq[curr] == 3);

      // Vertical gradient for this frame (bright top -> dark bottom).
      uint16_t ctop, cbot;
      if (glitching) { ctop = 0xFBCF; cbot = 0x5800; }   // light red -> dark red
      else {
        ctop = skullMix(shark_theme->accent, 0xFFFF, 0.65f);
        cbot = skullMix(shark_theme->accent, 0x0000, 0.78f);
      }
      for (int y = 0; y < SKULL_ROWS; y++)
        rowcol[y] = skullMix(ctop, cbot, (float)y / (SKULL_ROWS - 1));

      if (use_sprite) spr.fillSprite(TFT_BLACK);
      else            tft.fillRect(sx, sy, SW, SH, TFT_BLACK);

      for (int y = 0; y < SKULL_ROWS; y++) {
        float wave = sinf(time * 3.0f - y * 0.5f) * 0.15f;
        float gb = sinf(time * 4.0f - y * 2.0f);
        float glitchEffect = (gb > 0.95f) ? 0.4f : 0.0f;
        for (int x = 0; x < SKULL_COLS; x++) {
          float dA = dens[seq[curr]][y][x] / 255.0f;
          float dB = dens[seq[next]][y][x] / 255.0f;
          float base = dA + (dB - dA) * smoothT;
          if (base < 0.05f) continue;

          rng = rng * 1664525u + 1013904223u;
          float flicker = (((rng >> 8) & 0xFFFF) / 65535.0f) * 0.2f - 0.1f;
          float fd = base + wave + glitchEffect + flicker;
          if (fd < 0) fd = 0;
          if (fd > 1) fd = 1;

          char ch;
          rng = rng * 1664525u + 1013904223u;
          float rnd2 = ((rng >> 8) & 0xFFFF) / 65535.0f;
          if (gb > 0.98f || (smoothT > 0.4f && smoothT < 0.6f && rnd2 > 0.8f)) {
            rng = rng * 1664525u + 1013904223u;
            ch = hexc[(rng >> 8) % 16];
          } else {
            int idx = (int)(fd * rampMax);
            if (idx < 0) idx = 0;
            if (idx > rampMax) idx = rampMax;
            ch = ramp[idx];
          }
          if (ch == ' ') continue;

          uint16_t col = rowcol[y];
          if (use_sprite) {
            spr.setTextColor(col, TFT_BLACK);
            spr.drawChar((uint16_t)ch, x * CELL_W, y * CELL_H, 1);
          } else {
            tft.setTextColor(col, TFT_BLACK);
            tft.drawChar((uint16_t)ch, sx + x * CELL_W, sy + y * CELL_H, 1);
          }
        }
      }
      if (use_sprite) spr.pushSprite(sx, sy);

      if (millis() - last_chrome > 110) {
        last_chrome = millis();
        drawProgress(tft, (uint8_t)(((millis() - t0) * ART_COLS) / duration));
        if (curr != last_label) {
          last_label = curr;
          drawStatus(tft, labels[curr], glitching ? WD_AMBER : WD_BONE);
        }
      }
      delay(6);
    }

    if (use_sprite) spr.deleteSprite();
    drawProgress(tft, ART_COLS);
    drawStatus(tft, shark_theme->boot_ready, WD_CYAN);
  }

  // Base brightness for the spider art characters (adds the leg/web glyphs the
  // skull table does not use). Unknown non-space characters default to 0.5.
  float spiderCharWeight(char c) {
    switch (c) {
      case ' ':  return 0.0f;
      case '.':  return 0.1f;
      case ',':  return 0.15f;
      case ':':  return 0.2f;
      case '-':  return 0.25f;
      case ';':  return 0.3f;
      case '`':
      case '\'': return 0.3f;
      case '^':  return 0.35f;
      case '"':  return 0.4f;
      case '_':  return 0.4f;
      case '\\':
      case '/':  return 0.45f;
      case '|':
      case '=':
      case '*':
      case '(':
      case ')':  return 0.5f;
      case 'r':  return 0.5f;
      case 'i':
      case 'o':
      case 'v':
      case '+':
      case 'h':
      case '[':
      case ']':
      case '{':
      case '}':  return 0.6f;
      case 'I':
      case 'u':
      case 'V':
      case 'x':  return 0.7f;
      case 'j':
      case 'L':  return 0.75f;
      case '?':
      case '7':
      case 'k':
      case 's':
      case 'J':
      case 'O':  return 0.8f;
      case 'p':
      case 'y':
      case 'b':  return 0.85f;
      case 'd':
      case 'Z':
      case 'X':  return 0.9f;
      case 'S':  return 0.95f;
      case '$':
      case '%':
      case '#':  return 1.0f;
      default:   return 0.5f;
    }
  }

  // First splash for the Spider theme: the live ASCII liquid-morph spider. Same
  // engine as the skull, sized for the taller 42x32 art (7 px rows) so it fits
  // under the header with the standard progress/status chrome below.
  void playSpiderMorph(TFT_eSPI& tft, const String& version) {
    const int16_t w = tft.width();
    tft.fillScreen(TFT_BLACK);
    drawHeader(tft, w, version);

    static uint8_t sdens[4][SPIDER_ROWS][SPIDER_COLS];
    for (int f = 0; f < 4; f++)
      for (int y = 0; y < SPIDER_ROWS; y++) {
        const char* row = spider_splash_frames[f][y];
        int len = (int)strlen(row);
        for (int x = 0; x < SPIDER_COLS; x++) {
          char c = (x < len) ? row[x] : ' ';
          sdens[f][y][x] = (uint8_t)(spiderCharWeight(c) * 255.0f);
        }
      }

    const char ramp[] = " .,:;i1tfLCG08@#$";
    const int rampMax = 16;
    const char hexc[] = "0123456789ABCDEF";
    const int seq[6] = {0, 1, 0, 2, 0, 3};
    const char* labels[6] = {
      "SCAN: ARACHNID IDLE",
      "SCAN: KINETIC CRAWL",
      "SCAN: ARACHNID IDLE",
      "SCAN: THREAT DISPLAY",
      "SCAN: ARACHNID IDLE",
      "WARNING: BIO-WEB DETECTED"
    };

    const int CELL_W = 5, CELL_H = 7;
    const int SW = SPIDER_COLS * CELL_W + 1;   // 211
    const int SH = SPIDER_ROWS * CELL_H;       // 224
    const int16_t sx = (w - SW) / 2;
    const int16_t sy = 31;

    TFT_eSprite spr = TFT_eSprite(&tft);
    spr.setColorDepth(16);
    bool use_sprite = (spr.createSprite(SW, SH) != nullptr);
    if (use_sprite) { spr.setTextFont(1); spr.setTextWrap(false); }

    uint16_t rowcol[SPIDER_ROWS];
    uint32_t rng = 0x51de5a1du ^ millis();
    const float cycleDuration = 0.75f;
    const uint32_t t0 = millis();
    const uint32_t duration = (uint32_t)(6 * cycleDuration * 1000.0f);
    int last_label = -1;
    uint32_t last_chrome = 0;

    while (millis() - t0 < duration) {
      float time = (millis() - t0) * 0.001f;
      float totalProgress = time / cycleDuration;
      int curr = ((int)floorf(totalProgress)) % 6;
      int next = (curr + 1) % 6;
      float tt = totalProgress - floorf(totalProgress);
      float smoothT = tt * tt * (3.0f - 2.0f * tt);
      bool glitching = (seq[curr] == 3);

      uint16_t ctop, cbot;
      if (glitching) { ctop = 0xFBCF; cbot = 0x5800; }
      else {
        ctop = skullMix(shark_theme->accent, 0xFFFF, 0.65f);
        cbot = skullMix(shark_theme->accent, 0x0000, 0.78f);
      }
      for (int y = 0; y < SPIDER_ROWS; y++)
        rowcol[y] = skullMix(ctop, cbot, (float)y / (SPIDER_ROWS - 1));

      if (use_sprite) spr.fillSprite(TFT_BLACK);
      else            tft.fillRect(sx, sy, SW, SH, TFT_BLACK);

      for (int y = 0; y < SPIDER_ROWS; y++) {
        float wave = sinf(time * 3.0f - y * 0.5f) * 0.15f;
        float gb = sinf(time * 4.0f - y * 2.0f);
        float glitchEffect = (gb > 0.95f) ? 0.4f : 0.0f;
        for (int x = 0; x < SPIDER_COLS; x++) {
          float dA = sdens[seq[curr]][y][x] / 255.0f;
          float dB = sdens[seq[next]][y][x] / 255.0f;
          float base = dA + (dB - dA) * smoothT;
          if (base < 0.05f) continue;

          rng = rng * 1664525u + 1013904223u;
          float flicker = (((rng >> 8) & 0xFFFF) / 65535.0f) * 0.2f - 0.1f;
          float fd = base + wave + glitchEffect + flicker;
          if (fd < 0) fd = 0;
          if (fd > 1) fd = 1;

          char ch;
          rng = rng * 1664525u + 1013904223u;
          float rnd2 = ((rng >> 8) & 0xFFFF) / 65535.0f;
          if (gb > 0.98f || (smoothT > 0.4f && smoothT < 0.6f && rnd2 > 0.8f)) {
            rng = rng * 1664525u + 1013904223u;
            ch = hexc[(rng >> 8) % 16];
          } else {
            int idx = (int)(fd * rampMax);
            if (idx < 0) idx = 0;
            if (idx > rampMax) idx = rampMax;
            ch = ramp[idx];
          }
          if (ch == ' ') continue;

          uint16_t col = rowcol[y];
          if (use_sprite) {
            spr.setTextColor(col, TFT_BLACK);
            spr.drawChar((uint16_t)ch, x * CELL_W, y * CELL_H, 1);
          } else {
            tft.setTextColor(col, TFT_BLACK);
            tft.drawChar((uint16_t)ch, sx + x * CELL_W, sy + y * CELL_H, 1);
          }
        }
      }
      if (use_sprite) spr.pushSprite(sx, sy);

      if (millis() - last_chrome > 110) {
        last_chrome = millis();
        drawProgress(tft, (uint8_t)(((millis() - t0) * ART_COLS) / duration));
        if (curr != last_label) {
          last_label = curr;
          drawStatus(tft, labels[curr], glitching ? WD_AMBER : WD_BONE);
        }
      }
      delay(6);
    }

    if (use_sprite) spr.deleteSprite();
    drawProgress(tft, ART_COLS);
    drawStatus(tft, shark_theme->boot_ready, WD_CYAN);
  }

  // Second splash: the real M5 SHARK shark-head logo, revealed in two phases so
  // it reads as its own animation after the theme's character intro. Phase one
  // wipes the mark in column by column behind a bright scan bar; phase two
  // pulses the accent and prints the wordmark.
  const int16_t LOGO_X = (240 - SHARK_LOGO_W) / 2;
  const int16_t LOGO_Y = 40;

  void drawLogoColumns(TFT_eSPI& tft, int16_t from_col, int16_t to_col, uint16_t color) {
    const int16_t byte_w = (SHARK_LOGO_W + 7) / 8;
    for (int16_t x = from_col; x < to_col && x < SHARK_LOGO_W; x++) {
      for (int16_t y = 0; y < SHARK_LOGO_H; y++) {
        const uint8_t bits = pgm_read_byte(&shark_logo_bitmap[y * byte_w + (x >> 3)]);
        if (bits & (128 >> (x & 7)))
          tft.drawPixel(LOGO_X + x, LOGO_Y + y, color);
      }
    }
  }

  // Just the logo image: the small detailed DedSec skull bitmap on a plain black
  // field, with a scanline decrypt sweep + brief glitch, then a clean settle.
  void playDedsecLogo(TFT_eSPI& tft, int16_t width) {
    tft.fillRect(0, 24, width, 268 - 24 + 4, TFT_BLACK);
    wdCornerTicks(tft, 4, 28, width - 8, 268, 10, WD_CYAN_DIM);
    drawStatus(tft, "DECRYPTING IDENTITY", WD_GREY);

    const uint16_t c_hi  = skullMix(shark_theme->accent, 0xFFFF, 0.45f);
    const uint16_t c_mid = shark_theme->accent;
    const uint16_t c_lo  = WD_CYAN_DIM;

    const int CW = 2, CH = 3;                  // small pixel cells
    const int LW = DEDSEC_COLS * CW;           // 120
    const int LH = DEDSEC_ROWS * CH;           // 90
    const int16_t sx = (width - LW) / 2;
    const int16_t sy = 96;                      // small, centred

    TFT_eSprite spr = TFT_eSprite(&tft);
    spr.setColorDepth(16);
    bool use_sprite = (spr.createSprite(LW, LH) != nullptr);

    uint32_t rng = 0xdead5ec7u ^ millis();
    const uint32_t t0 = millis(), dur = 3200;

    while (millis() - t0 < dur) {
      float time = (millis() - t0) * 0.001f;
      bool glitching = ((millis() - t0) % 2000) < 140;
      int scan = (int)(fmodf(time * 26.0f, DEDSEC_ROWS + 8));

      if (use_sprite) spr.fillSprite(TFT_BLACK);
      else            tft.fillRect(sx, sy, LW, LH, TFT_BLACK);

      for (int y = 0; y < DEDSEC_ROWS; y++) {
        for (int x = 0; x < DEDSEC_COLS; x++) {
          uint8_t v = pgm_read_byte(&DEDSEC_LOGO[y * DEDSEC_COLS + x]);
          if (v < 40) continue;
          uint16_t col;
          rng = rng * 1664525u + 1013904223u;
          float r = ((rng >> 8) & 0xFFFF) / 65535.0f;
          if (glitching && r > 0.86f)          col = (r > 0.93f) ? 0xFFE0 : 0xF81F;
          else if (y == scan || y == scan - 1) col = WD_WHITE;
          else                                 col = (v > 170) ? c_hi : (v > 100) ? c_mid : c_lo;
          if (use_sprite) spr.fillRect(x * CW, y * CH, CW, CH, col);
          else            tft.fillRect(sx + x * CW, sy + y * CH, CW, CH, col);
        }
      }
      if (use_sprite) spr.pushSprite(sx, sy);
      drawProgress(tft, (uint8_t)(((millis() - t0) * ART_COLS) / dur));
      delay(22);
    }

    // Settle: clean, glitch-free.
    if (use_sprite) spr.fillSprite(TFT_BLACK);
    else            tft.fillRect(sx, sy, LW, LH, TFT_BLACK);
    for (int y = 0; y < DEDSEC_ROWS; y++) {
      for (int x = 0; x < DEDSEC_COLS; x++) {
        uint8_t v = pgm_read_byte(&DEDSEC_LOGO[y * DEDSEC_COLS + x]);
        if (v < 40) continue;
        uint16_t col = (v > 170) ? c_hi : (v > 100) ? c_mid : c_lo;
        if (use_sprite) spr.fillRect(x * CW, y * CH, CW, CH, col);
        else            tft.fillRect(sx + x * CW, sy + y * CH, CW, CH, col);
      }
    }
    if (use_sprite) { spr.pushSprite(sx, sy); spr.deleteSprite(); }

    // Wordmark plate under the logo.
    const int16_t wy = sy + LH + 8;
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WD_WHITE, TFT_BLACK);
    tft.drawString(SHARK_UI_NAME, width / 2, wy + 8, 4);
    tft.setTextColor(WD_GREY, TFT_BLACK);
    tft.drawString(String(shark_theme->name) + " // " + SHARK_UI_TAG, width / 2, wy + 28, 2);
    tft.setTextDatum(TL_DATUM);
    drawProgress(tft, ART_COLS);
    drawStatus(tft, shark_theme->boot_ready, WD_CYAN);
    delay(500);
  }

  void playLogoReveal(TFT_eSPI& tft, int16_t width) {
    // Clear the whole art field, keep the header/footer chrome.
    tft.fillRect(0, 24, width, 268 - 24 + 4, TFT_BLACK);
    wdCornerTicks(tft, 4, 28, width - 8, 268, 10, WD_CYAN_DIM);

    drawStatus(tft, "LOADING IDENTITY", WD_GREY);

    // Phase 1: scan bar sweeps left to right, painting the logo behind it.
    const int16_t step = 6;
    for (int16_t x = 0; x <= SHARK_LOGO_W; x += step) {
      drawLogoColumns(tft, x, x + step, WD_BONE);
      const int16_t bar_x = LOGO_X + x + step;
      if (bar_x < LOGO_X + SHARK_LOGO_W)
        tft.drawFastVLine(bar_x, LOGO_Y - 4, SHARK_LOGO_H + 8, WD_CYAN);
      drawProgress(tft, (uint8_t)((x * ART_COLS) / SHARK_LOGO_W));
      delay(16);
      if (bar_x < LOGO_X + SHARK_LOGO_W)
        tft.drawFastVLine(bar_x, LOGO_Y - 4, SHARK_LOGO_H + 8, TFT_BLACK);
    }

    // Phase 2: two accent pulses, then settle in accent with the wordmark.
    for (uint8_t p = 0; p < 2; p++) {
      drawLogoColumns(tft, 0, SHARK_LOGO_W, WD_CYAN);
      delay(70);
      drawLogoColumns(tft, 0, SHARK_LOGO_W, WD_WHITE);
      delay(70);
    }
    drawLogoColumns(tft, 0, SHARK_LOGO_W, WD_CYAN);

    // Wordmark plate under the logo.
    const int16_t wy = LOGO_Y + SHARK_LOGO_H + 6;
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WD_WHITE, TFT_BLACK);
    tft.drawString(SHARK_UI_NAME, width / 2, wy + 10, 4);
    tft.setTextColor(WD_GREY, TFT_BLACK);
    tft.drawString(String(shark_theme->name) + " // " + SHARK_UI_TAG, width / 2, wy + 34, 2);
    tft.setTextDatum(TL_DATUM);
    drawProgress(tft, ART_COLS);
    drawStatus(tft, shark_theme->boot_ready, WD_CYAN);
    delay(520);
  }
}

void playSharkBoot(TFT_eSPI& tft, const String& version) {
  const int16_t width = tft.width();

  // Required for the CP437 block glyphs used by the art and the progress bar.
  tft.setAttribute(CP437_SWITCH, 1);
  tft.setFreeFont(NULL);
  tft.setTextWrap(false);
  tft.setTextSize(1);

  // Splash 1: the live ASCII liquid-morph mark, tinted per theme. The Spider
  // theme morphs an arachnid; every other theme morphs the skull.
  if (shark_theme->mark == SHARK_MARK_SPIDER)
    playSpiderMorph(tft, version);
  else
    playSkullMorph(tft, version);
  delay(320);

  // Splash 2: the DedSec logo, glitch-revealed (ends on the theme's "ready" line).
  tft.fillScreen(TFT_BLACK);
  drawHeader(tft, width, version);
  wdCornerTicks(tft, 4, 28, width - 8, 268, 10, WD_CYAN_DIM);
  drawFooter(tft, width);
  playDedsecLogo(tft, width);

  tft.setTextDatum(TL_DATUM);
  delay(420);
}

namespace {
  bool wallpaperImageExists() {
    #ifdef HAS_SD
      return sd_obj.supported && SD.exists(SHARK_WALL_PATH);
    #else
      return false;
    #endif
  }

  // Streams the uploaded 240x320 RGB565 image from SD to the panel, row by row.
  // The browser writes little-endian RGB565 (a JS Uint16Array), so the panel
  // push must swap bytes -- without it the image renders as colour noise.
  void drawWallpaperImage(TFT_eSPI& tft) {
    #ifdef HAS_SD
      File f = SD.open(SHARK_WALL_PATH, FILE_READ);
      if (!f)
        return;
      tft.setSwapBytes(true);
      static uint16_t row[SHARK_WALL_W];
      for (int16_t y = 0; y < SHARK_WALL_H; y++) {
        if (f.read((uint8_t*)row, SHARK_WALL_W * 2) != SHARK_WALL_W * 2)
          break;
        tft.pushImage(0, y, SHARK_WALL_W, 1, row);
      }
      tft.setSwapBytes(false);
      f.close();
    #endif
  }

  // A theme-coloured falling-code screensaver used when no image is set.
  void animatedWallpaper(TFT_eSPI& tft, uint16_t frame) {
    const char glyphs[16] = {'0','1','A','Z','7','X','#','$','K','3','W','9','F','5','T','2'};
    const int16_t cols = tft.width() / 6;
    for (int16_t c = 0; c < cols; c++) {
      const uint16_t n = wdHash(c, frame >> 1, 0x1234);
      const uint8_t speed = 1 + (n & 1);
      const int16_t head = ((frame * speed + (n % 40)) % 60) - 6;   // wraps off-screen
      const int16_t y = head * 8;
      if (y >= 0 && y < tft.height())
        tft.drawChar(c * 6, y, (uint8_t)glyphs[wdHash(c, head, frame) & 0x0F],
                     shark_theme->accent, TFT_BLACK, 1);
      const int16_t ty = (head - 4) * 8;                            // erase the tail
      if (ty >= 0 && ty < tft.height())
        tft.fillRect(c * 6, ty, 6, 8, TFT_BLACK);
      const int16_t dy = (head - 2) * 8;                            // dim mid-trail
      if (dy >= 0 && dy < tft.height())
        tft.drawChar(c * 6, dy, (uint8_t)glyphs[wdHash(c, head + 1, 3) & 0x0F],
                     shark_theme->accent_dim, TFT_BLACK, 1);
    }
  }

  void wallpaperBrand(TFT_eSPI& tft) {
    const int16_t w = tft.width(), cy = tft.height() / 2;
    tft.fillRect(0, cy - 26, w, 62, TFT_BLACK);
    tft.drawRect(18, cy - 24, w - 36, 58, WD_EDGE);
    wdCornerTicks(tft, 18, cy - 24, w - 36, 58, 8, WD_CYAN);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString(SHARK_UI_NAME, w / 2, cy - 8, 4);
    tft.setTextColor(WD_GREY, TFT_BLACK);
    tft.drawString(String(shark_theme->name) + " // " + shark_theme->tag, w / 2, cy + 14, 1);
    tft.setTextColor(WD_DIM, TFT_BLACK);
    tft.drawString("TOUCH TO WAKE", w / 2, cy + 44, 1);
    tft.setTextDatum(TL_DATUM);
  }
}

void sharkIdleWallpaper(TFT_eSPI& tft) {
  tft.setAttribute(CP437_SWITCH, 1);
  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);

  const bool has_image = wallpaperImageExists();
  tft.fillScreen(TFT_BLACK);
  if (has_image) {
    drawWallpaperImage(tft);
    // Reserve a bottom strip for the wake prompt so the image stays intact.
    tft.fillRect(0, tft.height() - 16, tft.width(), 16, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString("TOUCH TO WAKE", tft.width() / 2, tft.height() - 8, 1);
    tft.setTextDatum(TL_DATUM);
  }

  uint16_t tx, ty, frame = 0;
  uint32_t last = 0;
  while (true) {
    if (display_obj.updateTouch(&tx, &ty)) {
      while (display_obj.updateTouch(&tx, &ty))
        delay(10);
      break;
    }
    if (!has_image && millis() - last >= 45) {
      last = millis();
      animatedWallpaper(tft, frame);
      wallpaperBrand(tft);   // redrawn every frame so the rain never covers it
      frame++;
    }
    delay(8);
  }
}

#endif
