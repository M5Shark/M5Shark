#include "SharkTheme.h"

#if defined(HAS_SCREEN) && defined(MARAUDER_V8)

#include <Preferences.h>

// Colors are RGB565. Each theme keeps the same semantic slots, so a screen
// written for one theme is legible in all of them: accent carries system
// state, warn carries warnings and toggles, danger marks offensive tooling.
const SharkThemeDef shark_themes[SHARK_THEME_COUNT] = {
  // 0 - WATCH_DOGS / ctOS: bone white on black with a single cyan accent.
  {
    "Watch Dogs", "ctOS", "WATCH_DOGS",
    "SCANNING CARRIER", "DECRYPTING PROFILE", "SIGNAL BREAK", "ACCESS GRANTED",
    SHARK_SPLASH_MARK, SHARK_MARK_SKULL,
    0x07FF, 0x063A, 0x036F,
    0xFFFF, 0xC618, 0x8410, 0x39E8, 0x29C8, 0x1904, 0x10A3,
    0xFD20, 0xF967,
    0xFFFF, 0xC618, 0x07FF,
    SHARK_STYLE_CTOS
  },
  // 1 - MATRIX: phosphor green rain, everything reads as terminal output.
  {
    "Matrix", "MTRX", "WAKE UP, NEO",
    "TRACING SIGNAL", "DECODING STREAM", "AGENT DETECTED", "THE MATRIX HAS YOU",
    SHARK_SPLASH_MARK, SHARK_MARK_CYBER,
    0x07E0, 0x05E0, 0x0300,
    // High-contrast phosphor ladder for the physical TFT.  Secondary and dim
    // text previously used 0x03E0/0x0180 against a 0x0120 surface, making the
    // deepest green labels effectively disappear at normal viewing angles.
    0xAFF5, 0x07E0, 0x06E0, 0x04E0, 0x0240, 0x0120, 0x00E0,
    0xFFE0, 0xF800,
    0xAFF5, 0x07E0, 0xFFFF,
    SHARK_STYLE_TERMINAL
  },
  // 2 - CYBER2077: 2077 yellow with cyan and red chromatic aberration.
  {
    "Cyber 2077", "2077", "SAMURAI",
    "BREACH PROTOCOL", "UPLOADING DAEMON", "ICE SPIKE", "WAKE UP SAMURAI",
    SHARK_SPLASH_CHROMA, SHARK_MARK_DEMON,
    0xF7A1, 0xC5E0, 0x6B40,
    0xFFFF, 0xE73C, 0x8410, 0x39A2, 0x4A02, 0x1881, 0x1040,
    0x07FF, 0xF807,
    0xF7A1, 0xFFFF, 0x07FF,
    SHARK_STYLE_NEON
  },
  // 3 - SPIDER: red body over a deep blue chassis, web-line accents.
  {
    "Spider", "WEB", "SPIDER_NET",
    "SPINNING WEB", "TRACING THREADS", "THREAD SNAPPED", "SPIDEY SENSE ON",
    SHARK_SPLASH_MARK, SHARK_MARK_SPIDER,
    0xD904, 0xFACB, 0x7082,
    0xFFFF, 0xC618, 0x8410, 0x39E8, 0x2B5C, 0x0884, 0x0842,
    0xFD20, 0xF800,
    0xD904, 0xFACB, 0x2CDF,
    SHARK_STYLE_BLOCK
  },
};

static_assert(SHARK_THEME_COUNT == 4, "This release exposes exactly four themes");

const SharkThemeDef* shark_theme = &shark_themes[0];
uint8_t shark_theme_index = 0;

namespace {
  Preferences shark_prefs;
  bool shark_prefs_ready = false;
}

void sharkThemeBegin() {
  if (!shark_prefs_ready)
    shark_prefs_ready = shark_prefs.begin("shark_ui", false);

  // Matrix (green terminal) is the factory default theme; an explicitly
  // saved theme choice still wins.
  uint8_t stored = shark_prefs_ready ? shark_prefs.getUChar("theme", 1) : 1;
  if (stored >= SHARK_THEME_COUNT)
    stored = 1;

  shark_theme_index = stored;
  shark_theme = &shark_themes[stored];
}

void sharkThemeSet(uint8_t index) {
  if (index >= SHARK_THEME_COUNT)
    return;

  shark_theme_index = index;
  shark_theme = &shark_themes[index];

  if (!shark_prefs_ready)
    shark_prefs_ready = shark_prefs.begin("shark_ui", false);
  if (shark_prefs_ready)
    shark_prefs.putUChar("theme", index);
}

#endif
