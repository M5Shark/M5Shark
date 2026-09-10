// SHARK Games module. Copyright (c) 2026 M5SHARK. All rights reserved.
//
// Built-in games (Snake, Reaction, Tic-Tac-Toe) plus a custom game
// engine that reads AI-created games from the SD card. Games run on
// the device's 240x320 resistive touch screen.
#pragma once
#include <Arduino.h>

class SharkGames {
  public:
    // Menu with all games (built-in + custom from SD)
    static void gamesMenu();

    // Built-in games
    static void snake();
    static void reaction();
    static void tictactoe();

    // Custom game engine: reads /GAMES/*.gme from SD
    // Format: simple line-based text the MCP server can write:
    //   TITLE:My Quiz Game
    //   TYPE:QUIZ
    //   Q:What is 2+2|3|4|5|6|1
    //   Q:Capital of France|London|Paris|Berlin|Tokyo|1
    //   Q:Which is not a fruit|Apple|Carrot|Banana|Mango|1
    //   END
    // TYPE:QUIZ = multiple choice (question|opt0|opt1|...|answerIdx)
    // TYPE:REACT = reaction timing game
    // TYPE:MEM = memory sequence game
    static void playCustom(const String& path);
    static uint8_t customCount();
    static String customName(uint8_t i);
};

extern SharkGames shark_games_obj;
