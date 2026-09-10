#pragma once

#include "configs.h"
#include "SharkTheme.h"

#if defined(HAS_SCREEN) && defined(MARAUDER_V8)
  #include <TFT_eSPI.h>

  void playSharkBoot(TFT_eSPI& tft, const String& version);

  // Full-screen idle wallpaper: an uploaded image (SD /shark/wall.bin, 240x320
  // RGB565) if present, otherwise a theme-coloured animated screensaver. Blocks
  // until the screen is touched, then returns so the menu can redraw.
  void sharkIdleWallpaper(TFT_eSPI& tft);

  // Path + geometry of the web-uploaded wallpaper, shared with SharkWeb.
  #define SHARK_WALL_PATH "/shark/wall.bin"
  #define SHARK_WALL_W 240
  #define SHARK_WALL_H 320
  #define SHARK_WALL_BYTES (SHARK_WALL_W * SHARK_WALL_H * 2)
#endif
