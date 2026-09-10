#pragma once

#include "configs.h"

#if defined(HAS_SCREEN) && defined(MARAUDER_V8)
  #include <TFT_eSPI.h>

  // Runtime UI themes. Every screen reads its colors through the macros at the
  // bottom of this header, so switching the active theme restyles the whole
  // firmware without touching a single call site.

  enum SharkSplash : uint8_t {
    SHARK_SPLASH_MARK = 0,   // character-cell mark, decoded line by line
    SHARK_SPLASH_CHROMA,     // same mark with an RGB split glitch
    SHARK_SPLASH_RAIN,       // falling code columns
    SHARK_SPLASH_WEB,        // radial web sweep
    SHARK_SPLASH_AURORA,     // polar light bands
    SHARK_SPLASH_EMBER,      // rising fire particles
    SHARK_SPLASH_SUNSET,     // synthwave horizon
    SHARK_SPLASH_RADAR       // low-light radar sweep
  };

  enum SharkMark : uint8_t {
    SHARK_MARK_SKULL = 0,
    SHARK_MARK_SPIDER,
    SHARK_MARK_NONE,
    SHARK_MARK_CYBER,   // visor skull
    SHARK_MARK_DEMON,   // horned skull
    SHARK_MARK_ALIEN,   // grey alien head
    SHARK_MARK_BIO,     // biohazard trefoil
    SHARK_MARK_REAPER   // hooded skull
  };

  // Each theme owns a menu-tile style, so themes differ in layout and shape,
  // not only color. The renderer reads these through sharkStyleParams().
  enum SharkStyle : uint8_t {
    SHARK_STYLE_CTOS = 0,   // hard edge, corner ticks, accent rail (Watch Dogs)
    SHARK_STYLE_TERMINAL,   // no chrome, a ">" prompt and a top rule (Matrix)
    SHARK_STYLE_NEON,       // double glowing outline, accent rail (2077 / neon)
    SHARK_STYLE_SOFT,       // rounded tiles, thin rail, mixed-case labels
    SHARK_STYLE_BLOCK,      // hard edge, thick accent block, corner ticks
    SHARK_STYLE_MINIMAL     // thin low-contrast frame, no chrome, mixed case
  };

  struct SharkStyleParams {
    bool rounded;    // rounded tile corners
    bool ticks;      // ctOS corner ticks
    uint8_t rail_w;  // accent rail width in px (0 = none)
    bool glow;       // inner accent outline
    bool prompt;     // ">" prompt + top rule
    bool upper;      // upper-case labels
  };

  inline SharkStyleParams sharkStyleParams(uint8_t style) {
    switch (style) {
      case SHARK_STYLE_TERMINAL: return {false, false, 0, false, true,  true};
      case SHARK_STYLE_NEON:     return {false, false, 2, true,  false, true};
      case SHARK_STYLE_SOFT:     return {true,  false, 2, false, false, false};
      case SHARK_STYLE_BLOCK:    return {false, true,  4, false, false, true};
      case SHARK_STYLE_MINIMAL:  return {false, false, 0, false, false, false};
      default:                   return {false, true,  2, false, false, true}; // CTOS
    }
  }

  struct SharkThemeDef {
    const char* name;      // label in the Theme menu
    const char* tag;       // short system tag in the header and banner
    const char* wordmark;  // printed into the boot art
    const char* boot_a;    // status line while the carrier is scanned
    const char* boot_b;    // status line while the mark decodes
    const char* boot_c;    // status line during the glitch
    const char* boot_ready;// status line once the boot art is locked
    uint8_t splash;        // SharkSplash
    uint8_t mark;          // SharkMark

    uint16_t accent;       // primary system color
    uint16_t accent_soft;
    uint16_t accent_dim;
    uint16_t content;      // brightest text
    uint16_t label;        // normal labels
    uint16_t grey;         // secondary text
    uint16_t dim;          // inactive
    uint16_t edge;         // borders
    uint16_t surface;      // tile fill
    uint16_t panel;        // recessed fill
    uint16_t warn;         // warnings, toggles, low battery
    uint16_t danger;       // offensive tooling, critical battery
    uint16_t mark_a;       // boot art body
    uint16_t mark_b;       // boot art outline
    uint16_t mark_c;       // boot art sparkle
    uint8_t  style;        // SharkStyle: menu-tile look for this theme
  };

  #define SHARK_THEME_COUNT 4

  extern const SharkThemeDef shark_themes[SHARK_THEME_COUNT];
  extern const SharkThemeDef* shark_theme;
  extern uint8_t shark_theme_index;

  void sharkThemeBegin();            // restore the stored theme (call early)
  void sharkThemeSet(uint8_t index); // apply and persist

  // ---------------------------------------------------------------- palette
  #define WD_CYAN       (shark_theme->accent)
  #define WD_CYAN_SOFT  (shark_theme->accent_soft)
  #define WD_CYAN_DIM   (shark_theme->accent_dim)
  #define WD_WHITE      (shark_theme->content)
  #define WD_BONE       (shark_theme->label)
  #define WD_GREY       (shark_theme->grey)
  #define WD_DIM        (shark_theme->dim)
  #define WD_EDGE       (shark_theme->edge)
  #define WD_SURFACE    (shark_theme->surface)
  #define WD_PANEL      (shark_theme->panel)
  #define WD_AMBER      (shark_theme->warn)
  #define WD_RED        (shark_theme->danger)
  #define SHARK_UI_TAG  (shark_theme->tag)

  // Legacy SHARK names kept as aliases so every older call site follows.
  #define SHARK_GREEN      WD_CYAN
  #define SHARK_GREEN_SOFT WD_BONE
  #define SHARK_GREEN_DIM  WD_EDGE
  #define SHARK_PANEL      WD_PANEL
  #define SHARK_SURFACE    WD_SURFACE

  // ------------------------------------------------------------- primitives
  // Two opposite L-shaped ticks. Cheaper than a full frame and it survives the
  // panel's viewing angle better than a 1 px outline on its own.
  inline void wdCornerTicks(TFT_eSPI& tft,
                            int16_t x,
                            int16_t y,
                            int16_t w,
                            int16_t h,
                            int16_t arm,
                            uint16_t color) {
    if (w < arm * 2 || h < arm * 2)
      return;
    tft.drawFastHLine(x, y, arm, color);
    tft.drawFastVLine(x, y, arm, color);
    tft.drawFastHLine(x + w - arm, y, arm, color);
    tft.drawFastVLine(x + w - 1, y, arm, color);
    tft.drawFastHLine(x, y + h - 1, arm, color);
    tft.drawFastVLine(x, y + h - arm, arm, color);
    tft.drawFastHLine(x + w - arm, y + h - 1, arm, color);
    tft.drawFastVLine(x + w - 1, y + h - arm, arm, color);
  }

  // Flat surface, 1 px edge, corner ticks in the accent color.
  inline void wdPanel(TFT_eSPI& tft,
                      int16_t x,
                      int16_t y,
                      int16_t w,
                      int16_t h,
                      uint16_t fill,
                      uint16_t edge,
                      uint16_t accent) {
    tft.fillRect(x, y, w, h, fill);
    tft.drawRect(x, y, w, h, edge);
    wdCornerTicks(tft, x, y, w, h, SHARK_TICK_ARM, accent);
  }

  // Dashed separator used under the status bar and around data readouts.
  inline void wdRule(TFT_eSPI& tft,
                     int16_t x,
                     int16_t y,
                     int16_t w,
                     uint16_t color) {
    for (int16_t i = 0; i < w; i += 3)
      tft.drawPixel(x + i, y, color);
  }

  // Deterministic 16 bit hash. The splash uses it instead of a frame buffer so
  // every character cell can be recomputed identically on demand.
  inline uint16_t wdHash(uint16_t x, uint16_t y, uint16_t salt) {
    uint32_t h = (uint32_t)x * 73856093UL ^ (uint32_t)y * 19349663UL ^
                 (uint32_t)salt * 83492791UL;
    h ^= h >> 13;
    h *= 2654435761UL;
    h ^= h >> 16;
    return (uint16_t)h;
  }
#endif
