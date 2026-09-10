#include "MenuFunctions.h"
#include "lang_var.h"
#include <algorithm>
#include <vector>

#ifdef MARAUDER_V8
  #include "SharkTheme.h"
  #include "SharkWeb.h"
  #include "SharkRadar.h"
  #include "SharkIceNav.h"
  #include "SharkUI.h"
  #include "SharkPrank.h"
  #include "SharkProfile.h"
  #include "SharkProfileSprites.h"
  #include <Preferences.h>
  #if defined(HAS_BT)
    #include "BadUsb.h"
    #include "SharkChameleon.h"
    #include <NimBLEDevice.h>
  #endif

  // NVS-backed toggle for the home-screen idle wallpaper (default OFF), stored
  // in the shared "shark_ui" namespace next to the theme choice.
  namespace {
    bool sharkLoadIdleWall() {
      Preferences p;
      if (!p.begin("shark_ui", true)) return false;
      bool v = p.getBool("idlewall", false);
      p.end();
      return v;
    }
    void sharkSaveIdleWall(bool v) {
      Preferences p;
      if (!p.begin("shark_ui", false)) return;
      p.putBool("idlewall", v);
      p.end();
    }

    void sharkHudDivider(TFT_eSPI& tft, int16_t x) {
      tft.drawFastVLine(x, 3, STATUS_BAR_WIDTH - 7, WD_EDGE);
    }

    // A filled status lamp is unmistakable on the small TFT and at the steep
    // viewing angle of the enclosure. Inactive features keep a quiet outline;
    // active features get a bright plate with a black high-contrast symbol.
    uint16_t sharkHudLamp(TFT_eSPI& tft,
                          int16_t x,
                          int16_t width,
                          bool active,
                          uint16_t active_color = WD_CYAN) {
      if (active) {
        tft.fillRoundRect(x, 2, width, STATUS_BAR_WIDTH - 4, 2, active_color);
        return TFT_BLACK;
      }
      tft.drawRoundRect(x, 2, width, STATUS_BAR_WIDTH - 4, 2, WD_EDGE);
      return WD_DIM;
    }

    // Compact antenna: the channel number beside it is easier to read than the
    // old CHxx text at the steep viewing angle of the physical enclosure.
    void sharkHudRadio(TFT_eSPI& tft, int16_t x, int16_t y, uint16_t color) {
      tft.fillCircle(x + 5, y + 10, 1, color);
      tft.drawFastVLine(x + 5, y + 5, 5, color);
      tft.drawLine(x + 5, y + 5, x + 2, y + 2, color);
      tft.drawLine(x + 5, y + 5, x + 8, y + 2, color);
      tft.drawPixel(x + 1, y + 1, color);
      tft.drawPixel(x + 9, y + 1, color);
    }

    // SD-card silhouette with the clipped corner and three contact fingers.
    void sharkHudSd(TFT_eSPI& tft, int16_t x, int16_t y, uint16_t color) {
      tft.drawLine(x, y, x + 6, y, color);
      tft.drawLine(x + 6, y, x + 10, y + 4, color);
      tft.drawFastVLine(x + 10, y + 4, 9, color);
      tft.drawFastHLine(x, y + 12, 11, color);
      tft.drawFastVLine(x, y, 13, color);
      tft.drawFastVLine(x + 2, y + 2, 3, color);
      tft.drawFastVLine(x + 5, y + 2, 3, color);
      tft.drawFastVLine(x + 8, y + 3, 2, color);
    }

    // Three angular radio arcs survive the low-resolution TFT better than a
    // font glyph and remain recognizable when dimmed.
    void sharkHudWifi(TFT_eSPI& tft, int16_t x, int16_t y, uint16_t color) {
      tft.drawLine(x, y + 4, x + 3, y + 1, color);
      tft.drawFastHLine(x + 3, y + 1, 7, color);
      tft.drawLine(x + 9, y + 1, x + 12, y + 4, color);
      tft.drawLine(x + 2, y + 7, x + 5, y + 4, color);
      tft.drawFastHLine(x + 5, y + 4, 3, color);
      tft.drawLine(x + 7, y + 4, x + 10, y + 7, color);
      tft.drawLine(x + 5, y + 9, x + 6, y + 8, color);
      tft.drawLine(x + 6, y + 8, x + 7, y + 9, color);
      tft.fillCircle(x + 6, y + 11, 1, color);
    }

    void sharkHudGps(TFT_eSPI& tft, int16_t x, int16_t y, uint16_t color) {
      tft.drawCircle(x + 5, y + 6, 4, color);
      tft.drawFastHLine(x, y + 6, 11, color);
      tft.drawFastVLine(x + 5, y + 1, 11, color);
      tft.fillCircle(x + 5, y + 6, 1, color);
    }

    // Globe = local Web Control service. It is independent from Wi-Fi link
    // state, so the operator can see whether browser control is available.
    void sharkHudWeb(TFT_eSPI& tft, int16_t x, int16_t y, uint16_t color) {
      tft.drawCircle(x + 5, y + 6, 5, color);
      tft.drawFastHLine(x + 1, y + 6, 9, color);
      tft.drawFastVLine(x + 5, y + 1, 11, color);
      tft.drawLine(x + 3, y + 2, x + 2, y + 5, color);
      tft.drawLine(x + 7, y + 2, x + 8, y + 5, color);
      tft.drawLine(x + 2, y + 7, x + 3, y + 10, color);
      tft.drawLine(x + 8, y + 7, x + 7, y + 10, color);
    }

    // Standard Bluetooth rune, drawn directly so it is always visible even
    // when the menu font lacks a Bluetooth character.
    void sharkHudBluetooth(TFT_eSPI& tft,
                           int16_t x,
                           int16_t y,
                           uint16_t color) {
      tft.drawFastVLine(x + 5, y, 13, color);
      tft.drawLine(x + 5, y, x + 9, y + 3, color);
      tft.drawLine(x + 9, y + 3, x + 1, y + 10, color);
      tft.drawLine(x + 1, y + 2, x + 9, y + 9, color);
      tft.drawLine(x + 9, y + 9, x + 5, y + 12, color);
    }

    void sharkHudBattery(TFT_eSPI& tft,
                         int16_t x,
                         int16_t y,
                         int8_t level,
                         uint16_t color) {
      tft.drawRect(x, y, 22, 11, color);
      tft.fillRect(x + 22, y + 3, 2, 5, color);
      const uint8_t blocks = level <= 0 ? 0 : (uint8_t)((level + 24) / 25);
      for (uint8_t i = 0; i < 4; i++) {
        const uint16_t block_color = i < blocks ? color : WD_EDGE;
        tft.fillRect(x + 2 + i * 5, y + 2, 4, 7, block_color);
      }
    }

    // The source sprites stay three-tone so one compact asset pack can serve
    // every theme. This buffer remaps those tones into the active theme just
    // before the small avatar/icon region is pushed to the TFT.
    uint16_t shark_profile_tint_buffer[64 * 64];

    enum SharkPetState : uint8_t {
      SHARK_PET_IDLE_STATE = 0,
      SHARK_PET_SWIM_STATE,
      SHARK_PET_CURIOUS_STATE,
      SHARK_PET_HAPPY_STATE,
      SHARK_PET_SLEEP_STATE,
      SHARK_PET_SCAN_STATE,
      SHARK_PET_ALERT_STATE,
      SHARK_PET_HUNGRY_STATE,
      SHARK_PET_SAD_STATE,
      SHARK_PET_GRUMPY_STATE,
      SHARK_PET_AFFECTION_STATE,
      SHARK_PET_LEVEL_UP_STATE,
      SHARK_PET_STATE_COUNT
    };

    bool sharkSpiderPetTheme() {
      return shark_theme != nullptr && shark_theme->mark == SHARK_MARK_SPIDER;
    }

    bool sharkMatrixPetTheme() {
      // Use the stable Matrix entry so this pet family remains exclusive to
      // the Matrix theme.
      return shark_theme == &shark_themes[1];
    }

    bool sharkCyber2077PetTheme() {
      // Use the stable Cyber 2077 entry so this pet family remains exclusive
      // to the Cyber 2077 theme.
      return shark_theme == &shark_themes[2];
    }

    const uint16_t* sharkPetFrame(SharkPetState state, uint8_t frame) {
      static const uint16_t* const idle[4] = {
        SHARK_PET_IDLE_FRAME_0,
        SHARK_PET_IDLE_FRAME_1,
        SHARK_PET_IDLE_FRAME_2,
        SHARK_PET_IDLE_FRAME_3
      };
      static const uint16_t* const swim[4] = {
        SHARK_PET_SWIM_FRAME_0,
        SHARK_PET_SWIM_FRAME_1,
        SHARK_PET_SWIM_FRAME_2,
        SHARK_PET_SWIM_FRAME_3
      };
      static const uint16_t* const curious[4] = {
        SHARK_PET_CURIOUS_FRAME_0,
        SHARK_PET_CURIOUS_FRAME_1,
        SHARK_PET_CURIOUS_FRAME_2,
        SHARK_PET_CURIOUS_FRAME_3
      };
      static const uint16_t* const happy[4] = {
        SHARK_PET_HAPPY_FRAME_0,
        SHARK_PET_HAPPY_FRAME_1,
        SHARK_PET_HAPPY_FRAME_2,
        SHARK_PET_HAPPY_FRAME_3
      };
      static const uint16_t* const sleep[4] = {
        SHARK_PET_SLEEP_FRAME_0,
        SHARK_PET_SLEEP_FRAME_1,
        SHARK_PET_SLEEP_FRAME_2,
        SHARK_PET_SLEEP_FRAME_3
      };
      static const uint16_t* const scan[4] = {
        SHARK_PET_SCAN_FRAME_0,
        SHARK_PET_SCAN_FRAME_1,
        SHARK_PET_SCAN_FRAME_2,
        SHARK_PET_SCAN_FRAME_3
      };
      static const uint16_t* const alert[4] = {
        SHARK_PET_ALERT_FRAME_0,
        SHARK_PET_ALERT_FRAME_1,
        SHARK_PET_ALERT_FRAME_2,
        SHARK_PET_ALERT_FRAME_3
      };
      static const uint16_t* const hungry[4] = {
        SHARK_PET_HUNGRY_FRAME_0,
        SHARK_PET_HUNGRY_FRAME_1,
        SHARK_PET_HUNGRY_FRAME_2,
        SHARK_PET_HUNGRY_FRAME_3
      };
      static const uint16_t* const sad[4] = {
        SHARK_PET_SAD_FRAME_0,
        SHARK_PET_SAD_FRAME_1,
        SHARK_PET_SAD_FRAME_2,
        SHARK_PET_SAD_FRAME_3
      };
      static const uint16_t* const grumpy[4] = {
        SHARK_PET_GRUMPY_FRAME_0,
        SHARK_PET_GRUMPY_FRAME_1,
        SHARK_PET_GRUMPY_FRAME_2,
        SHARK_PET_GRUMPY_FRAME_3
      };
      static const uint16_t* const affection[4] = {
        SHARK_PET_AFFECTION_FRAME_0,
        SHARK_PET_AFFECTION_FRAME_1,
        SHARK_PET_AFFECTION_FRAME_2,
        SHARK_PET_AFFECTION_FRAME_3
      };
      static const uint16_t* const level_up[4] = {
        SHARK_PET_LEVEL_UP_FRAME_0,
        SHARK_PET_LEVEL_UP_FRAME_1,
        SHARK_PET_LEVEL_UP_FRAME_2,
        SHARK_PET_LEVEL_UP_FRAME_3
      };

      static const uint16_t* const spider_idle[4] = {
        SHARK_SPIDER_PET_IDLE_FRAME_0,
        SHARK_SPIDER_PET_IDLE_FRAME_1,
        SHARK_SPIDER_PET_IDLE_FRAME_2,
        SHARK_SPIDER_PET_IDLE_FRAME_3
      };
      static const uint16_t* const spider_swim[4] = {
        SHARK_SPIDER_PET_SWIM_FRAME_0,
        SHARK_SPIDER_PET_SWIM_FRAME_1,
        SHARK_SPIDER_PET_SWIM_FRAME_2,
        SHARK_SPIDER_PET_SWIM_FRAME_3
      };
      static const uint16_t* const spider_curious[4] = {
        SHARK_SPIDER_PET_CURIOUS_FRAME_0,
        SHARK_SPIDER_PET_CURIOUS_FRAME_1,
        SHARK_SPIDER_PET_CURIOUS_FRAME_2,
        SHARK_SPIDER_PET_CURIOUS_FRAME_3
      };
      static const uint16_t* const spider_happy[4] = {
        SHARK_SPIDER_PET_HAPPY_FRAME_0,
        SHARK_SPIDER_PET_HAPPY_FRAME_1,
        SHARK_SPIDER_PET_HAPPY_FRAME_2,
        SHARK_SPIDER_PET_HAPPY_FRAME_3
      };
      static const uint16_t* const spider_sleep[4] = {
        SHARK_SPIDER_PET_SLEEP_FRAME_0,
        SHARK_SPIDER_PET_SLEEP_FRAME_1,
        SHARK_SPIDER_PET_SLEEP_FRAME_2,
        SHARK_SPIDER_PET_SLEEP_FRAME_3
      };
      static const uint16_t* const spider_scan[4] = {
        SHARK_SPIDER_PET_SCAN_FRAME_0,
        SHARK_SPIDER_PET_SCAN_FRAME_1,
        SHARK_SPIDER_PET_SCAN_FRAME_2,
        SHARK_SPIDER_PET_SCAN_FRAME_3
      };
      static const uint16_t* const spider_alert[4] = {
        SHARK_SPIDER_PET_ALERT_FRAME_0,
        SHARK_SPIDER_PET_ALERT_FRAME_1,
        SHARK_SPIDER_PET_ALERT_FRAME_2,
        SHARK_SPIDER_PET_ALERT_FRAME_3
      };
      static const uint16_t* const spider_hungry[4] = {
        SHARK_SPIDER_PET_HUNGRY_FRAME_0,
        SHARK_SPIDER_PET_HUNGRY_FRAME_1,
        SHARK_SPIDER_PET_HUNGRY_FRAME_2,
        SHARK_SPIDER_PET_HUNGRY_FRAME_3
      };
      static const uint16_t* const spider_sad[4] = {
        SHARK_SPIDER_PET_SAD_FRAME_0,
        SHARK_SPIDER_PET_SAD_FRAME_1,
        SHARK_SPIDER_PET_SAD_FRAME_2,
        SHARK_SPIDER_PET_SAD_FRAME_3
      };
      static const uint16_t* const spider_grumpy[4] = {
        SHARK_SPIDER_PET_GRUMPY_FRAME_0,
        SHARK_SPIDER_PET_GRUMPY_FRAME_1,
        SHARK_SPIDER_PET_GRUMPY_FRAME_2,
        SHARK_SPIDER_PET_GRUMPY_FRAME_3
      };
      static const uint16_t* const spider_affection[4] = {
        SHARK_SPIDER_PET_AFFECTION_FRAME_0,
        SHARK_SPIDER_PET_AFFECTION_FRAME_1,
        SHARK_SPIDER_PET_AFFECTION_FRAME_2,
        SHARK_SPIDER_PET_AFFECTION_FRAME_3
      };
      static const uint16_t* const spider_level_up[4] = {
        SHARK_SPIDER_PET_LEVEL_UP_FRAME_0,
        SHARK_SPIDER_PET_LEVEL_UP_FRAME_1,
        SHARK_SPIDER_PET_LEVEL_UP_FRAME_2,
        SHARK_SPIDER_PET_LEVEL_UP_FRAME_3
      };

      static const uint16_t* const* const modes[SHARK_PET_STATE_COUNT] = {
        idle, swim, curious, happy, sleep, scan,
        alert, hungry, sad, grumpy, affection, level_up
      };
      static const uint16_t* const* const spider_modes[SHARK_PET_STATE_COUNT] = {
        spider_idle, spider_swim, spider_curious, spider_happy,
        spider_sleep, spider_scan, spider_alert, spider_hungry,
        spider_sad, spider_grumpy, spider_affection, spider_level_up
      };
      frame &= 0x03;
      const uint8_t index = (uint8_t)state < SHARK_PET_STATE_COUNT
                              ? (uint8_t)state : 0;
      return (sharkSpiderPetTheme() ? spider_modes : modes)[index][frame];
    }

    const uint8_t* sharkMatrixPetFrame(SharkPetState state, uint8_t frame) {
      static const uint8_t* const idle[4] = {
        SHARK_MATRIX_PET_IDLE_FRAME_0, SHARK_MATRIX_PET_IDLE_FRAME_1,
        SHARK_MATRIX_PET_IDLE_FRAME_2, SHARK_MATRIX_PET_IDLE_FRAME_3
      };
      static const uint8_t* const swim[4] = {
        SHARK_MATRIX_PET_SWIM_FRAME_0, SHARK_MATRIX_PET_SWIM_FRAME_1,
        SHARK_MATRIX_PET_SWIM_FRAME_2, SHARK_MATRIX_PET_SWIM_FRAME_3
      };
      static const uint8_t* const curious[4] = {
        SHARK_MATRIX_PET_CURIOUS_FRAME_0, SHARK_MATRIX_PET_CURIOUS_FRAME_1,
        SHARK_MATRIX_PET_CURIOUS_FRAME_2, SHARK_MATRIX_PET_CURIOUS_FRAME_3
      };
      static const uint8_t* const happy[4] = {
        SHARK_MATRIX_PET_HAPPY_FRAME_0, SHARK_MATRIX_PET_HAPPY_FRAME_1,
        SHARK_MATRIX_PET_HAPPY_FRAME_2, SHARK_MATRIX_PET_HAPPY_FRAME_3
      };
      static const uint8_t* const sleep[4] = {
        SHARK_MATRIX_PET_SLEEP_FRAME_0, SHARK_MATRIX_PET_SLEEP_FRAME_1,
        SHARK_MATRIX_PET_SLEEP_FRAME_2, SHARK_MATRIX_PET_SLEEP_FRAME_3
      };
      static const uint8_t* const scan[4] = {
        SHARK_MATRIX_PET_SCAN_FRAME_0, SHARK_MATRIX_PET_SCAN_FRAME_1,
        SHARK_MATRIX_PET_SCAN_FRAME_2, SHARK_MATRIX_PET_SCAN_FRAME_3
      };
      static const uint8_t* const alert[4] = {
        SHARK_MATRIX_PET_ALERT_FRAME_0, SHARK_MATRIX_PET_ALERT_FRAME_1,
        SHARK_MATRIX_PET_ALERT_FRAME_2, SHARK_MATRIX_PET_ALERT_FRAME_3
      };
      static const uint8_t* const hungry[4] = {
        SHARK_MATRIX_PET_HUNGRY_FRAME_0, SHARK_MATRIX_PET_HUNGRY_FRAME_1,
        SHARK_MATRIX_PET_HUNGRY_FRAME_2, SHARK_MATRIX_PET_HUNGRY_FRAME_3
      };
      static const uint8_t* const sad[4] = {
        SHARK_MATRIX_PET_SAD_FRAME_0, SHARK_MATRIX_PET_SAD_FRAME_1,
        SHARK_MATRIX_PET_SAD_FRAME_2, SHARK_MATRIX_PET_SAD_FRAME_3
      };
      static const uint8_t* const grumpy[4] = {
        SHARK_MATRIX_PET_GRUMPY_FRAME_0, SHARK_MATRIX_PET_GRUMPY_FRAME_1,
        SHARK_MATRIX_PET_GRUMPY_FRAME_2, SHARK_MATRIX_PET_GRUMPY_FRAME_3
      };
      static const uint8_t* const affection[4] = {
        SHARK_MATRIX_PET_AFFECTION_FRAME_0,
        SHARK_MATRIX_PET_AFFECTION_FRAME_1,
        SHARK_MATRIX_PET_AFFECTION_FRAME_2,
        SHARK_MATRIX_PET_AFFECTION_FRAME_3
      };
      static const uint8_t* const level_up[4] = {
        SHARK_MATRIX_PET_LEVEL_UP_FRAME_0,
        SHARK_MATRIX_PET_LEVEL_UP_FRAME_1,
        SHARK_MATRIX_PET_LEVEL_UP_FRAME_2,
        SHARK_MATRIX_PET_LEVEL_UP_FRAME_3
      };
      static const uint8_t* const* const modes[SHARK_PET_STATE_COUNT] = {
        idle, swim, curious, happy, sleep, scan,
        alert, hungry, sad, grumpy, affection, level_up
      };
      frame &= 0x03;
      const uint8_t index = (uint8_t)state < SHARK_PET_STATE_COUNT
                              ? (uint8_t)state : 0;
      return modes[index][frame];
    }

    const uint8_t* sharkCyber2077PetFrame(SharkPetState state, uint8_t frame) {
      static const uint8_t* const idle[4] = {
        SHARK_CYBER2077_PET_IDLE_FRAME_0, SHARK_CYBER2077_PET_IDLE_FRAME_1,
        SHARK_CYBER2077_PET_IDLE_FRAME_2, SHARK_CYBER2077_PET_IDLE_FRAME_3
      };
      static const uint8_t* const swim[4] = {
        SHARK_CYBER2077_PET_SWIM_FRAME_0, SHARK_CYBER2077_PET_SWIM_FRAME_1,
        SHARK_CYBER2077_PET_SWIM_FRAME_2, SHARK_CYBER2077_PET_SWIM_FRAME_3
      };
      static const uint8_t* const curious[4] = {
        SHARK_CYBER2077_PET_CURIOUS_FRAME_0,
        SHARK_CYBER2077_PET_CURIOUS_FRAME_1,
        SHARK_CYBER2077_PET_CURIOUS_FRAME_2,
        SHARK_CYBER2077_PET_CURIOUS_FRAME_3
      };
      static const uint8_t* const happy[4] = {
        SHARK_CYBER2077_PET_HAPPY_FRAME_0, SHARK_CYBER2077_PET_HAPPY_FRAME_1,
        SHARK_CYBER2077_PET_HAPPY_FRAME_2, SHARK_CYBER2077_PET_HAPPY_FRAME_3
      };
      static const uint8_t* const sleep[4] = {
        SHARK_CYBER2077_PET_SLEEP_FRAME_0, SHARK_CYBER2077_PET_SLEEP_FRAME_1,
        SHARK_CYBER2077_PET_SLEEP_FRAME_2, SHARK_CYBER2077_PET_SLEEP_FRAME_3
      };
      static const uint8_t* const scan[4] = {
        SHARK_CYBER2077_PET_SCAN_FRAME_0, SHARK_CYBER2077_PET_SCAN_FRAME_1,
        SHARK_CYBER2077_PET_SCAN_FRAME_2, SHARK_CYBER2077_PET_SCAN_FRAME_3
      };
      static const uint8_t* const alert[4] = {
        SHARK_CYBER2077_PET_ALERT_FRAME_0, SHARK_CYBER2077_PET_ALERT_FRAME_1,
        SHARK_CYBER2077_PET_ALERT_FRAME_2, SHARK_CYBER2077_PET_ALERT_FRAME_3
      };
      static const uint8_t* const hungry[4] = {
        SHARK_CYBER2077_PET_HUNGRY_FRAME_0,
        SHARK_CYBER2077_PET_HUNGRY_FRAME_1,
        SHARK_CYBER2077_PET_HUNGRY_FRAME_2,
        SHARK_CYBER2077_PET_HUNGRY_FRAME_3
      };
      static const uint8_t* const sad[4] = {
        SHARK_CYBER2077_PET_SAD_FRAME_0, SHARK_CYBER2077_PET_SAD_FRAME_1,
        SHARK_CYBER2077_PET_SAD_FRAME_2, SHARK_CYBER2077_PET_SAD_FRAME_3
      };
      static const uint8_t* const grumpy[4] = {
        SHARK_CYBER2077_PET_GRUMPY_FRAME_0,
        SHARK_CYBER2077_PET_GRUMPY_FRAME_1,
        SHARK_CYBER2077_PET_GRUMPY_FRAME_2,
        SHARK_CYBER2077_PET_GRUMPY_FRAME_3
      };
      static const uint8_t* const affection[4] = {
        SHARK_CYBER2077_PET_AFFECTION_FRAME_0,
        SHARK_CYBER2077_PET_AFFECTION_FRAME_1,
        SHARK_CYBER2077_PET_AFFECTION_FRAME_2,
        SHARK_CYBER2077_PET_AFFECTION_FRAME_3
      };
      static const uint8_t* const level_up[4] = {
        SHARK_CYBER2077_PET_LEVEL_UP_FRAME_0,
        SHARK_CYBER2077_PET_LEVEL_UP_FRAME_1,
        SHARK_CYBER2077_PET_LEVEL_UP_FRAME_2,
        SHARK_CYBER2077_PET_LEVEL_UP_FRAME_3
      };
      static const uint8_t* const* const modes[SHARK_PET_STATE_COUNT] = {
        idle, swim, curious, happy, sleep, scan,
        alert, hungry, sad, grumpy, affection, level_up
      };
      frame &= 0x03;
      const uint8_t index = (uint8_t)state < SHARK_PET_STATE_COUNT
                              ? (uint8_t)state : 0;
      return modes[index][frame];
    }

    const char* sharkPetStateName(SharkPetState state) {
      static const char* const names[SHARK_PET_STATE_COUNT] = {
        "IDLE", "SWIM", "CURIOUS", "HAPPY", "SLEEP", "SCAN",
        "ALERT", "HUNGRY", "SAD", "GRUMPY", "AFFECTION", "LEVEL UP"
      };
      static const char* const spider_names[SHARK_PET_STATE_COUNT] = {
        "WATCH", "SWING", "TRACE", "JOY", "HANG", "RADAR",
        "SENSE", "SNACK", "DOWN", "DARK WEB", "HEART WEB", "HERO"
      };
      static const char* const matrix_names[SHARK_PET_STATE_COUNT] = {
        "WAKE", "STREAM", "TRACE", "FREE MIND", "STANDBY", "DECODE",
        "AGENT", "DATA BITE", "LOST", "GLITCH", "LINK", "THE ONE"
      };
      static const char* const cyber2077_names[SHARK_PET_STATE_COUNT] = {
        "IDLE", "RUSH", "BREACH", "JACK IN", "SLEEP", "SCAN",
        "ICE ALERT", "RAM BITE", "FLATLINE", "BERSERK", "SOUL LINK", "LEGEND"
      };
      const uint8_t index = (uint8_t)state < SHARK_PET_STATE_COUNT
                              ? (uint8_t)state : 0;
      if (sharkCyber2077PetTheme())
        return cyber2077_names[index];
      if (sharkMatrixPetTheme())
        return matrix_names[index];
      return (sharkSpiderPetTheme() ? spider_names : names)[index];
    }

    void sharkPushThemedProfileSprite(TFT_eSPI& tft,
                                      int16_t x,
                                      int16_t y,
                                      uint8_t width,
                                      uint8_t height,
                                      const uint16_t* source,
                                      uint16_t background) {
      const uint16_t count = (uint16_t)width * height;
      for (uint16_t i = 0; i < count; i++) {
        const uint16_t pixel = pgm_read_word(source + i);
        shark_profile_tint_buffer[i] = pixel == 0x0000 ? background
                                      : pixel == 0x8410 ? WD_CYAN_SOFT
                                      : WD_CYAN;
      }
      // pushImage() consumes host-memory RGB565 with byte swapping enabled.
      // Without this, Matrix green 0x07E0 reaches the panel as 0xE007 (red),
      // which made every pet and stat icon appear inside a red square.
      const bool previous_swap = tft.getSwapBytes();
      tft.setSwapBytes(true);
      tft.pushImage(x, y, width, height, shark_profile_tint_buffer);
      tft.setSwapBytes(previous_swap);
    }

    void sharkPushThemedPackedProfileSprite(TFT_eSPI& tft,
                                            int16_t x,
                                            int16_t y,
                                            uint8_t width,
                                            uint8_t height,
                                            const uint8_t* source,
                                            uint16_t background,
                                            uint16_t mid_color,
                                            uint16_t bright_color) {
      const uint16_t count = (uint16_t)width * height;
      for (uint16_t i = 0; i < count; i++) {
        const uint8_t packed = pgm_read_byte(source + (i >> 2));
        const uint8_t code = (packed >> (6 - ((i & 0x03) * 2))) & 0x03;
        shark_profile_tint_buffer[i] = code == 0 ? background
                                      : code == 1 ? mid_color
                                      : code == 2 ? bright_color
                                      : WD_WHITE;
      }
      const bool previous_swap = tft.getSwapBytes();
      tft.setSwapBytes(true);
      tft.pushImage(x, y, width, height, shark_profile_tint_buffer);
      tft.setSwapBytes(previous_swap);
    }

    void sharkPushActivePetFrame(TFT_eSPI& tft,
                                 int16_t x,
                                 int16_t y,
                                 SharkPetState state,
                                 uint8_t frame,
                                 uint16_t background) {
      if (sharkMatrixPetTheme()) {
        sharkPushThemedPackedProfileSprite(
            tft, x, y, 64, 64, sharkMatrixPetFrame(state, frame), background,
            WD_CYAN_SOFT, WD_CYAN);
      } else if (sharkCyber2077PetTheme()) {
        sharkPushThemedPackedProfileSprite(
            tft, x, y, 64, 64, sharkCyber2077PetFrame(state, frame), background,
            shark_theme->mark_c, WD_CYAN);
      } else {
        sharkPushThemedProfileSprite(
            tft, x, y, 64, 64, sharkPetFrame(state, frame), background);
      }
    }
  }
#endif

#ifdef HAS_SCREEN

extern const unsigned char menu_icons[][66];

#ifdef HAS_MINI_SCREEN
void MenuFunctions::drawMiniMenuButton(int b, int x, bool selected) {
  if (!current_menu || !current_menu->list || x < 0 || x >= current_menu->list->size())
    return;

  MenuNode mini_node = current_menu->list->get(x);
  bool is_setting_node = (mini_node.icon == SETTINGS && mini_node.color == TFTLIGHTGREY);
  uint16_t color = is_setting_node ? (mini_node.selected ? TFT_GREEN : TFT_RED) : this->getColor(mini_node.color);
  int16_t button_x = KEY_X - (KEY_W / 2);
  int16_t button_y = (KEY_Y + (b * (KEY_H + KEY_SPACING_Y))) - (KEY_H / 2);

  uint16_t background = selected ? (is_setting_node ? TFT_LIGHTGREY : color) : TFT_BLACK;
  uint16_t text_color = (selected && !is_setting_node) ? TFT_BLACK : color;

  display_obj.tft.setFreeFont(NULL);
  display_obj.tft.setTextSize(1);
  display_obj.tft.setTextWrap(false);
  display_obj.tft.fillRect(button_x, button_y - 4, KEY_W, KEY_H, background);
  display_obj.tft.setTextColor(text_color, background);
  display_obj.tft.setCursor(button_x + BUTTON_PADDING, button_y + (KEY_H / 2) - 8);
  display_obj.tft.print(current_menu->list->get(x).name);
}
#endif

#ifdef MARAUDER_V8
void MenuFunctions::drawSharkGridButton(int visible_index, int node_index, bool selected) {
  if (!current_menu || !current_menu->list ||
      visible_index < 0 || visible_index >= BUTTON_SCREEN_LIMIT ||
      node_index < 0 || node_index >= current_menu->list->size())
    return;

  MenuNode node = current_menu->list->get(node_index);
  const bool is_setting_node = (node.icon == SETTINGS && node.color == TFTLIGHTGREY);
  const uint16_t accent = is_setting_node ? (node.selected ? WD_CYAN : WD_DIM)
                                          : this->getColor(node.color);
  const uint16_t background = selected ? accent : WD_SURFACE;
  const uint16_t foreground = selected ? TFT_BLACK
                                       : (node.icon == PROFILE_ICON ? WD_CYAN : WD_BONE);
  const uint16_t tick_color = selected ? TFT_BLACK : accent;
  const int16_t column = visible_index % SHARK_GRID_COLUMNS;
  const int16_t row = visible_index / SHARK_GRID_COLUMNS;
  const int16_t card_x = SHARK_GRID_LEFT + column * (SHARK_GRID_CELL_W + SHARK_GRID_GAP_X);
  const int16_t card_y = SHARK_GRID_TOP + row * (SHARK_GRID_CELL_H + SHARK_GRID_GAP_Y);

  // Per-theme tile style: shape, chrome and label case all come from the active
  // theme, so each theme has its own UI/UX, not just its own palette.
  const SharkStyleParams sp = sharkStyleParams(shark_theme->style);
  const uint16_t border = selected ? accent : (sp.glow ? accent : WD_EDGE);

  if (sp.rounded) {
    display_obj.tft.fillRoundRect(card_x, card_y, SHARK_GRID_CELL_W, SHARK_GRID_CELL_H, 6, background);
    display_obj.tft.drawRoundRect(card_x, card_y, SHARK_GRID_CELL_W, SHARK_GRID_CELL_H, 6, border);
  } else {
    display_obj.tft.fillRect(card_x, card_y, SHARK_GRID_CELL_W, SHARK_GRID_CELL_H, background);
    display_obj.tft.drawRect(card_x, card_y, SHARK_GRID_CELL_W, SHARK_GRID_CELL_H, border);
    if (sp.glow && !selected)
      display_obj.tft.drawRect(card_x + 2, card_y + 2,
                               SHARK_GRID_CELL_W - 4, SHARK_GRID_CELL_H - 4, WD_CYAN_DIM);
  }
  if (sp.ticks)
    wdCornerTicks(display_obj.tft, card_x, card_y,
                  SHARK_GRID_CELL_W, SHARK_GRID_CELL_H, SHARK_TICK_ARM, tick_color);
  if (sp.rail_w > 0)
    display_obj.tft.fillRect(card_x + 2, card_y + 6,
                             sp.rail_w, SHARK_GRID_CELL_H - 12,
                             selected ? TFT_BLACK : accent);
  if (sp.prompt) {
    // Terminal look: a top rule and a ">" prompt before the label.
    display_obj.tft.drawFastHLine(card_x + 4, card_y + 3, SHARK_GRID_CELL_W - 8,
                                  selected ? TFT_BLACK : WD_EDGE);
    display_obj.tft.setTextColor(selected ? TFT_BLACK : accent, background);
    display_obj.tft.setTextDatum(ML_DATUM);
    display_obj.tft.drawString(">", card_x + 4, card_y + SHARK_GRID_CELL_H / 2, 2);
    display_obj.tft.setTextDatum(TL_DATUM);
  }

  const bool has_icon = (node.name != text09 && node.icon != 255);
  if (has_icon) {
    display_obj.tft.drawXBitmap(card_x + 6,
                                card_y + (SHARK_GRID_CELL_H - ICON_H) / 2,
                                menu_icons[node.icon],
                                ICON_W,
                                ICON_H,
                                background,
                                foreground);
  }

  // Selection marker on the reserved right-hand strip.
  if (selected) {
    const int16_t marker_x = card_x + SHARK_GRID_CELL_W - 9;
    const int16_t marker_y = card_y + SHARK_GRID_CELL_H / 2;
    display_obj.tft.fillTriangle(marker_x,
                                 marker_y - 5,
                                 marker_x,
                                 marker_y + 5,
                                 marker_x + 5,
                                 marker_y,
                                 TFT_BLACK);
  }

  const int16_t text_left = card_x + (has_icon ? 30 : (sp.prompt ? 18 : 8));
  const int16_t text_width = SHARK_GRID_CELL_W - (text_left - card_x) - 10;
  display_obj.tft.setFreeFont(NULL);
  display_obj.tft.setTextSize(1);
  display_obj.tft.setTextWrap(false);

  // Label case is part of each theme's look.
  String first_line = node.name;
  if (sp.upper)
    first_line.toUpperCase();
  String second_line = "";
  String original_second_line = "";
  if (display_obj.tft.textWidth(first_line, 2) > text_width) {
    int split_at = -1;
    for (int i = 1; i < first_line.length(); i++) {
      if (first_line.charAt(i) == ' ' &&
          display_obj.tft.textWidth(first_line.substring(0, i), 2) <= text_width)
        split_at = i;
    }

    if (split_at > 0) {
      second_line = first_line.substring(split_at + 1);
      original_second_line = second_line;
      first_line = first_line.substring(0, split_at);
    }
    else {
      while (first_line.length() > 2 &&
             display_obj.tft.textWidth(first_line + ".", 2) > text_width)
        first_line.remove(first_line.length() - 1);
      first_line += ".";
    }

    while (second_line.length() > 2 &&
           display_obj.tft.textWidth(second_line + ".", 2) > text_width)
      second_line.remove(second_line.length() - 1);
    if (second_line.length() > 0 && display_obj.tft.textWidth(second_line, 2) > text_width)
      second_line = "";
    else if (second_line.length() > 0 && second_line != original_second_line)
      second_line += ".";
  }

  display_obj.tft.setTextColor(foreground, background);
  display_obj.tft.setTextDatum(ML_DATUM);
  if (second_line.length() == 0) {
    display_obj.tft.drawString(first_line, text_left, card_y + SHARK_GRID_CELL_H / 2, 2);
  }
  else {
    display_obj.tft.drawString(first_line, text_left, card_y + 17, 2);
    display_obj.tft.drawString(second_line, text_left, card_y + 35, 2);
  }
  display_obj.tft.setTextDatum(TL_DATUM);
}

// One status chip: bright when the subsystem is live, near-black when it is
// not, so the bar reads as a row of indicator lamps instead of a text line.
// Lights the entry of the theme that is currently applied. The grid draws a
// node with `selected` set exactly like the focused tile, so the active theme
// is obvious without a second widget.
void MenuFunctions::markActiveTheme() {
  if (!themeMenu.list)
    return;

  for (int i = 1; i < themeMenu.list->size(); i++) {
    MenuNode node = themeMenu.list->get(i);
    const bool active = ((uint8_t)(i - 1) == shark_theme_index);
    if (node.selected != active) {
      node.selected = active;
      themeMenu.list->set(i, node);
    }
  }
}

// Live screen contents for the Web Control mirror: the current menu title and
// the visible tiles (name, selected, accent), plus paging and whether a tool
// is running. The web renders this into a faithful copy of the device screen,
// which works on hardware that cannot read its framebuffer back over SPI.
String MenuFunctions::screenStateJson() {
  auto hex = [](uint16_t c) -> String {
    uint8_t r = ((c >> 11) & 0x1F) * 255 / 31;
    uint8_t g = ((c >> 5) & 0x3F) * 255 / 63;
    uint8_t b = (c & 0x1F) * 255 / 31;
    char buf[8];
    snprintf(buf, sizeof(buf), "#%02x%02x%02x", r, g, b);
    return String(buf);
  };

  const bool scanning = (wifi_scan_obj.currentScanMode != WIFI_SCAN_OFF) &&
                        (wifi_scan_obj.currentScanMode != WIFI_CONNECTED);

  String title = current_menu ? current_menu->name : String(SHARK_UI_NAME);
  title.replace("\"", "");

  String out = "\"title\":\"" + title + "\"";
  out += ",\"scan\":" + String(scanning ? 1 : 0);

  int size = (current_menu && current_menu->list) ? current_menu->list->size() : 0;
  int pages = size > 0 ? (size + BUTTON_SCREEN_LIMIT - 1) / BUTTON_SCREEN_LIMIT : 1;
  int page = menu_start_index / BUTTON_SCREEN_LIMIT + 1;
  out += ",\"page\":" + String(page) + ",\"pages\":" + String(pages);

  out += ",\"tiles\":[";
  bool first = true;
  for (int i = menu_start_index;
       current_menu && current_menu->list && i < size && i < menu_start_index + BUTTON_SCREEN_LIMIT;
       i++) {
    MenuNode node = current_menu->list->get(i);
    const bool is_setting = (node.icon == SETTINGS && node.color == TFTLIGHTGREY);
    const bool sel = (current_menu->selected == (uint16_t)i) || (!is_setting && node.selected);
    String name = node.name;
    name.replace("\\", " ");
    name.replace("\"", "");
    if (!first)
      out += ",";
    first = false;
    out += "{\"n\":\"" + name + "\",\"s\":" + String(sel ? 1 : 0) +
           ",\"c\":\"" + hex(this->getColor(node.color)) + "\"}";
  }
  out += "]";
  return out;
}

// Remote menu control from the Web Control panel, executed on the main task so
// it shares the normal menu/touch code path. Codes:
//   0 HOME  1 BACK  2 PREV page  3 NEXT page  4 bright-  5 bright+
//   10..17 activate the visible tile 0..7 on the current page
void MenuFunctions::webNavAction(int code) {
  if (code == 0) {
    this->changeMenu(&mainMenu, true);
  }
  else if (code == 1) {
    if (current_menu && current_menu->parentMenu)
      this->changeMenu(current_menu->parentMenu, true);
  }
  else if (code == 2) {
    if (menu_start_index > 0) {
      int ns = menu_start_index - BUTTON_SCREEN_LIMIT;
      if (ns < 0) ns = 0;
      this->buildButtons(current_menu, ns);
      this->displayCurrentMenu(ns);
    }
  }
  else if (code == 3) {
    if (current_menu && current_menu->list &&
        menu_start_index + BUTTON_SCREEN_LIMIT < current_menu->list->size()) {
      int ns = menu_start_index + BUTTON_SCREEN_LIMIT;
      this->buildButtons(current_menu, ns);
      this->displayCurrentMenu(ns);
    }
  }
  else if (code == 4 || code == 5) {
    extern void brightnessSave(uint8_t level);
    extern uint8_t getBrightnessLevel();
    int lvl = (int)getBrightnessLevel() + (code == 5 ? 1 : -1);
    if (lvl < 0) lvl = 0;
    if (lvl > 9) lvl = 9;
    brightnessSave((uint8_t)lvl);
  }
  else if (code >= 10 && code <= 17) {
    int idx = menu_start_index + (code - 10);
    if (current_menu && current_menu->list && idx < current_menu->list->size()) {
      MenuNode node = current_menu->list->get(idx);
      if (node.callable)
        node.callable();
    }
  }
}

// Launches a scan mode requested from Web Control (foreground or background).
// StartScan tears the AP down for us, so this only sets up the screen and the
// per-mode render helpers.
void MenuFunctions::startWebTool(int mode) {
  // This tool is about to take the radio, so Web Control will drop. Remember to
  // bring it back automatically once the user returns to the home screen.
  shark_web_obj.resume_pending = true;
  switch (mode) {
    case WIFI_SCAN_CHAN_ACT:
      this->startWifiToolUI(WIFI_SCAN_CHAN_ACT, TFT_CYAN);
      break;
    case WIFI_SCAN_PACKET_RATE:
      this->startWifiToolUI(WIFI_SCAN_PACKET_RATE, TFT_ORANGE);
      break;
    case SHARK_RUVIEW_CSI:
      shark_web_obj.stop();  // RuView needs the radio to join Wi-Fi / read CSI
      this->startRuView();   // handles the join-if-needed flow
      break;
    case SHARK_WEB_APP_WIFI_RADAR:
      // Radar screens are blocking and own the radio, so drop Web Control first
      // and return to the menu when the user touches out of the radar.
      shark_web_obj.stop();
      shark_radar_obj.runWifi();
      this->changeMenu(&sharkDefenseMenu, true);
      break;
    #ifdef HAS_GPS
      case SHARK_WEB_APP_GPS_RADAR:
        shark_web_obj.stop();
        shark_radar_obj.runGps();
        this->changeMenu(&gpsMenu, true);
        break;
    #endif
    // Pranks: drop Web Control, run the blocking prank screen on the device
    // (its own START/STOP/BACK), then return to the Prank menu. resume_pending
    // (set above) auto-restarts Web Control once we are back on a menu.
    case SHARK_WEB_APP_MONKEY:        shark_web_obj.stop(); shark_prank_obj.monkeyWifi();     this->changeMenu(&wifiPrankMenu, true); break;
    case SHARK_WEB_APP_FUNNY_HOTSPOT: shark_web_obj.stop(); shark_prank_obj.funnyHotspot();   this->changeMenu(&wifiPrankMenu, true); break;
    case SHARK_WEB_APP_SSID_ROTATOR:  shark_web_obj.stop(); shark_prank_obj.ssidRotator();    this->changeMenu(&wifiPrankMenu, true); break;
    case SHARK_WEB_APP_GUEST_COUNTER: shark_web_obj.stop(); shark_prank_obj.guestCounter();   this->changeMenu(&wifiPrankMenu, true); break;
    case SHARK_WEB_APP_PRANK_PORTAL:  shark_web_obj.stop(); shark_prank_obj.prankPortal();    this->changeMenu(&wifiPrankMenu, true); break;
    case SHARK_WEB_APP_MEME_PORTAL:   shark_web_obj.stop(); shark_prank_obj.memePortal();     this->changeMenu(&wifiPrankMenu, true); break;
    case SHARK_WEB_APP_CUSTOM_PORTAL: shark_web_obj.stop(); shark_prank_obj.customPortal();   this->changeMenu(&wifiPrankMenu, true); break;
    #ifdef HAS_BT
      case SHARK_WEB_APP_BLE_NAME:    shark_web_obj.stop(); shark_prank_obj.bleNameBroadcast(); this->changeMenu(&btPrankMenu, true); break;
      case SHARK_WEB_APP_BLE_ROTATOR: shark_web_obj.stop(); shark_prank_obj.bleNameRotator();   this->changeMenu(&btPrankMenu, true); break;
      case SHARK_WEB_APP_BLE_ADVERT:  shark_web_obj.stop(); shark_prank_obj.bleAdvertiser();    this->changeMenu(&btPrankMenu, true); break;
      case SHARK_WEB_APP_BLE_BEACON:  shark_web_obj.stop(); shark_prank_obj.bleBeacon();        this->changeMenu(&btPrankMenu, true); break;
      case SHARK_WEB_APP_BLE_RADAR:   shark_web_obj.stop(); shark_prank_obj.bleRadar();         this->changeMenu(&btPrankMenu, true); break;
      case SHARK_WEB_APP_BLE_HUNT:    shark_web_obj.stop(); shark_prank_obj.bleHunt();          this->changeMenu(&btPrankMenu, true); break;
    #endif
    default:
      if (this->isWifiToolUiMode((uint8_t)mode))
        this->startWifiToolUI((uint8_t)mode, TFT_GREEN);
      else {
        display_obj.clearScreen();
        this->drawStatusBar();
        wifi_scan_obj.StartScan((uint8_t)mode, TFT_GREEN);
      }
      break;
  }
}

// Themed one-line result screen with a BACK key. Shared by the WiFi General
// clear actions so no legacy println flow remains there.
void MenuFunctions::sharkNotice(const char* title, const String& line) {
  TFT_eSPI& tft = display_obj.tft;
  tft.fillScreen(TFT_BLACK);
  this->drawStatusBar();
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(WD_CYAN, TFT_BLACK);
  tft.drawString("// RESULT", 6, STATUS_BAR_WIDTH + 9, 2);
  tft.setTextDatum(TL_DATUM);
  tft.drawFastHLine(0, STATUS_BAR_WIDTH + 17, tft.width(), WD_EDGE);
  wdPanel(tft, 10, 110, tft.width() - 20, 84, WD_SURFACE, WD_EDGE, WD_CYAN);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(WD_BONE, WD_SURFACE);
  tft.drawString(title, tft.width() / 2, 136, 2);
  tft.setTextColor(WD_GREY, WD_SURFACE);
  tft.drawString(line, tft.width() / 2, 166, 1);
  tft.setTextDatum(TL_DATUM);
  wdPanel(tft, 10, 288, tft.width() - 20, 24, WD_CYAN, WD_CYAN, WD_CYAN);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, WD_CYAN);
  tft.drawString("BACK", tft.width() / 2, 300, 2);
  tft.setTextDatum(TL_DATUM);
  uint16_t tx, ty;
  while (!display_obj.updateTouch(&tx, &ty, 350)) delay(20);
  while (display_obj.updateTouch(&tx, &ty, 350)) delay(10);
}

// WiFi General -> SSID generator. The old flow opened an empty back-only
// menu and printed three legacy text lines with a fixed count of 20; this
// screen exposes the count (steppers + presets), generation, custom SSIDs
// via the touch keyboard, list clearing, and live list stats.
// ---- Scan AP studio ---------------------------------------------------------
// Active dual-band access-point survey with full operator detail. Results
// also populate the shared access_points list, so rows selected here are
// the targets the attack tools (deauth targeted, beacon list, Evil Portal
// clone, Packet Count) already consume.

namespace {
  const char* scanApSecShort(wifi_auth_mode_t a) {
    switch (a) {
      case WIFI_AUTH_OPEN: return "OPEN";
      case WIFI_AUTH_WEP: return "WEP";
      case WIFI_AUTH_WPA_PSK: return "WPA";
      case WIFI_AUTH_WPA2_PSK: return "WPA2";
      case WIFI_AUTH_WPA_WPA2_PSK: return "MIX";
      case WIFI_AUTH_WPA3_PSK: return "WPA3";
      case WIFI_AUTH_WPA2_WPA3_PSK: return "2/3";
      default: return "SEC";
    }
  }

  String scanApMacString(const uint8_t* b) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             b[0], b[1], b[2], b[3], b[4], b[5]);
    return String(buf);
  }
}

void MenuFunctions::scanApStudio() {
  extern LinkedList<AccessPoint>* access_points;
  TFT_eSPI& tft = display_obj.tft;
  const int16_t w = tft.width();
  uint8_t page = 0;
  bool scanning = true;

  // Station mode + async dual-band scan (all channels, include hidden).
  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect(false);
  WiFi.scanDelete();
  WiFi.scanNetworks(true, true, false, 120);

  const uint8_t PER_PAGE = 6;
  const int16_t ROW_Y = 88, ROW_H = 36;

  auto draw = [&]() {
    tft.fillScreen(TFT_BLACK);
    tft.setFreeFont(NULL);
    tft.setTextWrap(false);
    tft.setTextSize(1);
    this->drawStatusBar();
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString("// SCAN AP", 6, STATUS_BAR_WIDTH + 9, 2);
    tft.setTextDatum(TL_DATUM);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 17, w, WD_EDGE);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 17, 66, WD_CYAN);

    int n = WiFi.scanComplete();
    const bool done = !scanning && n >= 0 ? true : (n >= 0);
    if (n == WIFI_SCAN_RUNNING || (scanning && n == WIFI_SCAN_RUNNING)) n = 0;
    if (n < 0) n = 0;

    // Tiles: networks found / distinct channels / strongest signal.
    uint8_t chans = 0, seen_ch[64] = {0};
    int8_t best = -128;
    for (int i = 0; i < n; i++) {
      const uint8_t ch = WiFi.channel(i);
      bool newch = true;
      for (int c = 0; c < chans; c++) if (seen_ch[c] == ch) newch = false;
      if (newch && chans < 64) seen_ch[chans++] = ch;
      if (WiFi.RSSI(i) > best) best = WiFi.RSSI(i);
    }
    const char* labels[3] = {"NETWORKS", "CHANNELS", "STRONGEST"};
    const String values[3] = {String(n), String(chans),
                              n ? String(best) + "dB" : String("-")};
    for (uint8_t i = 0; i < 3; i++) {
      const int16_t x = 2 + i * 80;
      wdPanel(tft, x, 39, 76, 42, WD_SURFACE, WD_EDGE, WD_CYAN);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_DIM, WD_SURFACE);
      tft.drawString(labels[i], x + 7, 44, 1);
      tft.setTextDatum(MR_DATUM);
      tft.setTextColor(WD_BONE, WD_SURFACE);
      tft.drawString(values[i], x + 69, 66, 2);
    }

    // Rows (paged).
    if (scanning && WiFi.scanComplete() == WIFI_SCAN_RUNNING) {
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(WD_AMBER, TFT_BLACK);
      tft.drawString("SCANNING RADIO SPECTRUM...", w / 2, 150, 2);
      tft.setTextColor(WD_GREY, TFT_BLACK);
      tft.drawString("active sweep, all channels, incl. hidden", w / 2, 175, 1);
      tft.setTextDatum(TL_DATUM);
    }
    else if (n == 0) {
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(WD_DIM, TFT_BLACK);
      tft.drawString("NO NETWORKS FOUND", w / 2, 150, 2);
      tft.setTextDatum(TL_DATUM);
    }

    uint16_t selected_count = 0;
    for (int i = 0; access_points && i < access_points->size(); i++)
      if (access_points->get(i).selected) selected_count++;

    const uint16_t pages = (n + PER_PAGE - 1) / PER_PAGE > 0 ?
                           (n + PER_PAGE - 1) / PER_PAGE : 1;
    if (page >= pages) page = pages - 1;
    for (uint8_t r = 0; r < PER_PAGE; r++) {
      const int idx = page * PER_PAGE + r;
      if (idx >= n) break;
      // Mirror of the shared-list selection state (matched by BSSID).
      bool sel = false;
      uint8_t* b = (uint8_t*)WiFi.BSSID(idx);
      for (int i = 0; access_points && i < access_points->size(); i++) {
        const AccessPoint ap = access_points->get(i);
        if (memcmp(ap.bssid, b, 6) == 0) { sel = ap.selected; break; }
      }
      const int16_t y = ROW_Y + r * ROW_H;
      const bool is_open = WiFi.encryptionType(idx) == WIFI_AUTH_OPEN;
      wdPanel(tft, 4, y, w - 8, ROW_H - 2,
              sel ? WD_SURFACE : WD_PANEL, WD_EDGE,
              sel ? WD_AMBER : (is_open ? WD_AMBER : WD_CYAN));
      // Selection box.
      tft.drawRect(9, y + 11, 14, 14, sel ? WD_AMBER : WD_EDGE);
      if (sel) tft.fillRect(11, y + 13, 10, 10, WD_AMBER);
      // SSID.
      String ssid = WiFi.SSID(idx);
      if (!ssid.length()) ssid = "<HIDDEN>";
      while (ssid.length() > 3 && tft.textWidth(ssid, 2) > 118) ssid.remove(ssid.length() - 1);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(sel ? WD_AMBER : WD_BONE, sel ? WD_SURFACE : WD_PANEL);
      tft.drawString(ssid, 28, y + 4, 2);
      // Detail block: channel/band, RSSI, security.
      const uint8_t ch = WiFi.channel(idx);
      const int8_t rr = WiFi.RSSI(idx);
      tft.setTextColor(WD_DIM, sel ? WD_SURFACE : WD_PANEL);
      tft.drawString(String("CH ") + String(ch) + (ch > 14 ? " 5G" : " 2.4G"),
                     152, y + 4, 1);
      tft.setTextColor(is_open ? WD_AMBER : WD_GREY, sel ? WD_SURFACE : WD_PANEL);
      tft.drawString(String(scanApSecShort(WiFi.encryptionType(idx))) + "  " +
                     String(rr) + "dBm", 152, y + 18, 1);
      // RSSI bar (-30..-95 mapped).
      const int16_t bw = (int16_t)constrain((rr + 95) * 76 / 65, 0, 76);
      tft.fillRect(152, y + 30, 76, 2, WD_EDGE);
      if (bw > 0) tft.fillRect(152, y + 30, bw, 2, rr > -60 ? WD_AMBER : WD_CYAN);
    }

    // Footer: selection state + keys.
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(selected_count ? WD_AMBER : WD_DIM, TFT_BLACK);
    tft.drawString(String(selected_count) + " SELECTED - TARGETS ATTACK TOOLS",
                   6, ROW_Y + PER_PAGE * ROW_H + 4, 1);
    const int16_t ky = 296;
    wdPanel(tft, 4, ky, 70, 22, WD_CYAN, WD_CYAN, WD_CYAN);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, WD_CYAN);
    tft.drawString("RESCAN", 39, ky + 11, 1);
    wdPanel(tft, 80, ky, 70, 22, WD_SURFACE, WD_GREY, WD_GREY);
    tft.setTextColor(WD_GREY, WD_SURFACE);
    tft.drawString(String(page + 1) + "/" + String(pages), 115, ky + 11, 1);
    wdPanel(tft, 166, ky, 70, 22, WD_SURFACE, WD_GREY, WD_GREY);
    tft.drawString("BACK", 201, ky + 11, 1);
    tft.setTextDatum(TL_DATUM);
  };

  auto syncToList = [&]() {
    // Rebuild the shared access_points list from the fresh scan so selected
    // rows become real attack targets.
    if (!access_points) return;
    const int n = WiFi.scanComplete();
    if (n <= 0) return;
    wifi_scan_obj.clearList(CLEAR_APS);
    for (int i = 0; i < n; i++) {
      AccessPoint ap;
      ap.essid = WiFi.SSID(i);
      ap.channel = WiFi.channel(i);
      memcpy(ap.bssid, WiFi.BSSID(i), 6);
      ap.selected = false;
      ap.rssi = WiFi.RSSI(i);
      ap.sec = WiFi.encryptionType(i);
      ap.stations = new LinkedList<uint16_t>();
      ap.packets = 0;
      ap.wps = false;
      ap.man = "";
      access_points->add(ap);
    }
  };

  uint32_t last_poll = 0;
  draw();
  while (true) {
    const int16_t st = WiFi.scanComplete();
    if (scanning && st != WIFI_SCAN_RUNNING) {
      scanning = false;
      syncToList();
      draw();
    }
    uint16_t tx, ty;
    if (display_obj.updateTouch(&tx, &ty, 350)) {
      while (display_obj.updateTouch(&tx, &ty, 350)) delay(10);
      if (ty >= 294) {
        if (tx < 76) {              // RESCAN
          WiFi.scanDelete();
          WiFi.scanNetworks(true, true, false, 120);
          scanning = true;
          page = 0;
          draw();
        }
        else if (tx < 152) {        // page
          const int16_t st_pg = WiFi.scanComplete();
          const int n = st_pg > 0 ? (int)st_pg : 0;
          const uint16_t pages = (n + PER_PAGE - 1) / PER_PAGE > 0 ?
                                 (n + PER_PAGE - 1) / PER_PAGE : 1;
          if (pages > 1) page = (page + 1) % pages;
          draw();
        }
        else {                      // BACK
          WiFi.scanDelete();
          return;
        }
      }
      else if (!scanning && ty >= ROW_Y && ty < ROW_Y + PER_PAGE * ROW_H) {
        const int idx = page * PER_PAGE + (ty - ROW_Y) / ROW_H;
        const int n = WiFi.scanComplete();
        if (idx < n && access_points) {
          const uint8_t* b = WiFi.BSSID(idx);
          for (int i = 0; i < access_points->size(); i++) {
            AccessPoint ap = access_points->get(i);
            if (memcmp(ap.bssid, b, 6) == 0) {
              ap.selected = !ap.selected;
              access_points->set(i, ap);
              break;
            }
          }
          draw();
        }
      }
    }
    // Refresh while the async scan runs (spinner line).
    if (scanning && millis() - last_poll > 1200) {
      last_poll = millis();
      draw();
    }
    delay(20);
  }
}

void MenuFunctions::ssidStudio() {
  extern LinkedList<ssid>* ssids;
  TFT_eSPI& tft = display_obj.tft;
  const int16_t w = tft.width();
  uint16_t count = 20;
  String status = "READY";

  const uint16_t presets[5] = {10, 50, 100, 500, 1000};

  auto draw = [&]() {
    tft.fillScreen(TFT_BLACK);
    tft.setFreeFont(NULL);
    tft.setTextWrap(false);
    tft.setTextSize(1);
    this->drawStatusBar();
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString("// SSID GENERATOR", 6, STATUS_BAR_WIDTH + 9, 2);
    tft.setTextDatum(TL_DATUM);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 17, w, WD_EDGE);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 17, 66, WD_CYAN);

    // Count panel with -/+ steppers.
    wdPanel(tft, 8, 40, w - 16, 56, WD_SURFACE, WD_EDGE, WD_CYAN);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_SURFACE);
    tft.drawString("GENERATE COUNT", 16, 48, 1);
    tft.setTextColor(WD_BONE, WD_SURFACE);
    tft.drawString(String(count), 16, 62, 4);
    wdPanel(tft, w - 96, 44, 40, 48, WD_PANEL, WD_EDGE, WD_AMBER);
    wdPanel(tft, w - 52, 44, 40, 48, WD_PANEL, WD_EDGE, WD_AMBER);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WD_AMBER, WD_PANEL);
    tft.drawString("-", w - 76, 68, 2);
    tft.drawString("+", w - 32, 68, 2);

    // Presets.
    for (uint8_t i = 0; i < 5; i++) {
      const int16_t x = 8 + i * 46;
      wdPanel(tft, x, 104, 42, 28, WD_PANEL, WD_EDGE,
              count == presets[i] ? WD_CYAN : WD_EDGE);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(count == presets[i] ? WD_CYAN : WD_GREY, WD_PANEL);
      tft.drawString(String(presets[i]), x + 21, 118, 1);
    }

    // Actions.
    wdPanel(tft, 8, 140, 108, 52, WD_CYAN, WD_CYAN, WD_CYAN);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, WD_CYAN);
    tft.drawString("GENERATE", 62, 166, 2);
    wdPanel(tft, 124, 140, 108, 52, WD_PANEL, WD_EDGE, WD_CYAN);
    tft.setTextColor(WD_BONE, WD_PANEL);
    tft.drawString("ADD SSID", 178, 166, 2);

    wdPanel(tft, 8, 198, w - 16, 44, WD_PANEL, WD_EDGE, WD_AMBER);
    tft.setTextColor(WD_AMBER, WD_PANEL);
    tft.drawString("CLEAR SSID LIST", w / 2, 220, 2);

    // Stats + last action.
    wdPanel(tft, 8, 248, w - 16, 30, WD_SURFACE, WD_EDGE, WD_EDGE);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_SURFACE);
    tft.drawString("LIST: " + String(ssids->size()) + " SSIDS", 16, 257, 1);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(WD_GREY, WD_SURFACE);
    tft.drawString(status, w - 16, 257, 1);
    tft.setTextDatum(TL_DATUM);

    wdPanel(tft, 8, 284, w - 16, 32, WD_SURFACE, WD_EDGE, WD_GREY);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WD_GREY, WD_SURFACE);
    tft.drawString("BACK", w / 2, 300, 2);
    tft.setTextDatum(TL_DATUM);
  };

  draw();
  uint16_t tx, ty;
  while (true) {
    if (!display_obj.updateTouch(&tx, &ty, 350)) { delay(20); continue; }
    while (display_obj.updateTouch(&tx, &ty, 350)) delay(10);

    if (ty >= 284)
      return;                                   // BACK
    else if (ty >= 248 || ty < 40) { }          // stats strip: dead zone
    else if (ty >= 198) {                       // CLEAR
      const int n = wifi_scan_obj.clearList(CLEAR_SSID);
      status = "CLEARED " + String(n);
      draw();
    }
    else if (ty >= 140) {
      if (tx < 120) {                           // GENERATE
        const int made = wifi_scan_obj.generateSSIDs(count);
        status = "GENERATED " + String(made);
      }
      else {                                    // ADD SSID
        char buf[33] = {0};
        if (keyboardInput(buf, sizeof(buf), "ADD SSID") && buf[0]) {
          wifi_scan_obj.addSSID(String(buf));
          status = "ADDED 1";
        }
      }
      draw();
    }
    else if (ty >= 104) {                       // presets
      count = presets[min(4, (tx - 8) / 46)];
      draw();
    }
    else {                                      // steppers
      if (tx >= w - 104 && tx < w - 52) count = count > 10 ? count - 10 : 1;
      else if (tx >= w - 52) count = count < 1990 ? count + 10 : 2000;
      draw();
    }
  }
}

// Local companion to the browser SD manager. It gives a readable storage
// summary on the small panel and points the user to Web Control for file-level
// upload/download/delete operations.
void MenuFunctions::cardControlScreen() {
  TFT_eSPI& tft = display_obj.tft;
  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(NULL);
  tft.setTextWrap(false);
  tft.fillRect(0, 0, 66, 20, WD_CYAN);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, WD_CYAN);
  tft.drawString(SHARK_UI_NAME, 33, 10, 2);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(WD_GREY, TFT_BLACK);
  tft.drawString("// CARD CONTROL", 72, 10, 1);
  tft.drawFastHLine(0, 21, tft.width(), WD_EDGE);

  #ifdef HAS_SD
    const bool ready = sd_obj.supported;
    uint64_t total_mb = ready ? SD.totalBytes() / (1024ULL * 1024ULL) : 0;
    uint64_t used_mb = ready ? SD.usedBytes() / (1024ULL * 1024ULL) : 0;
  #else
    const bool ready = false;
    uint64_t total_mb = 0, used_mb = 0;
  #endif

  wdPanel(tft, 10, 42, tft.width() - 20, 116, WD_SURFACE, WD_EDGE, ready ? WD_CYAN : WD_RED);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WD_DIM, WD_SURFACE);
  tft.drawString("STATE", 20, 54, 1);
  tft.drawString("TOTAL", 20, 88, 1);
  tft.drawString("USED", 20, 122, 1);
  tft.setTextColor(ready ? WD_CYAN : WD_RED, WD_SURFACE);
  tft.drawString(ready ? "MOUNTED" : "OFFLINE", 84, 50, 4);
  tft.setTextColor(WD_WHITE, WD_SURFACE);
  tft.drawString(String((uint32_t)total_mb) + " MB", 84, 84, 4);
  tft.setTextColor(WD_AMBER, WD_SURFACE);
  tft.drawString(String((uint32_t)used_mb) + " MB", 84, 118, 4);

  wdPanel(tft, 10, 174, tft.width() - 20, 76, WD_PANEL, WD_EDGE, WD_CYAN);
  tft.setTextColor(WD_BONE, WD_PANEL);
  tft.drawString("OPEN WEB CONTROL", 20, 186, 2);
  tft.setTextColor(WD_GREY, WD_PANEL);
  tft.drawString("Upload, download, preview", 20, 210, 1);
  tft.drawString("and delete SD card files.", 20, 224, 1);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(WD_AMBER, TFT_BLACK);
  tft.drawString("TOUCH TO RETURN", tft.width() / 2, 294, 2);
  tft.setTextDatum(TL_DATUM);

  uint16_t x, y;
  while (!display_obj.updateTouch(&x, &y)) delay(20);
  while (display_obj.updateTouch(&x, &y)) delay(10);
}

void MenuFunctions::profileScreen() {
  sharkProfileVisit();
  TFT_eSPI& tft = display_obj.tft;
  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  // Keep the same live HUD used by every tool. Its change-driven refresh
  // exposes real Wi-Fi/BLE/GPS/SD/battery state without a one-second blink.
  this->drawSharkTopBar(true);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WD_CYAN, TFT_BLACK);
  tft.drawString("// OPERATOR PROFILE", 6, STATUS_BAR_WIDTH + 7, 2);
  tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
  tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 72, WD_CYAN);

  const uint8_t level = sharkProfileLevel();
  const uint8_t progress = sharkProfileLevelProgress();
  const uint16_t rank_color = WD_CYAN;

  // Pick one meaningful mood and hold it instead of running a slideshow.
  // Battery state has priority. With a GPS fix the UTC clock and longitude
  // supply an approximate local hour; without GPS, a slow 15-minute uptime
  // block is the deterministic fallback. Touch moods temporarily override it.
  auto scheduledPetState = [&](uint32_t now) -> SharkPetState {
    #ifdef HAS_BATTERY
      const int8_t battery = battery_obj.battery_level;
      if (battery >= 0 && battery < 10) return SHARK_PET_SAD_STATE;
      if (battery >= 10 && battery < 20) return SHARK_PET_GRUMPY_STATE;
    #endif

    #ifdef HAS_GPS
      if (gps_obj.getFixStatus()) {
        const String utc = gps_obj.getDatetime();
        if (utc.length() >= 13) {
          const int utc_hour = utc.substring(11, 13).toInt();
          if (utc_hour >= 0 && utc_hour <= 23) {
            const int32_t longitude = gps_obj.getLonInt();
            const int32_t rounded_longitude = longitude >= 0
                ? longitude + 7500000L : longitude - 7500000L;
            const int timezone_hours = (int)(rounded_longitude / 15000000L);
            const int hour = (utc_hour + timezone_hours + 24) % 24;
            if (hour < 6 || hour >= 23) return SHARK_PET_SLEEP_STATE;
            if (hour < 8) return SHARK_PET_HUNGRY_STATE;
            if (hour < 11) return SHARK_PET_ALERT_STATE;
            if (hour < 13) return SHARK_PET_HUNGRY_STATE;
            if (hour < 17) return SHARK_PET_SCAN_STATE;
            if (hour < 20) return SHARK_PET_SWIM_STATE;
            if (hour < 22) return SHARK_PET_HAPPY_STATE;
            return SHARK_PET_CURIOUS_STATE;
          }
        }
      }
    #endif

    static const SharkPetState fallback[] = {
      SHARK_PET_IDLE_STATE, SHARK_PET_CURIOUS_STATE,
      SHARK_PET_SWIM_STATE, SHARK_PET_SCAN_STATE,
      SHARK_PET_HAPPY_STATE, SHARK_PET_HUNGRY_STATE,
      SHARK_PET_ALERT_STATE, SHARK_PET_SLEEP_STATE
    };
    const uint32_t block = now / 900000UL;
    return fallback[block % (sizeof(fallback) / sizeof(fallback[0]))];
  };

  SharkPetState pet_state = scheduledPetState(millis());
  uint8_t avatar_frame = 0;

  auto drawPet = [&](bool draw_label) {
    sharkPushActivePetFrame(tft, 9, 55, pet_state, avatar_frame, WD_PANEL);
    if (draw_label) {
      tft.fillRect(80, 54, 150, 22, WD_PANEL);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_WHITE, WD_PANEL);
      const char* pet_prefix = sharkCyber2077PetTheme() ? "2077 // "
                               : sharkMatrixPetTheme() ? "MTRX // "
                               : sharkSpiderPetTheme() ? "WEB // " : "PET // ";
      tft.drawString(String(pet_prefix)
                         + sharkPetStateName(pet_state),
                     80, 57, 2);
    }
  };

  wdPanel(tft, 4, 50, 232, 74, WD_PANEL, WD_EDGE, rank_color);
  drawPet(true);
  tft.setTextColor(WD_GREY, WD_PANEL);
  tft.drawString(String("RANK  ") + sharkProfileRank(), 80, 80, 1);
  sharkPushThemedProfileSprite(tft, 80, 94, 20, 20,
                               SHARK_PROFILE_LEVEL, WD_PANEL);
  tft.setTextColor(WD_WHITE, WD_PANEL);
  tft.drawString(String("LEVEL ") + level, 105, 97, 2);

  const int16_t stat_y = 130;
  const uint16_t* stat_icons[3] = {
    SHARK_PROFILE_XP, SHARK_PROFILE_ACTIVITY, SHARK_PROFILE_RANK
  };
  const char* stat_labels[3] = {"XP", "ACTIVE", "SESSIONS"};
  const String stat_values[3] = {
    String(sharkProfileXp()),
    String(sharkProfileMinutes()) + "m",
    String(sharkProfileSessions())
  };
  for (uint8_t i = 0; i < 3; i++) {
    const int16_t x = 2 + i * 80;
    wdPanel(tft, x, stat_y, 76, 45, WD_PANEL, WD_EDGE, WD_CYAN_DIM);
    sharkPushThemedProfileSprite(tft, x + 6, stat_y + 12, 20, 20,
                                 stat_icons[i], WD_PANEL);
    tft.setTextColor(WD_GREY, WD_PANEL);
    tft.drawString(stat_labels[i], x + 31, stat_y + 6, 1);
    tft.setTextColor(WD_WHITE, WD_PANEL);
    String value = stat_values[i];
    while (value.length() > 1 && tft.textWidth(value, 2) > 39)
      value.remove(value.length() - 1);
    tft.drawString(value, x + 31, stat_y + 21, 2);
  }

  wdPanel(tft, 6, 182, 228, 50, WD_PANEL, WD_EDGE, WD_CYAN);
  tft.setTextColor(WD_WHITE, WD_PANEL);
  tft.drawString("CYBER USER LEVEL", 14, 189, 1);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(WD_GREY, WD_PANEL);
  tft.drawString(String(progress) + " / 100 XP", 226, 189, 1);
  tft.setTextDatum(TL_DATUM);
  tft.fillRect(14, 207, 212, 13, WD_SURFACE);
  tft.drawRect(14, 207, 212, 13, WD_EDGE);
  const int16_t fill_w = (int16_t)((uint32_t)208 * progress / 100UL);
  if (fill_w > 0) tft.fillRect(16, 209, fill_w, 9, rank_color);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(WD_BONE, TFT_BLACK);
  tft.drawString("TOUCH PET  //  +1 XP / ACTIVE MIN", 120, 249, 1);
  tft.setTextColor(WD_GREY, TFT_BLACK);
  tft.drawString("RADIO ACTIONS DO NOT GRANT XP", 120, 262, 1);

  wdPanel(tft, 2, 274, 236, 42, WD_CYAN, WD_CYAN, WD_CYAN);
  tft.setTextColor(TFT_BLACK, WD_CYAN);
  tft.drawString("BACK TO SHARK", 120, 295, 2);
  tft.setTextDatum(TL_DATUM);

  uint16_t x = 0, y = 0;
  uint32_t avatar_tick = millis();
  uint32_t schedule_tick = avatar_tick;
  uint32_t hud_tick = avatar_tick;
  uint32_t touch_mood_until = 0;
  bool touch_mood_active = false;
  while (true) {
    const uint32_t now = millis();
    const uint32_t avatar_interval = pet_state == SHARK_PET_IDLE_STATE
        ? (sharkCyber2077PetTheme() ? 280UL
           : sharkMatrixPetTheme() ? 300UL
           : sharkSpiderPetTheme() ? 320UL : 420UL) : 220UL;
    if (now - avatar_tick >= avatar_interval) {
      avatar_tick = now;
      avatar_frame = (avatar_frame + 1) & 0x03;
      // Only the avatar's 64x64 cell is repainted. The rest of the profile
      // remains static, so the motion stays smooth and does not blink.
      sharkPushActivePetFrame(tft, 9, 55, pet_state, avatar_frame, WD_PANEL);
    }
    if (now - hud_tick >= 1000UL) {
      hud_tick = now;
      #ifdef HAS_GPS
        // profileScreen() owns this blocking loop, so keep the GPS parser live
        // here instead of waiting for the normal Arduino loop to run.
        gps_obj.main();
      #endif
      this->drawSharkTopBar(false);
    }
    if (touch_mood_active && (int32_t)(now - touch_mood_until) >= 0) {
      touch_mood_active = false;
      const SharkPetState scheduled = scheduledPetState(now);
      if (scheduled != pet_state) {
        pet_state = scheduled;
        avatar_frame = 0;
        drawPet(true);
      }
      schedule_tick = now;
    }
    else if (!touch_mood_active && now - schedule_tick >= 60000UL) {
      schedule_tick = now;
      const SharkPetState scheduled = scheduledPetState(now);
      if (scheduled != pet_state) {
        pet_state = scheduled;
        avatar_frame = 0;
        drawPet(true);
      }
    }
    if (display_obj.updateTouch(&x, &y)) {
      while (display_obj.updateTouch(&x, &y)) delay(10);
      if (y >= 268) break;
      if (x >= 4 && x <= 76 && y >= 50 && y <= 124) {
        if (touch_mood_active && pet_state == SHARK_PET_AFFECTION_STATE) {
          pet_state = SHARK_PET_LEVEL_UP_STATE;
        }
        else {
          pet_state = SHARK_PET_AFFECTION_STATE;
        }
        avatar_frame = 0;
        touch_mood_active = true;
        touch_mood_until = millis() + 8000UL;
        drawPet(true);
      }
    }
    delay(12);
  }
  sharkProfileSave();
}

// RuView needs a live Wi-Fi association to read CSI. If we already have one,
// start sensing; otherwise open the in-place connect picker, which itself
// starts RuView on a successful join. Either way the user ends up in RuView
// without ever seeing a dead "JOIN WIFI FIRST" screen.
void MenuFunctions::startRuView() {
  if (WiFi.status() == WL_CONNECTED) {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(SHARK_RUVIEW_CSI, TFT_GREEN);
    return;
  }
  this->buildRuViewConnect();
}

// Connects to one network with the on-screen keyboard's password, and on
// success drops straight into RuView CSI. On failure it stays in the picker.
void MenuFunctions::ruViewJoinAndRun(const String& ssid, const String& password) {
  display_obj.clearScreen();
  display_obj.tft.setTextColor(WD_CYAN, TFT_BLACK);
  display_obj.showCenterText(String("Connecting to " + ssid).c_str(), TFT_HEIGHT / 2, true);

  if (wifi_scan_obj.joinWiFi(ssid, password, false) && WiFi.status() == WL_CONNECTED) {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(SHARK_RUVIEW_CSI, TFT_GREEN);
  } else {
    display_obj.clearScreen();
    display_obj.tft.setTextColor(WD_RED, TFT_BLACK);
    display_obj.showCenterText("Could not join. Try again.", TFT_HEIGHT / 2, true);
    delay(1200);
    this->buildRuViewConnect();
  }
}

// Blocking AP scan, then a picker built from the results. The Back button and
// every SSID stay inside the RuView flow: Back returns to Cyber Defense, a
// tapped SSID connects and launches RuView.
void MenuFunctions::buildRuViewConnect() {
  display_obj.clearScreen();
  display_obj.tft.setTextColor(WD_CYAN, TFT_BLACK);
  display_obj.showCenterText("Scanning Wi-Fi...", TFT_HEIGHT / 2, true);

  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect(false);
  int found = WiFi.scanNetworks();

  ruviewMenu.parentMenu = &sharkDefenseMenu;
  ruviewMenu.list->clear();
  this->addNodes(&ruviewMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(&sharkDefenseMenu, true);
  });
  this->addNodes(&ruviewMenu, "Rescan", TFTCYAN, SCANNERS, [this]() {
    this->buildRuViewConnect();
  });

  // Saved credentials as a one-tap option.
  String saved_ssid = settings_obj.loadSetting<String>("ClientSSID");
  if (saved_ssid != "") {
    this->addNodes(&ruviewMenu, String("Saved: " + saved_ssid).c_str(), TFTGREEN, JOIN_WIFI, [this]() {
      String ssid = settings_obj.loadSetting<String>("ClientSSID");
      String pw = settings_obj.loadSetting<String>("ClientPW");
      this->ruViewJoinAndRun(ssid, pw);
    });
  }

  for (int i = 0; i < found; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.length() == 0)
      continue;
    bool open_net = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
    uint8_t color = rssiToMenuColor(WiFi.RSSI(i));
    String label = String(WiFi.RSSI(i)) + (open_net ? " [open] " : " ") + ssid;
    this->addNodes(&ruviewMenu, label.c_str(), color, JOIN_WIFI, [this, ssid, open_net]() {
      if (open_net) {
        this->ruViewJoinAndRun(ssid, "");
        return;
      }
      char passwordBuf[64] = {0};
      if (keyboardInput(passwordBuf, sizeof(passwordBuf), "Enter Password"))
        this->ruViewJoinAndRun(ssid, String(passwordBuf));
      else
        this->buildRuViewConnect();   // cancelled: stay in the picker
    });
  }

  this->changeMenu(&ruviewMenu, true);
}

void MenuFunctions::drawSharkTopBar(bool force) {
  static uint8_t previous_channel = 255;
  static int8_t previous_battery = -127;
  static bool previous_sd = false;
  static bool previous_wifi = false;
  static uint8_t previous_gps_state = 255;
  static bool previous_web = false;
  static bool previous_ble = false;

  uint8_t primary_channel = wifi_scan_obj.set_channel;
  wifi_second_chan_t secondary_channel;
  if (esp_wifi_get_channel(&primary_channel, &secondary_channel) != ESP_OK)
    primary_channel = wifi_scan_obj.set_channel;

  #ifdef HAS_SD
    const bool sd_ready = sd_obj.supported && SD.cardType() != CARD_NONE;
  #else
    const bool sd_ready = false;
  #endif

  const wifi_mode_t live_wifi_mode = WiFi.getMode();
  const bool web_flag = shark_web_obj.active || shark_web_obj.background;
  const bool web_ready = web_flag &&
                         (live_wifi_mode == WIFI_MODE_AP || live_wifi_mode == WIFI_MODE_APSTA);
  // Wi-Fi means either a real STA link or an actively initialized Wi-Fi tool.
  // The Web Control AP keeps its own globe and does not fake a Wi-Fi link lamp.
  bool wifi_operation_running = true;
  if (wifi_scan_obj.currentScanMode == WIFI_SCAN_SIG_STREN)
    wifi_operation_running = wifi_scan_obj.wifi_fox_running;
  const bool wifi_tool_ready = (wifi_scan_obj.wifi_initialized ||
                                (shark_hud_tool_activity & SHARK_HUD_WIFI_ACTIVITY)) &&
                               wifi_operation_running;
  const bool standalone_ap_ready = !web_ready &&
                                   (live_wifi_mode == WIFI_MODE_AP ||
                                    live_wifi_mode == WIFI_MODE_APSTA);
  const bool wifi_ready = wifi_operation_running &&
                          (WiFi.status() == WL_CONNECTED ||
                           (wifi_tool_ready && !web_ready) ||
                           standalone_ap_ready);

  #ifdef HAS_BT
    const bool ble_stack_ready = wifi_scan_obj.ble_initialized ||
                                 NimBLEDevice::isInitialized() ||
                                 (shark_hud_tool_activity & SHARK_HUD_BLE_ACTIVITY);
    const bool spam_dashboard_mode =
        wifi_scan_obj.currentScanMode == BT_ATTACK_SOUR_APPLE ||
        wifi_scan_obj.currentScanMode == BT_ATTACK_SWIFTPAIR_SPAM ||
        wifi_scan_obj.currentScanMode == BT_ATTACK_APPLE_JUICE ||
        wifi_scan_obj.currentScanMode == BT_ATTACK_SAMSUNG_SPAM;
    bool ble_operation_running = true;
    if (wifi_scan_obj.currentScanMode == BT_SCAN_ANALYZER)
      ble_operation_running = wifi_scan_obj.bt_analyzer_running;
    else if (wifi_scan_obj.currentScanMode == BT_SCAN_ALL)
      ble_operation_running = wifi_scan_obj.bt_sniffer_running;
    else if (wifi_scan_obj.currentScanMode == BT_SCAN_FLIPPER)
      ble_operation_running = wifi_scan_obj.bt_flipper_running;
    else if (wifi_scan_obj.currentScanMode == BT_SCAN_SKIMMERS)
      ble_operation_running = wifi_scan_obj.bt_skimmer_running;
    else if ((wifi_scan_obj.currentScanMode == BT_SCAN_AIRTAG) ||
             (wifi_scan_obj.currentScanMode == BT_SCAN_AIRTAG_MON) ||
             (wifi_scan_obj.currentScanMode == BT_SCAN_FLOCK) ||
             (wifi_scan_obj.currentScanMode == BT_SCAN_RAYBAN))
      ble_operation_running = wifi_scan_obj.bt_passive_running;
    else if (wifi_scan_obj.currentScanMode == BT_SCAN_FOX_HUNT)
      ble_operation_running = wifi_scan_obj.bt_fox_running;
    else if (spam_dashboard_mode)
      ble_operation_running = wifi_scan_obj.bt_sour_running;
    // Dashboard transmitters intentionally cycle NimBLE between bursts. Keep
    // the HUD tied to the real START/STOP loop so stack restarts do not blink.
    const bool ble_ready = spam_dashboard_mode
        ? wifi_scan_obj.bt_sour_running
        : (ble_stack_ready && ble_operation_running);
  #else
    const bool ble_ready = false;
  #endif

  #ifdef HAS_GPS
    const bool gps_present = gps_obj.getGpsModuleStatus();
    const bool gps_fix = gps_present && gps_obj.getFixStatus();
  #else
    const bool gps_present = false;
    const bool gps_fix = false;
  #endif
  const uint8_t gps_state = gps_fix ? 2 : gps_present ? 1 : 0;

  #ifdef HAS_BATTERY
    const int8_t battery_level = battery_obj.battery_level;
  #else
    const int8_t battery_level = -1;
  #endif

  const bool changed = primary_channel != previous_channel ||
                       battery_level != previous_battery ||
                       sd_ready != previous_sd ||
                       wifi_ready != previous_wifi ||
                       gps_state != previous_gps_state ||
                       web_ready != previous_web ||
                       ble_ready != previous_ble;
  // Do not repaint the complete HUD on a timer.  The old one-second refresh
  // cleared the bar before drawing it again, which looked like a blink on the
  // physical TFT.  Every real HUD input is included in `changed`, so a repaint
  // is still triggered immediately when channel, radio, GPS, SD or battery
  // state changes.
  if (!force && !changed)
    return;

  previous_channel = primary_channel;
  previous_battery = battery_level;
  previous_sd = sd_ready;
  previous_wifi = wifi_ready;
  previous_gps_state = gps_state;
  previous_web = web_ready;
  previous_ble = ble_ready;

  display_obj.tft.setFreeFont(NULL);
  display_obj.tft.setTextSize(1);
  display_obj.tft.setTextWrap(false);
  display_obj.tft.setTextDatum(TL_DATUM);
  display_obj.tft.fillRect(0, 0, SCREEN_WIDTH, STATUS_BAR_WIDTH, WD_PANEL);
  display_obj.tft.drawFastHLine(0, 0, SCREEN_WIDTH, WD_EDGE);

  // High-contrast brand plate. Every item after it is an icon, not a tiny text
  // abbreviation, leaving the bar readable without consuming menu space.
  display_obj.tft.fillRect(0, 1, 40, STATUS_BAR_WIDTH - 2, WD_CYAN);
  display_obj.tft.setTextColor(TFT_BLACK, WD_CYAN);
  display_obj.tft.drawString(SHARK_UI_NAME, 5, 6, 1);

  sharkHudDivider(display_obj.tft, 42);
  // Channel is telemetry rather than a second Wi-Fi status lamp. Brighten the
  // number during a Wi-Fi tool, but keep the antenna neutral so only the Wi-Fi
  // symbol itself shows the active state.
  sharkHudRadio(display_obj.tft, 44, 3, WD_DIM);
  display_obj.tft.setTextColor(wifi_tool_ready ? WD_WHITE : WD_DIM, WD_PANEL);
  char channel_label[3];
  snprintf(channel_label, sizeof(channel_label), "%02u", (unsigned)primary_channel);
  display_obj.tft.drawString(channel_label, 56, 6, 1);

  sharkHudDivider(display_obj.tft, 71);
  const uint16_t sd_icon = sharkHudLamp(display_obj.tft, 73, 15, sd_ready);
  sharkHudSd(display_obj.tft, 75, 3, sd_icon);
  sharkHudDivider(display_obj.tft, 90);
  const uint16_t wifi_icon = sharkHudLamp(display_obj.tft, 92, 17, wifi_ready);
  sharkHudWifi(display_obj.tft, 94, 3, wifi_icon);
  sharkHudDivider(display_obj.tft, 111);
  const uint16_t gps_lamp_color = gps_fix ? WD_CYAN : WD_AMBER;
  const uint16_t gps_icon = sharkHudLamp(display_obj.tft, 113, 15,
                                         gps_present, gps_lamp_color);
  sharkHudGps(display_obj.tft, 115, 3, gps_icon);
  sharkHudDivider(display_obj.tft, 130);
  const uint16_t web_icon = sharkHudLamp(display_obj.tft, 132, 15, web_ready);
  sharkHudWeb(display_obj.tft, 134, 3, web_icon);
  sharkHudDivider(display_obj.tft, 149);
  const uint16_t ble_icon = sharkHudLamp(display_obj.tft, 151, 24, ble_ready);
  sharkHudBluetooth(display_obj.tft, 158, 3, ble_icon);
  sharkHudDivider(display_obj.tft, 177);

  // Battery cell. Amber below 20 percent, red below 10.
  const uint16_t battery_color = battery_level < 0 ? WD_DIM
                               : battery_level < 10 ? WD_RED
                               : battery_level < 20 ? WD_AMBER
                                                    : WD_CYAN;
  // Keep the battery cell visually attached to its percentage. At 100% the
  // text begins near x=213, so the cell now ends at x=207 with only a 5 px gap.
  sharkHudBattery(display_obj.tft, 183, 4, battery_level, battery_color);

  display_obj.tft.setTextColor(battery_color, WD_PANEL);
  display_obj.tft.setTextDatum(TR_DATUM);
  if (battery_level >= 0)
    display_obj.tft.drawString(String(battery_level) + "%", SCREEN_WIDTH - 3, 6, 1);
  else
    display_obj.tft.drawString("--", SCREEN_WIDTH - 3, 6, 1);
  display_obj.tft.setTextDatum(TL_DATUM);

  // Static activity rail.  Its previous timer-driven animation forced the
  // entire HUD to repaint even when no device state changed.
  display_obj.tft.drawFastHLine(0, STATUS_BAR_WIDTH - 1, SCREEN_WIDTH, WD_EDGE);
  display_obj.tft.drawFastHLine(0, STATUS_BAR_WIDTH - 1, 18, WD_CYAN);
}

void MenuFunctions::drawBluetoothAnalyzerUI(bool full_redraw) {
  TFT_eSPI& tft = display_obj.tft;
  const bool running = wifi_scan_obj.bt_analyzer_running;

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  tft.setTextDatum(TL_DATUM);

  if (full_redraw) {
    tft.fillScreen(TFT_BLACK);
    this->drawSharkTopBar(true);
    tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, 25, TFT_BLACK);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString("// BLE ANALYZER", 6, STATUS_BAR_WIDTH + 7, 2);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 60, WD_CYAN);
  }

  // Explicit state chip: green/cyan means the NimBLE scanner is receiving;
  // amber PAUSED means the graph is frozen and no scan is running.
  const uint16_t state_color = running ? WD_CYAN : WD_AMBER;
  tft.fillRoundRect(172, STATUS_BAR_WIDTH + 4, 63, 17, 3, state_color);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, state_color);
  tft.drawString(running ? "RUNNING" : "PAUSED", 203, STATUS_BAR_WIDTH + 12, 1);

  const int16_t card_y = 49;
  const int16_t card_w = 76;
  const char* labels[3] = {"RATE", "PEAK", "TOTAL"};
  const String values[3] = {
    String(wifi_scan_obj.bt_analyzer_rate) + "/s",
    String(wifi_scan_obj.bt_analyzer_peak) + "/s",
    String((uint32_t)wifi_scan_obj.bt_analyzer_total)
  };
  for (uint8_t i = 0; i < 3; i++) {
    const int16_t x = 2 + i * 80;
    wdPanel(tft, x, card_y, card_w, 42, WD_SURFACE, WD_EDGE,
            i == 0 ? state_color : WD_CYAN);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_SURFACE);
    tft.drawString(labels[i], x + 7, card_y + 5, 1);
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(i == 0 ? state_color : WD_BONE, WD_SURFACE);
    tft.drawString(values[i], x + card_w - 7, card_y + 27, 2);
  }

  // Last advertiser panel. The existing analyzer callback supplies a genuine
  // RSSI + name/address string; no placeholder device is invented.
  wdPanel(tft, 2, 95, 236, 28, WD_PANEL, WD_EDGE, WD_CYAN);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WD_DIM, WD_PANEL);
  tft.drawString("LAST", 8, 101, 1);
  String last = wifi_scan_obj.analyzer_name_string;
  if (!last.length()) last = running ? "WAITING FOR ADVERTISEMENT" : "NO CAPTURE";
  while (last.length() > 3 && tft.textWidth(last, 1) > 184)
    last.remove(last.length() - 1);
  tft.setTextColor(last.length() ? WD_BONE : WD_DIM, WD_PANEL);
  tft.drawString(last, 48, 101, 1);

  // Bounded graph leaves the bottom controls permanently visible. New samples
  // enter from the right; auto-scaling keeps quiet and busy rooms readable.
  const int16_t gx = 6, gy = 128, gw = 228, gh = 136;
  wdPanel(tft, gx, gy, gw, gh, WD_SURFACE, WD_EDGE, state_color);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WD_DIM, WD_SURFACE);
  tft.drawString("ADVERTISEMENT ACTIVITY / 100MS", gx + 8, gy + 5, 1);
  const int16_t plot_x = gx + 6;
  const int16_t plot_y = gy + 20;
  const int16_t plot_w = gw - 12;
  const int16_t plot_h = gh - 27;
  tft.fillRect(plot_x, plot_y, plot_w, plot_h, TFT_BLACK);
  for (uint8_t line = 1; line < 4; line++)
    tft.drawFastHLine(plot_x, plot_y + (plot_h * line / 4), plot_w, WD_PANEL);

  int16_t graph_max = 1;
  for (int16_t i = 0; i < plot_w; i++)
    if (wifi_scan_obj._analyzer_values[i] > graph_max)
      graph_max = wifi_scan_obj._analyzer_values[i];
  const uint16_t graph_color = running ? WD_CYAN : WD_CYAN_DIM;
  for (int16_t i = 0; i < plot_w; i++) {
    const int16_t sample = max((int16_t)0, wifi_scan_obj._analyzer_values[i]);
    const int16_t bar = sample ? max((int16_t)1,
                                     (int16_t)((int32_t)sample * (plot_h - 2) / graph_max)) : 0;
    if (bar)
      tft.drawFastVLine(plot_x + plot_w - 1 - i, plot_y + plot_h - bar, bar, graph_color);
  }

  // Four persistent controls. Their hit zones are the four 60-pixel columns.
  static const char* const button_labels[4] = {"START", "STOP", "CLEAR", "BACK"};
  for (uint8_t i = 0; i < 4; i++) {
    const int16_t x = i * 60 + 2;
    bool enabled = true;
    uint16_t fill = WD_SURFACE;
    uint16_t accent = WD_CYAN;
    uint16_t text = WD_BONE;
    if (i == 0) {
      enabled = !running;
      fill = enabled ? WD_CYAN : WD_SURFACE;
      text = enabled ? TFT_BLACK : WD_DIM;
    } else if (i == 1) {
      enabled = running;
      fill = enabled ? WD_AMBER : WD_SURFACE;
      accent = WD_AMBER;
      text = enabled ? TFT_BLACK : WD_DIM;
    } else if (i == 2) {
      accent = WD_BONE;
    } else {
      accent = WD_AMBER;
      text = WD_AMBER;
    }
    wdPanel(tft, x, 274, 56, 42, fill, enabled ? accent : WD_EDGE, accent);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(text, fill);
    tft.drawString(button_labels[i], x + 28, 295, 1);
  }
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::handleBluetoothAnalyzerTouch(uint16_t touch_x, uint16_t touch_y) {
  if (touch_y < 268)
    return;

  const uint8_t action = min((uint8_t)3, (uint8_t)(touch_x / 60));
  if (action == 0) {
    wifi_scan_obj.startBluetoothAnalyzer();
  } else if (action == 1) {
    wifi_scan_obj.stopBluetoothAnalyzer();
  } else if (action == 2) {
    wifi_scan_obj.clearBluetoothAnalyzer();
  } else {
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
    display_obj.init();
    this->changeMenu(current_menu, true);
    return;
  }

  this->drawBluetoothAnalyzerUI(true);
}

void MenuFunctions::drawBluetoothSnifferUI(bool full_redraw) {
  TFT_eSPI& tft = display_obj.tft;
  const bool running = wifi_scan_obj.bt_sniffer_running;
  this->bt_sniffer_data_view = false;

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  tft.setTextDatum(TL_DATUM);

  if (full_redraw) {
    tft.fillScreen(TFT_BLACK);
    this->drawSharkTopBar(true);
    tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, 25, TFT_BLACK);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString("// BLE SNIFFER", 6, STATUS_BAR_WIDTH + 7, 2);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 60, WD_CYAN);
  }

  const uint16_t state_color = running ? WD_CYAN : WD_AMBER;
  tft.fillRoundRect(172, STATUS_BAR_WIDTH + 4, 63, 17, 3, state_color);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, state_color);
  tft.drawString(running ? "CAPTURE" : "PAUSED", 203, STATUS_BAR_WIDTH + 12, 1);

  const int16_t card_y = 49;
  const int16_t card_w = 76;
  const char* labels[3] = {"ADV/S", "DEVICES", "TOTAL"};
  const String values[3] = {
    String(wifi_scan_obj.bt_sniffer_rate),
    String(wifi_scan_obj.bt_sniffer_unique),
    String((uint32_t)wifi_scan_obj.bt_sniffer_total)
  };
  for (uint8_t i = 0; i < 3; i++) {
    const int16_t x = 2 + i * 80;
    wdPanel(tft, x, card_y, card_w, 42, WD_SURFACE, WD_EDGE,
            i == 0 ? state_color : WD_CYAN);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_SURFACE);
    tft.drawString(labels[i], x + 7, card_y + 5, 1);
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(i == 0 ? state_color : WD_BONE, WD_SURFACE);
    tft.drawString(values[i], x + card_w - 7, card_y + 27, 2);
  }

  // Last-device identity comes directly from the most recent NimBLE callback.
  wdPanel(tft, 2, 95, 236, 55, WD_PANEL, WD_EDGE, WD_CYAN);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WD_DIM, WD_PANEL);
  tft.drawString("LAST DETECTED", 8, 101, 1);
  String device_name = wifi_scan_obj.bt_sniffer_last_name;
  if (!device_name.length()) device_name = running ? "WAITING FOR DEVICE" : "NO CAPTURE";
  while (device_name.length() > 3 && tft.textWidth(device_name, 2) > 220)
    device_name.remove(device_name.length() - 1);
  tft.setTextColor(WD_BONE, WD_PANEL);
  tft.drawString(device_name, 8, 114, 2);
  String detail = wifi_scan_obj.bt_sniffer_last_mac;
  if (detail.length())
    detail += "  " + String(wifi_scan_obj.bt_sniffer_last_rssi) + "dBm";
  else
    detail = "--:--:--:--:--:--  --dBm";
  tft.setTextColor(detail.length() ? WD_CYAN : WD_DIM, WD_PANEL);
  tft.drawString(detail, 8, 137, 1);

  // Signal meter is clamped to the practical BLE range, so it never pretends
  // to be active before a real RSSI value arrives.
  const int16_t meter_x = 6, meter_y = 156, meter_w = 228, meter_h = 17;
  tft.fillRoundRect(meter_x, meter_y, meter_w, meter_h, 3, WD_SURFACE);
  tft.drawRoundRect(meter_x, meter_y, meter_w, meter_h, 3, WD_EDGE);
  const bool has_signal = wifi_scan_obj.bt_sniffer_last_rssi > -128;
  const int16_t clamped_rssi = constrain(wifi_scan_obj.bt_sniffer_last_rssi, -100, -35);
  const int16_t signal_w = has_signal ?
      (int16_t)((int32_t)(clamped_rssi + 100) * (meter_w - 4) / 65) : 0;
  const uint16_t meter_color = clamped_rssi > -65 ? WD_CYAN : WD_AMBER;
  if (signal_w > 0)
    tft.fillRoundRect(meter_x + 2, meter_y + 2, signal_w, meter_h - 4, 2, meter_color);
  tft.setTextDatum(MC_DATUM);
  const bool label_on_meter = has_signal && signal_w > meter_w / 2;
  tft.setTextColor(label_on_meter ? TFT_BLACK : (has_signal ? WD_BONE : WD_DIM),
                   label_on_meter ? meter_color : WD_SURFACE);
  tft.drawString(has_signal ? "LIVE RSSI" : "NO SIGNAL", meter_x + meter_w / 2,
                 meter_y + meter_h / 2, 1);

  // Scrolling advertisement pulse graph; the newest genuine sample is on the
  // right. The graph freezes when STOP is pressed.
  const int16_t gx = 6, gy = 178, gw = 228, gh = 86;
  wdPanel(tft, gx, gy, gw, gh, WD_SURFACE, WD_EDGE, state_color);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WD_DIM, WD_SURFACE);
  tft.drawString("ADVERTISEMENT PULSE", gx + 8, gy + 5, 1);
  const int16_t plot_x = gx + 6;
  const int16_t plot_y = gy + 20;
  const int16_t plot_w = gw - 12;
  const int16_t plot_h = gh - 27;
  tft.fillRect(plot_x, plot_y, plot_w, plot_h, TFT_BLACK);
  for (uint8_t line = 1; line < 3; line++)
    tft.drawFastHLine(plot_x, plot_y + (plot_h * line / 3), plot_w, WD_PANEL);

  int16_t graph_max = 1;
  for (int16_t i = 0; i < plot_w; i++)
    if (wifi_scan_obj._analyzer_values[i] > graph_max)
      graph_max = wifi_scan_obj._analyzer_values[i];
  const uint16_t graph_color = running ? WD_CYAN : WD_CYAN_DIM;
  for (int16_t i = 0; i < plot_w; i++) {
    const int16_t sample = max((int16_t)0, wifi_scan_obj._analyzer_values[i]);
    const int16_t bar = sample ? max((int16_t)1,
                                     (int16_t)((int32_t)sample * (plot_h - 2) / graph_max)) : 0;
    if (bar)
      tft.drawFastVLine(plot_x + plot_w - 1 - i, plot_y + plot_h - bar, bar, graph_color);
  }

  static const char* const button_labels[5] = {"START", "STOP", "CLEAR", "DATA", "BACK"};
  for (uint8_t i = 0; i < 5; i++) {
    const int16_t x = i * 48 + 2;
    bool enabled = true;
    uint16_t fill = WD_SURFACE;
    uint16_t accent = WD_CYAN;
    uint16_t text = WD_BONE;
    if (i == 0) {
      enabled = !running;
      fill = enabled ? WD_CYAN : WD_SURFACE;
      text = enabled ? TFT_BLACK : WD_DIM;
    } else if (i == 1) {
      enabled = running;
      fill = enabled ? WD_AMBER : WD_SURFACE;
      accent = WD_AMBER;
      text = enabled ? TFT_BLACK : WD_DIM;
    } else if (i == 2) {
      accent = WD_BONE;
    } else if (i == 3) {
      accent = WD_CYAN;
      text = WD_CYAN;
    } else {
      accent = WD_AMBER;
      text = WD_AMBER;
    }
    wdPanel(tft, x, 274, 44, 42, fill, enabled ? accent : WD_EDGE, accent);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(text, fill);
    tft.drawString(button_labels[i], x + 22, 295, 1);
  }
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::drawBluetoothSnifferDataUI(bool full_redraw) {
  extern LinkedList<BleDevice>* ble_devices;
  TFT_eSPI& tft = display_obj.tft;
  const uint8_t rows_per_page = 5;
  const uint16_t total_devices = ble_devices ? ble_devices->size() : 0;
  const uint16_t page_count = max((uint16_t)1,
                                  (uint16_t)((total_devices + rows_per_page - 1) / rows_per_page));
  if (this->bt_sniffer_data_page >= page_count)
    this->bt_sniffer_data_page = page_count - 1;

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  tft.setTextDatum(TL_DATUM);

  if (full_redraw) {
    tft.fillScreen(TFT_BLACK);
    this->drawSharkTopBar(true);
  }

  tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_WIDTH, TFT_BLACK);
  tft.setTextColor(WD_CYAN, TFT_BLACK);
  tft.drawString("// CAPTURE DATA", 6, STATUS_BAR_WIDTH + 7, 2);
  tft.fillRoundRect(174, STATUS_BAR_WIDTH + 4, 61, 17, 3, WD_AMBER);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, WD_AMBER);
  tft.drawString("SNAPSHOT", 204, STATUS_BAR_WIDTH + 12, 1);
  tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
  tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 60, WD_CYAN);

  wdPanel(tft, 2, 51, 236, 21, WD_PANEL, WD_EDGE, WD_CYAN);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WD_BONE, WD_PANEL);
  tft.drawString("DEVICES " + String(total_devices), 8, 58, 1);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(WD_DIM, WD_PANEL);
  tft.drawString("PAGE " + String(this->bt_sniffer_data_page + 1) + "/" + String(page_count),
                 232, 58, 1);

  if (total_devices == 0) {
    wdPanel(tft, 12, 104, 216, 112, WD_SURFACE, WD_EDGE, WD_AMBER);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WD_AMBER, WD_SURFACE);
    tft.drawString("NO DEVICES CAPTURED", 120, 145, 2);
    tft.setTextColor(WD_DIM, WD_SURFACE);
    tft.drawString("RETURN TO DASH AND START", 120, 176, 1);
  } else {
    for (uint8_t row = 0; row < rows_per_page; row++) {
      const uint16_t offset = this->bt_sniffer_data_page * rows_per_page + row;
      if (offset >= total_devices)
        break;

      // Newest captured devices appear first.
      const int32_t device_index = (int32_t)total_devices - 1 - offset;
      BleDevice device = ble_devices->get(device_index);
      const int16_t y = 76 + row * 38;
      const uint16_t signal_color = device.rssi >= -65 ? WD_CYAN :
                                    device.rssi >= -85 ? WD_AMBER : WD_DIM;
      wdPanel(tft, 2, y, 236, 34, WD_SURFACE, WD_EDGE, signal_color);

      String name = device.name.length() ? device.name : "UNKNOWN DEVICE";
      while (name.length() > 3 && tft.textWidth(name, 1) > 150)
        name.remove(name.length() - 1);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_BONE, WD_SURFACE);
      tft.drawString(name, 9, y + 5, 1);
      tft.setTextDatum(TR_DATUM);
      tft.setTextColor(signal_color, WD_SURFACE);
      tft.drawString(String(device.rssi) + " dBm", 231, y + 5, 1);

      char mac_text[18];
      snprintf(mac_text, sizeof(mac_text), "%02X:%02X:%02X:%02X:%02X:%02X",
               device.mac[0], device.mac[1], device.mac[2],
               device.mac[3], device.mac[4], device.mac[5]);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_DIM, WD_SURFACE);
      tft.drawString(mac_text, 9, y + 20, 1);
    }
  }

  static const char* const labels[3] = {"PREV", "NEXT", "DASH"};
  const bool can_prev = this->bt_sniffer_data_page > 0;
  const bool can_next = this->bt_sniffer_data_page + 1 < page_count;
  for (uint8_t i = 0; i < 3; i++) {
    const int16_t x = i * 80 + 2;
    const bool enabled = i == 0 ? can_prev : (i == 1 ? can_next : true);
    const uint16_t accent = i == 2 ? WD_CYAN : (enabled ? WD_BONE : WD_EDGE);
    const uint16_t fill = i == 2 ? WD_CYAN : WD_SURFACE;
    const uint16_t text = i == 2 ? TFT_BLACK : (enabled ? WD_BONE : WD_DIM);
    wdPanel(tft, x, 274, 76, 42, fill, accent, accent);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(text, fill);
    tft.drawString(labels[i], x + 38, 295, 1);
  }
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::handleBluetoothSnifferTouch(uint16_t touch_x, uint16_t touch_y) {
  if (touch_y < 268)
    return;

  if (this->bt_sniffer_data_view) {
    extern LinkedList<BleDevice>* ble_devices;
    const uint16_t total_devices = ble_devices ? ble_devices->size() : 0;
    const uint16_t page_count = max((uint16_t)1, (uint16_t)((total_devices + 4) / 5));
    if (touch_x < 80) {
      if (this->bt_sniffer_data_page > 0) this->bt_sniffer_data_page--;
      this->drawBluetoothSnifferDataUI(true);
    } else if (touch_x < 160) {
      if (this->bt_sniffer_data_page + 1 < page_count) this->bt_sniffer_data_page++;
      this->drawBluetoothSnifferDataUI(true);
    } else {
      const bool resume = this->bt_sniffer_resume_after_data;
      this->bt_sniffer_data_view = false;
      this->bt_sniffer_resume_after_data = false;
      if (resume) wifi_scan_obj.startBluetoothSniffer();
      this->drawBluetoothSnifferUI(true);
    }
    return;
  }

  const uint8_t action = min((uint8_t)4, (uint8_t)(touch_x / 48));
  if (action == 0) {
    wifi_scan_obj.startBluetoothSniffer();
  } else if (action == 1) {
    wifi_scan_obj.stopBluetoothSniffer();
  } else if (action == 2) {
    wifi_scan_obj.clearBluetoothSniffer();
  } else if (action == 3) {
    this->bt_sniffer_resume_after_data = wifi_scan_obj.bt_sniffer_running;
    if (wifi_scan_obj.bt_sniffer_running) {
      wifi_scan_obj.stopBluetoothSniffer();
      const uint32_t wait_started = millis();
      while (wifi_scan_obj.bt_cb_busy && millis() - wait_started < 150) delay(1);
    }
    this->bt_sniffer_data_page = 0;
    this->bt_sniffer_data_view = true;
    this->drawBluetoothSnifferDataUI(true);
    return;
  } else {
    this->bt_sniffer_data_view = false;
    this->bt_sniffer_resume_after_data = false;
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
    display_obj.init();
    this->changeMenu(current_menu, true);
    return;
  }

  this->drawBluetoothSnifferUI(true);
}

void MenuFunctions::drawFlipperSnifferUI(bool full_redraw) {
  TFT_eSPI& tft = display_obj.tft;
  const bool running = wifi_scan_obj.bt_flipper_running;
  this->bt_flipper_data_view = false;

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  tft.setTextDatum(TL_DATUM);

  if (full_redraw) {
    tft.fillScreen(TFT_BLACK);
    this->drawSharkTopBar(true);
    tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, 25, TFT_BLACK);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString("// FLIPPER SNIFF", 6, STATUS_BAR_WIDTH + 7, 2);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 60, WD_CYAN);
  }

  const uint16_t state_color = running ? WD_CYAN : WD_AMBER;
  tft.fillRoundRect(172, STATUS_BAR_WIDTH + 4, 63, 17, 3, state_color);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, state_color);
  tft.drawString(running ? "CAPTURE" : "PAUSED", 203, STATUS_BAR_WIDTH + 12, 1);

  const int16_t card_y = 49;
  const int16_t card_w = 76;
  const char* labels[3] = {"MATCH/S", "DEVICES", "TOTAL"};
  const String values[3] = {
    String(wifi_scan_obj.bt_flipper_rate),
    String(wifi_scan_obj.bt_flipper_unique),
    String((uint32_t)wifi_scan_obj.bt_flipper_total)
  };
  for (uint8_t i = 0; i < 3; i++) {
    const int16_t x = 2 + i * 80;
    wdPanel(tft, x, card_y, card_w, 42, WD_SURFACE, WD_EDGE,
            i == 0 ? state_color : WD_CYAN);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_SURFACE);
    tft.drawString(labels[i], x + 7, card_y + 5, 1);
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(i == 0 ? state_color : WD_BONE, WD_SURFACE);
    tft.drawString(values[i], x + card_w - 7, card_y + 27, 2);
  }

  wdPanel(tft, 2, 95, 236, 55, WD_PANEL, WD_EDGE, WD_CYAN);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WD_DIM, WD_PANEL);
  tft.drawString("LAST FLIPPER MATCH", 8, 101, 1);
  String device_name = wifi_scan_obj.bt_flipper_last_name;
  if (!device_name.length()) device_name = running ? "WAITING FOR SIGNATURE" : "NO MATCH";
  while (device_name.length() > 3 && tft.textWidth(device_name, 2) > 220)
    device_name.remove(device_name.length() - 1);
  tft.setTextColor(WD_BONE, WD_PANEL);
  tft.drawString(device_name, 8, 114, 2);
  String detail = wifi_scan_obj.bt_flipper_last_mac;
  if (detail.length()) {
    detail += "  " + String(wifi_scan_obj.bt_flipper_last_rssi) + "dBm";
    if (wifi_scan_obj.bt_flipper_last_color.length())
      detail += "  " + wifi_scan_obj.bt_flipper_last_color;
  } else {
    detail = "--:--:--:--:--:--  --dBm";
  }
  while (detail.length() > 3 && tft.textWidth(detail, 1) > 224)
    detail.remove(detail.length() - 1);
  tft.setTextColor(WD_CYAN, WD_PANEL);
  tft.drawString(detail, 8, 137, 1);

  const int16_t meter_x = 6, meter_y = 156, meter_w = 228, meter_h = 17;
  tft.fillRoundRect(meter_x, meter_y, meter_w, meter_h, 3, WD_SURFACE);
  tft.drawRoundRect(meter_x, meter_y, meter_w, meter_h, 3, WD_EDGE);
  const bool has_signal = wifi_scan_obj.bt_flipper_last_rssi > -128;
  const int16_t clamped_rssi = constrain(wifi_scan_obj.bt_flipper_last_rssi, -100, -35);
  const int16_t signal_w = has_signal ?
      (int16_t)((int32_t)(clamped_rssi + 100) * (meter_w - 4) / 65) : 0;
  const uint16_t meter_color = clamped_rssi > -65 ? WD_CYAN : WD_AMBER;
  if (signal_w > 0)
    tft.fillRoundRect(meter_x + 2, meter_y + 2, signal_w, meter_h - 4, 2, meter_color);
  const bool label_on_meter = has_signal && signal_w > meter_w / 2;
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(label_on_meter ? TFT_BLACK : (has_signal ? WD_BONE : WD_DIM),
                   label_on_meter ? meter_color : WD_SURFACE);
  tft.drawString(has_signal ? "MATCH RSSI" : "NO FLIPPER SIGNAL",
                 meter_x + meter_w / 2, meter_y + meter_h / 2, 1);

  const int16_t gx = 6, gy = 178, gw = 228, gh = 86;
  wdPanel(tft, gx, gy, gw, gh, WD_SURFACE, WD_EDGE, state_color);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WD_DIM, WD_SURFACE);
  tft.drawString("FLIPPER MATCH PULSE", gx + 8, gy + 5, 1);
  const int16_t plot_x = gx + 6;
  const int16_t plot_y = gy + 20;
  const int16_t plot_w = gw - 12;
  const int16_t plot_h = gh - 27;
  tft.fillRect(plot_x, plot_y, plot_w, plot_h, TFT_BLACK);
  for (uint8_t line = 1; line < 3; line++)
    tft.drawFastHLine(plot_x, plot_y + (plot_h * line / 3), plot_w, WD_PANEL);
  int16_t graph_max = 1;
  for (int16_t i = 0; i < plot_w; i++)
    if (wifi_scan_obj._analyzer_values[i] > graph_max)
      graph_max = wifi_scan_obj._analyzer_values[i];
  const uint16_t graph_color = running ? WD_CYAN : WD_CYAN_DIM;
  for (int16_t i = 0; i < plot_w; i++) {
    const int16_t sample = max((int16_t)0, wifi_scan_obj._analyzer_values[i]);
    const int16_t bar = sample ? max((int16_t)1,
                                     (int16_t)((int32_t)sample * (plot_h - 2) / graph_max)) : 0;
    if (bar)
      tft.drawFastVLine(plot_x + plot_w - 1 - i, plot_y + plot_h - bar, bar, graph_color);
  }

  static const char* const button_labels[5] = {"START", "STOP", "CLEAR", "DATA", "BACK"};
  for (uint8_t i = 0; i < 5; i++) {
    const int16_t x = i * 48 + 2;
    bool enabled = true;
    uint16_t fill = WD_SURFACE;
    uint16_t accent = WD_CYAN;
    uint16_t text = WD_BONE;
    if (i == 0) {
      enabled = !running;
      fill = enabled ? WD_CYAN : WD_SURFACE;
      text = enabled ? TFT_BLACK : WD_DIM;
    } else if (i == 1) {
      enabled = running;
      fill = enabled ? WD_AMBER : WD_SURFACE;
      accent = WD_AMBER;
      text = enabled ? TFT_BLACK : WD_DIM;
    } else if (i == 2) {
      accent = WD_BONE;
    } else if (i == 3) {
      text = WD_CYAN;
    } else {
      accent = WD_AMBER;
      text = WD_AMBER;
    }
    wdPanel(tft, x, 274, 44, 42, fill, enabled ? accent : WD_EDGE, accent);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(text, fill);
    tft.drawString(button_labels[i], x + 22, 295, 1);
  }
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::drawFlipperSnifferDataUI(bool full_redraw) {
  extern LinkedList<Flipper>* flippers;
  TFT_eSPI& tft = display_obj.tft;
  const uint8_t rows_per_page = 5;
  const uint16_t total_devices = flippers ? flippers->size() : 0;
  const uint16_t page_count = max((uint16_t)1,
                                  (uint16_t)((total_devices + rows_per_page - 1) / rows_per_page));
  if (this->bt_flipper_data_page >= page_count)
    this->bt_flipper_data_page = page_count - 1;

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  tft.setTextDatum(TL_DATUM);
  if (full_redraw) {
    tft.fillScreen(TFT_BLACK);
    this->drawSharkTopBar(true);
  }
  tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_WIDTH, TFT_BLACK);
  tft.setTextColor(WD_CYAN, TFT_BLACK);
  tft.drawString("// FLIPPER DATA", 6, STATUS_BAR_WIDTH + 7, 2);
  tft.fillRoundRect(174, STATUS_BAR_WIDTH + 4, 61, 17, 3, WD_AMBER);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, WD_AMBER);
  tft.drawString("SNAPSHOT", 204, STATUS_BAR_WIDTH + 12, 1);
  tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
  tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 60, WD_CYAN);

  wdPanel(tft, 2, 51, 236, 21, WD_PANEL, WD_EDGE, WD_CYAN);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WD_BONE, WD_PANEL);
  tft.drawString("MATCHED " + String(total_devices), 8, 58, 1);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(WD_DIM, WD_PANEL);
  tft.drawString("PAGE " + String(this->bt_flipper_data_page + 1) + "/" + String(page_count),
                 232, 58, 1);

  if (total_devices == 0) {
    wdPanel(tft, 12, 104, 216, 112, WD_SURFACE, WD_EDGE, WD_AMBER);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WD_AMBER, WD_SURFACE);
    tft.drawString("NO FLIPPER MATCHES", 120, 145, 2);
    tft.setTextColor(WD_DIM, WD_SURFACE);
    tft.drawString("RETURN TO DASH AND START", 120, 176, 1);
  } else {
    for (uint8_t row = 0; row < rows_per_page; row++) {
      const uint16_t offset = this->bt_flipper_data_page * rows_per_page + row;
      if (offset >= total_devices) break;
      const int32_t device_index = (int32_t)total_devices - 1 - offset;
      Flipper device = flippers->get(device_index);
      const int16_t y = 76 + row * 38;
      const uint16_t signal_color = device.rssi >= -65 ? WD_CYAN :
                                    device.rssi >= -85 ? WD_AMBER : WD_DIM;
      wdPanel(tft, 2, y, 236, 34, WD_SURFACE, WD_EDGE, signal_color);
      String name = device.name.length() ? device.name : "FLIPPER ZERO";
      while (name.length() > 3 && tft.textWidth(name, 1) > 150)
        name.remove(name.length() - 1);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_BONE, WD_SURFACE);
      tft.drawString(name, 9, y + 5, 1);
      tft.setTextDatum(TR_DATUM);
      tft.setTextColor(signal_color, WD_SURFACE);
      tft.drawString(String(device.rssi) + " dBm", 231, y + 5, 1);
      String detail = device.mac;
      if (device.color.length()) detail += "  " + device.color;
      while (detail.length() > 3 && tft.textWidth(detail, 1) > 220)
        detail.remove(detail.length() - 1);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_DIM, WD_SURFACE);
      tft.drawString(detail, 9, y + 20, 1);
    }
  }

  static const char* const labels[3] = {"PREV", "NEXT", "DASH"};
  const bool can_prev = this->bt_flipper_data_page > 0;
  const bool can_next = this->bt_flipper_data_page + 1 < page_count;
  for (uint8_t i = 0; i < 3; i++) {
    const int16_t x = i * 80 + 2;
    const bool enabled = i == 0 ? can_prev : (i == 1 ? can_next : true);
    const uint16_t accent = i == 2 ? WD_CYAN : (enabled ? WD_BONE : WD_EDGE);
    const uint16_t fill = i == 2 ? WD_CYAN : WD_SURFACE;
    const uint16_t text = i == 2 ? TFT_BLACK : (enabled ? WD_BONE : WD_DIM);
    wdPanel(tft, x, 274, 76, 42, fill, accent, accent);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(text, fill);
    tft.drawString(labels[i], x + 38, 295, 1);
  }
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::handleFlipperSnifferTouch(uint16_t touch_x, uint16_t touch_y) {
  if (touch_y < 268) return;

  if (this->bt_flipper_data_view) {
    extern LinkedList<Flipper>* flippers;
    const uint16_t total_devices = flippers ? flippers->size() : 0;
    const uint16_t page_count = max((uint16_t)1, (uint16_t)((total_devices + 4) / 5));
    if (touch_x < 80) {
      if (this->bt_flipper_data_page > 0) this->bt_flipper_data_page--;
      this->drawFlipperSnifferDataUI(true);
    } else if (touch_x < 160) {
      if (this->bt_flipper_data_page + 1 < page_count) this->bt_flipper_data_page++;
      this->drawFlipperSnifferDataUI(true);
    } else {
      const bool resume = this->bt_flipper_resume_after_data;
      this->bt_flipper_data_view = false;
      this->bt_flipper_resume_after_data = false;
      if (resume) wifi_scan_obj.startFlipperSniffer();
      this->drawFlipperSnifferUI(true);
    }
    return;
  }

  const uint8_t action = min((uint8_t)4, (uint8_t)(touch_x / 48));
  if (action == 0) {
    wifi_scan_obj.startFlipperSniffer();
  } else if (action == 1) {
    wifi_scan_obj.stopFlipperSniffer();
  } else if (action == 2) {
    wifi_scan_obj.clearFlipperSniffer();
  } else if (action == 3) {
    this->bt_flipper_resume_after_data = wifi_scan_obj.bt_flipper_running;
    if (wifi_scan_obj.bt_flipper_running) {
      wifi_scan_obj.stopFlipperSniffer();
      const uint32_t wait_started = millis();
      while (wifi_scan_obj.bt_cb_busy && millis() - wait_started < 150) delay(1);
    }
    this->bt_flipper_data_page = 0;
    this->bt_flipper_data_view = true;
    this->drawFlipperSnifferDataUI(true);
    return;
  } else {
    this->bt_flipper_data_view = false;
    this->bt_flipper_resume_after_data = false;
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
    display_obj.init();
    this->changeMenu(current_menu, true);
    return;
  }
  this->drawFlipperSnifferUI(true);
}

void MenuFunctions::drawCardSkimmerUI(bool full_redraw) {
  TFT_eSPI& tft = display_obj.tft;
  const bool running = wifi_scan_obj.bt_skimmer_running;
  this->bt_skimmer_data_view = false;

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  tft.setTextDatum(TL_DATUM);
  if (full_redraw) {
    tft.fillScreen(TFT_BLACK);
    this->drawSharkTopBar(true);
    tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, 25, TFT_BLACK);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString("// SKIM DETECT", 6, STATUS_BAR_WIDTH + 7, 2);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 66, WD_CYAN);
  }

  const uint16_t state_color = running ? WD_CYAN : WD_AMBER;
  tft.fillRoundRect(176, STATUS_BAR_WIDTH + 4, 59, 17, 3, state_color);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, state_color);
  tft.drawString(running ? "SEARCH" : "PAUSED", 205, STATUS_BAR_WIDTH + 12, 1);

  const int16_t card_y = 49;
  const int16_t card_w = 76;
  const char* labels[3] = {"MATCH/S", "SUSPECTS", "TOTAL"};
  const String values[3] = {
    String(wifi_scan_obj.bt_skimmer_rate),
    String(wifi_scan_obj.bt_skimmer_unique),
    String((uint32_t)wifi_scan_obj.bt_skimmer_total)
  };
  for (uint8_t i = 0; i < 3; i++) {
    const int16_t x = 2 + i * 80;
    wdPanel(tft, x, card_y, card_w, 42, WD_SURFACE, WD_EDGE,
            i == 1 && wifi_scan_obj.bt_skimmer_unique ? WD_AMBER : state_color);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_SURFACE);
    tft.drawString(labels[i], x + 7, card_y + 5, 1);
    tft.setTextDatum(MR_DATUM);
    const uint16_t value_color = i == 1 && wifi_scan_obj.bt_skimmer_unique ? WD_AMBER : WD_BONE;
    tft.setTextColor(value_color, WD_SURFACE);
    tft.drawString(values[i], x + card_w - 7, card_y + 27, 2);
  }

  wdPanel(tft, 2, 95, 236, 55, WD_PANEL, WD_EDGE,
          wifi_scan_obj.bt_skimmer_unique ? WD_AMBER : WD_CYAN);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WD_DIM, WD_PANEL);
  tft.drawString("LAST POTENTIAL MATCH", 8, 101, 1);
  String device_name = wifi_scan_obj.bt_skimmer_last_name;
  if (!device_name.length()) device_name = running ? "MONITORING HC-03/05/06" : "NO MATCH";
  while (device_name.length() > 3 && tft.textWidth(device_name, 2) > 220)
    device_name.remove(device_name.length() - 1);
  tft.setTextColor(wifi_scan_obj.bt_skimmer_unique ? WD_AMBER : WD_BONE, WD_PANEL);
  tft.drawString(device_name, 8, 114, 2);
  String detail = wifi_scan_obj.bt_skimmer_last_mac;
  if (detail.length()) detail += "  " + String(wifi_scan_obj.bt_skimmer_last_rssi) + "dBm";
  else detail = "NAME HEURISTIC - VERIFY DEVICE";
  while (detail.length() > 3 && tft.textWidth(detail, 1) > 224)
    detail.remove(detail.length() - 1);
  tft.setTextColor(wifi_scan_obj.bt_skimmer_unique ? WD_AMBER : WD_CYAN, WD_PANEL);
  tft.drawString(detail, 8, 137, 1);

  const int16_t meter_x = 6, meter_y = 156, meter_w = 228, meter_h = 17;
  tft.fillRoundRect(meter_x, meter_y, meter_w, meter_h, 3, WD_SURFACE);
  tft.drawRoundRect(meter_x, meter_y, meter_w, meter_h, 3, WD_EDGE);
  const bool has_signal = wifi_scan_obj.bt_skimmer_last_rssi > -128;
  const int16_t clamped_rssi = constrain(wifi_scan_obj.bt_skimmer_last_rssi, -100, -35);
  const int16_t signal_w = has_signal ?
      (int16_t)((int32_t)(clamped_rssi + 100) * (meter_w - 4) / 65) : 0;
  const uint16_t meter_color = clamped_rssi > -65 ? WD_AMBER : WD_CYAN;
  if (signal_w > 0)
    tft.fillRoundRect(meter_x + 2, meter_y + 2, signal_w, meter_h - 4, 2, meter_color);
  const bool label_on_meter = has_signal && signal_w > meter_w / 2;
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(label_on_meter ? TFT_BLACK : (has_signal ? WD_BONE : WD_DIM),
                   label_on_meter ? meter_color : WD_SURFACE);
  tft.drawString(has_signal ? "POTENTIAL DEVICE RSSI" : "NO SIGNATURE SIGNAL",
                 meter_x + meter_w / 2, meter_y + meter_h / 2, 1);

  const int16_t gx = 6, gy = 178, gw = 228, gh = 86;
  wdPanel(tft, gx, gy, gw, gh, WD_SURFACE, WD_EDGE,
          wifi_scan_obj.bt_skimmer_unique ? WD_AMBER : state_color);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WD_DIM, WD_SURFACE);
  tft.drawString("SIGNATURE MATCH PULSE", gx + 8, gy + 5, 1);
  const int16_t plot_x = gx + 6;
  const int16_t plot_y = gy + 20;
  const int16_t plot_w = gw - 12;
  const int16_t plot_h = gh - 27;
  tft.fillRect(plot_x, plot_y, plot_w, plot_h, TFT_BLACK);
  for (uint8_t line = 1; line < 3; line++)
    tft.drawFastHLine(plot_x, plot_y + (plot_h * line / 3), plot_w, WD_PANEL);
  int16_t graph_max = 1;
  for (int16_t i = 0; i < plot_w; i++)
    if (wifi_scan_obj._analyzer_values[i] > graph_max)
      graph_max = wifi_scan_obj._analyzer_values[i];
  const uint16_t graph_color = wifi_scan_obj.bt_skimmer_unique ? WD_AMBER :
                               (running ? WD_CYAN : WD_CYAN_DIM);
  for (int16_t i = 0; i < plot_w; i++) {
    const int16_t sample = max((int16_t)0, wifi_scan_obj._analyzer_values[i]);
    const int16_t bar = sample ? max((int16_t)1,
                                     (int16_t)((int32_t)sample * (plot_h - 2) / graph_max)) : 0;
    if (bar)
      tft.drawFastVLine(plot_x + plot_w - 1 - i, plot_y + plot_h - bar, bar, graph_color);
  }

  static const char* const button_labels[5] = {"START", "STOP", "CLEAR", "DATA", "BACK"};
  for (uint8_t i = 0; i < 5; i++) {
    const int16_t x = i * 48 + 2;
    bool enabled = true;
    uint16_t fill = WD_SURFACE;
    uint16_t accent = WD_CYAN;
    uint16_t text = WD_BONE;
    if (i == 0) {
      enabled = !running;
      fill = enabled ? WD_CYAN : WD_SURFACE;
      text = enabled ? TFT_BLACK : WD_DIM;
    } else if (i == 1) {
      enabled = running;
      fill = enabled ? WD_AMBER : WD_SURFACE;
      accent = WD_AMBER;
      text = enabled ? TFT_BLACK : WD_DIM;
    } else if (i == 2) {
      accent = WD_BONE;
    } else if (i == 3) {
      text = WD_CYAN;
    } else {
      accent = WD_AMBER;
      text = WD_AMBER;
    }
    wdPanel(tft, x, 274, 44, 42, fill, enabled ? accent : WD_EDGE, accent);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(text, fill);
    tft.drawString(button_labels[i], x + 22, 295, 1);
  }
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::drawCardSkimmerDataUI(bool full_redraw) {
  extern LinkedList<CardSkimmer>* card_skimmers;
  TFT_eSPI& tft = display_obj.tft;
  const uint8_t rows_per_page = 5;
  const uint16_t total_devices = card_skimmers ? card_skimmers->size() : 0;
  const uint16_t page_count = max((uint16_t)1,
                                  (uint16_t)((total_devices + rows_per_page - 1) / rows_per_page));
  if (this->bt_skimmer_data_page >= page_count)
    this->bt_skimmer_data_page = page_count - 1;

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  tft.setTextDatum(TL_DATUM);
  if (full_redraw) {
    tft.fillScreen(TFT_BLACK);
    this->drawSharkTopBar(true);
  }
  tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_WIDTH, TFT_BLACK);
  tft.setTextColor(WD_CYAN, TFT_BLACK);
  tft.drawString("// SKIMMER DATA", 6, STATUS_BAR_WIDTH + 7, 2);
  tft.fillRoundRect(174, STATUS_BAR_WIDTH + 4, 61, 17, 3, WD_AMBER);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, WD_AMBER);
  tft.drawString("SNAPSHOT", 204, STATUS_BAR_WIDTH + 12, 1);
  tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
  tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 60, WD_CYAN);

  wdPanel(tft, 2, 51, 236, 21, WD_PANEL, WD_EDGE,
          total_devices ? WD_AMBER : WD_CYAN);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WD_BONE, WD_PANEL);
  tft.drawString("POTENTIAL " + String(total_devices), 8, 58, 1);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(WD_DIM, WD_PANEL);
  tft.drawString("PAGE " + String(this->bt_skimmer_data_page + 1) + "/" + String(page_count),
                 232, 58, 1);

  if (total_devices == 0) {
    wdPanel(tft, 12, 104, 216, 112, WD_SURFACE, WD_EDGE, WD_CYAN);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WD_CYAN, WD_SURFACE);
    tft.drawString("NO NAME MATCHES", 120, 140, 2);
    tft.setTextColor(WD_DIM, WD_SURFACE);
    tft.drawString("HC-03 / HC-05 / HC-06", 120, 170, 1);
    tft.drawString("NOT PROOF OF A SKIMMER", 120, 188, 1);
  } else {
    for (uint8_t row = 0; row < rows_per_page; row++) {
      const uint16_t offset = this->bt_skimmer_data_page * rows_per_page + row;
      if (offset >= total_devices) break;
      const int32_t device_index = (int32_t)total_devices - 1 - offset;
      CardSkimmer device = card_skimmers->get(device_index);
      const int16_t y = 76 + row * 38;
      const uint16_t signal_color = device.rssi >= -65 ? WD_AMBER :
                                    device.rssi >= -85 ? WD_CYAN : WD_DIM;
      wdPanel(tft, 2, y, 236, 34, WD_SURFACE, WD_EDGE, signal_color);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_BONE, WD_SURFACE);
      tft.drawString(device.name, 9, y + 5, 1);
      tft.setTextDatum(TR_DATUM);
      tft.setTextColor(signal_color, WD_SURFACE);
      tft.drawString(String(device.rssi) + " dBm", 231, y + 5, 1);
      String detail = device.mac + "  HITS " + String(device.hits);
      while (detail.length() > 3 && tft.textWidth(detail, 1) > 220)
        detail.remove(detail.length() - 1);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_DIM, WD_SURFACE);
      tft.drawString(detail, 9, y + 20, 1);
    }
  }

  static const char* const labels[3] = {"PREV", "NEXT", "DASH"};
  const bool can_prev = this->bt_skimmer_data_page > 0;
  const bool can_next = this->bt_skimmer_data_page + 1 < page_count;
  for (uint8_t i = 0; i < 3; i++) {
    const int16_t x = i * 80 + 2;
    const bool enabled = i == 0 ? can_prev : (i == 1 ? can_next : true);
    const uint16_t accent = i == 2 ? WD_CYAN : (enabled ? WD_BONE : WD_EDGE);
    const uint16_t fill = i == 2 ? WD_CYAN : WD_SURFACE;
    const uint16_t text = i == 2 ? TFT_BLACK : (enabled ? WD_BONE : WD_DIM);
    wdPanel(tft, x, 274, 76, 42, fill, accent, accent);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(text, fill);
    tft.drawString(labels[i], x + 38, 295, 1);
  }
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::handleCardSkimmerTouch(uint16_t touch_x, uint16_t touch_y) {
  if (touch_y < 268) return;

  if (this->bt_skimmer_data_view) {
    extern LinkedList<CardSkimmer>* card_skimmers;
    const uint16_t total_devices = card_skimmers ? card_skimmers->size() : 0;
    const uint16_t page_count = max((uint16_t)1, (uint16_t)((total_devices + 4) / 5));
    if (touch_x < 80) {
      if (this->bt_skimmer_data_page > 0) this->bt_skimmer_data_page--;
      this->drawCardSkimmerDataUI(true);
    } else if (touch_x < 160) {
      if (this->bt_skimmer_data_page + 1 < page_count) this->bt_skimmer_data_page++;
      this->drawCardSkimmerDataUI(true);
    } else {
      const bool resume = this->bt_skimmer_resume_after_data;
      this->bt_skimmer_data_view = false;
      this->bt_skimmer_resume_after_data = false;
      if (resume) wifi_scan_obj.startCardSkimmerDetector();
      this->drawCardSkimmerUI(true);
    }
    return;
  }

  const uint8_t action = min((uint8_t)4, (uint8_t)(touch_x / 48));
  if (action == 0) {
    wifi_scan_obj.startCardSkimmerDetector();
  } else if (action == 1) {
    wifi_scan_obj.stopCardSkimmerDetector();
  } else if (action == 2) {
    wifi_scan_obj.clearCardSkimmerDetector();
  } else if (action == 3) {
    this->bt_skimmer_resume_after_data = wifi_scan_obj.bt_skimmer_running;
    if (wifi_scan_obj.bt_skimmer_running) {
      wifi_scan_obj.stopCardSkimmerDetector();
      const uint32_t wait_started = millis();
      while (wifi_scan_obj.bt_cb_busy && millis() - wait_started < 150) delay(1);
    }
    this->bt_skimmer_data_page = 0;
    this->bt_skimmer_data_view = true;
    this->drawCardSkimmerDataUI(true);
    return;
  } else {
    this->bt_skimmer_data_view = false;
    this->bt_skimmer_resume_after_data = false;
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
    display_obj.init();
    this->changeMenu(current_menu, true);
    return;
  }
  this->drawCardSkimmerUI(true);
}

void MenuFunctions::drawPassiveBleDetectorUI(bool full_redraw) {
  TFT_eSPI& tft = display_obj.tft;
  const uint8_t mode = wifi_scan_obj.currentScanMode;
  const bool running = wifi_scan_obj.bt_passive_running;
  this->bt_passive_data_view = false;
  const char* title = mode == BT_SCAN_FLOCK ? "// FLOCK SNIFF" :
                      mode == BT_SCAN_RAYBAN ? "// META DETECT" :
                      mode == BT_SCAN_AIRTAG_MON ? "// FINDMY MONITOR" : "// FINDMY SNIFF";
  const char* entity = mode == BT_SCAN_FLOCK ? "FLOCK" :
                       mode == BT_SCAN_RAYBAN ? "META" : "TRACKERS";
  const char* waiting = mode == BT_SCAN_FLOCK ? "LISTENING FLOCK SIGNATURES" :
                        mode == BT_SCAN_RAYBAN ? "LISTENING META IDENTIFIERS" :
                        "LISTENING FINDMY/DULT";

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  tft.setTextDatum(TL_DATUM);
  if (full_redraw) {
    tft.fillScreen(TFT_BLACK);
    this->drawSharkTopBar(true);
    tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, 25, TFT_BLACK);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString(title, 6, STATUS_BAR_WIDTH + 7, 2);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 60, WD_CYAN);
  }

  const uint16_t state_color = running ? WD_CYAN : WD_AMBER;
  tft.fillRoundRect(176, STATUS_BAR_WIDTH + 4, 59, 17, 3, state_color);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, state_color);
  tft.drawString(running ? "SCAN" : "PAUSED", 205, STATUS_BAR_WIDTH + 12, 1);

  // Flock counts its own detections (flock_devices, incremented by both the
  // BLE-signature and the WiFi-side callbacks); the other passive tools use
  // the BLE unique-device counter.
  const uint32_t match_count = mode == BT_SCAN_FLOCK
                                 ? wifi_scan_obj.flock_devices
                                 : wifi_scan_obj.bt_passive_unique;
  const bool has_match = match_count > 0;

  const char* labels[3] = {"MATCH/S", entity, "TOTAL"};
  const String values[3] = {String(wifi_scan_obj.bt_passive_rate),
                            String(match_count),
                            String((uint32_t)wifi_scan_obj.bt_passive_total)};
  for (uint8_t i = 0; i < 3; i++) {
    const int16_t x = 2 + i * 80;
    wdPanel(tft, x, 49, 76, 42, WD_SURFACE, WD_EDGE,
            i == 1 && has_match ? WD_AMBER : state_color);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_SURFACE);
    tft.drawString(labels[i], x + 7, 54, 1);
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(i == 1 && has_match ? WD_AMBER : WD_BONE,
                     WD_SURFACE);
    tft.drawString(values[i], x + 69, 76, 2);
  }

  wdPanel(tft, 2, 95, 236, 55, WD_PANEL, WD_EDGE,
          has_match ? WD_AMBER : WD_CYAN);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WD_DIM, WD_PANEL);
  tft.drawString("LAST REAL MATCH", 8, 101, 1);
  String name = wifi_scan_obj.bt_passive_last_name;
  if (!name.length()) name = running ? waiting : "NO MATCH";
  while (name.length() > 3 && tft.textWidth(name, 2) > 220) name.remove(name.length() - 1);
  tft.setTextColor(has_match ? WD_AMBER : WD_BONE, WD_PANEL);
  tft.drawString(name, 8, 114, 2);
  String detail = wifi_scan_obj.bt_passive_last_mac;
  if (detail.length()) detail += "  " + String(wifi_scan_obj.bt_passive_last_rssi) + "dBm";
  else detail = "SIGNATURE-BASED DETECTION";
  while (detail.length() > 3 && tft.textWidth(detail, 1) > 224) detail.remove(detail.length() - 1);
  tft.setTextColor(WD_CYAN, WD_PANEL);
  tft.drawString(detail, 8, 137, 1);

  const bool has_signal = wifi_scan_obj.bt_passive_last_rssi > -128;
  const int16_t clamped = constrain(wifi_scan_obj.bt_passive_last_rssi, -100, -35);
  const int16_t signal_w = has_signal ? (int16_t)((int32_t)(clamped + 100) * 224 / 65) : 0;
  const uint16_t signal_color = clamped > -65 ? WD_AMBER : WD_CYAN;
  tft.fillRoundRect(6, 156, 228, 17, 3, WD_SURFACE);
  tft.drawRoundRect(6, 156, 228, 17, 3, WD_EDGE);
  if (signal_w > 0) tft.fillRoundRect(8, 158, signal_w, 13, 2, signal_color);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(has_signal && signal_w > 114 ? TFT_BLACK : (has_signal ? WD_BONE : WD_DIM),
                   has_signal && signal_w > 114 ? signal_color : WD_SURFACE);
  tft.drawString(has_signal ? "MATCH RSSI" : "NO MATCH SIGNAL", 120, 164, 1);

  wdPanel(tft, 6, 178, 228, 86, WD_SURFACE, WD_EDGE, state_color);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WD_DIM, WD_SURFACE);
  tft.drawString("REAL MATCH ACTIVITY", 14, 183, 1);
  const int16_t px = 12, py = 198, pw = 216, ph = 59;
  tft.fillRect(px, py, pw, ph, TFT_BLACK);
  for (uint8_t line = 1; line < 3; line++) tft.drawFastHLine(px, py + ph * line / 3, pw, WD_PANEL);
  int16_t graph_max = 1;
  for (int16_t i = 0; i < pw; i++)
    if (wifi_scan_obj._analyzer_values[i] > graph_max) graph_max = wifi_scan_obj._analyzer_values[i];
  for (int16_t i = 0; i < pw; i++) {
    const int16_t sample = max((int16_t)0, wifi_scan_obj._analyzer_values[i]);
    const int16_t bar = sample ? max((int16_t)1, (int16_t)((int32_t)sample * (ph - 2) / graph_max)) : 0;
    if (bar) tft.drawFastVLine(px + pw - 1 - i, py + ph - bar, bar,
                               running ? WD_CYAN : WD_CYAN_DIM);
  }

  static const char* const buttons[5] = {"START", "STOP", "CLEAR", "DATA", "BACK"};
  for (uint8_t i = 0; i < 5; i++) {
    const int16_t x = i * 48 + 2;
    bool enabled = i == 0 ? !running : (i == 1 ? running : true);
    uint16_t fill = WD_SURFACE, accent = WD_CYAN, text = WD_BONE;
    if (i == 0) { fill = enabled ? WD_CYAN : WD_SURFACE; text = enabled ? TFT_BLACK : WD_DIM; }
    else if (i == 1) { fill = enabled ? WD_AMBER : WD_SURFACE; accent = WD_AMBER; text = enabled ? TFT_BLACK : WD_DIM; }
    else if (i == 2) accent = WD_BONE;
    else if (i == 3) text = WD_CYAN;
    else { accent = WD_AMBER; text = WD_AMBER; }
    wdPanel(tft, x, 274, 44, 42, fill, enabled ? accent : WD_EDGE, accent);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(text, fill);
    tft.drawString(buttons[i], x + 22, 295, 1);
  }
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::drawPassiveBleDetectorDataUI(bool full_redraw) {
  extern LinkedList<PassiveBleFinding>* passive_ble_findings;
  TFT_eSPI& tft = display_obj.tft;
  const uint8_t rows_per_page = 5;
  const uint16_t total = passive_ble_findings ? passive_ble_findings->size() : 0;
  const uint16_t pages = max((uint16_t)1, (uint16_t)((total + rows_per_page - 1) / rows_per_page));
  if (this->bt_passive_data_page >= pages) this->bt_passive_data_page = pages - 1;
  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  if (full_redraw) { tft.fillScreen(TFT_BLACK); this->drawSharkTopBar(true); }
  tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_WIDTH, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WD_CYAN, TFT_BLACK);
  tft.drawString("// DETECTOR DATA", 6, STATUS_BAR_WIDTH + 7, 2);
  tft.fillRoundRect(174, STATUS_BAR_WIDTH + 4, 61, 17, 3, WD_AMBER);
  tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_BLACK, WD_AMBER);
  tft.drawString("SNAPSHOT", 204, STATUS_BAR_WIDTH + 12, 1);
  tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
  wdPanel(tft, 2, 51, 236, 21, WD_PANEL, WD_EDGE, total ? WD_AMBER : WD_CYAN);
  tft.setTextDatum(TL_DATUM); tft.setTextColor(WD_BONE, WD_PANEL);
  tft.drawString("REAL MATCHES " + String(total), 8, 58, 1);
  tft.setTextDatum(TR_DATUM); tft.setTextColor(WD_DIM, WD_PANEL);
  tft.drawString("PAGE " + String(this->bt_passive_data_page + 1) + "/" + String(pages), 232, 58, 1);
  if (!total) {
    wdPanel(tft, 12, 104, 216, 112, WD_SURFACE, WD_EDGE, WD_CYAN);
    tft.setTextDatum(MC_DATUM); tft.setTextColor(WD_CYAN, WD_SURFACE);
    tft.drawString("NO DETECTIONS", 120, 145, 2);
    tft.setTextColor(WD_DIM, WD_SURFACE);
    tft.drawString("DASH TO CONTINUE SCANNING", 120, 178, 1);
  } else {
    for (uint8_t row = 0; row < rows_per_page; row++) {
      const uint16_t offset = this->bt_passive_data_page * rows_per_page + row;
      if (offset >= total) break;
      PassiveBleFinding item = passive_ble_findings->get((int32_t)total - 1 - offset);
      const int16_t y = 76 + row * 38;
      const uint16_t c = item.rssi >= -65 ? WD_AMBER : (item.rssi >= -85 ? WD_CYAN : WD_DIM);
      wdPanel(tft, 2, y, 236, 34, WD_SURFACE, WD_EDGE, c);
      String name = item.name.length() ? item.name : "MATCH";
      while (name.length() > 3 && tft.textWidth(name, 1) > 145) name.remove(name.length() - 1);
      tft.setTextDatum(TL_DATUM); tft.setTextColor(WD_BONE, WD_SURFACE); tft.drawString(name, 9, y + 5, 1);
      tft.setTextDatum(TR_DATUM); tft.setTextColor(c, WD_SURFACE); tft.drawString(String(item.rssi) + " dBm", 231, y + 5, 1);
      String detail = item.mac + "  H" + String(item.hits);
      tft.setTextDatum(TL_DATUM); tft.setTextColor(WD_DIM, WD_SURFACE); tft.drawString(detail, 9, y + 20, 1);
    }
  }
  static const char* const labels[3] = {"PREV", "NEXT", "DASH"};
  const bool can_prev = this->bt_passive_data_page > 0;
  const bool can_next = this->bt_passive_data_page + 1 < pages;
  for (uint8_t i = 0; i < 3; i++) {
    const bool enabled = i == 0 ? can_prev : (i == 1 ? can_next : true);
    const int16_t x = i * 80 + 2;
    const uint16_t fill = i == 2 ? WD_CYAN : WD_SURFACE;
    wdPanel(tft, x, 274, 76, 42, fill, i == 2 ? WD_CYAN : (enabled ? WD_BONE : WD_EDGE), WD_CYAN);
    tft.setTextDatum(MC_DATUM); tft.setTextColor(i == 2 ? TFT_BLACK : (enabled ? WD_BONE : WD_DIM), fill);
    tft.drawString(labels[i], x + 38, 295, 1);
  }
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::handlePassiveBleDetectorTouch(uint16_t touch_x, uint16_t touch_y) {
  if (touch_y < 268) return;
  if (this->bt_passive_data_view) {
    extern LinkedList<PassiveBleFinding>* passive_ble_findings;
    const uint16_t total = passive_ble_findings ? passive_ble_findings->size() : 0;
    const uint16_t pages = max((uint16_t)1, (uint16_t)((total + 4) / 5));
    if (touch_x < 80) {
      if (this->bt_passive_data_page) this->bt_passive_data_page--;
      this->drawPassiveBleDetectorDataUI(true);
    } else if (touch_x < 160) {
      if (this->bt_passive_data_page + 1 < pages) this->bt_passive_data_page++;
      this->drawPassiveBleDetectorDataUI(true);
    } else {
      const bool resume = this->bt_passive_resume_after_data;
      this->bt_passive_data_view = false;
      this->bt_passive_resume_after_data = false;
      if (resume) wifi_scan_obj.startPassiveBleDetector();
      this->drawPassiveBleDetectorUI(true);
    }
    return;
  }
  const uint8_t action = min((uint8_t)4, (uint8_t)(touch_x / 48));
  if (action == 0) wifi_scan_obj.startPassiveBleDetector();
  else if (action == 1) wifi_scan_obj.stopPassiveBleDetector();
  else if (action == 2) wifi_scan_obj.clearPassiveBleDetector();
  else if (action == 3) {
    this->bt_passive_resume_after_data = wifi_scan_obj.bt_passive_running;
    if (wifi_scan_obj.bt_passive_running) {
      wifi_scan_obj.stopPassiveBleDetector();
      const uint32_t started = millis();
      while (wifi_scan_obj.bt_cb_busy && millis() - started < 150) delay(1);
    }
    this->bt_passive_data_page = 0;
    this->bt_passive_data_view = true;
    this->drawPassiveBleDetectorDataUI(true);
    return;
  } else {
    this->bt_passive_data_view = false;
    this->bt_passive_resume_after_data = false;
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
    display_obj.init();
    this->changeMenu(current_menu, true);
    return;
  }
  this->drawPassiveBleDetectorUI(true);
}

// --- Shared Wi-Fi monitor dashboards ----------------------------------------

bool MenuFunctions::isWifiToolUiMode(uint8_t mode) const {
  return mode == WIFI_SCAN_PACKET_RATE ||
         mode == WIFI_SCAN_EAPOL ||
         mode == WIFI_SCAN_ACTIVE_EAPOL ||
         mode == WIFI_SCAN_ACTIVE_LIST_EAPOL ||
         mode == WIFI_PACKET_MONITOR ||
         mode == WIFI_SCAN_CHAN_ANALYZER ||
         mode == WIFI_SCAN_CHAN_ACT ||
         mode == WIFI_SCAN_RAW_CAPTURE ||
         mode == WIFI_SCAN_PWN ||
         mode == WIFI_SCAN_PINESCAN ||
         mode == WIFI_SCAN_MULTISSID ||
         mode == WIFI_SCAN_AP_STA ||
         mode == WIFI_SCAN_DETECT_FOLLOW ||
         mode == WIFI_SCAN_SAE_COMMIT ||
         mode == WIFI_SCAN_WAR_DRIVE ||
         mode == SHARK_DRONE_RID_SCAN;
}

void MenuFunctions::startPassiveWifiToolUI(uint8_t mode, uint16_t color) {
  this->wifi_tool_ui = false;
  this->wifi_packet_target_view = false;
  this->wifi_packet_target_scan = false;
  this->wifi_packet_target_resume_after_select = false;
  this->wifi_passive_ui = true;
  this->wifi_passive_data_view = false;
  this->wifi_passive_draw_valid = false;
  wifi_scan_obj.wifi_tool_ui_owned = true;

  // Paint the final theme before radio setup. RunProbe/Beacon/Deauth now skip
  // their legacy title/buttons while this ownership flag is set.
  display_obj.tft.fillScreen(TFT_BLACK);
  this->drawSharkTopBar(true);
  wifi_scan_obj.StartScan(mode, color);
  wifi_scan_obj.clearPassiveWifiDetector();
  wifi_scan_obj.startPassiveWifiDetector();
  this->drawPassiveWifiDetectorUI(true);
}

void MenuFunctions::startWifiToolUI(uint8_t mode, uint16_t color) {
  this->wifi_passive_ui = false;
  this->wifi_passive_data_view = false;
  this->wifi_tool_ui = true;
  this->wifi_tool_touch_active = false;
  this->wifi_tool_touch_region = -1;
  this->wifi_tool_touch_ms = 0;
  // The menu tile that launched this screen can remain electrically pressed
  // for a few frames. Ignore that tail without waiting indefinitely for a
  // release event the controller may miss.
  this->wifi_tool_touch_ready_at = millis() + 320;
  this->wifi_packet_target_view = false;
  if (mode != WIFI_SCAN_AP_STA)
    this->wifi_packet_target_scan = false;
  if (mode == WIFI_SCAN_PACKET_RATE)
    this->wifi_packet_target_resume_after_select = false;
  this->wifi_tool_draw_valid = false;
  this->wifi_tool_draw_mode = 0xFF;
  wifi_scan_obj.wifi_tool_ui_owned = true;

  // The themed shell is visible immediately, so there is no legacy-to-new
  // frame between tapping the menu tile and the radio becoming ready.
  display_obj.tft.fillScreen(TFT_BLACK);
  this->drawSharkTopBar(true);
  wifi_scan_obj.StartScan(mode, color);
  if (mode == WIFI_SCAN_PACKET_RATE)
    this->resetPacketTargetCounters();
  this->drawWifiToolUI(true);
}

void MenuFunctions::drawWifiToolUI(bool full_redraw) {
  extern LinkedList<AccessPoint>* access_points;
  extern LinkedList<Station>* stations;
  TFT_eSPI& tft = display_obj.tft;
  const uint8_t mode = wifi_scan_obj.currentScanMode;
  if (!this->isWifiToolUiMode(mode)) return;

  // Wardrive and MAC Monitor stream new state constantly, so the dirty
  // checks would repaint tiles and the whole content panel on EVERY loop
  // pass -- that is the visible flicker/glitch on those screens. Cap the
  // live refresh at ~1 Hz; touch actions still redraw immediately
  // (full_redraw bypasses).
  if (mode == WIFI_SCAN_WAR_DRIVE || mode == WIFI_SCAN_DETECT_FOLLOW) {
    if (!full_redraw && millis() - this->wifi_tool_wardrive_ms < 1000)
      return;
    this->wifi_tool_wardrive_ms = millis();
  }

  const bool running = wifi_scan_obj.wifi_tool_running;
  const char* title = "// WIFI MONITOR";
  const char* labels[3] = {"FRAMES", "APS", "CHANNEL"};
  uint32_t stats[6] = {0};
  String signature;
  bool graph_mode = false;
  bool packet_graph = false;
  bool channel_graph = false;

  stats[0] = wifi_scan_obj.mgmt_frames + wifi_scan_obj.data_frames;
  stats[1] = access_points ? access_points->size() : 0;
  stats[2] = wifi_scan_obj.set_channel;
  stats[3] = wifi_scan_obj.mgmt_frames;
  stats[4] = wifi_scan_obj.data_frames;
  stats[5] = wifi_scan_obj.deauth_frames;

  if (mode == WIFI_SCAN_PACKET_RATE) {
    title = "// PACKET COUNT";
    labels[0] = "TARGETS"; labels[1] = "PACKETS"; labels[2] = "CHANNEL";
    stats[0] = 0; stats[1] = 0;
    for (int i = 0; access_points && i < access_points->size(); i++) {
      AccessPoint ap = access_points->get(i);
      if (!ap.selected) continue;
      stats[0]++; stats[1] += ap.packets;
      signature += ap.essid + ":" + String(ap.packets) + ";";
    }
    for (int i = 0; stations && i < stations->size(); i++) {
      Station sta = stations->get(i);
      if (!sta.selected) continue;
      stats[0]++; stats[1] += sta.packets;
      signature += macToString(sta.mac) + ":" + String(sta.packets) + ";";
    }
  } else if (mode == WIFI_SCAN_EAPOL || mode == WIFI_SCAN_ACTIVE_EAPOL ||
             mode == WIFI_SCAN_ACTIVE_LIST_EAPOL) {
    title = "// EAPOL / PMKID";
    labels[0] = "EAPOL"; labels[1] = "COMPLETE"; labels[2] = "CHANNEL";
    stats[0] = wifi_scan_obj.eapol_frames;
    stats[1] = wifi_scan_obj.getCompleteEapol();
    stats[2] = wifi_scan_obj.set_channel;
    stats[3] = wifi_scan_obj.mgmt_frames;
    stats[4] = wifi_scan_obj.data_frames;
    stats[5] = wifi_scan_obj.deauth_frames;
  } else if (mode == SHARK_DRONE_RID_SCAN) {
    // Cyber Defense Drone RID detector: unique Remote ID broadcasts heard.
    title = "// DRONE RID";
    labels[0] = "DRONES"; labels[1] = "FRAMES"; labels[2] = "CHANNEL";
    stats[0] = wifi_scan_obj.drone_rid_count;
    stats[1] = wifi_scan_obj.mgmt_frames + wifi_scan_obj.data_frames;
    stats[2] = wifi_scan_obj.set_channel;
    stats[3] = wifi_scan_obj.mgmt_frames;
    stats[4] = wifi_scan_obj.data_frames;
    stats[5] = wifi_scan_obj.deauth_frames;
  } else if (mode == WIFI_PACKET_MONITOR) {
    title = "// PACKET MONITOR";
    labels[0] = "BEACON"; labels[1] = "DEAUTH"; labels[2] = "PROBE";
    // Cards show stable session totals; the graph remains the live 200 ms rate
    // history, so a quiet final sample no longer makes capture appear broken.
    stats[0] = wifi_scan_obj.getPacketMonitorTotal(0);
    stats[1] = wifi_scan_obj.getPacketMonitorTotal(1);
    stats[2] = wifi_scan_obj.getPacketMonitorTotal(2);
    graph_mode = true;
    packet_graph = true;
  } else if (mode == WIFI_SCAN_CHAN_ANALYZER) {
    title = "// CHANNEL ANALYZER";
    labels[0] = "RATE"; labels[1] = "PEAK"; labels[2] = "CHANNEL";
    stats[0] = wifi_scan_obj._analyzer_values[0] > 0 ?
               wifi_scan_obj._analyzer_values[0] / BASE_MULTIPLIER : 0;
    for (uint16_t i = 0; i < TFT_WIDTH; i++)
      if (wifi_scan_obj._analyzer_values[i] > (int16_t)(stats[1] * BASE_MULTIPLIER))
        stats[1] = wifi_scan_obj._analyzer_values[i] / BASE_MULTIPLIER;
    stats[2] = wifi_scan_obj.set_channel;
    graph_mode = true;
  } else if (mode == WIFI_SCAN_CHAN_ACT) {
    title = "// CHANNEL SUMMARY";
    labels[0] = "BUSY CH"; labels[1] = "FRAMES"; labels[2] = "SCANNED";
    stats[0] = 1; stats[1] = 0;
    for (uint8_t i = 0; i < MAX_CHANNEL; i++) {
      if (wifi_scan_obj.channel_activity[i] > stats[1]) {
        stats[0] = i + 1;
        stats[1] = wifi_scan_obj.channel_activity[i];
      }
    }
    stats[2] = MAX_CHANNEL;
    graph_mode = true;
    channel_graph = true;
  } else if (mode == WIFI_SCAN_RAW_CAPTURE) {
    title = "// RAW PCAP";
    labels[0] = "FRAMES"; labels[1] = "MGMT"; labels[2] = "DATA";
    stats[0] = wifi_scan_obj.mgmt_frames + wifi_scan_obj.data_frames;
    stats[1] = wifi_scan_obj.mgmt_frames;
    stats[2] = wifi_scan_obj.data_frames;
  } else if (mode == WIFI_SCAN_PWN) title = "// PWNAGOTCHI";
  else if (mode == WIFI_SCAN_PINESCAN) {
    title = "// PINEAPPLE WATCH";
    // Real detection state: confirmed pineapples and tracked candidate radios.
    labels[0] = "PINEAPPLES"; labels[1] = "WATCHED"; labels[2] = "FRAMES";
    stats[0] = wifi_scan_obj.pineScanConfirmedCount();
    stats[1] = wifi_scan_obj.pineScanTrackedCount();
    stats[2] = wifi_scan_obj.mgmt_frames + wifi_scan_obj.data_frames;
  }
  else if (mode == WIFI_SCAN_MULTISSID) {
    title = "// MULTISSID WATCH";
    // Real detection state: APs confirmed to advertise multiple SSIDs.
    labels[0] = "MULTISSID"; labels[1] = "WATCHED"; labels[2] = "FRAMES";
    stats[0] = wifi_scan_obj.multissidConfirmedCount();
    stats[1] = wifi_scan_obj.multissidTrackedCount();
    stats[2] = wifi_scan_obj.mgmt_frames + wifi_scan_obj.data_frames;
  }
  else if (mode == WIFI_SCAN_AP_STA) title = "// AP + CLIENT MAP";
  else if (mode == WIFI_SCAN_DETECT_FOLLOW) {
    title = "// MAC MONITOR";
    MacEntry macs[10];
    const uint8_t n = wifi_scan_obj.build_top10_for_ui(macs, MacSortMode::MOST_FRAMES);
    uint32_t frames_sum = 0;
    for (uint8_t i = 0; i < n; i++)
      frames_sum += macs[i].frame_count;
    labels[0] = "TRACKED"; labels[1] = "FRAMES"; labels[2] = "CHANNEL";
    stats[0] = n;
    stats[1] = frames_sum;
    stats[2] = wifi_scan_obj.set_channel;
    // The busiest MAC drives the refresh signature (its age ticks every
    // second, so the list stays live even when nothing new appears).
    uint16_t focus_frames = 0;
    int32_t focus_age = 0;
    int8_t focus_rssi = -128;
    if (n) {
      focus_frames = macs[0].frame_count;
      focus_age = macs[0].last_seen_ms / 1000;
      focus_rssi = macs[0].rssi;
    }
    signature = String(n) + ":" + String(frames_sum) + ":" +
                String(focus_frames) + ":" + String(focus_age) + ":" +
                String(focus_rssi);
  }
  else if (mode == WIFI_SCAN_SAE_COMMIT) title = "// SAE COMMIT";
  else if (mode == WIFI_SCAN_WAR_DRIVE) {
    title = "// WARDRIVE";
    #ifdef HAS_GPS
      // Real survey state: unique networks logged, BLE rows, GPS satellites.
      // beacon_frames only increments for NEW BSSIDs, so it is the true
      // "networks logged this session" count (mac_history_cursor wraps).
      labels[0] = "NETWORKS"; labels[1] = "BLE"; labels[2] = "SATS";
      stats[0] = wifi_scan_obj.beacon_frames;
      stats[1] = wifi_scan_obj.bt_frames;
      stats[2] = (uint32_t)gps_obj.getNumSats();
      signature = String(stats[0]) + ":" + String(stats[1]) + ":" +
                  String(wifi_scan_obj.poiCount);
    #endif
  }

  if (!signature.length()) {
    signature = String(stats[3]) + ":" + String(stats[4]) + ":" +
                String(stats[5]) + ":" + wifi_scan_obj.analyzer_name_string;
  }

  const bool reset = full_redraw || !this->wifi_tool_draw_valid ||
                     this->wifi_tool_draw_mode != mode;
  const bool running_dirty = !this->wifi_tool_draw_valid ||
                             this->wifi_tool_draw_running != running;
  bool stats_dirty[3] = {false, false, false};
  for (uint8_t i = 0; i < 3; i++)
    stats_dirty[i] = !this->wifi_tool_draw_valid ||
                     this->wifi_tool_draw_stats[i] != stats[i];
  const bool content_dirty = !this->wifi_tool_draw_valid ||
                             this->wifi_tool_draw_signature != signature ||
                             this->wifi_tool_draw_stats[3] != stats[3] ||
                             this->wifi_tool_draw_stats[4] != stats[4] ||
                             this->wifi_tool_draw_stats[5] != stats[5];

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  tft.setTextDatum(TL_DATUM);

  if (reset) {
    tft.fillScreen(TFT_BLACK);
    this->drawSharkTopBar(true);
    tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, 25, TFT_BLACK);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString(title, 6, STATUS_BAR_WIDTH + 7, 2);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 66, WD_CYAN);
    memset(this->wifi_tool_graph_bars, 0xFF,
           sizeof(this->wifi_tool_graph_bars));
  }

  if (reset || running_dirty) {
    // A second BACK control sits well above the lower bezel. Some physical
    // panels compress the final touch columns/rows even when the display itself
    // draws them correctly, so users always have a reachable exit target.
    const uint16_t back_color = WD_CYAN;
    tft.fillRoundRect(177, STATUS_BAR_WIDTH + 3, 59, 19, 3, back_color);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, back_color);
    tft.drawString("BACK", 206, STATUS_BAR_WIDTH + 12, 1);
  }

  for (uint8_t i = 0; i < 3; i++) {
    if (!reset && !stats_dirty[i]) continue;
    const int16_t x = 2 + i * 80;
    const uint16_t accent = i == 1 && stats[i] ? WD_AMBER : WD_CYAN;
    wdPanel(tft, x, 51, 76, 42, WD_SURFACE, WD_EDGE, accent);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_SURFACE);
    tft.drawString(labels[i], x + 7, 56, 1);
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(i == 1 && stats[i] ? WD_AMBER : WD_BONE, WD_SURFACE);
    tft.drawString(String(stats[i]), x + 69, 79, 2);
  }

  if (graph_mode) {
    if (reset) {
      wdPanel(tft, 2, 99, 236, 166, WD_PANEL, WD_EDGE, WD_CYAN);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_DIM, WD_PANEL);
      tft.drawString(packet_graph ? "LIVE FRAME MIX" :
                     (channel_graph ? "ACTIVITY BY CHANNEL" : "LIVE SPECTRUM"),
                     9, 105, 1);
    }
    if (packet_graph) {
      const uint8_t len = wifi_scan_obj.getPacketMonitorLength();
      const uint16_t lane_colors[3] = {WD_CYAN, WD_RED, WD_AMBER};
      const char* lane_names[3] = {"BCN", "DEA", "PRB"};
      for (uint8_t lane = 0; lane < 3; lane++) {
        const int16_t y0 = 118 + lane * 47;
        uint16_t peak = 1;
        for (uint8_t i = 0; i < len; i++)
          peak = max(peak, wifi_scan_obj.getPacketMonitorSample(lane, i));
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(lane_colors[lane], WD_PANEL);
        tft.drawString(lane_names[lane], 8, y0, 1);
        for (uint8_t i = 0; i < len && i < 72; i++) {
          const uint16_t value = wifi_scan_obj.getPacketMonitorSample(lane, i);
          const uint8_t bar = value ? max((uint8_t)1,
              (uint8_t)((uint32_t)value * 32 / peak)) : 0;
          const uint16_t cache = lane * 72 + i;
          if (!reset && this->wifi_tool_graph_bars[cache] == bar) continue;
          const int16_t x = 18 + i * 3;
          tft.fillRect(x, y0 + 9, 2, 33, WD_PANEL);
          if (bar) tft.fillRect(x, y0 + 42 - bar, 2, bar, lane_colors[lane]);
          this->wifi_tool_graph_bars[cache] = bar;
        }
      }
    } else if (channel_graph) {
      uint16_t peak = 1;
      for (uint8_t i = 0; i < MAX_CHANNEL; i++)
        peak = max(peak, (uint16_t)wifi_scan_obj.channel_activity[i]);
      for (uint8_t i = 0; i < MAX_CHANNEL; i++) {
        const uint8_t bar = wifi_scan_obj.channel_activity[i] ?
          max((uint8_t)1, (uint8_t)((uint32_t)wifi_scan_obj.channel_activity[i] * 118 / peak)) : 0;
        if (!reset && this->wifi_tool_graph_bars[i] == bar) continue;
        const int16_t x = 10 + i * 16;
        tft.fillRect(x, 126, 11, 122, WD_PANEL);
        if (bar) tft.fillRect(x, 244 - bar, 11, bar,
                              i + 1 == stats[0] ? WD_AMBER : WD_CYAN);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(WD_DIM, WD_PANEL);
        tft.drawString(String(i + 1), x + 5, 255, 1);
        this->wifi_tool_graph_bars[i] = bar;
      }
    } else {
      int16_t peak = 1;
      for (uint16_t i = 0; i < 216; i++)
        if (wifi_scan_obj._analyzer_values[i] > peak)
          peak = wifi_scan_obj._analyzer_values[i];
      for (uint16_t i = 0; i < 216; i++) {
        const int16_t value = max((int16_t)0, wifi_scan_obj._analyzer_values[i]);
        const uint8_t bar = value ? max((uint8_t)1,
          (uint8_t)((int32_t)value * 128 / peak)) : 0;
        if (!reset && this->wifi_tool_graph_bars[i] == bar) continue;
        const int16_t x = 12 + 215 - i;
        tft.drawFastVLine(x, 124, 130, WD_PANEL);
        if (bar) tft.drawFastVLine(x, 253 - bar, bar, WD_CYAN);
        this->wifi_tool_graph_bars[i] = bar;
      }
    }
  } else if (reset || content_dirty || stats_dirty[0] || stats_dirty[1] || stats_dirty[2]) {
    wdPanel(tft, 2, 99, 236, 166, WD_PANEL, WD_EDGE, WD_CYAN);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_PANEL);
    tft.drawString(mode == WIFI_SCAN_PACKET_RATE ? "SELECTED TARGETS" :
                   (mode == WIFI_SCAN_WAR_DRIVE ? "SURVEY LOG" :
                   (mode == WIFI_SCAN_DETECT_FOLLOW ? "TOP TALKERS" : "CAPTURE DETAILS")), 9, 105, 1);
    if (mode == WIFI_SCAN_DETECT_FOLLOW) {
      // Busiest MACs first: RSSI, address, frames, age; followed target in
      // amber, BLE-origin rows in cyan (same data the legacy tracker had).
      MacEntry macs[10];
      const uint8_t n = wifi_scan_obj.build_top10_for_ui(macs, MacSortMode::MOST_FRAMES);
      if (!n) {
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(WD_DIM, WD_PANEL);
        tft.drawString("NO DEVICES YET", 120, 164, 2);
        tft.drawString("waiting for frames...", 120, 192, 1);
        tft.setTextDatum(TL_DATUM);
      }
      for (uint8_t i = 0; i < n && i < 7; i++) {
        const int16_t y = 122 + i * 24;
        const uint16_t col = macs[i].following ? WD_AMBER :
                             (macs[i].bt ? WD_CYAN : WD_BONE);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(col, WD_PANEL);
        String row = (macs[i].following ? String("> ") : String("  ")) +
                     macToString((uint8_t*)macs[i].mac);
        while (row.length() > 3 && tft.textWidth(row, 1) > 150) row.remove(row.length() - 1);
        tft.drawString(row, 9, y, 1);
        tft.setTextDatum(TR_DATUM);
        tft.setTextColor(macs[i].following ? WD_AMBER : WD_GREY, WD_PANEL);
        tft.drawString(String(macs[i].rssi) + "dBm " + String(macs[i].frame_count) + "f " +
                       String((millis() - macs[i].last_seen_ms) / 1000) + "s",
                       230, y, 1);
        tft.setTextDatum(TL_DATUM);
      }
    } else if (mode == WIFI_SCAN_WAR_DRIVE) {
      #ifdef HAS_GPS
        const bool fix = gps_obj.getFixStatus();
        // Left column: live GPS state.
        tft.setTextColor(WD_DIM, WD_PANEL);
        tft.drawString("GPS FIX", 8, 124, 1);
        tft.setTextColor(fix ? WD_CYAN : WD_AMBER, WD_PANEL);
        tft.drawString(fix ? "LOCKED" : "SEARCHING", 8, 138, 2);
        tft.setTextColor(WD_DIM, WD_PANEL);
        tft.drawString("SATELLITES", 8, 168, 1);
        tft.setTextColor(WD_BONE, WD_PANEL);
        tft.drawString(String(gps_obj.getNumSats()), 8, 182, 2);
        tft.setTextColor(WD_DIM, WD_PANEL);
        tft.drawString("POSITION", 8, 214, 1);
        tft.setTextColor(WD_GREY, WD_PANEL);
        String pos = fix ? (gps_obj.getLat() + "," + gps_obj.getLon())
                         : String("NO FIX YET");
        while (pos.length() > 3 && tft.textWidth(pos, 2) > 224)
          pos.remove(pos.length() - 1);
        tft.drawString(pos, 8, 228, 2);
        // Right column: what the survey is writing.
        tft.setTextColor(WD_DIM, WD_PANEL);
        tft.drawString("LOG FILE", 122, 124, 1);
        tft.setTextColor(WD_BONE, WD_PANEL);
        String fname = buffer_obj.getFileName();
        while (fname.length() > 3 && tft.textWidth(fname, 1) > 112)
          fname.remove(fname.length() - 1);
        tft.drawString(fname.length() ? fname : "-", 122, 138, 1);
        tft.setTextColor(WD_DIM, WD_PANEL);
        tft.drawString("POI TAGS", 122, 168, 1);
        tft.setTextColor(wifi_scan_obj.poiCount ? WD_AMBER : WD_BONE, WD_PANEL);
        tft.drawString(String(wifi_scan_obj.poiCount), 122, 182, 2);
        tft.setTextColor(WD_DIM, WD_PANEL);
        tft.drawString("LOGGING", 122, 214, 1);
        tft.setTextColor(fix ? WD_CYAN : WD_AMBER, WD_PANEL);
        tft.drawString(fix ? "WITH GPS COORDS" : "COORDS 0 TIL FIX", 122, 228, 1);
      #endif
    } else if (mode == WIFI_SCAN_PACKET_RATE) {
      uint8_t row = 0;
      for (int i = 0; access_points && i < access_points->size() && row < 4; i++) {
        AccessPoint ap = access_points->get(i);
        if (!ap.selected) continue;
        String name = ap.essid.length() ? ap.essid : macToString(ap.bssid);
        while (name.length() > 3 && tft.textWidth(name, 1) > 150) name.remove(name.length() - 1);
        const int16_t y = 124 + row * 22;
        tft.setTextColor(WD_BONE, WD_PANEL); tft.drawString(name, 9, y, 1);
        tft.setTextDatum(TR_DATUM); tft.setTextColor(WD_AMBER, WD_PANEL);
        tft.drawString(String(ap.packets), 230, y, 1); tft.setTextDatum(TL_DATUM);
        row++;
      }
      for (int i = 0; stations && i < stations->size() && row < 4; i++) {
        Station sta = stations->get(i);
        if (!sta.selected) continue;
        const int16_t y = 124 + row * 22;
        tft.setTextColor(WD_BONE, WD_PANEL); tft.drawString(macToString(sta.mac), 9, y, 1);
        tft.setTextDatum(TR_DATUM); tft.setTextColor(WD_AMBER, WD_PANEL);
        tft.drawString(String(sta.packets), 230, y, 1); tft.setTextDatum(TL_DATUM);
        row++;
      }
      if (!row) {
        tft.setTextDatum(MC_DATUM); tft.setTextColor(WD_DIM, WD_PANEL);
        tft.drawString("NO TARGET SELECTED", 120, 164, 2);
      }
      // Packet Count always exposes target management in the content area,
      // including the empty state. It remains available after targets exist.
      const int16_t target_x = 16, target_y = 218, target_w = 208, target_h = 38;
      wdPanel(tft, target_x, target_y, target_w, target_h,
              WD_CYAN, WD_CYAN, WD_CYAN);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(TFT_BLACK, WD_CYAN);
      tft.drawString("SELECT TARGETS", target_x + target_w / 2,
                     target_y + target_h / 2, 2);
    } else {
      const char* detail_labels[6] = {"MGMT", "DATA", "DEAUTH", "PROBE REQ", "EAPOL", "RSSI"};
      const int32_t detail_values[6] = {
        (int32_t)wifi_scan_obj.mgmt_frames, (int32_t)wifi_scan_obj.data_frames,
        (int32_t)wifi_scan_obj.deauth_frames, (int32_t)wifi_scan_obj.req_frames,
        (int32_t)wifi_scan_obj.eapol_frames, (int32_t)wifi_scan_obj.max_rssi
      };
      for (uint8_t i = 0; i < 6; i++) {
        const int16_t x = 8 + (i % 2) * 114;
        const int16_t y = 124 + (i / 2) * 42;
        tft.setTextDatum(TL_DATUM); tft.setTextColor(WD_DIM, WD_PANEL);
        tft.drawString(detail_labels[i], x, y, 1);
        tft.setTextDatum(TR_DATUM); tft.setTextColor(i == 2 && detail_values[i] ? WD_RED : WD_BONE, WD_PANEL);
        tft.drawString(String(detail_values[i]), x + 104, y + 14, 2);
      }
    }
  }

  if (reset || running_dirty) {
    const char* buttons[4] = {"CH-", "CH+", running ? "PAUSE" : "RESUME", "BACK"};
    const int16_t button_x[4] = {2, 58, 114, 170};
    const int16_t button_w[4] = {52, 52, 52, 68};
    uint8_t first_button = 0;
    if (mode == WIFI_SCAN_WAR_DRIVE) {
      // Wardrive hops channels itself, so the two channel cells merge into
      // one wide POI tag key; PAUSE and BACK keep their usual places.
      wdPanel(tft, 2, 260, 108, 56, WD_CYAN, WD_CYAN, WD_CYAN);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(TFT_BLACK, WD_CYAN);
      tft.drawString("TAG POI", 56, 288, 2);
      first_button = 2;
    }
    for (uint8_t i = first_button; i < 4; i++) {
      const int16_t x = button_x[i];
      const uint16_t fill = i == 2 ? (running ? WD_SURFACE : WD_AMBER) :
                            (i == 3 ? WD_CYAN : WD_SURFACE);
      wdPanel(tft, x, 260, button_w[i], 56, fill,
              i == 3 ? WD_CYAN : WD_EDGE, i == 2 ? WD_AMBER : WD_CYAN);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor((i == 3 || (i == 2 && !running)) ? TFT_BLACK : WD_BONE, fill);
      tft.drawString(buttons[i], x + button_w[i] / 2, 288, 1);
    }
  }

  this->wifi_tool_draw_valid = true;
  this->wifi_tool_draw_mode = mode;
  this->wifi_tool_draw_running = running;
  for (uint8_t i = 0; i < 6; i++) this->wifi_tool_draw_stats[i] = stats[i];
  this->wifi_tool_draw_signature = signature;
  tft.setTextDatum(TL_DATUM);
}

// Themed GPS Data screen (WIFI_SCAN_GPS_DATA). Replaces the legacy println
// painter (RunGPSInfo's display block, non-V8 only now). Exit is the
// existing tap-anywhere handler that stops the scan and returns to the GPS
// menu, so the footer bar states it plainly.
void MenuFunctions::drawGpsDataUI(bool full_redraw) {
  #if defined(HAS_GPS)
    TFT_eSPI& tft = display_obj.tft;
    if (wifi_scan_obj.currentScanMode != WIFI_SCAN_GPS_DATA) return;

    // GPS ticks ~1 Hz; cap repaints to match so the screen never flickers.
    if (!full_redraw && millis() - this->gps_data_ui_ms < 1000) return;
    this->gps_data_ui_ms = millis();

    const bool fix = gps_obj.getFixStatus();
    const String lat = gps_obj.getLat();
    const String lon = gps_obj.getLon();
    const String dt = gps_obj.getDatetime();
    const String gps_text = gps_obj.getText();
    const bool ant_open = gps_text.indexOf("ANTENNA") >= 0;
    // While searching, an animated elapsed counter proves the screen (and
    // the tool) are alive; it also drives the once-a-second time-zone
    // repaint so a no-fix screen never looks frozen.
    static uint32_t search_start_ms = 0;
    static bool searching_last = true;
    if (fix) {
      searching_last = false;
    }
    else if (!searching_last || search_start_ms == 0) {
      searching_last = true;
      search_start_ms = millis();
    }
    const uint32_t search_secs = searching_last ? (millis() - search_start_ms) / 1000 : 0;

    // Zone-gated repaints: a time tick only refreshes the small TIME row,
    // tiles/position repaint only when a value really changes. This keeps
    // every pass cheap so the screen never feels slow.
    const String pos_sig = String(fix ? 1 : 0) + ":" + String(gps_obj.getNumSats()) +
                           ":" + String(gps_obj.getTrackedSignals()) +
                           ":" + String(gps_obj.getBestSignalSnr()) +
                           ":" + lat + ":" + lon + ":" +
                           String(gps_obj.getAccuracy()) + ":" +
                           String(gps_obj.getAlt());
    const bool pos_dirty = full_redraw || !this->gps_data_ui_valid ||
                           this->gps_data_ui_sig != pos_sig;

    tft.setFreeFont(NULL);
    tft.setTextSize(1);
    tft.setTextWrap(false);
    tft.setTextDatum(TL_DATUM);

    if (full_redraw) {
      tft.fillScreen(TFT_BLACK);
      this->drawSharkTopBar(true);
      tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, 25, TFT_BLACK);
      tft.setTextColor(WD_CYAN, TFT_BLACK);
      tft.drawString("// GPS DATA", 6, STATUS_BAR_WIDTH + 7, 2);
      tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
      tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 60, WD_CYAN);

      wdPanel(tft, 8, 262, 224, 44, WD_SURFACE, WD_GREY, WD_GREY);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(WD_GREY, WD_SURFACE);
      tft.drawString("TAP ANYWHERE TO EXIT", 120, 284, 2);
      tft.setTextDatum(TL_DATUM);
    }

    if (pos_dirty) {
      this->gps_data_ui_sig = pos_sig;
      this->gps_data_ui_valid = true;

      // Explicit ready/not-ready pill in the title strip, so the fix state
      // is readable at a glance. NO ANT calls out the open-antenna fault
      // specifically.
      tft.fillRect(166, STATUS_BAR_WIDTH + 2, 72, 21, TFT_BLACK);
      const uint16_t pill = fix ? WD_CYAN : WD_AMBER;
      tft.fillRoundRect(170, STATUS_BAR_WIDTH + 3, 66, 18, 3, pill);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(TFT_BLACK, pill);
      tft.drawString(fix ? "GPS READY" : (ant_open ? "NO ANT" : "NO FIX"),
                     203, STATUS_BAR_WIDTH + 12, 1);
      tft.setTextDatum(TL_DATUM);

      // Tiles zone. While searching, the tiles become a signal meter:
      // SIGNALS (count heard) + BEST dB (strongest SNR) - a fix typically
      // needs 4+ signals at ~30 dB or better.
      tft.fillRect(0, 45, SCREEN_WIDTH, 48, TFT_BLACK);
      const bool show_signals = !fix && gps_obj.getTrackedSignals() > 0;
      const String tile_labels[3] = {"SATS",
                                     show_signals ? "SIGNALS" : "ACCURACY",
                                     show_signals ? "BEST dB" : "ALTITUDE"};
      const String tile_values[3] = {String(gps_obj.getNumSats()),
                                     show_signals ? String(gps_obj.getTrackedSignals())
                                                  : String(gps_obj.getAccuracy()),
                                     show_signals ? String(gps_obj.getBestSignalSnr())
                                                  : String(gps_obj.getAlt()) + "m"};
      for (uint8_t i = 0; i < 3; i++) {
        const int16_t x = 2 + i * 80;
        wdPanel(tft, x, 49, 76, 42, WD_SURFACE, WD_EDGE, fix ? WD_CYAN : WD_AMBER);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(WD_DIM, WD_SURFACE);
        tft.drawString(tile_labels[i], x + 7, 54, 1);
        tft.setTextDatum(MR_DATUM);
        tft.setTextColor(WD_BONE, WD_SURFACE);
        tft.drawString(tile_values[i], x + 69, 76, 2);
      }

      // Position panel (the time value is its own zone below).
      wdPanel(tft, 2, 99, 236, 118, WD_PANEL, WD_EDGE, fix ? WD_CYAN : WD_AMBER);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_DIM, WD_PANEL);
      tft.drawString("POSITION", 8, 105, 1);
      tft.setTextColor(fix ? WD_CYAN : WD_AMBER, WD_PANEL);
      tft.drawString(fix ? "READY" : "NO FIX", 190, 105, 1);
      tft.setTextColor(WD_DIM, WD_PANEL);
      tft.drawString("LAT", 8, 122, 1);
      tft.drawString("LON", 8, 152, 1);
      tft.drawString("TIME", 8, 186, 1);
      tft.setTextColor(WD_BONE, WD_PANEL);
      tft.drawString(lat.length() ? lat : "-", 46, 122, 2);
      tft.drawString(lon.length() ? lon : "-", 46, 152, 2);
    }

    // TIME row zone (ticks every second while the fix is live; while
    // searching it shows an animated SEARCHING counter so the screen is
    // visibly alive).
    const String time_shown = fix ? dt
                                  : (String("SEARCHING ") +
                                     String("...").substring(0, (millis() / 400) % 4) +
                                     " " + String(search_secs) + "s");
    if (full_redraw || pos_dirty || this->gps_data_time_sig != time_shown) {
      this->gps_data_time_sig = time_shown;
      tft.fillRect(44, 182, 190, 20, WD_PANEL);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_GREY, WD_PANEL);
      String shown = time_shown;
      while (shown.length() > 3 && tft.textWidth(shown, 2) > 186) shown.remove(shown.length() - 1);
      tft.drawString(shown, 46, 186, 2);
    }

    // NMEA text strip zone. A module status of "ANTENNA OPEN" (receiver
    // sees no antenna) is escalated to an amber warning instead of a grey
    // raw line, so the reason for NO FIX is on the screen.
    String text = gps_text;
    while (text.length() > 3 && tft.textWidth(text, 1) > 224) text.remove(text.length() - 1);
    if (full_redraw || this->gps_data_text_sig != text) {
      this->gps_data_text_sig = text;
      tft.fillRect(0, 219, SCREEN_WIDTH, 38, TFT_BLACK);
      if (ant_open) {
        wdPanel(tft, 2, 223, 236, 30, WD_SURFACE, WD_AMBER, WD_AMBER);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(WD_AMBER, WD_SURFACE);
        tft.drawString("ANTENNA OPEN - CHECK GPS ANTENNA", 120, 233, 1);
        tft.setTextDatum(TL_DATUM);
      }
      else if (text.length()) {
        wdPanel(tft, 2, 223, 236, 30, WD_SURFACE, WD_EDGE, WD_EDGE);
        tft.setTextColor(WD_GREY, WD_SURFACE);
        tft.drawString(text, 8, 233, 1);
      }
    }
  #endif
}

// Themed GPS Tracker screen (GPS_TRACKER). The tracker logs GPX track
// points once per second while a fix exists; this dashboard shows the
// survey growing. Exit is the existing tap-anywhere GPS handler.
void MenuFunctions::drawGpsTrackerUI(bool full_redraw) {
  #if defined(HAS_GPS)
    TFT_eSPI& tft = display_obj.tft;
    if (wifi_scan_obj.currentScanMode != GPS_TRACKER) return;

    if (!full_redraw && millis() - this->gps_trk_ui_ms < 1000) return;
    this->gps_trk_ui_ms = millis();

    const bool fix = gps_obj.getFixStatus();
    const String lat = gps_obj.getLat();
    const String lon = gps_obj.getLon();
    const String dt = gps_obj.getDatetime();
    const String text = gps_obj.getText();
    const bool ant_open = text.indexOf("ANTENNA") >= 0;

    const String sig = String(fix ? 1 : 0) + ":" + String(gps_obj.getNumSats()) +
                       ":" + String(gps_obj.getTrackedSignals()) +
                       ":" + String(gps_obj.getBestSignalSnr()) +
                       ":" + lat + ":" + lon + ":" + dt + ":" +
                       String(wifi_scan_obj.gpx_points) + ":" + text;
    const bool reset = full_redraw || !this->gps_trk_ui_valid ||
                       this->gps_trk_ui_sig != sig;
    if (!reset) return;
    this->gps_trk_ui_valid = true;
    this->gps_trk_ui_sig = sig;

    tft.setFreeFont(NULL);
    tft.setTextSize(1);
    tft.setTextWrap(false);
    tft.setTextDatum(TL_DATUM);

    if (full_redraw) {
      tft.fillScreen(TFT_BLACK);
      this->drawSharkTopBar(true);
      tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, 25, TFT_BLACK);
      tft.setTextColor(WD_CYAN, TFT_BLACK);
      tft.drawString("// GPS TRACKER", 6, STATUS_BAR_WIDTH + 7, 2);
      tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
      tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 60, WD_CYAN);
    }

    // Ready/not-ready pill (same as GPS Data).
    tft.fillRect(166, STATUS_BAR_WIDTH + 2, 72, 21, TFT_BLACK);
    const uint16_t pill = fix ? WD_CYAN : WD_AMBER;
    tft.fillRoundRect(170, STATUS_BAR_WIDTH + 3, 66, 18, 3, pill);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, pill);
    tft.drawString(fix ? "GPS READY" : "NO FIX", 203, STATUS_BAR_WIDTH + 12, 1);
    tft.setTextDatum(TL_DATUM);

    // Tiles: while searching the middle+right tiles become a signal meter
    // (SIGNALS count + BEST dB); with a fix they return to SATS/ACCURACY.
    const bool trk_show_signals = !fix && gps_obj.getTrackedSignals() > 0;
    const String tile_labels[3] = {"POINTS",
                                   trk_show_signals ? "SIGNALS" : "SATS",
                                   trk_show_signals ? "BEST dB" : "ACCURACY"};
    const String tile_values[3] = {String(wifi_scan_obj.gpx_points),
                                   trk_show_signals ? String(gps_obj.getTrackedSignals())
                                                    : String(gps_obj.getNumSats()),
                                   trk_show_signals ? String(gps_obj.getBestSignalSnr())
                                                    : String(gps_obj.getAccuracy())};
    for (uint8_t i = 0; i < 3; i++) {
      const int16_t x = 2 + i * 80;
      wdPanel(tft, x, 49, 76, 42, WD_SURFACE, WD_EDGE,
              (i == 0 && wifi_scan_obj.gpx_points) ? WD_AMBER : (fix ? WD_CYAN : WD_AMBER));
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_DIM, WD_SURFACE);
      tft.drawString(tile_labels[i], x + 7, 54, 1);
      tft.setTextDatum(MR_DATUM);
      tft.setTextColor((i == 0 && wifi_scan_obj.gpx_points) ? WD_AMBER : WD_BONE, WD_SURFACE);
      tft.drawString(tile_values[i], x + 69, 76, 2);
    }

    // Track panel: LAT/LON large, TIME and GPX file compact below.
    wdPanel(tft, 2, 99, 236, 118, WD_PANEL, WD_EDGE, fix ? WD_CYAN : WD_AMBER);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_PANEL);
    tft.drawString("TRACK", 8, 104, 1);
    tft.drawString("LAT", 8, 118, 1);
    tft.drawString("LON", 8, 148, 1);
    tft.drawString("TIME", 8, 178, 1);
    tft.drawString("FILE", 8, 200, 1);
    tft.setTextColor(WD_BONE, WD_PANEL);
    tft.drawString(lat.length() ? lat : "-", 46, 118, 2);
    tft.drawString(lon.length() ? lon : "-", 46, 148, 2);
    String shown = dt.length() ? dt : String("-");
    while (shown.length() > 3 && tft.textWidth(shown, 2) > 178) shown.remove(shown.length() - 1);
    tft.setTextColor(WD_GREY, WD_PANEL);
    tft.drawString(shown, 46, 178, 2);
    String fname = buffer_obj.getFileName();
    while (fname.length() > 3 && tft.textWidth(fname, 1) > 172) fname.remove(fname.length() - 1);
    tft.drawString(fname.length() ? fname : "-", 46, 200, 1);
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(fix ? WD_CYAN : WD_AMBER, WD_PANEL);
    tft.drawString(fix ? "LOGGING" : "HOLD", 230, 104, 1);
    tft.setTextDatum(TL_DATUM);

    // GPS detector strip: module + antenna + satellite reception in one
    // readout, escalated to the amber antenna warning when the receiver
    // reports its antenna circuit open.
    tft.fillRect(0, 219, SCREEN_WIDTH, 38, TFT_BLACK);
    if (ant_open) {
      wdPanel(tft, 2, 223, 236, 30, WD_SURFACE, WD_AMBER, WD_AMBER);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(WD_AMBER, WD_SURFACE);
      tft.drawString("ANTENNA OPEN - CHECK GPS ANTENNA", 120, 233, 1);
      tft.setTextDatum(TL_DATUM);
    }
    else {
      wdPanel(tft, 2, 223, 236, 30, WD_SURFACE, WD_EDGE,
              fix ? WD_CYAN : WD_EDGE);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(WD_DIM, WD_SURFACE);
      String detect = "GPS DETECT  MODULE OK";
      detect += fix ? "  ·  FIX" : String("  ·  ") + String(gps_obj.getNumSats()) + " SATS";
      tft.drawString(detect, 120, 233, 1);
      tft.setTextDatum(TL_DATUM);
    }

    if (full_redraw) {
      wdPanel(tft, 8, 262, 224, 44, WD_SURFACE, WD_GREY, WD_GREY);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(WD_GREY, WD_SURFACE);
      tft.drawString("TAP ANYWHERE TO EXIT", 120, 284, 2);
      tft.setTextDatum(TL_DATUM);
    }
  #endif
}

void MenuFunctions::handleWifiToolTouch(uint16_t touch_x, uint16_t touch_y) {
  if (this->wifi_packet_target_view) {
    this->handlePacketTargetPickerTouch(touch_x, touch_y);
    return;
  }

  // The complete title strip is an emergency BACK target, while the lower
  // target begins above/left of its artwork to tolerate bezel compression.
  if ((touch_x >= 154 && touch_y >= 238) ||
      (touch_y >= STATUS_BAR_WIDTH && touch_y < 51)) {
    this->exitWifiToolUI();
    return;
  }

  // The dedicated target action sits above the shared bottom controls.
  if (wifi_scan_obj.currentScanMode == WIFI_SCAN_PACKET_RATE &&
      touch_x >= 12 && touch_x <= 228 && touch_y >= 210 && touch_y < 266) {
    this->openPacketTargetPicker();
    return;
  }

  // The visible controls begin at y=260. Accept another 12 px above them so
  // touches near the lower bezel still land reliably on the intended button.
  if (touch_y < 248) return;
  // Wardrive merges the channel cells into one wide POI tag key.
  if (wifi_scan_obj.currentScanMode == WIFI_SCAN_WAR_DRIVE && touch_x < 112) {
    const uint16_t poi_before = wifi_scan_obj.poiCount;
    wifi_scan_obj.tagPOI(nullptr);
    // Confirm the tag the same way the legacy bar did, then refresh the
    // dashboard (POI count rides in the redraw signature).
    TFT_eSPI& tft = display_obj.tft;
    const bool tagged = wifi_scan_obj.poiCount > poi_before;
    tft.fillRect(2, 260, 108, 56, tagged ? WD_CYAN : WD_RED);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, tagged ? WD_CYAN : WD_RED);
    #ifdef HAS_GPS
      tft.drawString(tagged ? "POI OK" : "NO GPS FIX", 56, 281, 2);
    #else
      tft.drawString(tagged ? "POI OK" : "NO TAG", 56, 281, 2);
    #endif
    tft.setTextDatum(TL_DATUM);
    delay(220);
    this->drawWifiToolUI(true);
    return;
  }
  const uint8_t action = touch_x < 56 ? 0 : (touch_x < 112 ? 1 : 2);
  if (action == 0) {
    // A manual channel choice locks the current session instead of being
    // immediately replaced by the next automatic Channel Hop tick.
    wifi_scan_obj.channel_hop = false;
    #ifndef HAS_DUAL_BAND
      wifi_scan_obj.changeChannel(wifi_scan_obj.set_channel > 1 ?
                                  wifi_scan_obj.set_channel - 1 : MAX_CHANNEL);
    #else
      if (wifi_scan_obj.dual_band_channel_index > 0)
        wifi_scan_obj.dual_band_channel_index--;
      else
        wifi_scan_obj.dual_band_channel_index = DUAL_BAND_CHANNELS - 1;
      wifi_scan_obj.changeChannel(
        wifi_scan_obj.dual_band_channels[wifi_scan_obj.dual_band_channel_index]);
    #endif
  } else if (action == 1) {
    wifi_scan_obj.channel_hop = false;
    #ifndef HAS_DUAL_BAND
      wifi_scan_obj.changeChannel(wifi_scan_obj.set_channel < MAX_CHANNEL ?
                                  wifi_scan_obj.set_channel + 1 : 1);
    #else
      if (wifi_scan_obj.dual_band_channel_index + 1 < DUAL_BAND_CHANNELS)
        wifi_scan_obj.dual_band_channel_index++;
      else
        wifi_scan_obj.dual_band_channel_index = 0;
      wifi_scan_obj.changeChannel(
        wifi_scan_obj.dual_band_channels[wifi_scan_obj.dual_band_channel_index]);
    #endif
  } else if (action == 2) {
    wifi_scan_obj.setOwnedWifiToolPaused(wifi_scan_obj.wifi_tool_running);
  }
  this->drawWifiToolUI(true);
}

void MenuFunctions::exitWifiToolUI() {
  // Scan Targets uses the real AP/client monitor, then returns directly to
  // this picker instead of dropping the user into the Wi-Fi menu.
  if (this->wifi_packet_target_scan &&
      wifi_scan_obj.currentScanMode == WIFI_SCAN_AP_STA) {
    const bool resume_after_select =
        this->wifi_packet_target_resume_after_select;
    this->wifi_packet_target_scan = false;
    wifi_scan_obj.StartScan(WIFI_SCAN_PACKET_RATE, TFT_ORANGE);
    this->resetPacketTargetCounters();
    wifi_scan_obj.setOwnedWifiToolPaused(true);
    this->wifi_packet_target_resume_after_select = resume_after_select;
    this->wifi_packet_target_view = true;
    this->wifi_packet_target_page = 0;
    this->wifi_tool_draw_valid = false;
    this->drawPacketTargetPickerUI(true);
    return;
  }

  // Confirm the tap before radio teardown. This makes a working touch obvious
  // even if esp_wifi shutdown takes a moment on a busy capture session.
  TFT_eSPI& tft = display_obj.tft;
  tft.fillRoundRect(34, 132, 172, 56, 5, WD_CYAN);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, WD_CYAN);
  tft.drawString("EXITING...", 120, 160, 2);
  if (wifi_scan_obj.wifi_tool_running)
    wifi_scan_obj.setOwnedWifiToolPaused(true);
  delay(1);

  this->wifi_tool_ui = false;
  this->wifi_tool_touch_active = false;
  this->wifi_tool_touch_region = -1;
  this->wifi_tool_touch_ms = 0;
  this->wifi_tool_touch_ready_at = 0;
  this->wifi_tool_draw_valid = false;
  wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  display_obj.init();
  this->changeMenu(current_menu, true);
}

void MenuFunctions::resetPacketTargetCounters() {
  extern LinkedList<AccessPoint>* access_points;
  extern LinkedList<Station>* stations;
  for (int i = 0; access_points && i < access_points->size(); i++) {
    AccessPoint ap = access_points->get(i);
    if (!ap.selected) continue;
    ap.packets = 0;
    access_points->set(i, ap);
  }
  for (int i = 0; stations && i < stations->size(); i++) {
    Station station = stations->get(i);
    if (!station.selected) continue;
    station.packets = 0;
    stations->set(i, station);
  }
}

void MenuFunctions::openPacketTargetPicker() {
  if (wifi_scan_obj.currentScanMode != WIFI_SCAN_PACKET_RATE) return;
  this->wifi_packet_target_resume_after_select = wifi_scan_obj.wifi_tool_running;
  if (wifi_scan_obj.wifi_tool_running)
    wifi_scan_obj.setOwnedWifiToolPaused(true);
  this->wifi_packet_target_view = true;
  this->wifi_packet_target_page = 0;
  this->drawPacketTargetPickerUI(true);
}

void MenuFunctions::drawPacketTargetPickerUI(bool full_redraw) {
  extern LinkedList<AccessPoint>* access_points;
  extern LinkedList<Station>* stations;
  TFT_eSPI& tft = display_obj.tft;
  const uint8_t rows_per_page = 5;
  const uint16_t ap_count = access_points ? access_points->size() : 0;
  const uint16_t station_count = stations ? stations->size() : 0;
  const uint16_t total = ap_count + station_count;
  const uint16_t pages = max((uint16_t)1,
      (uint16_t)((total + rows_per_page - 1) / rows_per_page));
  if (this->wifi_packet_target_page >= pages)
    this->wifi_packet_target_page = pages - 1;

  uint16_t selected = 0;
  for (int i = 0; access_points && i < access_points->size(); i++)
    if (access_points->get(i).selected) selected++;
  for (int i = 0; stations && i < stations->size(); i++)
    if (stations->get(i).selected) selected++;

  (void)full_redraw;
  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  tft.fillScreen(TFT_BLACK);
  this->drawSharkTopBar(true);
  tft.setTextDatum(TL_DATUM);
  tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, 25, TFT_BLACK);
  tft.setTextColor(WD_CYAN, TFT_BLACK);
  tft.drawString("// SELECT TARGETS", 6, STATUS_BAR_WIDTH + 7, 2);
  tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
  tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 72, WD_CYAN);

  wdPanel(tft, 2, 51, 236, 21, WD_PANEL, WD_EDGE,
          selected ? WD_AMBER : WD_CYAN);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WD_BONE, WD_PANEL);
  tft.drawString(String(selected) + " SELECTED / " + String(total) + " FOUND",
                 8, 58, 1);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(WD_DIM, WD_PANEL);
  tft.drawString("PAGE " + String(this->wifi_packet_target_page + 1) +
                 "/" + String(pages), 232, 58, 1);

  if (!total) {
    wdPanel(tft, 12, 100, 216, 145, WD_SURFACE, WD_EDGE, WD_CYAN);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WD_BONE, WD_SURFACE);
    tft.drawString("NO SCANNED TARGETS", 120, 129, 2);
    tft.setTextColor(WD_DIM, WD_SURFACE);
    tft.drawString("DISCOVER APS + CLIENTS FIRST", 120, 154, 1);
    wdPanel(tft, 34, 177, 172, 44, WD_CYAN, WD_CYAN, WD_CYAN);
    tft.setTextColor(TFT_BLACK, WD_CYAN);
    tft.drawString("SCAN TARGETS", 120, 199, 2);
  } else {
    for (uint8_t row = 0; row < rows_per_page; row++) {
      const uint16_t item = this->wifi_packet_target_page * rows_per_page + row;
      if (item >= total) break;
      const int16_t y = 76 + row * 38;
      bool is_selected = false;
      String name;
      String detail;
      if (item < ap_count) {
        AccessPoint ap = access_points->get(item);
        is_selected = ap.selected;
        name = "AP  " + (ap.essid.length() ? ap.essid : macToString(ap.bssid));
        detail = "CH " + String(ap.channel) + "  " + macToString(ap.bssid);
      } else {
        const uint16_t station_index = item - ap_count;
        Station station = stations->get(station_index);
        is_selected = station.selected;
        name = "STA " + macToString(station.mac);
        detail = "CLIENT";
        if (access_points && station.ap < access_points->size()) {
          String ap_name = access_points->get(station.ap).essid;
          if (ap_name.length()) detail += " OF " + ap_name;
        }
      }
      while (name.length() > 4 && tft.textWidth(name, 1) > 174)
        name.remove(name.length() - 1);
      while (detail.length() > 4 && tft.textWidth(detail, 1) > 184)
        detail.remove(detail.length() - 1);
      const uint16_t fill = is_selected ? WD_CYAN : WD_SURFACE;
      const uint16_t text_color = is_selected ? TFT_BLACK : WD_BONE;
      wdPanel(tft, 2, y, 236, 34, fill,
              is_selected ? WD_CYAN : WD_EDGE,
              is_selected ? WD_CYAN : WD_CYAN_DIM);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(text_color, fill);
      tft.drawString(name, 9, y + 5, 1);
      tft.setTextColor(is_selected ? TFT_BLACK : WD_DIM, fill);
      tft.drawString(detail, 9, y + 20, 1);
      tft.setTextDatum(MR_DATUM);
      tft.setTextColor(is_selected ? TFT_BLACK : WD_DIM, fill);
      tft.drawString(is_selected ? "ON" : "OFF", 229, y + 17, 1);
    }
  }

  static const char* const buttons[3] = {"PREV", "NEXT", "DONE"};
  const bool can_prev = this->wifi_packet_target_page > 0;
  const bool can_next = this->wifi_packet_target_page + 1 < pages;
  for (uint8_t i = 0; i < 3; i++) {
    const bool enabled = i == 0 ? can_prev : (i == 1 ? can_next : true);
    const int16_t x = i * 80 + 2;
    const uint16_t fill = i == 2 ? WD_CYAN : WD_SURFACE;
    wdPanel(tft, x, 274, 76, 42, fill,
            i == 2 ? WD_CYAN : (enabled ? WD_BONE : WD_EDGE), WD_CYAN);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(i == 2 ? TFT_BLACK : (enabled ? WD_BONE : WD_DIM), fill);
    tft.drawString(buttons[i], x + 38, 295, 1);
  }
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::handlePacketTargetPickerTouch(uint16_t touch_x,
                                                   uint16_t touch_y) {
  extern LinkedList<AccessPoint>* access_points;
  extern LinkedList<Station>* stations;
  const uint8_t rows_per_page = 5;
  const uint16_t ap_count = access_points ? access_points->size() : 0;
  const uint16_t station_count = stations ? stations->size() : 0;
  const uint16_t total = ap_count + station_count;
  const uint16_t pages = max((uint16_t)1,
      (uint16_t)((total + rows_per_page - 1) / rows_per_page));

  if (!total && touch_x >= 28 && touch_x <= 212 &&
      touch_y >= 168 && touch_y <= 230) {
    this->wifi_packet_target_view = false;
    this->wifi_packet_target_scan = true;
    this->startWifiToolUI(WIFI_SCAN_AP_STA, TFT_GREEN);
    return;
  }

  if (total && touch_y >= 76 && touch_y < 266) {
    const uint8_t row = min((uint8_t)4, (uint8_t)((touch_y - 76) / 38));
    const uint16_t item = this->wifi_packet_target_page * rows_per_page + row;
    if (item < total) {
      if (item < ap_count) {
        AccessPoint ap = access_points->get(item);
        ap.selected = !ap.selected;
        access_points->set(item, ap);
      } else {
        const uint16_t station_index = item - ap_count;
        Station station = stations->get(station_index);
        station.selected = !station.selected;
        stations->set(station_index, station);
      }
      this->drawPacketTargetPickerUI(true);
    }
    return;
  }

  if (touch_y < 270) return;
  if (touch_x < 80) {
    if (this->wifi_packet_target_page) this->wifi_packet_target_page--;
    this->drawPacketTargetPickerUI(true);
  } else if (touch_x < 160) {
    if (this->wifi_packet_target_page + 1 < pages)
      this->wifi_packet_target_page++;
    this->drawPacketTargetPickerUI(true);
  } else {
    this->wifi_packet_target_view = false;
    this->wifi_packet_target_scan = false;
    this->resetPacketTargetCounters();
    if (this->wifi_packet_target_resume_after_select)
      wifi_scan_obj.setOwnedWifiToolPaused(false);
    this->wifi_packet_target_resume_after_select = false;
    this->wifi_tool_draw_valid = false;
    this->drawWifiToolUI(true);
  }
}

// --- Connected LAN scanner dashboard --------------------------------------

const char* MenuFunctions::networkScannerTitle(uint8_t mode) const {
  if (mode == WIFI_PING_SCAN) return "// PING DISCOVERY";
  if (mode == WIFI_PORT_SCAN_ALL) return "// ALL PORTS";
  if (mode == WIFI_SCAN_FTP) return "// FTP 21";
  if (mode == WIFI_SCAN_SSH) return "// SSH 22";
  if (mode == WIFI_SCAN_TELNET) return "// TELNET 23";
  if (mode == WIFI_SCAN_SMTP) return "// SMTP 25";
  if (mode == WIFI_SCAN_DNS) return "// DNS 53";
  if (mode == WIFI_SCAN_HTTP) return "// HTTP 80";
  if (mode == WIFI_SCAN_HTTPS) return "// HTTPS 443";
  if (mode == WIFI_SCAN_SMB) return "// SMB 445";
  if (mode == WIFI_SCAN_MQTT) return "// MQTT 1883";
  if (mode == WIFI_SCAN_MYSQL) return "// MYSQL 3306";
  if (mode == WIFI_SCAN_RDP) return "// RDP 3389";
  if (mode == WIFI_SCAN_POSTGRES) return "// POSTGRES 5432";
  if (mode == WIFI_SCAN_VNC) return "// VNC 5900";
  if (mode == WIFI_SCAN_REDIS) return "// REDIS 6379";
  if (mode == WIFI_SCAN_MQTTS) return "// MQTT TLS 8883";
  return "// NETWORK SCAN";
}

uint8_t MenuFunctions::networkScannerIcon(uint8_t mode) const {
  if (mode == WIFI_PING_SCAN) return PING_SCAN_ICON;
  if (mode == WIFI_PORT_SCAN_ALL) return PORT_SCAN_ICON;
  if (mode == WIFI_SCAN_FTP) return FTP_SCAN_ICON;
  if (mode == WIFI_SCAN_SSH) return SSH_SCAN_ICON;
  if (mode == WIFI_SCAN_TELNET) return TELNET_SCAN_ICON;
  if (mode == WIFI_SCAN_SMTP) return SMTP_SCAN_ICON;
  if (mode == WIFI_SCAN_DNS) return DNS_SCAN_ICON;
  if (mode == WIFI_SCAN_HTTP) return HTTP_SCAN_ICON;
  if (mode == WIFI_SCAN_HTTPS) return HTTPS_SCAN_ICON;
  if (mode == WIFI_SCAN_SMB) return SMB_SCAN_ICON;
  if (mode == WIFI_SCAN_MQTT) return MQTT_SCAN_ICON;
  if (mode == WIFI_SCAN_MYSQL) return MYSQL_SCAN_ICON;
  if (mode == WIFI_SCAN_RDP) return RDP_SCAN_ICON;
  if (mode == WIFI_SCAN_POSTGRES) return POSTGRES_SCAN_ICON;
  if (mode == WIFI_SCAN_VNC) return VNC_SCAN_ICON;
  if (mode == WIFI_SCAN_REDIS) return REDIS_SCAN_ICON;
  if (mode == WIFI_SCAN_MQTTS) return MQTTS_SCAN_ICON;
  return SCANNERS;
}

void MenuFunctions::startNetworkScannerUI(uint8_t mode, uint16_t color) {
  this->wifi_passive_ui = false;
  this->wifi_tool_ui = false;
  this->network_scan_ui = true;
  this->network_scan_requested_mode = mode;
  this->network_scan_draw_valid = false;
  this->network_scan_touch_active = false;
  this->network_scan_touch_region = -1;
  this->network_scan_touch_ms = 0;
  this->network_scan_touch_ready_at = millis() + 320;
  wifi_scan_obj.network_scan_ui_owned = true;

  display_obj.tft.fillScreen(TFT_BLACK);
  this->drawSharkTopBar(true);

  if (WiFi.status() == WL_CONNECTED) {
    wifi_scan_obj.StartScan(mode, color);
  } else {
    wifi_scan_obj.network_scan_running = false;
    wifi_scan_obj.network_scan_checked = 0;
    wifi_scan_obj.network_scan_total = mode == WIFI_PORT_SCAN_ALL ? MAX_PORT : 254;
    wifi_scan_obj.network_scan_target_port = wifi_scan_obj.networkServicePort(mode);
    wifi_scan_obj.clearNetworkScanResults();
  }
  this->drawNetworkScannerUI(true);
}

void MenuFunctions::drawNetworkScannerUI(bool full_redraw) {
  if (!this->network_scan_ui) return;
  TFT_eSPI& tft = display_obj.tft;
  const uint8_t mode = wifi_scan_obj.isNetworkScannerMode(
      wifi_scan_obj.currentScanMode) ? wifi_scan_obj.currentScanMode :
      this->network_scan_requested_mode;
  const bool connected = WiFi.status() == WL_CONNECTED;
  const bool running = connected && wifi_scan_obj.network_scan_running;
  const bool complete = connected && wifi_scan_obj.networkScanComplete();
  const uint32_t checked = wifi_scan_obj.network_scan_checked;
  const uint32_t total = max((uint32_t)1, wifi_scan_obj.network_scan_total);
  const uint32_t progress = min((uint32_t)100, checked * 100UL / total);
  String cursor = wifi_scan_obj.current_scan_ip.toString();
  if (mode == WIFI_PORT_SCAN_ALL)
    cursor += ":" + String(wifi_scan_obj.current_scan_port);

  const bool reset = full_redraw || !this->network_scan_draw_valid;
  const bool state_dirty = reset ||
      this->network_scan_draw_running != running ||
      this->network_scan_draw_complete != complete;
  const bool stats_dirty = reset ||
      this->network_scan_draw_checked != checked ||
      this->network_scan_draw_hits != wifi_scan_obj.network_scan_hits;
  const bool findings_dirty = reset ||
      this->network_scan_draw_findings !=
          wifi_scan_obj.network_scan_finding_count;
  const bool cursor_dirty = reset ||
      this->network_scan_draw_cursor != cursor;

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  tft.setTextDatum(TL_DATUM);

  if (reset) {
    tft.fillScreen(TFT_BLACK);
    this->drawSharkTopBar(true);
    tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, 25, TFT_BLACK);
    tft.drawXBitmap(5, STATUS_BAR_WIDTH + 1,
                    menu_icons[this->networkScannerIcon(mode)],
                    ICON_W, ICON_H, TFT_BLACK, WD_CYAN);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString(this->networkScannerTitle(mode), 31,
                   STATUS_BAR_WIDTH + 7, 2);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 72, WD_CYAN);
    tft.fillRoundRect(177, STATUS_BAR_WIDTH + 3, 59, 19, 3, WD_CYAN);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, WD_CYAN);
    tft.drawString("BACK", 206, STATUS_BAR_WIDTH + 12, 1);

    const char* labels[3] = {"CHECKED", "FOUND", "PROGRESS"};
    for (uint8_t i = 0; i < 3; i++) {
      const int16_t x = 2 + i * 80;
      wdPanel(tft, x, 51, 76, 42, WD_SURFACE, WD_EDGE, WD_CYAN);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_DIM, WD_SURFACE);
      tft.drawString(labels[i], x + 7, 56, 1);
    }
    wdPanel(tft, 2, 99, 236, 42, WD_PANEL, WD_EDGE, WD_CYAN);
    wdPanel(tft, 2, 147, 236, 108, WD_PANEL, WD_EDGE, WD_CYAN);
  }

  if (stats_dirty) {
    const String values[3] = {
      String(checked), String(wifi_scan_obj.network_scan_hits),
      String(progress) + "%"
    };
    for (uint8_t i = 0; i < 3; i++) {
      const int16_t x = 2 + i * 80;
      tft.fillRect(x + 5, 70, 66, 19, WD_SURFACE);
      tft.setTextDatum(MR_DATUM);
      tft.setTextColor(i == 1 && wifi_scan_obj.network_scan_hits ?
                       WD_AMBER : WD_BONE, WD_SURFACE);
      tft.drawString(values[i], x + 69, 80, 2);
    }

  }

  if (findings_dirty || state_dirty) {
    tft.fillRect(6, 165, 228, 84, WD_PANEL);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_PANEL);
    tft.drawString(mode == WIFI_PING_SCAN ? "LIVE HOSTS" : "OPEN SERVICES",
                   9, 153, 1);
    if (!wifi_scan_obj.network_scan_finding_count) {
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(WD_DIM, WD_PANEL);
      tft.drawString(running ? "WAITING FOR A REAL REPLY" :
                     (connected ? "NO RESULTS" : "CONNECT WIFI FIRST"),
                     120, 204, 1);
    } else {
      const uint8_t count = wifi_scan_obj.network_scan_finding_count;
      const uint8_t start = count > 5 ? count - 5 : 0;
      uint8_t row = 0;
      for (uint8_t i = start; i < count; i++, row++) {
        const NetworkScanFinding& finding =
            wifi_scan_obj.network_scan_findings[i];
        String line = finding.ip.toString();
        if (finding.port) line += ":" + String(finding.port);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(row + 1 == count - start ? WD_AMBER : WD_BONE,
                         WD_PANEL);
        tft.drawString(line, 9, 169 + row * 15, 1);
      }
    }
  }

  if (cursor_dirty || state_dirty) {
    tft.fillRect(6, 103, 228, 34, WD_PANEL);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_PANEL);
    tft.drawString(mode == WIFI_PORT_SCAN_ALL ? "TARGET / CURRENT PORT" :
                   "LOCAL /24 CURSOR", 9, 105, 1);
    tft.setTextColor(connected ? WD_BONE : WD_RED, WD_PANEL);
    tft.drawString(connected ? cursor : "NOT CONNECTED", 9, 121, 1);
    tft.setTextDatum(TR_DATUM);
    const uint16_t state_color = !connected ? WD_RED :
        (complete ? WD_AMBER : (running ? WD_CYAN : WD_DIM));
    tft.setTextColor(state_color, WD_PANEL);
    tft.drawString(!connected ? "OFFLINE" :
                   (complete ? "COMPLETE" : (running ? "SCANNING" : "PAUSED")),
                   231, 121, 1);
  }

  if (reset || state_dirty) {
    const char* buttons[4] = {
      connected ? "RESTART" : "RETRY",
      running ? "PAUSE" : "RESUME", "CLEAR", "BACK"
    };
    const int16_t button_x[4] = {2, 58, 114, 170};
    const int16_t button_w[4] = {52, 52, 52, 68};
    for (uint8_t i = 0; i < 4; i++) {
      const uint16_t fill = i == 3 ? WD_CYAN :
          (i == 1 && !running && connected ? WD_AMBER : WD_SURFACE);
      wdPanel(tft, button_x[i], 260, button_w[i], 56, fill,
              i == 3 ? WD_CYAN : WD_EDGE, i == 1 ? WD_AMBER : WD_CYAN);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor((i == 3 || (i == 1 && !running && connected)) ?
                       TFT_BLACK : WD_BONE, fill);
      tft.drawString(buttons[i], button_x[i] + button_w[i] / 2, 288, 1);
    }
  }

  this->network_scan_draw_valid = true;
  this->network_scan_draw_checked = checked;
  this->network_scan_draw_hits = wifi_scan_obj.network_scan_hits;
  this->network_scan_draw_findings =
      wifi_scan_obj.network_scan_finding_count;
  this->network_scan_draw_running = running;
  this->network_scan_draw_complete = complete;
  this->network_scan_draw_cursor = cursor;
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::handleNetworkScannerTouch(uint16_t touch_x,
                                               uint16_t touch_y) {
  if ((touch_x >= 154 && touch_y >= 238) ||
      (touch_y >= STATUS_BAR_WIDTH && touch_y < 51)) {
    this->exitNetworkScannerUI();
    return;
  }
  if (touch_y < 238) return;
  const uint8_t action = touch_x < 56 ? 0 :
      (touch_x < 112 ? 1 : (touch_x < 168 ? 2 : 3));
  if (action == 0) {
    if (WiFi.status() == WL_CONNECTED) {
      if (wifi_scan_obj.isNetworkScannerMode(wifi_scan_obj.currentScanMode))
        wifi_scan_obj.restartNetworkScan();
      else
        wifi_scan_obj.StartScan(this->network_scan_requested_mode, TFT_CYAN);
    }
  } else if (action == 1) {
    if (WiFi.status() == WL_CONNECTED &&
        wifi_scan_obj.isNetworkScannerMode(wifi_scan_obj.currentScanMode)) {
      if (wifi_scan_obj.networkScanComplete())
        wifi_scan_obj.restartNetworkScan();
      else
        wifi_scan_obj.setNetworkScanPaused(
            wifi_scan_obj.network_scan_running);
    }
  } else if (action == 2) {
    wifi_scan_obj.clearNetworkScanResults();
  } else {
    this->exitNetworkScannerUI();
    return;
  }
  this->network_scan_draw_valid = false;
  this->drawNetworkScannerUI(true);
}

void MenuFunctions::exitNetworkScannerUI() {
  TFT_eSPI& tft = display_obj.tft;
  tft.fillRoundRect(34, 132, 172, 56, 5, WD_CYAN);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, WD_CYAN);
  tft.drawString("EXITING...", 120, 160, 2);
  this->network_scan_ui = false;
  this->network_scan_draw_valid = false;
  this->network_scan_touch_active = false;
  this->network_scan_touch_region = -1;
  wifi_scan_obj.network_scan_ui_owned = false;
  if (wifi_scan_obj.isNetworkScannerMode(wifi_scan_obj.currentScanMode))
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  display_obj.init();
  this->changeMenu(&wifiScannerMenu, true);
}

// --- Custom Beacon List dashboard ----------------------------------------

void MenuFunctions::startBeaconListUI() {
  // R111 guardrail: transmit apps can be PIN-locked (AttackPin setting).
  if (!this->attackPinOk()) {
    this->attackPinDenied();
    return;
  }
  this->wifi_passive_ui = false;
  this->wifi_tool_ui = false;
  this->network_scan_ui = false;
  this->beacon_spam_ui = false;
  this->funny_beacon_ui = false;
  this->rick_roll_ui = false;
  this->probe_flood_ui = false;
  this->deauth_flood_ui = false;
  this->attack_dash_ui = false;
  this->beacon_list_ui = true;
  this->beacon_list_page = 0;
  this->beacon_list_draw_valid = false;
  this->beacon_list_touch_active = false;
  this->beacon_list_touch_region = -1;
  this->beacon_list_touch_ms = 0;
  this->beacon_list_touch_ready_at = millis() + 320;

  wifi_scan_obj.wifi_tool_ui_owned = false;
  wifi_scan_obj.network_scan_ui_owned = false;
  wifi_scan_obj.beacon_spam_ui_owned = false;
  wifi_scan_obj.funny_beacon_ui_owned = false;
  wifi_scan_obj.rick_roll_ui_owned = false;
  wifi_scan_obj.probe_flood_ui_owned = false;
  wifi_scan_obj.deauth_flood_ui_owned = false;
  wifi_scan_obj.attack_dash_ui_owned = false;
  wifi_scan_obj.beacon_list_ui_owned = true;
  wifi_scan_obj.clearBeaconListStats();

  display_obj.tft.fillScreen(TFT_BLACK);
  this->drawSharkTopBar(true);
  wifi_scan_obj.StartScan(WIFI_ATTACK_BEACON_LIST, TFT_RED);
  // Opening the dashboard never transmits. START is a deliberate touch.
  wifi_scan_obj.setBeaconListRunning(false);
  this->drawBeaconListUI(true);
}

void MenuFunctions::drawBeaconListUI(bool full_redraw) {
  if (!this->beacon_list_ui) return;
  extern LinkedList<ssid>* ssids;
  TFT_eSPI& tft = display_obj.tft;
  const uint16_t count = ssids ? ssids->size() : 0;
  const uint16_t pages = max((uint16_t)1, (uint16_t)((count + 4) / 5));
  if (this->beacon_list_page >= pages)
    this->beacon_list_page = pages - 1;
  const bool running = wifi_scan_obj.beacon_list_running && count > 0;
  const bool reset = full_redraw || !this->beacon_list_draw_valid;
  const bool state_dirty = reset ||
      this->beacon_list_draw_running != running;
  const bool stats_dirty = reset || state_dirty ||
      this->beacon_list_draw_rate != wifi_scan_obj.beacon_list_rate ||
      this->beacon_list_draw_total != wifi_scan_obj.beacon_list_total ||
      this->beacon_list_draw_count != count;
  const bool list_dirty = reset ||
      this->beacon_list_draw_page != this->beacon_list_page ||
      this->beacon_list_draw_count != count;

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  tft.setTextDatum(TL_DATUM);

  if (reset) {
    tft.fillScreen(TFT_BLACK);
    this->drawSharkTopBar(true);
    tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, 25, TFT_BLACK);
    tft.drawXBitmap(5, STATUS_BAR_WIDTH + 1,
                    menu_icons[BEACON_LIST_ATTACK_ICON],
                    ICON_W, ICON_H, TFT_BLACK, WD_RED);
    tft.setTextColor(WD_RED, TFT_BLACK);
    tft.drawString("// BEACON LIST", 31, STATUS_BAR_WIDTH + 7, 2);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 72, WD_RED);
    tft.fillRoundRect(177, STATUS_BAR_WIDTH + 3, 59, 19, 3, WD_RED);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, WD_RED);
    tft.drawString("BACK", 206, STATUS_BAR_WIDTH + 12, 1);

    const char* labels[3] = {"SSIDS", "RATE/S", "TOTAL"};
    for (uint8_t i = 0; i < 3; i++) {
      const int16_t x = 2 + i * 80;
      wdPanel(tft, x, 51, 76, 42, WD_SURFACE, WD_EDGE, WD_RED);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_DIM, WD_SURFACE);
      tft.drawString(labels[i], x + 7, 56, 1);
    }
    wdPanel(tft, 2, 99, 236, 156, WD_PANEL, WD_EDGE, WD_RED);
  }

  if (stats_dirty) {
    const String values[3] = {
      String(count), String(wifi_scan_obj.beacon_list_rate),
      String(wifi_scan_obj.beacon_list_total)
    };
    for (uint8_t i = 0; i < 3; i++) {
      const int16_t x = 2 + i * 80;
      tft.fillRect(x + 5, 70, 66, 19, WD_SURFACE);
      tft.setTextDatum(MR_DATUM);
      tft.setTextColor(i == 1 && running ? WD_RED : WD_BONE, WD_SURFACE);
      tft.drawString(values[i], x + 69, 80, 2);
    }
  }

  if (list_dirty || state_dirty) {
    tft.fillRect(6, 103, 228, 146, WD_PANEL);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_PANEL);
    tft.drawString("CUSTOM SSID LIST", 9, 105, 1);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(running ? WD_RED : WD_CYAN, WD_PANEL);
    tft.drawString(running ? "TRANSMITTING" : "READY", 231, 105, 1);

    if (!count) {
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(WD_BONE, WD_PANEL);
      tft.drawString("NO CUSTOM SSIDS", 120, 169, 2);
      tft.setTextColor(WD_DIM, WD_PANEL);
      tft.drawString("ADD OR LOAD SSIDS FIRST", 120, 193, 1);
    } else {
      const uint16_t start = this->beacon_list_page * 5;
      for (uint8_t row = 0; row < 5; row++) {
        const uint16_t index = start + row;
        const int16_t y = 123 + row * 24;
        tft.drawFastHLine(9, y + 21, 222, WD_EDGE);
        if (index >= count) continue;
        ssid item = ssids->get(index);
        String shown = item.essid;
        shown.replace("\r", " ");
        shown.replace("\n", " ");
        shown.replace("\t", " ");
        if (shown.length() > 18) shown = shown.substring(0, 17) + "~";
        if (!shown.length()) shown = "<HIDDEN>";
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(WD_RED, WD_PANEL);
        tft.drawString(String(index + 1), 10, y + 5, 1);
        tft.setTextColor(WD_BONE, WD_PANEL);
        tft.drawString(shown, 30, y + 5, 1);
        tft.setTextDatum(TR_DATUM);
        tft.setTextColor(WD_DIM, WD_PANEL);
        tft.drawString("CH" + String(item.channel), 231, y + 5, 1);
      }
      tft.setTextDatum(TR_DATUM);
      tft.setTextColor(WD_DIM, WD_PANEL);
      tft.drawString(String(this->beacon_list_page + 1) + "/" +
                     String(pages), 231, 238, 1);
    }
  }

  if (reset || state_dirty || this->beacon_list_draw_count != count) {
    const char* labels[4] = {
      running ? "STOP" : "START", "PREV", "NEXT", "BACK"
    };
    const int16_t x[4] = {2, 72, 126, 180};
    const int16_t width[4] = {66, 50, 50, 58};
    for (uint8_t i = 0; i < 4; i++) {
      uint16_t fill = WD_SURFACE;
      if (i == 0) fill = !count ? WD_SURFACE : (running ? WD_RED : WD_CYAN);
      if (i == 3) fill = WD_RED;
      wdPanel(tft, x[i], 260, width[i], 56, fill,
              (i == 0 || i == 3) ? fill : WD_EDGE, WD_RED);
      tft.setTextDatum(MC_DATUM);
      const bool dark_text = (i == 3) || (i == 0 && count);
      tft.setTextColor(dark_text ? TFT_BLACK :
                       (i == 0 && !count ? WD_DIM : WD_BONE), fill);
      tft.drawString(labels[i], x[i] + width[i] / 2, 288, 1);
    }
  }

  this->beacon_list_draw_valid = true;
  this->beacon_list_draw_running = running;
  this->beacon_list_draw_rate = wifi_scan_obj.beacon_list_rate;
  this->beacon_list_draw_total = wifi_scan_obj.beacon_list_total;
  this->beacon_list_draw_count = count;
  this->beacon_list_draw_page = this->beacon_list_page;
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::handleBeaconListTouch(uint16_t touch_x,
                                          uint16_t touch_y) {
  if ((touch_x >= 176 && touch_y >= 238) ||
      (touch_y >= STATUS_BAR_WIDTH && touch_y < 51)) {
    this->exitBeaconListUI();
    return;
  }
  if (touch_y < 238) return;
  extern LinkedList<ssid>* ssids;
  const uint16_t count = ssids ? ssids->size() : 0;
  const uint16_t pages = max((uint16_t)1, (uint16_t)((count + 4) / 5));
  if (touch_x < 70) {
    if (count)
      wifi_scan_obj.setBeaconListRunning(!wifi_scan_obj.beacon_list_running);
  } else if (touch_x < 124) {
    this->beacon_list_page = this->beacon_list_page == 0 ?
        pages - 1 : this->beacon_list_page - 1;
  } else if (touch_x < 178) {
    this->beacon_list_page = (this->beacon_list_page + 1) % pages;
  } else {
    this->exitBeaconListUI();
    return;
  }
  this->beacon_list_draw_valid = false;
  this->drawBeaconListUI(true);
}

void MenuFunctions::exitBeaconListUI() {
  TFT_eSPI& tft = display_obj.tft;
  tft.fillRoundRect(34, 132, 172, 56, 5, WD_RED);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, WD_RED);
  tft.drawString("STOPPING...", 120, 160, 2);
  this->beacon_list_ui = false;
  this->beacon_list_draw_valid = false;
  this->beacon_list_touch_active = false;
  this->beacon_list_touch_region = -1;
  wifi_scan_obj.beacon_list_running = false;
  wifi_scan_obj.beacon_list_ui_owned = false;
  if (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BEACON_LIST)
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  display_obj.init();
  this->changeMenu(&wifiAttackMenu, true);
}

// --- Random Beacon Spam dashboard ---------------------------------------

void MenuFunctions::startBeaconSpamUI() {
  // R111 guardrail: transmit apps can be PIN-locked (AttackPin setting).
  if (!this->attackPinOk()) {
    this->attackPinDenied();
    return;
  }
  this->wifi_passive_ui = false;
  this->wifi_tool_ui = false;
  this->network_scan_ui = false;
  this->beacon_list_ui = false;
  this->funny_beacon_ui = false;
  this->rick_roll_ui = false;
  this->probe_flood_ui = false;
  this->deauth_flood_ui = false;
  this->attack_dash_ui = false;
  this->beacon_spam_ui = true;
  this->beacon_spam_draw_valid = false;
  this->beacon_spam_touch_active = false;
  this->beacon_spam_touch_region = -1;
  this->beacon_spam_touch_ms = 0;
  this->beacon_spam_touch_ready_at = millis() + 320;
  memset(this->beacon_spam_graph_bars, 0,
         sizeof(this->beacon_spam_graph_bars));

  wifi_scan_obj.wifi_tool_ui_owned = false;
  wifi_scan_obj.network_scan_ui_owned = false;
  wifi_scan_obj.beacon_list_ui_owned = false;
  wifi_scan_obj.funny_beacon_ui_owned = false;
  wifi_scan_obj.rick_roll_ui_owned = false;
  wifi_scan_obj.probe_flood_ui_owned = false;
  wifi_scan_obj.deauth_flood_ui_owned = false;
  wifi_scan_obj.attack_dash_ui_owned = false;
  wifi_scan_obj.beacon_spam_ui_owned = true;
  wifi_scan_obj.clearBeaconSpamStats();

  display_obj.tft.fillScreen(TFT_BLACK);
  this->drawSharkTopBar(true);
  wifi_scan_obj.StartScan(WIFI_ATTACK_BEACON_SPAM, TFT_ORANGE);
  // Opening the dashboard never transmits. START is a deliberate touch.
  wifi_scan_obj.setBeaconSpamRunning(false);
  this->drawBeaconSpamUI(true);
}

void MenuFunctions::drawBeaconSpamUI(bool full_redraw) {
  if (!this->beacon_spam_ui) return;
  TFT_eSPI& tft = display_obj.tft;
  const bool running = wifi_scan_obj.beacon_spam_running;
  const bool reset = full_redraw || !this->beacon_spam_draw_valid;
  const bool state_dirty = reset ||
      this->beacon_spam_draw_running != running;
  const bool sample_dirty = reset ||
      this->beacon_spam_draw_total != wifi_scan_obj.beacon_spam_total;
  const bool stats_dirty = reset || state_dirty || sample_dirty ||
      this->beacon_spam_draw_rate != wifi_scan_obj.beacon_spam_rate ||
      this->beacon_spam_draw_channel != wifi_scan_obj.set_channel;

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  tft.setTextDatum(TL_DATUM);

  if (reset) {
    tft.fillScreen(TFT_BLACK);
    this->drawSharkTopBar(true);
    tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, 25, TFT_BLACK);
    tft.drawXBitmap(5, STATUS_BAR_WIDTH + 1,
                    menu_icons[BEACON_SPAM_ATTACK_ICON],
                    ICON_W, ICON_H, TFT_BLACK, WD_RED);
    tft.setTextColor(WD_RED, TFT_BLACK);
    tft.drawString("// BEACON SPAM", 31, STATUS_BAR_WIDTH + 7, 2);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 82, WD_RED);
    tft.fillRoundRect(177, STATUS_BAR_WIDTH + 3, 59, 19, 3, WD_RED);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, WD_RED);
    tft.drawString("BACK", 206, STATUS_BAR_WIDTH + 12, 1);

    const char* labels[3] = {"RATE/S", "TOTAL", "CHANNEL"};
    for (uint8_t i = 0; i < 3; i++) {
      const int16_t x = 2 + i * 80;
      wdPanel(tft, x, 51, 76, 42, WD_SURFACE, WD_EDGE, WD_RED);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_DIM, WD_SURFACE);
      tft.drawString(labels[i], x + 7, 56, 1);
    }

    wdPanel(tft, 2, 99, 236, 156, WD_PANEL, WD_EDGE, WD_RED);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_PANEL);
    tft.drawString("RANDOM SSID ENGINE", 9, 105, 1);
    tft.setTextColor(WD_BONE, WD_PANEL);
    tft.drawString("SSID", 10, 128, 1);
    tft.drawString("LENGTH", 10, 148, 1);
    tft.drawString("HOP", 10, 168, 1);
    tft.setTextColor(WD_CYAN, WD_PANEL);
    tft.drawString("RANDOM ASCII", 73, 128, 1);
    tft.drawString("1..32 CHARS", 73, 148, 1);
    tft.drawString("AUTO CH 1..11", 73, 168, 1);
    tft.setTextColor(WD_DIM, WD_PANEL);
    tft.drawString("REAL FRAME ACTIVITY", 9, 190, 1);
  }

  if (stats_dirty) {
    const String values[3] = {
      String(wifi_scan_obj.beacon_spam_rate),
      String(wifi_scan_obj.beacon_spam_total),
      String(wifi_scan_obj.set_channel)
    };
    for (uint8_t i = 0; i < 3; i++) {
      const int16_t x = 2 + i * 80;
      tft.fillRect(x + 5, 70, 66, 19, WD_SURFACE);
      tft.setTextDatum(MR_DATUM);
      tft.setTextColor(i == 0 && running ? WD_RED : WD_BONE, WD_SURFACE);
      tft.drawString(values[i], x + 69, 80, 2);
    }
    tft.fillRect(150, 184, 80, 14, WD_PANEL);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(WD_DIM, WD_PANEL);
    tft.drawString("PEAK " + String(wifi_scan_obj.beacon_spam_peak),
                   230, 190, 1);
  }

  if (state_dirty) {
    tft.fillRect(137, 103, 94, 15, WD_PANEL);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(running ? WD_RED : WD_CYAN, WD_PANEL);
    tft.drawString(running ? "TRANSMITTING" : "READY", 231, 105, 1);
  }

  if (sample_dirty && !reset && running) {
    memmove(this->beacon_spam_graph_bars,
            this->beacon_spam_graph_bars + 1,
            sizeof(this->beacon_spam_graph_bars) - 1);
    const uint16_t peak = max((uint16_t)1, wifi_scan_obj.beacon_spam_peak);
    this->beacon_spam_graph_bars[53] = min(
        (uint16_t)38,
        (uint16_t)(((uint32_t)wifi_scan_obj.beacon_spam_rate * 38UL) / peak));
  }

  if (reset || sample_dirty || state_dirty) {
    tft.fillRect(9, 202, 222, 46, WD_PANEL);
    tft.drawFastHLine(9, 247, 222, WD_EDGE);
    for (uint8_t i = 0; i < 54; i++) {
      const uint8_t height = this->beacon_spam_graph_bars[i];
      if (!height) continue;
      const uint16_t color = i == 53 && running ? WD_RED : WD_CYAN;
      tft.fillRect(11 + i * 4, 246 - height, 2, height, color);
    }
  }

  if (reset || state_dirty) {
    const char* labels[3] = {
      running ? "STOP" : "START", "CLEAR", "BACK"
    };
    const int16_t x[3] = {2, 102, 170};
    const int16_t width[3] = {96, 64, 68};
    for (uint8_t i = 0; i < 3; i++) {
      uint16_t fill = i == 0 ? (running ? WD_RED : WD_CYAN) : WD_SURFACE;
      if (i == 2) fill = WD_RED;
      wdPanel(tft, x[i], 260, width[i], 56, fill,
              (i == 0 || i == 2) ? fill : WD_EDGE, WD_RED);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(i == 0 || i == 2 ? TFT_BLACK : WD_BONE, fill);
      tft.drawString(labels[i], x[i] + width[i] / 2, 288, 1);
    }
  }

  this->beacon_spam_draw_valid = true;
  this->beacon_spam_draw_running = running;
  this->beacon_spam_draw_rate = wifi_scan_obj.beacon_spam_rate;
  this->beacon_spam_draw_total = wifi_scan_obj.beacon_spam_total;
  this->beacon_spam_draw_channel = wifi_scan_obj.set_channel;
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::handleBeaconSpamTouch(uint16_t touch_x,
                                          uint16_t touch_y) {
  if ((touch_x >= 168 && touch_y >= 238) ||
      (touch_y >= STATUS_BAR_WIDTH && touch_y < 51)) {
    this->exitBeaconSpamUI();
    return;
  }
  if (touch_y < 238) return;
  if (touch_x < 100) {
    wifi_scan_obj.setBeaconSpamRunning(!wifi_scan_obj.beacon_spam_running);
  } else if (touch_x < 168) {
    wifi_scan_obj.clearBeaconSpamStats();
    memset(this->beacon_spam_graph_bars, 0,
           sizeof(this->beacon_spam_graph_bars));
  } else {
    this->exitBeaconSpamUI();
    return;
  }
  this->beacon_spam_draw_valid = false;
  this->drawBeaconSpamUI(true);
}

void MenuFunctions::exitBeaconSpamUI() {
  TFT_eSPI& tft = display_obj.tft;
  tft.fillRoundRect(34, 132, 172, 56, 5, WD_RED);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, WD_RED);
  tft.drawString("STOPPING...", 120, 160, 2);
  this->beacon_spam_ui = false;
  this->beacon_spam_draw_valid = false;
  this->beacon_spam_touch_active = false;
  this->beacon_spam_touch_region = -1;
  wifi_scan_obj.beacon_spam_running = false;
  wifi_scan_obj.beacon_spam_ui_owned = false;
  if (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BEACON_SPAM)
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  display_obj.init();
  this->changeMenu(&wifiAttackMenu, true);
}

// --- Probe Request Flood dashboard -------------------------------------

void MenuFunctions::startProbeFloodUI() {
  // R111 guardrail: transmit apps can be PIN-locked (AttackPin setting).
  if (!this->attackPinOk()) {
    this->attackPinDenied();
    return;
  }
  this->wifi_passive_ui = false;
  this->wifi_tool_ui = false;
  this->network_scan_ui = false;
  this->beacon_list_ui = false;
  this->beacon_spam_ui = false;
  this->funny_beacon_ui = false;
  this->rick_roll_ui = false;
  this->probe_flood_ui = true;
  this->deauth_flood_ui = false;
  this->attack_dash_ui = false;
  this->probe_flood_page = 0;
  this->probe_flood_draw_valid = false;
  this->probe_flood_touch_active = false;
  this->probe_flood_touch_region = -1;
  this->probe_flood_touch_ms = 0;
  this->probe_flood_touch_ready_at = millis() + 320;

  wifi_scan_obj.wifi_tool_ui_owned = false;
  wifi_scan_obj.network_scan_ui_owned = false;
  wifi_scan_obj.beacon_list_ui_owned = false;
  wifi_scan_obj.beacon_spam_ui_owned = false;
  wifi_scan_obj.funny_beacon_ui_owned = false;
  wifi_scan_obj.rick_roll_ui_owned = false;
  wifi_scan_obj.probe_flood_ui_owned = true;
  wifi_scan_obj.deauth_flood_ui_owned = false;
  wifi_scan_obj.attack_dash_ui_owned = false;
  wifi_scan_obj.clearProbeFloodStats();

  display_obj.tft.fillScreen(TFT_BLACK);
  this->drawSharkTopBar(true);
  wifi_scan_obj.StartScan(WIFI_ATTACK_AUTH, TFT_RED);
  // Opening the dashboard never transmits. START is a deliberate touch.
  wifi_scan_obj.setProbeFloodRunning(false);
  this->drawProbeFloodUI(true);
}

void MenuFunctions::drawProbeFloodUI(bool full_redraw) {
  if (!this->probe_flood_ui) return;
  extern LinkedList<AccessPoint>* access_points;
  TFT_eSPI& tft = display_obj.tft;
  const uint16_t count = access_points ? access_points->size() : 0;
  uint16_t selected = 0;
  for (uint16_t i = 0; i < count; i++)
    if (access_points->get(i).selected) selected++;
  const uint16_t pages = max((uint16_t)1, (uint16_t)((count + 4) / 5));
  if (this->probe_flood_page >= pages)
    this->probe_flood_page = pages - 1;
  const bool running = wifi_scan_obj.probe_flood_running && selected > 0;
  // R106 in-dashboard scan: self-heal on external mode changes and auto-stop
  // after a full channel sweep, re-arming this attack paused.
  if (wifi_scan_obj.attack_dash_scan_active) {
    const uint8_t cur = wifi_scan_obj.currentScanMode;
    if (cur != WIFI_SCAN_AP && cur != WIFI_SCAN_AP_STA) {
      wifi_scan_obj.attack_dash_scan_active = false;
    } else if (millis() - wifi_scan_obj.attack_dash_scan_start >= 16000) {
      wifi_scan_obj.attack_dash_scan_active = false;
      wifi_scan_obj.StartScan(WIFI_ATTACK_AUTH, TFT_RED);
      wifi_scan_obj.setProbeFloodRunning(false);
    }
  }
  const bool scanning = wifi_scan_obj.attack_dash_scan_active;
  const bool reset = full_redraw || !this->probe_flood_draw_valid;
  const bool state_dirty = reset ||
      this->probe_flood_draw_running != running ||
      this->attack_dash_draw_scanning != scanning;
  const bool stats_dirty = reset || state_dirty ||
      this->probe_flood_draw_rate != wifi_scan_obj.probe_flood_rate ||
      this->probe_flood_draw_total != wifi_scan_obj.probe_flood_total ||
      this->probe_flood_draw_selected != selected ||
      this->probe_flood_draw_count != count;
  const bool list_dirty = reset ||
      this->probe_flood_draw_page != this->probe_flood_page ||
      this->probe_flood_draw_count != count ||
      this->probe_flood_draw_selected != selected;

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  tft.setTextDatum(TL_DATUM);

  // Redesigned header: a red command strip with a beveled icon plate, a bold
  // wordmark, a direct-select subtitle, and a matching corner-ticked BACK chip.
  if (reset) {
    tft.fillScreen(TFT_BLACK);
    this->drawSharkTopBar(true);
    tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, 26, TFT_BLACK);
    // Icon plate.
    tft.fillRoundRect(3, STATUS_BAR_WIDTH + 1, 26, 24, 3, WD_SURFACE);
    tft.drawRoundRect(3, STATUS_BAR_WIDTH + 1, 26, 24, 3, WD_RED);
    tft.drawXBitmap(5, STATUS_BAR_WIDTH + 2,
                    menu_icons[PROBE_FLOOD_ATTACK_ICON],
                    ICON_W, ICON_H, WD_SURFACE, WD_RED);
    // Wordmark + subtitle.
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_RED, TFT_BLACK);
    tft.drawString("PROBE FLOOD", 34, STATUS_BAR_WIDTH + 3, 2);
    tft.setTextColor(WD_DIM, TFT_BLACK);
    tft.drawString("// DIRECT TARGET SELECT", 34, STATUS_BAR_WIDTH + 16, 1);
    // Double rule under the strip: full dim edge + red progress segment.
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 25, SCREEN_WIDTH, WD_EDGE);
    tft.fillRect(0, STATUS_BAR_WIDTH + 26, 92, 2, WD_RED);

    // Stat tiles, each capped with its own accent bar.
    const char* labels[3] = {"TARGETS", "RATE/S", "TOTAL"};
    const uint16_t tint[3] = {WD_CYAN, WD_RED, WD_AMBER};
    for (uint8_t i = 0; i < 3; i++) {
      const int16_t x = 2 + i * 80;
      wdPanel(tft, x, 51, 76, 42, WD_SURFACE, WD_EDGE, WD_RED);
      tft.fillRect(x + 1, 52, 74, 3, tint[i]);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_DIM, WD_SURFACE);
      tft.drawString(labels[i], x + 7, 59, 1);
    }
    wdPanel(tft, 2, 99, 236, 156, WD_PANEL, WD_EDGE, WD_RED);
  }

  if (stats_dirty) {
    const String values[3] = {
      String(selected) + "/" + String(count),
      String(wifi_scan_obj.probe_flood_rate),
      String(wifi_scan_obj.probe_flood_total)
    };
    const uint16_t tint[3] = {WD_CYAN, WD_RED, WD_AMBER};
    for (uint8_t i = 0; i < 3; i++) {
      const int16_t x = 2 + i * 80;
      tft.fillRect(x + 4, 70, 68, 20, WD_SURFACE);
      tft.setTextDatum(MR_DATUM);
      const bool hot = (i != 2) && running;
      tft.setTextColor(hot ? tint[i] : WD_BONE, WD_SURFACE);
      tft.drawString(values[i], x + 70, 81, 2);
    }
  }

  if (list_dirty || state_dirty) {
    tft.fillRect(6, 103, 228, 146, WD_PANEL);
    // List header band with a rounded state pill.
    tft.fillRect(6, 103, 228, 15, WD_SURFACE);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_SURFACE);
    tft.drawString("TARGETS", 10, 106, 1);
    // In-dashboard discovery chip: SCAN starts it, STOP ends it early.
    const uint16_t chip = scanning ? WD_RED : WD_CYAN;
    tft.fillRoundRect(92, 104, 70, 13, 6, chip);
    tft.drawRoundRect(92, 104, 70, 13, 6, WD_EDGE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, chip);
    tft.drawString(scanning ? "STOP SCAN" : "SCAN", 127, 111, 1);
    const uint16_t pill = scanning ? WD_AMBER :
                          (running ? WD_RED : WD_CYAN);
    tft.fillRoundRect(168, 104, 62, 13, 6, pill);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, pill);
    tft.drawString(scanning ? "SCANNING" :
                   (running ? "FLOODING" : "READY"), 199, 111, 1);
    tft.drawFastHLine(6, 118, 228, WD_EDGE);

    if (!count) {
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(WD_BONE, WD_PANEL);
      tft.drawString(scanning ? "SCANNING CHANNELS"
                              : "NO NETWORKS FOUND", 120, 172, 2);
      tft.setTextColor(WD_DIM, WD_PANEL);
      tft.drawString(scanning ? "TARGETS APPEAR HERE"
                              : "TAP SCAN TO FIND NETWORKS", 120, 196, 1);
    } else {
      const uint16_t start = this->probe_flood_page * 5;
      for (uint8_t row = 0; row < 5; row++) {
        const uint16_t index = start + row;
        const int16_t y = 123 + row * 24;
        if (index >= count) {
          tft.drawFastHLine(9, y + 21, 222, WD_EDGE);
          continue;
        }
        AccessPoint item = access_points->get(index);
        const bool is_sel = item.selected;
        // Selected rows get a full-width highlight plate + accent spine.
        if (is_sel) {
          tft.fillRoundRect(8, y, 224, 22, 3, WD_SURFACE);
          tft.fillRect(8, y, 3, 22, WD_RED);
        }
        const uint16_t row_bg = is_sel ? WD_SURFACE : WD_PANEL;
        // Selection marker box (filled check when targeted).
        tft.drawRect(14, y + 4, 13, 13, is_sel ? WD_RED : WD_EDGE);
        if (is_sel) {
          tft.fillRect(17, y + 7, 7, 7, WD_RED);
        }
        String shown = item.essid;
        shown.replace("\r", " ");
        shown.replace("\n", " ");
        shown.replace("\t", " ");
        if (shown.length() > 15) shown = shown.substring(0, 14) + "~";
        if (!shown.length()) shown = "<HIDDEN>";
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(is_sel ? WD_RED : WD_BONE, row_bg);
        tft.drawString(shown, 34, y + 3, 1);
        // Channel tag under the ssid.
        tft.setTextColor(WD_DIM, row_bg);
        tft.drawString("CH" + String(item.channel), 34, y + 12, 1);
        // Signal bars from rssi on the right edge.
        const int8_t r = item.rssi;
        const uint8_t bars = r >= -55 ? 4 : r >= -67 ? 3 : r >= -78 ? 2 : 1;
        for (uint8_t b = 0; b < 4; b++) {
          const int16_t bx = 204 + b * 7;
          const int16_t bh = 4 + b * 3;
          const uint16_t bc = b < bars ? (is_sel ? WD_RED : WD_CYAN)
                                       : WD_EDGE;
          tft.fillRect(bx, y + 15 - bh, 5, bh, bc);
        }
        if (!is_sel) tft.drawFastHLine(9, y + 21, 222, WD_EDGE);
      }
      tft.setTextDatum(TR_DATUM);
      tft.setTextColor(WD_DIM, WD_PANEL);
      tft.drawString("PAGE " + String(this->probe_flood_page + 1) + "/" +
                     String(pages), 231, 240, 1);
    }
  }

  if (reset || state_dirty ||
      this->probe_flood_draw_count != count ||
      this->probe_flood_draw_selected != selected) {
    const char* labels[4] = {
      running ? "STOP" : "START", "<", ">", "EXIT"
    };
    const int16_t x[4] = {2, 72, 126, 180};
    const int16_t width[4] = {66, 50, 50, 58};
    for (uint8_t i = 0; i < 4; i++) {
      uint16_t fill = WD_SURFACE;
      if (i == 0) fill = (!selected || scanning) ? WD_SURFACE
                       : (running ? WD_RED : WD_CYAN);
      if (i == 3) fill = WD_RED;
      const bool solid = (i == 0 && selected && !scanning) || i == 3;
      tft.fillRoundRect(x[i], 260, width[i], 56, 4, fill);
      tft.drawRoundRect(x[i], 260, width[i], 56, 4, solid ? fill : WD_EDGE);
      wdCornerTicks(tft, x[i], 260, width[i], 56, 4, WD_RED);
      tft.setTextDatum(MC_DATUM);
      const bool dark_text = solid;
      tft.setTextColor(dark_text ? TFT_BLACK :
                       (i == 0 && (!selected || scanning) ? WD_DIM : WD_BONE), fill);
      const uint8_t font = (i == 1 || i == 2) ? 2 : 1;
      tft.drawString(labels[i], x[i] + width[i] / 2, 288, font);
    }
  }

  this->probe_flood_draw_valid = true;
  this->probe_flood_draw_running = running;
  this->attack_dash_draw_scanning = scanning;
  this->probe_flood_draw_rate = wifi_scan_obj.probe_flood_rate;
  this->probe_flood_draw_total = wifi_scan_obj.probe_flood_total;
  this->probe_flood_draw_count = count;
  this->probe_flood_draw_selected = selected;
  this->probe_flood_draw_page = this->probe_flood_page;
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::handleProbeFloodTouch(uint16_t touch_x,
                                          uint16_t touch_y) {
  extern LinkedList<AccessPoint>* access_points;
  const uint16_t count = access_points ? access_points->size() : 0;
  const uint16_t pages = max((uint16_t)1, (uint16_t)((count + 4) / 5));

  // Only the lower EXIT button leaves the dashboard now.
  if (touch_x >= 176 && touch_y >= 255) {
    this->exitProbeFloodUI();
    return;
  }

  // In-dashboard discovery chip in the list header band.
  if (touch_x >= 88 && touch_x < 168 && touch_y >= 100 && touch_y < 120) {
    this->toggleAttackDashScan(WIFI_ATTACK_AUTH, false);
    this->probe_flood_draw_valid = false;
    this->drawProbeFloodUI(true);
    return;
  }

  // While scanning, the list is being rebuilt: keep only SCAN and EXIT live.
  if (wifi_scan_obj.attack_dash_scan_active) return;

  // List body: tap a row to toggle it as a probe-flood target directly.
  if (touch_y >= 120 && touch_y < 243 && count) {
    const int16_t row = (touch_y - 123) / 24;
    if (row >= 0 && row < 5) {
      const uint16_t index = this->probe_flood_page * 5 + row;
      if (index < count) {
        AccessPoint ap = access_points->get(index);
        ap.selected = !ap.selected;
        access_points->set(index, ap);
        this->probe_flood_draw_valid = false;
        this->drawProbeFloodUI(true);
      }
    }
    return;
  }

  if (touch_y < 255) return;

  if (touch_x < 70) {
    uint16_t selected = 0;
    for (uint16_t i = 0; i < count; i++)
      if (access_points->get(i).selected) selected++;
    if (selected)
      wifi_scan_obj.setProbeFloodRunning(!wifi_scan_obj.probe_flood_running);
  } else if (touch_x < 124) {
    this->probe_flood_page = this->probe_flood_page == 0 ?
        pages - 1 : this->probe_flood_page - 1;
  } else if (touch_x < 178) {
    this->probe_flood_page = (this->probe_flood_page + 1) % pages;
  } else {
    this->exitProbeFloodUI();
    return;
  }
  this->probe_flood_draw_valid = false;
  this->drawProbeFloodUI(true);
}

void MenuFunctions::exitProbeFloodUI() {
  TFT_eSPI& tft = display_obj.tft;
  tft.fillRoundRect(34, 132, 172, 56, 5, WD_RED);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, WD_RED);
  tft.drawString("STOPPING...", 120, 160, 2);
  this->probe_flood_ui = false;
  this->probe_flood_draw_valid = false;
  this->probe_flood_touch_active = false;
  this->probe_flood_touch_region = -1;
  wifi_scan_obj.probe_flood_running = false;
  wifi_scan_obj.probe_flood_ui_owned = false;
  if (wifi_scan_obj.attack_dash_scan_active) {
    wifi_scan_obj.attack_dash_scan_active = false;
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  } else if (wifi_scan_obj.currentScanMode == WIFI_ATTACK_AUTH)
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  display_obj.init();
  this->changeMenu(&wifiAttackMenu, true);
}

// --- Deauth Flood dashboard (mirrors Probe Request Flood) ----------------

void MenuFunctions::startDeauthFloodUI() {
  // R111 guardrail: transmit apps can be PIN-locked (AttackPin setting).
  if (!this->attackPinOk()) {
    this->attackPinDenied();
    return;
  }
  this->wifi_passive_ui = false;
  this->wifi_tool_ui = false;
  this->network_scan_ui = false;
  this->beacon_list_ui = false;
  this->beacon_spam_ui = false;
  this->funny_beacon_ui = false;
  this->rick_roll_ui = false;
  this->probe_flood_ui = false;
  this->attack_dash_ui = false;
  this->deauth_flood_ui = true;
  this->deauth_flood_page = 0;
  this->deauth_flood_draw_valid = false;
  this->deauth_flood_touch_active = false;
  this->deauth_flood_touch_region = -1;
  this->deauth_flood_touch_ms = 0;
  this->deauth_flood_touch_ready_at = millis() + 320;

  wifi_scan_obj.wifi_tool_ui_owned = false;
  wifi_scan_obj.network_scan_ui_owned = false;
  wifi_scan_obj.beacon_list_ui_owned = false;
  wifi_scan_obj.beacon_spam_ui_owned = false;
  wifi_scan_obj.funny_beacon_ui_owned = false;
  wifi_scan_obj.rick_roll_ui_owned = false;
  wifi_scan_obj.probe_flood_ui_owned = false;
  wifi_scan_obj.attack_dash_ui_owned = false;
  wifi_scan_obj.deauth_flood_ui_owned = true;
  wifi_scan_obj.clearDeauthFloodStats();

  display_obj.tft.fillScreen(TFT_BLACK);
  this->drawSharkTopBar(true);
  wifi_scan_obj.StartScan(WIFI_ATTACK_DEAUTH, TFT_RED);
  // Opening the dashboard never transmits. START is a deliberate touch.
  wifi_scan_obj.setDeauthFloodRunning(false);
  this->drawDeauthFloodUI(true);
}

void MenuFunctions::drawDeauthFloodUI(bool full_redraw) {
  if (!this->deauth_flood_ui) return;
  extern LinkedList<AccessPoint>* access_points;
  TFT_eSPI& tft = display_obj.tft;
  const uint16_t count = access_points ? access_points->size() : 0;
  uint16_t selected = 0;
  for (uint16_t i = 0; i < count; i++)
    if (access_points->get(i).selected) selected++;
  const uint16_t pages = max((uint16_t)1, (uint16_t)((count + 4) / 5));
  if (this->deauth_flood_page >= pages)
    this->deauth_flood_page = pages - 1;
  const bool running = wifi_scan_obj.deauth_flood_running && selected > 0;
  // R106 in-dashboard scan: self-heal on external mode changes and auto-stop
  // after a full channel sweep, re-arming this attack paused.
  if (wifi_scan_obj.attack_dash_scan_active) {
    const uint8_t cur = wifi_scan_obj.currentScanMode;
    if (cur != WIFI_SCAN_AP && cur != WIFI_SCAN_AP_STA) {
      wifi_scan_obj.attack_dash_scan_active = false;
    } else if (millis() - wifi_scan_obj.attack_dash_scan_start >= 16000) {
      wifi_scan_obj.attack_dash_scan_active = false;
      wifi_scan_obj.StartScan(WIFI_ATTACK_DEAUTH, TFT_RED);
      wifi_scan_obj.setDeauthFloodRunning(false);
    }
  }
  const bool scanning = wifi_scan_obj.attack_dash_scan_active;
  const bool reset = full_redraw || !this->deauth_flood_draw_valid;
  const bool state_dirty = reset ||
      this->deauth_flood_draw_running != running ||
      this->attack_dash_draw_scanning != scanning;
  const bool stats_dirty = reset || state_dirty ||
      this->deauth_flood_draw_rate != wifi_scan_obj.deauth_flood_rate ||
      this->deauth_flood_draw_total != wifi_scan_obj.deauth_flood_total ||
      this->deauth_flood_draw_selected != selected ||
      this->deauth_flood_draw_count != count;
  const bool list_dirty = reset ||
      this->deauth_flood_draw_page != this->deauth_flood_page ||
      this->deauth_flood_draw_count != count ||
      this->deauth_flood_draw_selected != selected;

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  tft.setTextDatum(TL_DATUM);

  // Same command-strip header as the probe dashboard: beveled icon plate,
  // bold wordmark, direct-select subtitle, and a matching corner-ticked look.
  if (reset) {
    tft.fillScreen(TFT_BLACK);
    this->drawSharkTopBar(true);
    tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, 26, TFT_BLACK);
    // Icon plate.
    tft.fillRoundRect(3, STATUS_BAR_WIDTH + 1, 26, 24, 3, WD_SURFACE);
    tft.drawRoundRect(3, STATUS_BAR_WIDTH + 1, 26, 24, 3, WD_RED);
    tft.drawXBitmap(5, STATUS_BAR_WIDTH + 2,
                    menu_icons[DEAUTH_FLOOD_ATTACK_ICON],
                    ICON_W, ICON_H, WD_SURFACE, WD_RED);
    // Wordmark + subtitle.
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_RED, TFT_BLACK);
    tft.drawString("DEAUTH FLOOD", 34, STATUS_BAR_WIDTH + 3, 2);
    tft.setTextColor(WD_DIM, TFT_BLACK);
    tft.drawString("// DIRECT TARGET SELECT", 34, STATUS_BAR_WIDTH + 16, 1);
    // Double rule under the strip: full dim edge + red progress segment.
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 25, SCREEN_WIDTH, WD_EDGE);
    tft.fillRect(0, STATUS_BAR_WIDTH + 26, 92, 2, WD_RED);

    // Stat tiles, each capped with its own accent bar.
    const char* labels[3] = {"TARGETS", "RATE/S", "TOTAL"};
    const uint16_t tint[3] = {WD_CYAN, WD_RED, WD_AMBER};
    for (uint8_t i = 0; i < 3; i++) {
      const int16_t x = 2 + i * 80;
      wdPanel(tft, x, 51, 76, 42, WD_SURFACE, WD_EDGE, WD_RED);
      tft.fillRect(x + 1, 52, 74, 3, tint[i]);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_DIM, WD_SURFACE);
      tft.drawString(labels[i], x + 7, 59, 1);
    }
    wdPanel(tft, 2, 99, 236, 156, WD_PANEL, WD_EDGE, WD_RED);
  }

  if (stats_dirty) {
    const String values[3] = {
      String(selected) + "/" + String(count),
      String(wifi_scan_obj.deauth_flood_rate),
      String(wifi_scan_obj.deauth_flood_total)
    };
    const uint16_t tint[3] = {WD_CYAN, WD_RED, WD_AMBER};
    for (uint8_t i = 0; i < 3; i++) {
      const int16_t x = 2 + i * 80;
      tft.fillRect(x + 4, 70, 68, 20, WD_SURFACE);
      tft.setTextDatum(MR_DATUM);
      const bool hot = (i != 2) && running;
      tft.setTextColor(hot ? tint[i] : WD_BONE, WD_SURFACE);
      tft.drawString(values[i], x + 70, 81, 2);
    }
  }

  if (list_dirty || state_dirty) {
    tft.fillRect(6, 103, 228, 146, WD_PANEL);
    // List header band with a rounded state pill.
    tft.fillRect(6, 103, 228, 15, WD_SURFACE);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_SURFACE);
    tft.drawString("TARGETS", 10, 106, 1);
    // In-dashboard discovery chip: SCAN starts it, STOP ends it early.
    const uint16_t chip = scanning ? WD_RED : WD_CYAN;
    tft.fillRoundRect(92, 104, 70, 13, 6, chip);
    tft.drawRoundRect(92, 104, 70, 13, 6, WD_EDGE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, chip);
    tft.drawString(scanning ? "STOP SCAN" : "SCAN", 127, 111, 1);
    const uint16_t pill = scanning ? WD_AMBER :
                          (running ? WD_RED : WD_CYAN);
    tft.fillRoundRect(168, 104, 62, 13, 6, pill);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, pill);
    tft.drawString(scanning ? "SCANNING" :
                   (running ? "FLOODING" : "READY"), 199, 111, 1);
    tft.drawFastHLine(6, 118, 228, WD_EDGE);

    if (!count) {
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(WD_BONE, WD_PANEL);
      tft.drawString(scanning ? "SCANNING CHANNELS"
                              : "NO NETWORKS FOUND", 120, 172, 2);
      tft.setTextColor(WD_DIM, WD_PANEL);
      tft.drawString(scanning ? "TARGETS APPEAR HERE"
                              : "TAP SCAN TO FIND NETWORKS", 120, 196, 1);
    } else {
      const uint16_t start = this->deauth_flood_page * 5;
      for (uint8_t row = 0; row < 5; row++) {
        const uint16_t index = start + row;
        const int16_t y = 123 + row * 24;
        if (index >= count) {
          tft.drawFastHLine(9, y + 21, 222, WD_EDGE);
          continue;
        }
        AccessPoint item = access_points->get(index);
        const bool is_sel = item.selected;
        // Selected rows get a full-width highlight plate + accent spine.
        if (is_sel) {
          tft.fillRoundRect(8, y, 224, 22, 3, WD_SURFACE);
          tft.fillRect(8, y, 3, 22, WD_RED);
        }
        const uint16_t row_bg = is_sel ? WD_SURFACE : WD_PANEL;
        // Selection marker box (filled check when targeted).
        tft.drawRect(14, y + 4, 13, 13, is_sel ? WD_RED : WD_EDGE);
        if (is_sel) {
          tft.fillRect(17, y + 7, 7, 7, WD_RED);
        }
        String shown = item.essid;
        shown.replace("\r", " ");
        shown.replace("\n", " ");
        shown.replace("\t", " ");
        if (shown.length() > 15) shown = shown.substring(0, 14) + "~";
        if (!shown.length()) shown = "<HIDDEN>";
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(is_sel ? WD_RED : WD_BONE, row_bg);
        tft.drawString(shown, 34, y + 3, 1);
        // Channel tag under the ssid.
        tft.setTextColor(WD_DIM, row_bg);
        tft.drawString("CH" + String(item.channel), 34, y + 12, 1);
        // Signal bars from rssi on the right edge.
        const int8_t r = item.rssi;
        const uint8_t bars = r >= -55 ? 4 : r >= -67 ? 3 : r >= -78 ? 2 : 1;
        for (uint8_t b = 0; b < 4; b++) {
          const int16_t bx = 204 + b * 7;
          const int16_t bh = 4 + b * 3;
          const uint16_t bc = b < bars ? (is_sel ? WD_RED : WD_CYAN)
                                       : WD_EDGE;
          tft.fillRect(bx, y + 15 - bh, 5, bh, bc);
        }
        if (!is_sel) tft.drawFastHLine(9, y + 21, 222, WD_EDGE);
      }
      tft.setTextDatum(TR_DATUM);
      tft.setTextColor(WD_DIM, WD_PANEL);
      tft.drawString("PAGE " + String(this->deauth_flood_page + 1) + "/" +
                     String(pages), 231, 240, 1);
    }
  }

  if (reset || state_dirty ||
      this->deauth_flood_draw_count != count ||
      this->deauth_flood_draw_selected != selected) {
    const char* labels[4] = {
      running ? "STOP" : "START", "<", ">", "EXIT"
    };
    const int16_t x[4] = {2, 72, 126, 180};
    const int16_t width[4] = {66, 50, 50, 58};
    for (uint8_t i = 0; i < 4; i++) {
      uint16_t fill = WD_SURFACE;
      if (i == 0) fill = (!selected || scanning) ? WD_SURFACE
                       : (running ? WD_RED : WD_CYAN);
      if (i == 3) fill = WD_RED;
      const bool solid = (i == 0 && selected && !scanning) || i == 3;
      tft.fillRoundRect(x[i], 260, width[i], 56, 4, fill);
      tft.drawRoundRect(x[i], 260, width[i], 56, 4, solid ? fill : WD_EDGE);
      wdCornerTicks(tft, x[i], 260, width[i], 56, 4, WD_RED);
      tft.setTextDatum(MC_DATUM);
      const bool dark_text = solid;
      tft.setTextColor(dark_text ? TFT_BLACK :
                       (i == 0 && (!selected || scanning) ? WD_DIM : WD_BONE), fill);
      const uint8_t font = (i == 1 || i == 2) ? 2 : 1;
      tft.drawString(labels[i], x[i] + width[i] / 2, 288, font);
    }
  }

  this->deauth_flood_draw_valid = true;
  this->deauth_flood_draw_running = running;
  this->attack_dash_draw_scanning = scanning;
  this->deauth_flood_draw_rate = wifi_scan_obj.deauth_flood_rate;
  this->deauth_flood_draw_total = wifi_scan_obj.deauth_flood_total;
  this->deauth_flood_draw_count = count;
  this->deauth_flood_draw_selected = selected;
  this->deauth_flood_draw_page = this->deauth_flood_page;
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::handleDeauthFloodTouch(uint16_t touch_x,
                                          uint16_t touch_y) {
  extern LinkedList<AccessPoint>* access_points;
  const uint16_t count = access_points ? access_points->size() : 0;
  const uint16_t pages = max((uint16_t)1, (uint16_t)((count + 4) / 5));

  // Only the lower EXIT button leaves the dashboard now.
  if (touch_x >= 176 && touch_y >= 255) {
    this->exitDeauthFloodUI();
    return;
  }

  // In-dashboard discovery chip in the list header band.
  if (touch_x >= 88 && touch_x < 168 && touch_y >= 100 && touch_y < 120) {
    this->toggleAttackDashScan(WIFI_ATTACK_DEAUTH, false);
    this->deauth_flood_draw_valid = false;
    this->drawDeauthFloodUI(true);
    return;
  }

  // While scanning, the list is being rebuilt: keep only SCAN and EXIT live.
  if (wifi_scan_obj.attack_dash_scan_active) return;

  // List body: tap a row to toggle it as a deauth-flood target directly.
  if (touch_y >= 120 && touch_y < 243 && count) {
    const int16_t row = (touch_y - 123) / 24;
    if (row >= 0 && row < 5) {
      const uint16_t index = this->deauth_flood_page * 5 + row;
      if (index < count) {
        AccessPoint ap = access_points->get(index);
        ap.selected = !ap.selected;
        access_points->set(index, ap);
        this->deauth_flood_draw_valid = false;
        this->drawDeauthFloodUI(true);
      }
    }
    return;
  }

  if (touch_y < 255) return;

  if (touch_x < 70) {
    uint16_t selected = 0;
    for (uint16_t i = 0; i < count; i++)
      if (access_points->get(i).selected) selected++;
    if (selected)
      wifi_scan_obj.setDeauthFloodRunning(!wifi_scan_obj.deauth_flood_running);
  } else if (touch_x < 124) {
    this->deauth_flood_page = this->deauth_flood_page == 0 ?
        pages - 1 : this->deauth_flood_page - 1;
  } else if (touch_x < 178) {
    this->deauth_flood_page = (this->deauth_flood_page + 1) % pages;
  } else {
    this->exitDeauthFloodUI();
    return;
  }
  this->deauth_flood_draw_valid = false;
  this->drawDeauthFloodUI(true);
}

void MenuFunctions::exitDeauthFloodUI() {
  TFT_eSPI& tft = display_obj.tft;
  tft.fillRoundRect(34, 132, 172, 56, 5, WD_RED);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, WD_RED);
  tft.drawString("STOPPING...", 120, 160, 2);
  this->deauth_flood_ui = false;
  this->attack_dash_ui = false;
  this->deauth_flood_draw_valid = false;
  this->deauth_flood_touch_active = false;
  this->deauth_flood_touch_region = -1;
  wifi_scan_obj.deauth_flood_running = false;
  wifi_scan_obj.deauth_flood_ui_owned = false;
  wifi_scan_obj.attack_dash_ui_owned = false;
  if (wifi_scan_obj.attack_dash_scan_active) {
    wifi_scan_obj.attack_dash_scan_active = false;
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  } else if (wifi_scan_obj.currentScanMode == WIFI_ATTACK_DEAUTH)
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  display_obj.init();
  this->changeMenu(&wifiAttackMenu, true);
}

// --- Generic attack dashboard (R104) -------------------------------------
// One paused-first dashboard drives every remaining Wi-Fi transmit attack.
// Each entry keeps its existing scan mode, frame format, and transmission
// loop; only the launch UX (direct target selection + START gate) and the
// live telemetry tiles are shared.

struct AttackDashDef {
  const char* title;
  const char* subtitle;
  uint8_t icon;
  uint8_t scan_mode;
  bool station_targets;  // true: rows are stations; false: rows are APs
};

static const AttackDashDef attackDashDefs[] = {
  {"AP CLONE SPAM",  "// CLONE TARGET APS",    BEACON_LIST, WIFI_ATTACK_AP_SPAM,          false},
  {"DEAUTH TARGET",  "// PICK VICTIM STATIONS", DEAUTH_SNIFF, WIFI_ATTACK_DEAUTH_TARGETED, true},
  {"BAD MSG",        "// PICK TARGET APS",     DEAUTH_SNIFF, WIFI_ATTACK_BAD_MSG,         false},
  {"BAD MSG TARGET", "// PICK VICTIM STATIONS", DEAUTH_SNIFF, WIFI_ATTACK_BAD_MSG_TARGETED, true},
  {"ASSOC SLEEP",    "// PICK TARGET APS",     DEAUTH_SNIFF, WIFI_ATTACK_SLEEP,           false},
  {"SLEEP TARGET",   "// PICK VICTIM STATIONS", DEAUTH_SNIFF, WIFI_ATTACK_SLEEP_TARGETED,  true},
  {"SAE FLOOD",      "// PICK TARGET APS",     EAPOL,        WIFI_ATTACK_SAE_COMMIT,      false},
  {"CHANNEL SWITCH", "// CLONE TARGET APS",    BEACON_LIST,  WIFI_ATTACK_CSA,             false},
  {"QUIET TIME",     "// CLONE TARGET APS",    BEACON_LIST,  WIFI_ATTACK_QUIET,           false},
  // R105 apps: open-system auth flood and faithful network cloning.
  {"AUTH RUSH",      "// FLOOD TARGET APS",    EAPOL,        WIFI_ATTACK_AUTH_RUSH,       false},
  {"MIMIC CLONE",    "// CLONE PICKED APS",    BEACON_LIST,  WIFI_ATTACK_MIMIC,           false},
};

// --- R111 guardrails + accountability -------------------------------------

// Optional PIN lock on every transmit app (AttackPin setting, empty = off).
// The keyboard is physical and on-device: authorization stays in the
// operator's hands, never over the network.
bool MenuFunctions::attackPinOk() {
  const String pin = settings_obj.loadSetting<String>("AttackPin");
  if (!pin.length()) return true;
  char buf[9] = {0};
  if (!keyboardInput(buf, sizeof(buf), "ENTER ATTACK PIN")) return false;
  return String(buf) == pin;
}

void MenuFunctions::attackPinDenied() {
  TFT_eSPI& tft = display_obj.tft;
  tft.fillRoundRect(34, 132, 172, 56, 5, TFT_RED);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, TFT_RED);
  tft.drawString("PIN REQUIRED", 120, 150, 2);
  tft.drawString("TRANSMIT APPS LOCKED", 120, 172, 1);
  tft.setTextDatum(TL_DATUM);
  delay(1100);
  this->changeMenu(&wifiAttackMenu, true);
}

// One-tap engagement documentation: everything the radio has detected so far
// is written to a timestamped text file on the SD card.
void MenuFunctions::exportSessionReport() {
    #ifdef HAS_SD
    extern LinkedList<AccessPoint>* access_points;
    extern LinkedList<Station>* stations;
    extern LinkedList<ProbeReqSsid>* probe_req_ssids;
    if (!sd_obj.supported) {
      TFT_eSPI& tft = display_obj.tft;
      tft.fillRoundRect(34, 132, 172, 56, 5, TFT_ORANGE);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(TFT_BLACK, TFT_ORANGE);
      tft.drawString("NO SD CARD", 120, 152, 2);
      tft.setTextDatum(TL_DATUM);
      delay(1100);
      this->changeMenu(&sharkDefenseMenu, true);
      return;
    }
    if (!SD.exists("/shark")) SD.mkdir("/shark");
    const String path = "/shark/report-" + String(millis()) + ".txt";
    File report = SD.open(path, FILE_WRITE);
    if (!report) {
      this->changeMenu(&sharkDefenseMenu, true);
      return;
    }
    report.println("M5SHARK SESSION REPORT");
    report.println("firmware: " + String(display_obj.version_number));
    report.println("uptime: " + String(millis() / 1000) + "s");
    #ifdef HAS_GPS
      if (gps_obj.getFixStatus())
        report.println("gps: " + gps_obj.getDatetime());
      else
        report.println("gps: no fix");
    #endif
    report.println("");
    report.println("[DETECTED NETWORKS] " +
                   String(access_points ? access_points->size() : 0));
    if (access_points) {
      const uint16_t n = access_points->size();
      for (uint16_t i = 0; i < n && i < 40; i++) {
        AccessPoint ap = access_points->get(i);
        String line = "  CH" + String(ap.channel) + " " + String(ap.rssi) + "dBm ";
        line += ap.essid.length() ? ap.essid : "<hidden>";
        report.println(line);
      }
      if (n > 40) report.println("  ... " + String(n - 40) + " more");
    }
    report.println("[CLIENTS] " + String(stations ? stations->size() : 0));
    report.println("[PROBE SSIDS] " + String(probe_req_ssids->size()));
    report.println("");
    report.println("[DETECTIONS]");
    report.println("eapol frames: " + String(wifi_scan_obj.eapol_frames));
    report.println("complete handshakes: " + String(wifi_scan_obj.getCompleteEapol()));
    report.println("deauth frames: " + String(wifi_scan_obj.deauth_frames));
    report.println("evil twins: " + String(wifi_scan_obj.evil_twin_count));
    report.println("pineapples confirmed: " + String(wifi_scan_obj.pineScanConfirmedCount()));
    report.println("multissid confirmed: " + String(wifi_scan_obj.multissidConfirmedCount()));
    report.println("drone rid broadcasts: " + String(wifi_scan_obj.drone_rid_count));
    report.println("");
    report.println("[TRANSMIT TOTALS]");
    report.println("probe flood frames: " + String(wifi_scan_obj.probe_flood_total));
    report.println("deauth flood frames: " + String(wifi_scan_obj.deauth_flood_total));
    report.println("attack app frames: " + String(wifi_scan_obj.attack_dash_total));
    report.close();
    TFT_eSPI& tft2 = display_obj.tft;
    tft2.fillRoundRect(24, 132, 192, 56, 5, TFT_GREEN);
    tft2.setTextDatum(MC_DATUM);
    tft2.setTextColor(TFT_BLACK, TFT_GREEN);
    tft2.drawString("REPORT SAVED", 120, 148, 2);
    tft2.setTextColor(TFT_DARKGREY, TFT_GREEN);
    tft2.drawString("/shark/report-" + String(millis()) + ".txt", 120, 170, 1);
    tft2.setTextDatum(TL_DATUM);
    delay(1300);
    this->changeMenu(&sharkDefenseMenu, true);
  #endif
}

// R106: run target discovery without leaving the dashboard. Tapping SCAN
// again (or the dashboard's auto-stop after a full channel sweep) re-arms
// the attack paused; scanning also pauses a running attack first.
void MenuFunctions::toggleAttackDashScan(uint8_t rearm_mode,
                                         bool station_targets) {
  if (wifi_scan_obj.attack_dash_scan_active) {
    wifi_scan_obj.attack_dash_scan_active = false;
    wifi_scan_obj.StartScan(rearm_mode, TFT_RED);
    wifi_scan_obj.setAttackDashRunning(false);
  } else {
    wifi_scan_obj.setAttackDashRunning(false);
    wifi_scan_obj.attack_dash_scan_start = millis();
    wifi_scan_obj.attack_dash_scan_active = true;
    wifi_scan_obj.StartScan(station_targets ? WIFI_SCAN_AP_STA : WIFI_SCAN_AP,
                            TFT_CYAN);
  }
}

void MenuFunctions::startAttackDashUI(uint8_t def_index) {
  // R111 guardrail: transmit apps can be PIN-locked (AttackPin setting).
  if (!this->attackPinOk()) {
    this->attackPinDenied();
    return;
  }
  const uint8_t count = sizeof(attackDashDefs) / sizeof(attackDashDefs[0]);
  if (def_index >= count) return;
  this->wifi_passive_ui = false;
  this->wifi_tool_ui = false;
  this->network_scan_ui = false;
  this->beacon_list_ui = false;
  this->beacon_spam_ui = false;
  this->funny_beacon_ui = false;
  this->rick_roll_ui = false;
  this->probe_flood_ui = false;
  this->deauth_flood_ui = false;
  this->attack_dash_ui = true;
  this->attack_dash_index = def_index;
  this->attack_dash_page = 0;
  this->attack_dash_draw_valid = false;
  this->attack_dash_touch_active = false;
  this->attack_dash_touch_region = -1;
  this->attack_dash_touch_ms = 0;
  this->attack_dash_touch_ready_at = millis() + 320;

  wifi_scan_obj.wifi_tool_ui_owned = false;
  wifi_scan_obj.network_scan_ui_owned = false;
  wifi_scan_obj.beacon_list_ui_owned = false;
  wifi_scan_obj.beacon_spam_ui_owned = false;
  wifi_scan_obj.funny_beacon_ui_owned = false;
  wifi_scan_obj.rick_roll_ui_owned = false;
  wifi_scan_obj.probe_flood_ui_owned = false;
  wifi_scan_obj.deauth_flood_ui_owned = false;
  wifi_scan_obj.attack_dash_ui_owned = true;
  wifi_scan_obj.attack_dash_scan_active = false;
  wifi_scan_obj.clearAttackDashStats();

  display_obj.tft.fillScreen(TFT_BLACK);
  this->drawSharkTopBar(true);
  wifi_scan_obj.StartScan(attackDashDefs[def_index].scan_mode, TFT_RED);
  // Opening the dashboard never transmits. START is a deliberate touch.
  wifi_scan_obj.setAttackDashRunning(false);
  this->drawAttackDashUI(true);
}

void MenuFunctions::drawAttackDashUI(bool full_redraw) {
  if (!this->attack_dash_ui) return;
  extern LinkedList<AccessPoint>* access_points;
  extern LinkedList<Station>* stations;
  const AttackDashDef& def = attackDashDefs[this->attack_dash_index];
  TFT_eSPI& tft = display_obj.tft;
  // Self-heal the in-dashboard scan: stop it if the scan mode was changed
  // elsewhere (web/serial stop), or auto-stop after a full channel sweep
  // and re-arm this attack paused.
  if (wifi_scan_obj.attack_dash_scan_active) {
    const uint8_t cur = wifi_scan_obj.currentScanMode;
    if (cur != WIFI_SCAN_AP && cur != WIFI_SCAN_AP_STA) {
      wifi_scan_obj.attack_dash_scan_active = false;
    } else if (millis() - wifi_scan_obj.attack_dash_scan_start >= 16000) {
      wifi_scan_obj.attack_dash_scan_active = false;
      wifi_scan_obj.StartScan(def.scan_mode, TFT_RED);
      wifi_scan_obj.setAttackDashRunning(false);
    }
  }
  const bool scanning = wifi_scan_obj.attack_dash_scan_active;
  const uint16_t count = def.station_targets
      ? (stations ? stations->size() : 0)
      : (access_points ? access_points->size() : 0);
  uint16_t selected = 0;
  for (uint16_t i = 0; i < count; i++)
    if (def.station_targets ? stations->get(i).selected
                            : access_points->get(i).selected) selected++;
  const uint16_t pages = max((uint16_t)1, (uint16_t)((count + 4) / 5));
  if (this->attack_dash_page >= pages)
    this->attack_dash_page = pages - 1;
  const bool running = wifi_scan_obj.attack_dash_running && selected > 0;
  const bool reset = full_redraw || !this->attack_dash_draw_valid;
  const bool state_dirty = reset ||
      this->attack_dash_draw_running != running ||
      this->attack_dash_draw_scanning != scanning;
  const bool stats_dirty = reset || state_dirty ||
      this->attack_dash_draw_rate != wifi_scan_obj.attack_dash_rate ||
      this->attack_dash_draw_total != wifi_scan_obj.attack_dash_total ||
      this->attack_dash_draw_selected != selected ||
      this->attack_dash_draw_count != count;
  const bool list_dirty = reset ||
      this->attack_dash_draw_page != this->attack_dash_page ||
      this->attack_dash_draw_count != count ||
      this->attack_dash_draw_selected != selected;

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  tft.setTextDatum(TL_DATUM);

  // Same command-strip header as the flood dashboards: beveled icon plate,
  // bold wordmark, direct-select subtitle, and the double rule under it.
  if (reset) {
    tft.fillScreen(TFT_BLACK);
    this->drawSharkTopBar(true);
    tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, 26, TFT_BLACK);
    tft.fillRoundRect(3, STATUS_BAR_WIDTH + 1, 26, 24, 3, WD_SURFACE);
    tft.drawRoundRect(3, STATUS_BAR_WIDTH + 1, 26, 24, 3, WD_RED);
    tft.drawXBitmap(5, STATUS_BAR_WIDTH + 2,
                    menu_icons[def.icon],
                    ICON_W, ICON_H, WD_SURFACE, WD_RED);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_RED, TFT_BLACK);
    tft.drawString(def.title, 34, STATUS_BAR_WIDTH + 3, 2);
    tft.setTextColor(WD_DIM, TFT_BLACK);
    tft.drawString(def.subtitle, 34, STATUS_BAR_WIDTH + 16, 1);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 25, SCREEN_WIDTH, WD_EDGE);
    tft.fillRect(0, STATUS_BAR_WIDTH + 26, 92, 2, WD_RED);

    const char* labels[3] = {"TARGETS", "RATE/S", "TOTAL"};
    const uint16_t tint[3] = {WD_CYAN, WD_RED, WD_AMBER};
    for (uint8_t i = 0; i < 3; i++) {
      const int16_t x = 2 + i * 80;
      wdPanel(tft, x, 51, 76, 42, WD_SURFACE, WD_EDGE, WD_RED);
      tft.fillRect(x + 1, 52, 74, 3, tint[i]);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_DIM, WD_SURFACE);
      tft.drawString(labels[i], x + 7, 59, 1);
    }
    wdPanel(tft, 2, 99, 236, 156, WD_PANEL, WD_EDGE, WD_RED);
  }

  if (stats_dirty) {
    const String values[3] = {
      String(selected) + "/" + String(count),
      String(wifi_scan_obj.attack_dash_rate),
      String(wifi_scan_obj.attack_dash_total)
    };
    const uint16_t tint[3] = {WD_CYAN, WD_RED, WD_AMBER};
    for (uint8_t i = 0; i < 3; i++) {
      const int16_t x = 2 + i * 80;
      tft.fillRect(x + 4, 70, 68, 20, WD_SURFACE);
      tft.setTextDatum(MR_DATUM);
      const bool hot = (i != 2) && running;
      tft.setTextColor(hot ? tint[i] : WD_BONE, WD_SURFACE);
      tft.drawString(values[i], x + 70, 81, 2);
    }
  }

  if (list_dirty || state_dirty) {
    tft.fillRect(6, 103, 228, 146, WD_PANEL);
    tft.fillRect(6, 103, 228, 15, WD_SURFACE);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_SURFACE);
    tft.drawString(def.station_targets ? "STATIONS" : "TARGETS",
                   10, 106, 1);
    // In-dashboard discovery chip: SCAN starts it, STOP ends it early.
    const uint16_t chip = scanning ? WD_RED : WD_CYAN;
    tft.fillRoundRect(92, 104, 70, 13, 6, chip);
    tft.drawRoundRect(92, 104, 70, 13, 6, WD_EDGE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, chip);
    tft.drawString(scanning ? "STOP SCAN" : "SCAN", 127, 111, 1);
    const uint16_t pill = scanning ? WD_AMBER :
                          (running ? WD_RED : WD_CYAN);
    tft.fillRoundRect(168, 104, 62, 13, 6, pill);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, pill);
    tft.drawString(scanning ? "SCANNING" :
                   (running ? "FLOODING" : "READY"), 199, 111, 1);
    tft.drawFastHLine(6, 118, 228, WD_EDGE);

    if (!count) {
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(WD_BONE, WD_PANEL);
      tft.drawString(scanning
          ? (def.station_targets ? "SCANNING APS + STAS" : "SCANNING CHANNELS")
          : (def.station_targets ? "NO STATIONS FOUND"
                                 : "NO NETWORKS FOUND"), 120, 172, 2);
      tft.setTextColor(WD_DIM, WD_PANEL);
      tft.drawString(scanning ? "TARGETS APPEAR HERE"
                              : "TAP SCAN TO FIND NETWORKS", 120, 196, 1);
    } else {
      const uint16_t start = this->attack_dash_page * 5;
      for (uint8_t row = 0; row < 5; row++) {
        const uint16_t index = start + row;
        const int16_t y = 123 + row * 24;
        if (index >= count) {
          tft.drawFastHLine(9, y + 21, 222, WD_EDGE);
          continue;
        }
        String line1;
        String line2;
        bool is_sel;
        if (def.station_targets) {
          Station sta = stations->get(index);
          is_sel = sta.selected;
          line1 = macToString(sta.mac);
          line2 = "STA";
          if (access_points && sta.ap < access_points->size())
            line2 = "CH" + String(access_points->get(sta.ap).channel);
        } else {
          AccessPoint item = access_points->get(index);
          is_sel = item.selected;
          line1 = item.essid;
          line1.replace("\r", " ");
          line1.replace("\n", " ");
          line1.replace("\t", " ");
          if (line1.length() > 15) line1 = line1.substring(0, 14) + "~";
          if (!line1.length()) line1 = "<HIDDEN>";
          line2 = "CH" + String(item.channel);
        }
        if (is_sel) {
          tft.fillRoundRect(8, y, 224, 22, 3, WD_SURFACE);
          tft.fillRect(8, y, 3, 22, WD_RED);
        }
        const uint16_t row_bg = is_sel ? WD_SURFACE : WD_PANEL;
        tft.drawRect(14, y + 4, 13, 13, is_sel ? WD_RED : WD_EDGE);
        if (is_sel) tft.fillRect(17, y + 7, 7, 7, WD_RED);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(is_sel ? WD_RED : WD_BONE, row_bg);
        tft.drawString(line1, 34, y + 3, 1);
        tft.setTextColor(WD_DIM, row_bg);
        tft.drawString(line2, 34, y + 12, 1);
        if (!def.station_targets) {
          // Signal bars from rssi on the right edge (AP rows only).
          const int8_t r = access_points->get(index).rssi;
          const uint8_t bars = r >= -55 ? 4 : r >= -67 ? 3 : r >= -78 ? 2 : 1;
          for (uint8_t b = 0; b < 4; b++) {
            const int16_t bx = 204 + b * 7;
            const int16_t bh = 4 + b * 3;
            const uint16_t bc = b < bars ? (is_sel ? WD_RED : WD_CYAN)
                                         : WD_EDGE;
            tft.fillRect(bx, y + 15 - bh, 5, bh, bc);
          }
        }
        if (!is_sel) tft.drawFastHLine(9, y + 21, 222, WD_EDGE);
      }
      tft.setTextDatum(TR_DATUM);
      tft.setTextColor(WD_DIM, WD_PANEL);
      tft.drawString("PAGE " + String(this->attack_dash_page + 1) + "/" +
                     String(pages), 231, 240, 1);
    }
  }

  if (reset || state_dirty ||
      this->attack_dash_draw_count != count ||
      this->attack_dash_draw_selected != selected) {
    const char* labels[4] = {
      running ? "STOP" : "START", "<", ">", "EXIT"
    };
    const int16_t x[4] = {2, 72, 126, 180};
    const int16_t width[4] = {66, 50, 50, 58};
    for (uint8_t i = 0; i < 4; i++) {
      uint16_t fill = WD_SURFACE;
      if (i == 0) fill = (!selected || scanning) ? WD_SURFACE
                       : (running ? WD_RED : WD_CYAN);
      if (i == 3) fill = WD_RED;
      const bool solid = (i == 0 && selected && !scanning) || i == 3;
      tft.fillRoundRect(x[i], 260, width[i], 56, 4, fill);
      tft.drawRoundRect(x[i], 260, width[i], 56, 4, solid ? fill : WD_EDGE);
      wdCornerTicks(tft, x[i], 260, width[i], 56, 4, WD_RED);
      tft.setTextDatum(MC_DATUM);
      const bool dark_text = solid;
      tft.setTextColor(dark_text ? TFT_BLACK :
                       (i == 0 && (!selected || scanning) ? WD_DIM : WD_BONE), fill);
      const uint8_t font = (i == 1 || i == 2) ? 2 : 1;
      tft.drawString(labels[i], x[i] + width[i] / 2, 288, font);
    }
  }

  this->attack_dash_draw_valid = true;
  this->attack_dash_draw_running = running;
  this->attack_dash_draw_scanning = scanning;
  this->attack_dash_draw_rate = wifi_scan_obj.attack_dash_rate;
  this->attack_dash_draw_total = wifi_scan_obj.attack_dash_total;
  this->attack_dash_draw_count = count;
  this->attack_dash_draw_selected = selected;
  this->attack_dash_draw_page = this->attack_dash_page;
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::handleAttackDashTouch(uint16_t touch_x,
                                          uint16_t touch_y) {
  extern LinkedList<AccessPoint>* access_points;
  extern LinkedList<Station>* stations;
  const AttackDashDef& def = attackDashDefs[this->attack_dash_index];
  const uint16_t count = def.station_targets
      ? (stations ? stations->size() : 0)
      : (access_points ? access_points->size() : 0);
  const uint16_t pages = max((uint16_t)1, (uint16_t)((count + 4) / 5));
  const bool scanning = wifi_scan_obj.attack_dash_scan_active;

  // Only the lower EXIT button leaves the dashboard now.
  if (touch_x >= 176 && touch_y >= 255) {
    this->exitAttackDashUI();
    return;
  }

  // In-dashboard discovery chip in the list header band.
  if (touch_x >= 88 && touch_x < 168 && touch_y >= 100 && touch_y < 120) {
    this->toggleAttackDashScan(def.scan_mode, def.station_targets);
    this->attack_dash_draw_valid = false;
    this->drawAttackDashUI(true);
    return;
  }

  // While scanning, the list is being rebuilt: keep only SCAN and EXIT live.
  if (scanning) return;

  // List body: tap a row to toggle it as a target directly.
  if (touch_y >= 120 && touch_y < 243 && count) {
    const int16_t row = (touch_y - 123) / 24;
    if (row >= 0 && row < 5) {
      const uint16_t index = this->attack_dash_page * 5 + row;
      if (index < count) {
        if (def.station_targets) {
          Station sta = stations->get(index);
          sta.selected = !sta.selected;
          stations->set(index, sta);
          // Targeted deauth needs the parent AP selected too; linking it on
          // select costs one extra flag and never blocks an untoggle.
          if (sta.selected &&
              def.scan_mode == WIFI_ATTACK_DEAUTH_TARGETED &&
              access_points &&
              sta.ap < access_points->size()) {
            AccessPoint ap = access_points->get(sta.ap);
            if (!ap.selected) {
              ap.selected = true;
              access_points->set(sta.ap, ap);
            }
          }
        } else {
          AccessPoint ap = access_points->get(index);
          ap.selected = !ap.selected;
          access_points->set(index, ap);
        }
        this->attack_dash_draw_valid = false;
        this->drawAttackDashUI(true);
      }
    }
    return;
  }

  if (touch_y < 255) return;

  if (touch_x < 70) {
    uint16_t selected = 0;
    for (uint16_t i = 0; i < count; i++)
      if (def.station_targets ? stations->get(i).selected
                              : access_points->get(i).selected) selected++;
    if (selected)
      wifi_scan_obj.setAttackDashRunning(!wifi_scan_obj.attack_dash_running);
  } else if (touch_x < 124) {
    this->attack_dash_page = this->attack_dash_page == 0 ?
        pages - 1 : this->attack_dash_page - 1;
  } else if (touch_x < 178) {
    this->attack_dash_page = (this->attack_dash_page + 1) % pages;
  } else {
    this->exitAttackDashUI();
    return;
  }
  this->attack_dash_draw_valid = false;
  this->drawAttackDashUI(true);
}

void MenuFunctions::exitAttackDashUI() {
  TFT_eSPI& tft = display_obj.tft;
  tft.fillRoundRect(34, 132, 172, 56, 5, WD_RED);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, WD_RED);
  tft.drawString("STOPPING...", 120, 160, 2);
  this->attack_dash_ui = false;
  this->attack_dash_draw_valid = false;
  this->attack_dash_touch_active = false;
  this->attack_dash_touch_region = -1;
  wifi_scan_obj.attack_dash_running = false;
  wifi_scan_obj.attack_dash_ui_owned = false;
  if (wifi_scan_obj.attack_dash_scan_active) {
    wifi_scan_obj.attack_dash_scan_active = false;
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  } else if (wifi_scan_obj.isAttackDashMode(wifi_scan_obj.currentScanMode)) {
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  }
  display_obj.init();
  this->changeMenu(&wifiAttackMenu, true);
}

// --- Funny SSID Beacon dashboard ---------------------------------------

void MenuFunctions::startFunnyBeaconUI() {
  // R111 guardrail: transmit apps can be PIN-locked (AttackPin setting).
  if (!this->attackPinOk()) {
    this->attackPinDenied();
    return;
  }
  this->wifi_passive_ui = false;
  this->wifi_tool_ui = false;
  this->network_scan_ui = false;
  this->beacon_list_ui = false;
  this->beacon_spam_ui = false;
  this->rick_roll_ui = false;
  this->probe_flood_ui = false;
  this->deauth_flood_ui = false;
  this->attack_dash_ui = false;
  this->funny_beacon_ui = true;
  this->funny_beacon_page = 0;
  this->funny_beacon_draw_valid = false;
  this->funny_beacon_touch_active = false;
  this->funny_beacon_touch_region = -1;
  this->funny_beacon_touch_ms = 0;
  this->funny_beacon_touch_ready_at = millis() + 320;

  wifi_scan_obj.wifi_tool_ui_owned = false;
  wifi_scan_obj.network_scan_ui_owned = false;
  wifi_scan_obj.beacon_list_ui_owned = false;
  wifi_scan_obj.beacon_spam_ui_owned = false;
  wifi_scan_obj.rick_roll_ui_owned = false;
  wifi_scan_obj.probe_flood_ui_owned = false;
  wifi_scan_obj.deauth_flood_ui_owned = false;
  wifi_scan_obj.attack_dash_ui_owned = false;
  wifi_scan_obj.funny_beacon_ui_owned = true;
  wifi_scan_obj.clearFunnyBeaconStats();

  display_obj.tft.fillScreen(TFT_BLACK);
  this->drawSharkTopBar(true);
  wifi_scan_obj.StartScan(WIFI_ATTACK_FUNNY_BEACON, TFT_CYAN);
  // Opening the dashboard never transmits. START is a deliberate touch.
  wifi_scan_obj.setFunnyBeaconRunning(false);
  this->drawFunnyBeaconUI(true);
}

void MenuFunctions::drawFunnyBeaconUI(bool full_redraw) {
  if (!this->funny_beacon_ui) return;
  TFT_eSPI& tft = display_obj.tft;
  const uint8_t count = wifi_scan_obj.funnyBeaconCount();
  const uint8_t pages = max((uint8_t)1, (uint8_t)((count + 4) / 5));
  if (this->funny_beacon_page >= pages)
    this->funny_beacon_page = pages - 1;
  const bool running = wifi_scan_obj.funny_beacon_running;
  const bool reset = full_redraw || !this->funny_beacon_draw_valid;
  const bool state_dirty = reset ||
      this->funny_beacon_draw_running != running ||
      this->funny_beacon_draw_channel != wifi_scan_obj.set_channel;
  const bool stats_dirty = reset || state_dirty ||
      this->funny_beacon_draw_rate != wifi_scan_obj.funny_beacon_rate ||
      this->funny_beacon_draw_total != wifi_scan_obj.funny_beacon_total;
  const bool list_dirty = reset ||
      this->funny_beacon_draw_page != this->funny_beacon_page;

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  tft.setTextDatum(TL_DATUM);

  if (reset) {
    tft.fillScreen(TFT_BLACK);
    this->drawSharkTopBar(true);
    tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, 25, TFT_BLACK);
    tft.drawXBitmap(5, STATUS_BAR_WIDTH + 1,
                    menu_icons[FUNNY_BEACON_ATTACK_ICON],
                    ICON_W, ICON_H, TFT_BLACK, WD_RED);
    tft.setTextColor(WD_RED, TFT_BLACK);
    tft.drawString("// FUNNY BEACON", 31, STATUS_BAR_WIDTH + 7, 2);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 88, WD_RED);
    tft.fillRoundRect(177, STATUS_BAR_WIDTH + 3, 59, 19, 3, WD_RED);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, WD_RED);
    tft.drawString("BACK", 206, STATUS_BAR_WIDTH + 12, 1);

    const char* labels[3] = {"SSIDS", "RATE/S", "TOTAL"};
    for (uint8_t i = 0; i < 3; i++) {
      const int16_t x = 2 + i * 80;
      wdPanel(tft, x, 51, 76, 42, WD_SURFACE, WD_EDGE, WD_RED);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_DIM, WD_SURFACE);
      tft.drawString(labels[i], x + 7, 56, 1);
    }
    wdPanel(tft, 2, 99, 236, 156, WD_PANEL, WD_EDGE, WD_RED);
  }

  if (stats_dirty) {
    const String values[3] = {
      String(count), String(wifi_scan_obj.funny_beacon_rate),
      String(wifi_scan_obj.funny_beacon_total)
    };
    for (uint8_t i = 0; i < 3; i++) {
      const int16_t x = 2 + i * 80;
      tft.fillRect(x + 5, 70, 66, 19, WD_SURFACE);
      tft.setTextDatum(MR_DATUM);
      tft.setTextColor(i == 1 && running ? WD_RED : WD_BONE, WD_SURFACE);
      tft.drawString(values[i], x + 69, 80, 2);
    }
  }

  if (list_dirty || state_dirty) {
    tft.fillRect(6, 103, 228, 146, WD_PANEL);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_PANEL);
    tft.drawString("FUNNY SSID SET", 9, 105, 1);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(running ? WD_RED : WD_CYAN, WD_PANEL);
    String state = "READY";
    if (running)
      state = String("CH") + String(wifi_scan_obj.set_channel) + " LIVE";
    tft.drawString(state, 231, 105, 1);

    const uint8_t start = this->funny_beacon_page * 5;
    for (uint8_t row = 0; row < 5; row++) {
      const uint8_t index = start + row;
      const int16_t y = 123 + row * 24;
      tft.drawFastHLine(9, y + 21, 222, WD_EDGE);
      if (index >= count) continue;
      String shown = wifi_scan_obj.funnyBeaconName(index);
      if (shown.length() > 27) shown = shown.substring(0, 26) + "~";
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_RED, WD_PANEL);
      String number = index < 9 ? String("0") + String(index + 1) :
                                 String(index + 1);
      tft.drawString(number, 10, y + 5, 1);
      tft.setTextColor(WD_BONE, WD_PANEL);
      tft.drawString(shown, 30, y + 5, 1);
    }
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(WD_DIM, WD_PANEL);
    tft.drawString(String(this->funny_beacon_page + 1) + "/" +
                   String(pages), 231, 238, 1);
  }

  if (reset || state_dirty) {
    const char* labels[4] = {
      running ? "STOP" : "START", "PREV", "NEXT", "BACK"
    };
    const int16_t x[4] = {2, 72, 126, 180};
    const int16_t width[4] = {66, 50, 50, 58};
    for (uint8_t i = 0; i < 4; i++) {
      uint16_t fill = WD_SURFACE;
      if (i == 0) fill = running ? WD_RED : WD_CYAN;
      if (i == 3) fill = WD_RED;
      wdPanel(tft, x[i], 260, width[i], 56, fill,
              (i == 0 || i == 3) ? fill : WD_EDGE, WD_RED);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor((i == 0 || i == 3) ? TFT_BLACK : WD_BONE, fill);
      tft.drawString(labels[i], x[i] + width[i] / 2, 288, 1);
    }
  }

  this->funny_beacon_draw_valid = true;
  this->funny_beacon_draw_running = running;
  this->funny_beacon_draw_rate = wifi_scan_obj.funny_beacon_rate;
  this->funny_beacon_draw_total = wifi_scan_obj.funny_beacon_total;
  this->funny_beacon_draw_channel = wifi_scan_obj.set_channel;
  this->funny_beacon_draw_page = this->funny_beacon_page;
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::handleFunnyBeaconTouch(uint16_t touch_x,
                                           uint16_t touch_y) {
  if ((touch_x >= 176 && touch_y >= 238) ||
      (touch_y >= STATUS_BAR_WIDTH && touch_y < 51)) {
    this->exitFunnyBeaconUI();
    return;
  }
  if (touch_y < 238) return;
  const uint8_t count = wifi_scan_obj.funnyBeaconCount();
  const uint8_t pages = max((uint8_t)1, (uint8_t)((count + 4) / 5));
  if (touch_x < 70) {
    wifi_scan_obj.setFunnyBeaconRunning(!wifi_scan_obj.funny_beacon_running);
  } else if (touch_x < 124) {
    this->funny_beacon_page = this->funny_beacon_page == 0 ?
        pages - 1 : this->funny_beacon_page - 1;
  } else if (touch_x < 178) {
    this->funny_beacon_page = (this->funny_beacon_page + 1) % pages;
  } else {
    this->exitFunnyBeaconUI();
    return;
  }
  this->funny_beacon_draw_valid = false;
  this->drawFunnyBeaconUI(true);
}

void MenuFunctions::exitFunnyBeaconUI() {
  TFT_eSPI& tft = display_obj.tft;
  tft.fillRoundRect(34, 132, 172, 56, 5, WD_RED);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, WD_RED);
  tft.drawString("STOPPING...", 120, 160, 2);
  this->funny_beacon_ui = false;
  this->funny_beacon_draw_valid = false;
  this->funny_beacon_touch_active = false;
  this->funny_beacon_touch_region = -1;
  wifi_scan_obj.funny_beacon_running = false;
  wifi_scan_obj.funny_beacon_ui_owned = false;
  if (wifi_scan_obj.currentScanMode == WIFI_ATTACK_FUNNY_BEACON)
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  display_obj.init();
  this->changeMenu(&wifiAttackMenu, true);
}

// --- Rick Roll Beacon dashboard ---------------------------------------

void MenuFunctions::startRickRollUI() {
  // R111 guardrail: transmit apps can be PIN-locked (AttackPin setting).
  if (!this->attackPinOk()) {
    this->attackPinDenied();
    return;
  }
  this->wifi_passive_ui = false;
  this->wifi_tool_ui = false;
  this->network_scan_ui = false;
  this->beacon_list_ui = false;
  this->beacon_spam_ui = false;
  this->funny_beacon_ui = false;
  this->probe_flood_ui = false;
  this->deauth_flood_ui = false;
  this->attack_dash_ui = false;
  this->rick_roll_ui = true;
  this->rick_roll_page = 0;
  this->rick_roll_draw_valid = false;
  this->rick_roll_touch_active = false;
  this->rick_roll_touch_region = -1;
  this->rick_roll_touch_ms = 0;
  this->rick_roll_touch_ready_at = millis() + 320;

  wifi_scan_obj.wifi_tool_ui_owned = false;
  wifi_scan_obj.network_scan_ui_owned = false;
  wifi_scan_obj.beacon_list_ui_owned = false;
  wifi_scan_obj.beacon_spam_ui_owned = false;
  wifi_scan_obj.funny_beacon_ui_owned = false;
  wifi_scan_obj.probe_flood_ui_owned = false;
  wifi_scan_obj.deauth_flood_ui_owned = false;
  wifi_scan_obj.attack_dash_ui_owned = false;
  wifi_scan_obj.rick_roll_ui_owned = true;
  wifi_scan_obj.clearRickRollStats();

  display_obj.tft.fillScreen(TFT_BLACK);
  this->drawSharkTopBar(true);
  wifi_scan_obj.StartScan(WIFI_ATTACK_RICK_ROLL, TFT_YELLOW);
  // Opening the dashboard never transmits. START is a deliberate touch.
  wifi_scan_obj.setRickRollRunning(false);
  this->drawRickRollUI(true);
}

void MenuFunctions::drawRickRollUI(bool full_redraw) {
  if (!this->rick_roll_ui) return;
  TFT_eSPI& tft = display_obj.tft;
  const uint8_t count = wifi_scan_obj.rickRollCount();
  const uint8_t pages = max((uint8_t)1, (uint8_t)((count + 4) / 5));
  if (this->rick_roll_page >= pages)
    this->rick_roll_page = pages - 1;
  const bool running = wifi_scan_obj.rick_roll_running;
  const bool reset = full_redraw || !this->rick_roll_draw_valid;
  const bool state_dirty = reset ||
      this->rick_roll_draw_running != running ||
      this->rick_roll_draw_channel != wifi_scan_obj.set_channel;
  const bool stats_dirty = reset || state_dirty ||
      this->rick_roll_draw_rate != wifi_scan_obj.rick_roll_rate ||
      this->rick_roll_draw_total != wifi_scan_obj.rick_roll_total;
  const bool list_dirty = reset ||
      this->rick_roll_draw_page != this->rick_roll_page;

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  tft.setTextDatum(TL_DATUM);

  if (reset) {
    tft.fillScreen(TFT_BLACK);
    this->drawSharkTopBar(true);
    tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, 25, TFT_BLACK);
    tft.drawXBitmap(5, STATUS_BAR_WIDTH + 1,
                    menu_icons[RICK_ROLL_BEACON_ICON],
                    ICON_W, ICON_H, TFT_BLACK, WD_AMBER);
    tft.setTextColor(WD_AMBER, TFT_BLACK);
    tft.drawString("// RICK ROLL", 31, STATUS_BAR_WIDTH + 7, 2);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 88, WD_AMBER);
    tft.fillRoundRect(177, STATUS_BAR_WIDTH + 3, 59, 19, 3, WD_RED);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, WD_RED);
    tft.drawString("BACK", 206, STATUS_BAR_WIDTH + 12, 1);

    const char* labels[3] = {"LINES", "RATE/S", "TOTAL"};
    for (uint8_t i = 0; i < 3; i++) {
      const int16_t x = 2 + i * 80;
      wdPanel(tft, x, 51, 76, 42, WD_SURFACE, WD_EDGE, WD_AMBER);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_DIM, WD_SURFACE);
      tft.drawString(labels[i], x + 7, 56, 1);
    }
    wdPanel(tft, 2, 99, 236, 156, WD_PANEL, WD_EDGE, WD_AMBER);
  }

  if (stats_dirty) {
    const String values[3] = {
      String(count), String(wifi_scan_obj.rick_roll_rate),
      String(wifi_scan_obj.rick_roll_total)
    };
    for (uint8_t i = 0; i < 3; i++) {
      const int16_t x = 2 + i * 80;
      tft.fillRect(x + 5, 70, 66, 19, WD_SURFACE);
      tft.setTextDatum(MR_DATUM);
      tft.setTextColor(i == 1 && running ? WD_AMBER : WD_BONE, WD_SURFACE);
      tft.drawString(values[i], x + 69, 80, 2);
    }
  }

  if (list_dirty || state_dirty) {
    tft.fillRect(6, 103, 228, 146, WD_PANEL);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_PANEL);
    tft.drawString("SSID TRACK", 9, 105, 1);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(running ? WD_AMBER : WD_CYAN, WD_PANEL);
    String state = "READY";
    if (running)
      state = String("CH") + String(wifi_scan_obj.set_channel) + " LIVE";
    tft.drawString(state, 231, 105, 1);

    const uint8_t start = this->rick_roll_page * 5;
    for (uint8_t row = 0; row < 5; row++) {
      const uint8_t index = start + row;
      const int16_t y = 123 + row * 24;
      tft.drawFastHLine(9, y + 21, 222, WD_EDGE);
      if (index >= count) continue;
      String shown = wifi_scan_obj.rickRollName(index);
      if (shown.length() > 32) shown = shown.substring(0, 31) + "~";
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_AMBER, WD_PANEL);
      String number = index < 9 ? String("0") + String(index + 1) :
                                 String(index + 1);
      tft.drawString(number, 10, y + 5, 1);
      tft.setTextColor(WD_BONE, WD_PANEL);
      tft.drawString(shown, 30, y + 5, 1);
    }
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(WD_DIM, WD_PANEL);
    tft.drawString(String(this->rick_roll_page + 1) + "/" +
                   String(pages), 231, 238, 1);
  }

  if (reset || state_dirty) {
    const char* labels[4] = {
      running ? "STOP" : "START", "PREV", "NEXT", "BACK"
    };
    const int16_t x[4] = {2, 72, 126, 180};
    const int16_t width[4] = {66, 50, 50, 58};
    for (uint8_t i = 0; i < 4; i++) {
      uint16_t fill = WD_SURFACE;
      if (i == 0) fill = running ? WD_AMBER : WD_CYAN;
      if (i == 3) fill = WD_RED;
      const uint16_t accent = i == 3 ? WD_RED : WD_AMBER;
      wdPanel(tft, x[i], 260, width[i], 56, fill,
              (i == 0 || i == 3) ? fill : WD_EDGE, accent);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor((i == 0 || i == 3) ? TFT_BLACK : WD_BONE, fill);
      tft.drawString(labels[i], x[i] + width[i] / 2, 288, 1);
    }
  }

  this->rick_roll_draw_valid = true;
  this->rick_roll_draw_running = running;
  this->rick_roll_draw_rate = wifi_scan_obj.rick_roll_rate;
  this->rick_roll_draw_total = wifi_scan_obj.rick_roll_total;
  this->rick_roll_draw_channel = wifi_scan_obj.set_channel;
  this->rick_roll_draw_page = this->rick_roll_page;
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::handleRickRollTouch(uint16_t touch_x,
                                        uint16_t touch_y) {
  if ((touch_x >= 176 && touch_y >= 238) ||
      (touch_y >= STATUS_BAR_WIDTH && touch_y < 51)) {
    this->exitRickRollUI();
    return;
  }
  if (touch_y < 238) return;
  const uint8_t count = wifi_scan_obj.rickRollCount();
  const uint8_t pages = max((uint8_t)1, (uint8_t)((count + 4) / 5));
  if (touch_x < 70) {
    wifi_scan_obj.setRickRollRunning(!wifi_scan_obj.rick_roll_running);
  } else if (touch_x < 124) {
    this->rick_roll_page = this->rick_roll_page == 0 ?
        pages - 1 : this->rick_roll_page - 1;
  } else if (touch_x < 178) {
    this->rick_roll_page = (this->rick_roll_page + 1) % pages;
  } else {
    this->exitRickRollUI();
    return;
  }
  this->rick_roll_draw_valid = false;
  this->drawRickRollUI(true);
}

void MenuFunctions::exitRickRollUI() {
  TFT_eSPI& tft = display_obj.tft;
  tft.fillRoundRect(34, 132, 172, 56, 5, WD_RED);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, WD_RED);
  tft.drawString("STOPPING...", 120, 160, 2);
  this->rick_roll_ui = false;
  this->rick_roll_draw_valid = false;
  this->rick_roll_touch_active = false;
  this->rick_roll_touch_region = -1;
  wifi_scan_obj.rick_roll_running = false;
  wifi_scan_obj.rick_roll_ui_owned = false;
  if (wifi_scan_obj.currentScanMode == WIFI_ATTACK_RICK_ROLL)
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  display_obj.init();
  this->changeMenu(&wifiAttackMenu, true);
}

// --- Wi-Fi passive detector dashboard ---------------------------------------
// One-for-one with drawPassiveBleDetectorUI so the Wi-Fi sniffers share the
// FindMy/Flock/Meta look: state pill, three stat panels, last-match card, RSSI
// bar, activity graph and the START/STOP/CLEAR/DATA/BACK key row.

void MenuFunctions::drawPassiveWifiDetectorUI(bool full_redraw) {
  TFT_eSPI& tft = display_obj.tft;
  const uint8_t mode = wifi_scan_obj.currentScanMode;
  const bool running = wifi_scan_obj.wifi_passive_running;
  const uint16_t rate = wifi_scan_obj.wifi_passive_rate;
  const uint16_t unique = wifi_scan_obj.wifi_passive_unique;
  const uint32_t total = wifi_scan_obj.wifi_passive_total;
  int16_t last_rssi = -128;
  String last_name;
  String last_mac;
  wifi_scan_obj.snapshotPassiveWifiUi(last_name, last_mac, last_rssi);
  this->wifi_passive_data_view = false;
  const char* title = mode == WIFI_SCAN_PROBE ? "// PROBE SNIFF" :
                      mode == WIFI_SCAN_DEAUTH ? "// DEAUTH SNIFF" : "// BEACON SNIFF";
  const char* entity = mode == WIFI_SCAN_PROBE ? "CLIENTS" :
                       mode == WIFI_SCAN_DEAUTH ? "SOURCES" : "APS";
  const char* waiting = mode == WIFI_SCAN_PROBE ? "LISTENING PROBE REQUESTS" :
                        mode == WIFI_SCAN_DEAUTH ? "LISTENING DEAUTH FRAMES" :
                        "LISTENING BEACON FRAMES";

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  tft.setTextDatum(TL_DATUM);

  // The old dashboard repainted every panel and blanked the complete graph every
  // 100 ms. On the physical SPI TFT that exposed each clear before replacement
  // pixels arrived, producing the visible blink. Live updates are now dirty-only.
  const bool reset = full_redraw || !this->wifi_passive_draw_valid ||
                     this->wifi_passive_draw_mode != mode;
  if (reset) {
    tft.fillScreen(TFT_BLACK);
    this->drawSharkTopBar(true);
    tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, 25, TFT_BLACK);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString(title, 6, STATUS_BAR_WIDTH + 7, 2);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 60, WD_CYAN);
    this->wifi_passive_draw_valid = false;
    this->wifi_passive_graph_valid = false;
    this->wifi_passive_strip_on = false;
  }

  // Cyber Defense overlays (Deauth Alarm / Evil Twin) share the title strip:
  // the dashboard owns them, so they never fight the legacy scan banners.
  // The alarm wins when both are active; the title redraws once it clears.
  {
    const bool alarm_trip = wifi_scan_obj.deauth_alarm &&
                            wifi_scan_obj.deauth_alarm_rate >= 5;
    const bool twin_trip = !alarm_trip && wifi_scan_obj.evil_twin &&
                           wifi_scan_obj.evil_twin_count > 0;
    if (alarm_trip) {
      if (millis() - this->wifi_passive_alarm_ms >= 500) {
        this->wifi_passive_alarm_ms = millis();
        this->wifi_passive_alarm_flash = !this->wifi_passive_alarm_flash;
      }
      const uint16_t bg = this->wifi_passive_alarm_flash ? TFT_RED : TFT_BLACK;
      const uint16_t fg = this->wifi_passive_alarm_flash ? TFT_BLACK : TFT_RED;
      tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, 25, bg);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(fg, bg);
      tft.drawString("! DEAUTH FLOOD " + String(wifi_scan_obj.deauth_alarm_rate) + "/s",
                     SCREEN_WIDTH / 2, STATUS_BAR_WIDTH + 12, 2);
      tft.setTextDatum(TL_DATUM);
      this->wifi_passive_strip_on = true;
    } else if (twin_trip) {
      tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, 25, TFT_YELLOW);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(TFT_BLACK, TFT_YELLOW);
      tft.drawString("! EVIL TWIN x" + String(wifi_scan_obj.evil_twin_count),
                     SCREEN_WIDTH / 2, STATUS_BAR_WIDTH + 12, 2);
      tft.setTextDatum(TL_DATUM);
      this->wifi_passive_strip_on = true;
    } else if (this->wifi_passive_strip_on) {
      tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, 25, TFT_BLACK);
      tft.setTextColor(WD_CYAN, TFT_BLACK);
      tft.drawString(title, 6, STATUS_BAR_WIDTH + 7, 2);
      tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
      tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 60, WD_CYAN);
      this->wifi_passive_strip_on = false;
    }
  }

  const bool state_dirty = !this->wifi_passive_draw_valid ||
                           this->wifi_passive_draw_running != running;
  const bool rate_dirty = !this->wifi_passive_draw_valid ||
                          this->wifi_passive_draw_rate != rate;
  const bool unique_dirty = !this->wifi_passive_draw_valid ||
                            this->wifi_passive_draw_unique != unique;
  const bool total_dirty = !this->wifi_passive_draw_valid ||
                           this->wifi_passive_draw_total != total;
  const bool detection_dirty = !this->wifi_passive_draw_valid || state_dirty ||
                               unique_dirty ||
                               this->wifi_passive_draw_last_rssi != last_rssi ||
                               this->wifi_passive_draw_last_name != last_name ||
                               this->wifi_passive_draw_last_mac != last_mac;
  const bool signal_dirty = !this->wifi_passive_draw_valid ||
                            this->wifi_passive_draw_last_rssi != last_rssi;

  const uint16_t state_color = running ? WD_CYAN : WD_AMBER;
  if (state_dirty) {
    tft.fillRoundRect(176, STATUS_BAR_WIDTH + 4, 59, 17, 3, state_color);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, state_color);
    tft.drawString(running ? "SCAN" : "PAUSED", 205, STATUS_BAR_WIDTH + 12, 1);
  }

  const char* labels[3] = {"HITS/S", entity, "TOTAL"};
  const bool stat_dirty[3] = {rate_dirty, unique_dirty, total_dirty};
  const uint32_t stat_values[3] = {rate, unique, total};
  for (uint8_t i = 0; i < 3; i++) {
    if (!stat_dirty[i]) continue;
    const int16_t x = 2 + i * 80;
    wdPanel(tft, x, 49, 76, 42, WD_SURFACE, WD_EDGE,
            i == 1 && unique ? WD_AMBER : state_color);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_SURFACE);
    tft.drawString(labels[i], x + 7, 54, 1);
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(i == 1 && unique ? WD_AMBER : WD_BONE, WD_SURFACE);
    tft.drawString(String(stat_values[i]), x + 69, 76, 2);
  }

  if (detection_dirty) {
    wdPanel(tft, 2, 95, 236, 55, WD_PANEL, WD_EDGE,
            unique ? WD_AMBER : WD_CYAN);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_PANEL);
    tft.drawString("LAST DETECTION", 8, 101, 1);
    String name = last_name;
    if (!name.length()) name = running ? waiting : "NO DETECTION";
    while (name.length() > 3 && tft.textWidth(name, 2) > 220)
      name.remove(name.length() - 1);
    tft.setTextColor(unique ? WD_AMBER : WD_BONE, WD_PANEL);
    tft.drawString(name, 8, 114, 2);
    String detail = last_mac;
    if (detail.length()) detail += "  " + String(last_rssi) + "dBm";
    else detail = "PASSIVE FRAME CAPTURE";
    while (detail.length() > 3 && tft.textWidth(detail, 1) > 224)
      detail.remove(detail.length() - 1);
    tft.setTextColor(WD_CYAN, WD_PANEL);
    tft.drawString(detail, 8, 137, 1);
  }

  if (signal_dirty) {
    const bool has_signal = last_rssi > -128;
    const int16_t clamped = constrain(last_rssi, -100, -35);
    const int16_t signal_w =
      has_signal ? (int16_t)((int32_t)(clamped + 100) * 224 / 65) : 0;
    const uint16_t signal_color = clamped > -65 ? WD_AMBER : WD_CYAN;
    tft.fillRoundRect(6, 156, 228, 17, 3, WD_SURFACE);
    tft.drawRoundRect(6, 156, 228, 17, 3, WD_EDGE);
    if (signal_w > 0)
      tft.fillRoundRect(8, 158, signal_w, 13, 2, signal_color);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(has_signal && signal_w > 114 ? TFT_BLACK :
                     (has_signal ? WD_BONE : WD_DIM),
                     has_signal && signal_w > 114 ? signal_color : WD_SURFACE);
    tft.drawString(has_signal ? "LAST RSSI" : "NO SIGNAL YET", 120, 164, 1);
  }

  const int16_t px = 12, py = 198, pw = 216, ph = 59;
  if (!this->wifi_passive_draw_valid || state_dirty) {
    wdPanel(tft, 6, 178, 228, 86, WD_SURFACE, WD_EDGE, state_color);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, WD_SURFACE);
    tft.drawString("CAPTURE ACTIVITY", 14, 183, 1);
    tft.fillRect(px, py, pw, ph, TFT_BLACK);
    this->wifi_passive_graph_valid = false;
  }

  // Replace only changed graph columns. The graph is never globally blanked
  // during live capture, so a refresh cannot expose a black intermediate frame.
  int16_t graph_max = 1;
  for (int16_t i = 0; i < pw; i++)
    if (wifi_scan_obj._analyzer_values[i] > graph_max)
      graph_max = wifi_scan_obj._analyzer_values[i];
  for (int16_t i = 0; i < pw; i++) {
    const int16_t sample = max((int16_t)0, wifi_scan_obj._analyzer_values[i]);
    const uint8_t bar = sample ?
      (uint8_t)max((int16_t)1,
                   (int16_t)((int32_t)sample * (ph - 2) / graph_max)) : 0;
    if (this->wifi_passive_graph_valid &&
        this->wifi_passive_graph_bars[i] == bar)
      continue;
    const int16_t x = px + pw - 1 - i;
    tft.drawFastVLine(x, py, ph, TFT_BLACK);
    for (uint8_t line = 1; line < 3; line++)
      tft.drawPixel(x, py + ph * line / 3, WD_PANEL);
    if (bar)
      tft.drawFastVLine(x, py + ph - bar, bar,
                        running ? WD_CYAN : WD_CYAN_DIM);
    this->wifi_passive_graph_bars[i] = bar;
  }
  this->wifi_passive_graph_valid = true;

  if (state_dirty) {
    static const char* const buttons[5] = {"START", "STOP", "CLEAR", "DATA", "BACK"};
    for (uint8_t i = 0; i < 5; i++) {
      const int16_t x = i * 48 + 2;
      const bool enabled = i == 0 ? !running : (i == 1 ? running : true);
      uint16_t fill = WD_SURFACE, accent = WD_CYAN, text = WD_BONE;
      if (i == 0) {
        fill = enabled ? WD_CYAN : WD_SURFACE;
        text = enabled ? TFT_BLACK : WD_DIM;
      } else if (i == 1) {
        fill = enabled ? WD_AMBER : WD_SURFACE;
        accent = WD_AMBER;
        text = enabled ? TFT_BLACK : WD_DIM;
      } else if (i == 2) {
        accent = WD_BONE;
      } else if (i == 3) {
        text = WD_CYAN;
      } else {
        accent = WD_AMBER;
        text = WD_AMBER;
      }
      wdPanel(tft, x, 274, 44, 42, fill,
              enabled ? accent : WD_EDGE, accent);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(text, fill);
      tft.drawString(buttons[i], x + 22, 295, 1);
    }
  }

  this->wifi_passive_draw_mode = mode;
  this->wifi_passive_draw_running = running;
  this->wifi_passive_draw_rate = rate;
  this->wifi_passive_draw_unique = unique;
  this->wifi_passive_draw_total = total;
  this->wifi_passive_draw_last_rssi = last_rssi;
  this->wifi_passive_draw_last_name = last_name;
  this->wifi_passive_draw_last_mac = last_mac;
  this->wifi_passive_draw_valid = true;
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::drawPassiveWifiDetectorDataUI(bool full_redraw) {
  extern LinkedList<PassiveWifiFinding>* passive_wifi_findings;
  TFT_eSPI& tft = display_obj.tft;
  const uint8_t rows_per_page = 5;
  const uint16_t total = passive_wifi_findings ? passive_wifi_findings->size() : 0;
  const uint16_t pages = max((uint16_t)1, (uint16_t)((total + rows_per_page - 1) / rows_per_page));
  if (this->wifi_passive_data_page >= pages) this->wifi_passive_data_page = pages - 1;
  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  if (full_redraw) { tft.fillScreen(TFT_BLACK); this->drawSharkTopBar(true); }
  tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_WIDTH, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WD_CYAN, TFT_BLACK);
  tft.drawString("// CAPTURE DATA", 6, STATUS_BAR_WIDTH + 7, 2);
  tft.fillRoundRect(174, STATUS_BAR_WIDTH + 4, 61, 17, 3, WD_AMBER);
  tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_BLACK, WD_AMBER);
  tft.drawString("SNAPSHOT", 204, STATUS_BAR_WIDTH + 12, 1);
  tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
  wdPanel(tft, 2, 51, 236, 21, WD_PANEL, WD_EDGE, total ? WD_AMBER : WD_CYAN);
  tft.setTextDatum(TL_DATUM); tft.setTextColor(WD_BONE, WD_PANEL);
  tft.drawString("DETECTIONS " + String(total), 8, 58, 1);
  tft.setTextDatum(TR_DATUM); tft.setTextColor(WD_DIM, WD_PANEL);
  tft.drawString("PAGE " + String(this->wifi_passive_data_page + 1) + "/" + String(pages), 232, 58, 1);
  if (!total) {
    wdPanel(tft, 12, 104, 216, 112, WD_SURFACE, WD_EDGE, WD_CYAN);
    tft.setTextDatum(MC_DATUM); tft.setTextColor(WD_CYAN, WD_SURFACE);
    tft.drawString("NO DETECTIONS", 120, 145, 2);
    tft.setTextColor(WD_DIM, WD_SURFACE);
    tft.drawString("DASH TO CONTINUE SCANNING", 120, 178, 1);
  } else {
    for (uint8_t row = 0; row < rows_per_page; row++) {
      const uint16_t offset = this->wifi_passive_data_page * rows_per_page + row;
      if (offset >= total) break;
      PassiveWifiFinding item = passive_wifi_findings->get((int32_t)total - 1 - offset);
      const int16_t y = 76 + row * 38;
      const uint16_t c = item.rssi >= -65 ? WD_AMBER : (item.rssi >= -85 ? WD_CYAN : WD_DIM);
      wdPanel(tft, 2, y, 236, 34, WD_SURFACE, WD_EDGE, c);
      String name = item.name.length() ? item.name : "DETECTION";
      while (name.length() > 3 && tft.textWidth(name, 1) > 145) name.remove(name.length() - 1);
      tft.setTextDatum(TL_DATUM); tft.setTextColor(WD_BONE, WD_SURFACE); tft.drawString(name, 9, y + 5, 1);
      tft.setTextDatum(TR_DATUM); tft.setTextColor(c, WD_SURFACE); tft.drawString(String(item.rssi) + " dBm", 231, y + 5, 1);
      String detail = item.mac + "  H" + String(item.hits);
      tft.setTextDatum(TL_DATUM); tft.setTextColor(WD_DIM, WD_SURFACE); tft.drawString(detail, 9, y + 20, 1);
    }
  }
  static const char* const labels[3] = {"PREV", "NEXT", "DASH"};
  const bool can_prev = this->wifi_passive_data_page > 0;
  const bool can_next = this->wifi_passive_data_page + 1 < pages;
  for (uint8_t i = 0; i < 3; i++) {
    const bool enabled = i == 0 ? can_prev : (i == 1 ? can_next : true);
    const int16_t x = i * 80 + 2;
    const uint16_t fill = i == 2 ? WD_CYAN : WD_SURFACE;
    wdPanel(tft, x, 274, 76, 42, fill, i == 2 ? WD_CYAN : (enabled ? WD_BONE : WD_EDGE), WD_CYAN);
    tft.setTextDatum(MC_DATUM); tft.setTextColor(i == 2 ? TFT_BLACK : (enabled ? WD_BONE : WD_DIM), fill);
    tft.drawString(labels[i], x + 38, 295, 1);
  }
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::handlePassiveWifiDetectorTouch(uint16_t touch_x, uint16_t touch_y) {
  if (touch_y < 268) return;
  if (this->wifi_passive_data_view) {
    extern LinkedList<PassiveWifiFinding>* passive_wifi_findings;
    const uint16_t total = passive_wifi_findings ? passive_wifi_findings->size() : 0;
    const uint16_t pages = max((uint16_t)1, (uint16_t)((total + 4) / 5));
    if (touch_x < 80) {
      if (this->wifi_passive_data_page) this->wifi_passive_data_page--;
      this->drawPassiveWifiDetectorDataUI(true);
    } else if (touch_x < 160) {
      if (this->wifi_passive_data_page + 1 < pages) this->wifi_passive_data_page++;
      this->drawPassiveWifiDetectorDataUI(true);
    } else {
      const bool resume = this->wifi_passive_resume_after_data;
      this->wifi_passive_data_view = false;
      this->wifi_passive_resume_after_data = false;
      if (resume) wifi_scan_obj.startPassiveWifiDetector();
      this->drawPassiveWifiDetectorUI(true);
    }
    return;
  }
  const uint8_t action = min((uint8_t)4, (uint8_t)(touch_x / 48));
  if (action == 0) wifi_scan_obj.startPassiveWifiDetector();
  else if (action == 1) wifi_scan_obj.stopPassiveWifiDetector();
  else if (action == 2) wifi_scan_obj.clearPassiveWifiDetector();
  else if (action == 3) {
    this->wifi_passive_resume_after_data = wifi_scan_obj.wifi_passive_running;
    if (wifi_scan_obj.wifi_passive_running) wifi_scan_obj.stopPassiveWifiDetector();
    this->wifi_passive_data_page = 0;
    this->wifi_passive_data_view = true;
    this->drawPassiveWifiDetectorDataUI(true);
    return;
  } else {
    this->wifi_passive_data_view = false;
    this->wifi_passive_resume_after_data = false;
    this->wifi_passive_ui = false;
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
    display_obj.init();
    this->changeMenu(current_menu, true);
    return;
  }
  this->drawPassiveWifiDetectorUI(true);
}

void MenuFunctions::drawFoxHuntUI(bool full_redraw) {
  extern LinkedList<BleDevice>* ble_devices;
  TFT_eSPI& tft = display_obj.tft;
  BleDevice target;
  bool found = false;
  for (int i = 0; ble_devices && i < ble_devices->size(); i++) {
    if (ble_devices->get(i).selected) { target = ble_devices->get(i); found = true; break; }
  }
  const int rssi = found ? target.rssi : -128;
  const char* proximity = rssi >= -50 ? "VERY CLOSE" : rssi >= -65 ? "CLOSE" :
                          rssi >= -80 ? "NEAR" : rssi > -128 ? "FAR" : "NO SIGNAL";
  const bool running = wifi_scan_obj.bt_fox_running;
  tft.setFreeFont(NULL); tft.setTextSize(1); tft.setTextWrap(false);
  if (full_redraw) { tft.fillScreen(TFT_BLACK); this->drawSharkTopBar(true); }
  tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_WIDTH, TFT_BLACK);
  tft.setTextDatum(TL_DATUM); tft.setTextColor(WD_CYAN, TFT_BLACK);
  tft.drawString("// FOX HUNT", 6, STATUS_BAR_WIDTH + 7, 2);
  tft.fillRoundRect(176, STATUS_BAR_WIDTH + 4, 59, 17, 3, running ? WD_CYAN : WD_AMBER);
  tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_BLACK, running ? WD_CYAN : WD_AMBER);
  tft.drawString(running ? "TRACK" : "PAUSED", 205, STATUS_BAR_WIDTH + 12, 1);
  tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
  wdPanel(tft, 6, 52, 228, 58, WD_PANEL, WD_EDGE, WD_CYAN);
  String name = found ? target.name : "NO TARGET SELECTED";
  while (name.length() > 3 && tft.textWidth(name, 2) > 216) name.remove(name.length() - 1);
  tft.setTextDatum(TL_DATUM); tft.setTextColor(WD_DIM, WD_PANEL); tft.drawString("SELECTED BLE TARGET", 14, 58, 1);
  tft.setTextColor(WD_BONE, WD_PANEL); tft.drawString(name, 14, 75, 2);
  if (found) { tft.setTextColor(WD_CYAN, WD_PANEL); tft.drawString(macToString(target.mac), 14, 96, 1); }

  const uint16_t range_color = rssi >= -65 ? WD_AMBER : WD_CYAN;
  wdPanel(tft, 6, 116, 228, 138, WD_SURFACE, WD_EDGE, range_color);
  tft.setTextDatum(MC_DATUM); tft.setTextColor(range_color, WD_SURFACE);
  tft.drawString(proximity, 120, 139, 2);
  tft.setTextColor(WD_BONE, WD_SURFACE);
  tft.drawString(found ? String(rssi) + " dBm" : "-- dBm", 120, 171, 4);
  for (uint8_t ring = 0; ring < 3; ring++) tft.drawCircle(120, 220, 15 + ring * 17, ring == 2 ? range_color : WD_EDGE);
  const int16_t strength = found ? constrain((rssi + 100) * 48 / 65, 0, 48) : 0;
  const int16_t dot_radius = max((int16_t)3, (int16_t)(strength / 3));
  if (strength) tft.fillCircle(120, 220, dot_radius, range_color);

  static const char* const buttons[4] = {"START", "STOP", "TARGET", "BACK"};
  for (uint8_t i = 0; i < 4; i++) {
    const bool enabled = i == 0 ? !running : (i == 1 ? running : true);
    const int16_t x = i * 60 + 2;
    uint16_t fill = WD_SURFACE, accent = WD_CYAN, text = WD_BONE;
    if (i == 0) { fill = enabled ? WD_CYAN : WD_SURFACE; text = enabled ? TFT_BLACK : WD_DIM; }
    else if (i == 1) { fill = enabled ? WD_AMBER : WD_SURFACE; accent = WD_AMBER; text = enabled ? TFT_BLACK : WD_DIM; }
    else if (i == 2) text = WD_CYAN;
    else { accent = WD_AMBER; text = WD_AMBER; }
    wdPanel(tft, x, 274, 56, 42, fill, enabled ? accent : WD_EDGE, accent);
    tft.setTextDatum(MC_DATUM); tft.setTextColor(text, fill); tft.drawString(buttons[i], x + 28, 295, 1);
  }
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::handleFoxHuntTouch(uint16_t touch_x, uint16_t touch_y) {
  if (touch_y < 268) return;
  const uint8_t action = min((uint8_t)3, (uint8_t)(touch_x / 60));
  if (action == 0) wifi_scan_obj.startFoxHunt();
  else if (action == 1) wifi_scan_obj.stopFoxHunt();
  else {
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
    display_obj.init();
    this->changeMenu(action == 2 ? &foxHuntMenu : &bluetoothSnifferMenu, true);
    return;
  }
  this->drawFoxHuntUI(true);
}

void MenuFunctions::drawWiFiFoxHuntUI(bool full_redraw) {
  extern LinkedList<AccessPoint>* access_points;
  TFT_eSPI& tft = display_obj.tft;
  AccessPoint target;
  bool found = false;
  for (int i = 0; access_points && i < access_points->size(); i++) {
    if (access_points->get(i).selected) {
      target = access_points->get(i);
      found = true;
      break;
    }
  }

  const int rssi = found ? target.rssi : -128;
  const char* proximity = rssi >= -48 ? "VERY CLOSE" : rssi >= -62 ? "CLOSE" :
                          rssi >= -77 ? "NEAR" : rssi > -128 ? "FAR" : "NO SIGNAL";
  const bool running = wifi_scan_obj.wifi_fox_running;
  const uint16_t state_color = running ? WD_CYAN : WD_AMBER;
  const uint16_t range_color = rssi >= -62 ? WD_AMBER : WD_CYAN;

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  if (full_redraw) {
    tft.fillScreen(TFT_BLACK);
    this->drawSharkTopBar(true);
  } else {
    this->drawSharkTopBar(false);
  }
  tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH,
               SCREEN_HEIGHT - STATUS_BAR_WIDTH, TFT_BLACK);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WD_CYAN, TFT_BLACK);
  tft.drawString("// WI-FI FOX HUNT", 6, STATUS_BAR_WIDTH + 7, 2);
  tft.fillRoundRect(174, STATUS_BAR_WIDTH + 4, 61, 17, 3, state_color);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, state_color);
  tft.drawString(running ? "TRACK" : "PAUSED", 204, STATUS_BAR_WIDTH + 12, 1);
  tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
  tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 72, WD_CYAN);

  wdPanel(tft, 6, 49, 228, 67, WD_PANEL, WD_EDGE, WD_CYAN);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WD_DIM, WD_PANEL);
  tft.drawString("SELECTED ACCESS POINT", 14, 55, 1);
  String name = found && target.essid.length() ? target.essid :
                found ? "HIDDEN ACCESS POINT" : "NO TARGET SELECTED";
  while (name.length() > 3 && tft.textWidth(name, 2) > 212)
    name.remove(name.length() - 1);
  tft.setTextColor(WD_BONE, WD_PANEL);
  tft.drawString(name, 14, 70, 2);
  String detail = found ? macToString(target.bssid) + "  CH " + String(target.channel) :
                          "--:--:--:--:--:--  CH --";
  tft.setTextColor(found ? WD_CYAN : WD_DIM, WD_PANEL);
  tft.drawString(detail, 14, 96, 1);

  wdPanel(tft, 6, 122, 228, 132, WD_SURFACE, WD_EDGE, range_color);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(range_color, WD_SURFACE);
  tft.drawString(proximity, 120, 140, 2);
  tft.setTextColor(WD_BONE, WD_SURFACE);
  tft.drawString(found ? String(rssi) + " dBm" : "-- dBm", 120, 174, 4);

  // The meter is based only on the RSSI written by the existing promiscuous
  // callback for the selected BSSID. When paused, retain that last real value.
  const int16_t meter_x = 18, meter_y = 207, meter_w = 204, meter_h = 17;
  tft.fillRoundRect(meter_x, meter_y, meter_w, meter_h, 3, TFT_BLACK);
  tft.drawRoundRect(meter_x, meter_y, meter_w, meter_h, 3, WD_EDGE);
  const int16_t clamped_rssi = constrain(rssi, -100, -35);
  const int16_t signal_w = found ?
      (int16_t)((int32_t)(clamped_rssi + 100) * (meter_w - 4) / 65) : 0;
  if (signal_w > 0)
    tft.fillRoundRect(meter_x + 2, meter_y + 2, signal_w, meter_h - 4, 2,
                      range_color);
  tft.setTextColor(WD_DIM, WD_SURFACE);
  tft.drawString(running ? "LIVE BSSID RSSI" : "LAST REAL RSSI", 120, 237, 1);

  static const char* const buttons[4] = {"START", "STOP", "TARGET", "BACK"};
  for (uint8_t i = 0; i < 4; i++) {
    const bool enabled = i == 0 ? (!running && found) :
                         i == 1 ? running : true;
    const int16_t x = i * 60 + 2;
    uint16_t fill = WD_SURFACE, accent = WD_CYAN, text = WD_BONE;
    if (i == 0) {
      fill = enabled ? WD_CYAN : WD_SURFACE;
      text = enabled ? TFT_BLACK : WD_DIM;
    } else if (i == 1) {
      fill = enabled ? WD_AMBER : WD_SURFACE;
      accent = WD_AMBER;
      text = enabled ? TFT_BLACK : WD_DIM;
    } else if (i == 2) {
      text = WD_CYAN;
    } else {
      accent = WD_AMBER;
      text = WD_AMBER;
    }
    wdPanel(tft, x, 274, 56, 42, fill, enabled ? accent : WD_EDGE, accent);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(text, fill);
    tft.drawString(buttons[i], x + 28, 295, 1);
  }
  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::handleWiFiFoxHuntTouch(uint16_t touch_x, uint16_t touch_y) {
  if (touch_y < 268)
    return;

  const uint8_t action = min((uint8_t)3, (uint8_t)(touch_x / 60));
  if (action == 0) {
    wifi_scan_obj.startWiFiFoxHunt();
  } else if (action == 1) {
    wifi_scan_obj.stopWiFiFoxHunt();
  } else {
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
    display_obj.init();
    this->changeMenu(action == 2 ? &foxHuntMenu : &wifiSnifferMenu, true);
    return;
  }
  this->drawWiFiFoxHuntUI(true);
}

void MenuFunctions::drawBleSpamUI(bool full_redraw) {
  TFT_eSPI& tft = display_obj.tft;
  const bool running = wifi_scan_obj.bt_sour_running;
  const uint16_t state_color = running ? WD_RED : WD_AMBER;
  const int16_t card_y = 49;
  const int16_t card_w = 76;
  const int16_t gx = 6, gy = 177, gw = 228, gh = 87;
  const int16_t plot_x = gx + 6;
  const int16_t plot_y = gy + 20;
  const int16_t plot_w = gw - 12;
  const int16_t plot_h = gh - 27;

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  tft.setTextDatum(TL_DATUM);

  if (full_redraw) {
    tft.fillScreen(TFT_BLACK);
    this->drawSharkTopBar(true);
    const uint8_t dash_mode = wifi_scan_obj.currentScanMode;
    const char* dash_title = dash_mode == BT_ATTACK_SWIFTPAIR_SPAM ? "// SWIFTPAIR SPAM" :
                             dash_mode == BT_ATTACK_APPLE_JUICE ? "// APPLE JUICE" :
                             dash_mode == BT_ATTACK_SAMSUNG_SPAM ? "// SAMSUNG SPAM" :
                             dash_mode == BT_ATTACK_GOOGLE_SPAM ? "// GOOGLE SPAM" :
                             dash_mode == BT_ATTACK_FLIPPER_SPAM ? "// FLIPPER SPAM" :
                             dash_mode == BT_ATTACK_SPAM_ALL ? "// BLE SPAM ALL" : "// SOUR APPLE";
    tft.setTextColor(WD_RED, TFT_BLACK);
    tft.drawString(dash_title, 6, STATUS_BAR_WIDTH + 7, 2);
    tft.fillRoundRect(169, STATUS_BAR_WIDTH + 4, 66, 17, 3, state_color);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, state_color);
    tft.drawString(running ? "TX ACTIVE" : "PAUSED", 202,
                   STATUS_BAR_WIDTH + 12, 1);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, SCREEN_WIDTH, WD_EDGE);
    tft.drawFastHLine(0, STATUS_BAR_WIDTH + 24, 58, WD_RED);

    const char* labels[3] = {"RATE / S", "PEAK / S", "TOTAL"};
    for (uint8_t i = 0; i < 3; i++) {
      const int16_t x = 2 + i * 80;
      wdPanel(tft, x, card_y, card_w, 42, WD_SURFACE, WD_EDGE,
              i == 0 ? state_color : WD_RED);
      tft.setTextDatum(TL_DATUM);
      // WD_DIM is intentionally near-black in the Matrix theme and vanished
      // on the physical TFT.  These are data labels, not disabled controls,
      // so use the theme's brightest content color.
      tft.setTextColor(WD_WHITE, WD_SURFACE);
      tft.drawString(labels[i], x + 7, card_y + 5, 1);
    }

    wdPanel(tft, 6, 96, 228, 48, WD_PANEL, WD_EDGE, WD_RED);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_WHITE, WD_PANEL);
    tft.drawString("TRANSMIT PROFILE", 14, 102, 1);
    tft.setTextColor(WD_BONE, WD_PANEL);
    tft.drawString(dash_mode == BT_ATTACK_SWIFTPAIR_SPAM ? "MS SWIFTPAIR ADV" :
                   dash_mode == BT_ATTACK_APPLE_JUICE ? "APPLE JUICE ADV" :
                   dash_mode == BT_ATTACK_SAMSUNG_SPAM ? "SAMSUNG BLE ADV" :
                   dash_mode == BT_ATTACK_GOOGLE_SPAM ? "GOOGLE FAST PAIR" :
                   dash_mode == BT_ATTACK_FLIPPER_SPAM ? "FLIPPER BLE ADV" :
                   dash_mode == BT_ATTACK_SPAM_ALL ? "ALL BLE FAMILIES" : "APPLE BLE ADV",
                   14, 116, 2);
    tft.setTextColor(WD_RED, WD_PANEL);
    const char* timing = dash_mode == BT_ATTACK_SWIFTPAIR_SPAM ? "RANDOM MAC / 10 MS BURST" :
                         dash_mode == BT_ATTACK_APPLE_JUICE ? "RANDOM MAC / 500 MS BURST" :
                         dash_mode == BT_ATTACK_SAMSUNG_SPAM ? "RANDOM MAC / 10 MS BURST" :
                                                               "RANDOM MAC / 60 MS BURST";
    tft.drawString(timing, 14, 135, 1);

    tft.fillRect(6, 150, 228, 21, WD_AMBER);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, WD_AMBER);
    tft.drawString("AUTHORIZED TEST DEVICES ONLY", 120, 160, 1);

    wdPanel(tft, gx, gy, gw, gh, WD_SURFACE, WD_EDGE, state_color);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_WHITE, WD_SURFACE);
    tft.drawString("BURST HISTORY / 100 MS", gx + 8, gy + 5, 1);

    static const char* const buttons[4] = {"START", "STOP", "CLEAR", "BACK"};
    for (uint8_t i = 0; i < 4; i++) {
      const bool enabled = i == 0 ? !running : (i == 1 ? running : true);
      const int16_t x = i * 60 + 2;
      uint16_t fill = WD_SURFACE, accent = WD_RED, text = WD_BONE;
      if (i == 0) {
        fill = enabled ? WD_RED : WD_SURFACE;
        text = enabled ? TFT_BLACK : WD_GREY;
      } else if (i == 1) {
        fill = enabled ? WD_AMBER : WD_SURFACE;
        accent = WD_AMBER;
        text = enabled ? TFT_BLACK : WD_GREY;
      } else if (i == 2) {
        accent = WD_BONE;
      } else {
        accent = WD_AMBER;
        text = WD_AMBER;
      }
      wdPanel(tft, x, 274, 56, 42, fill, enabled ? accent : WD_EDGE, accent);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(text, fill);
      tft.drawString(buttons[i], x + 28, 295, 1);
    }
  } else {
    this->drawSharkTopBar(false);
  }

  // Periodic refreshes only repaint the live value cells and plot.  Keeping
  // the static frame on-screen avoids the visible full-screen black flash.
  const uint32_t raw_values[3] = {
    wifi_scan_obj.bt_sour_rate,
    wifi_scan_obj.bt_sour_peak,
    wifi_scan_obj.bt_sour_total
  };
  static uint32_t previous_values[3] = {
    UINT32_MAX, UINT32_MAX, UINT32_MAX
  };
  for (uint8_t i = 0; i < 3; i++) {
    if (!full_redraw && raw_values[i] == previous_values[i])
      continue;
    const int16_t x = 2 + i * 80;
    tft.fillRect(x + 3, card_y + 17, card_w - 6, 21, WD_SURFACE);
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(i == 0 ? state_color : WD_BONE, WD_SURFACE);
    tft.drawString(String(raw_values[i]), x + card_w - 7, card_y + 27, 2);
    previous_values[i] = raw_values[i];
  }

  int16_t graph_max = 1;
  for (int16_t i = 0; i < plot_w; i++)
    if (wifi_scan_obj._analyzer_values[i] > graph_max)
      graph_max = wifi_scan_obj._analyzer_values[i];
  const uint16_t graph_color = running ? WD_RED : WD_AMBER;
  for (int16_t i = 0; i < plot_w; i++) {
    const int16_t column_x = plot_x + plot_w - 1 - i;
    // Replace one column at a time instead of blanking the entire plot first.
    // This keeps the previous graph visible while the next frame is painted.
    tft.drawFastVLine(column_x, plot_y, plot_h, TFT_BLACK);
    for (uint8_t line = 1; line < 4; line++)
      tft.drawPixel(column_x, plot_y + (plot_h * line / 4), WD_PANEL);
    const int16_t sample = max((int16_t)0, wifi_scan_obj._analyzer_values[i]);
    const int16_t bar = sample ? max((int16_t)1,
        (int16_t)((int32_t)sample * (plot_h - 2) / graph_max)) : 0;
    if (bar)
      tft.drawFastVLine(column_x, plot_y + plot_h - bar, bar, graph_color);
  }

  tft.setTextDatum(TL_DATUM);
}

void MenuFunctions::handleBleSpamTouch(uint16_t touch_x, uint16_t touch_y) {
  if (touch_y < 268)
    return;

  const uint8_t action = min((uint8_t)3, (uint8_t)(touch_x / 60));
  if (action == 0) {
    wifi_scan_obj.startSourApple();
  } else if (action == 1) {
    wifi_scan_obj.stopSourApple();
  } else if (action == 2) {
    wifi_scan_obj.clearSourApple();
  } else {
    wifi_scan_obj.stopSourApple();
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
    display_obj.init();
    this->changeMenu(&bluetoothAttackMenu, true);
    return;
  }
  this->drawBleSpamUI(true);
}
#endif

void MenuFunctions::buttonNotSelected(int b, int x) {
  if (x == -1)
    x = b;

  // Ensure b is within valid button index range
  b = (x - menu_start_index) % BUTTON_SCREEN_LIMIT;

  #ifdef HAS_MINI_SCREEN
    this->drawMiniMenuButton(b, x, false);
  #endif

  uint16_t color = (current_menu->list->get(x).icon == SETTINGS && current_menu->list->get(x).color == TFTLIGHTGREY) ? (current_menu->list->get(x).selected ? TFT_GREEN : TFT_RED) : this->getColor(current_menu->list->get(x).color);
  uint16_t icon_color = (current_menu->list->get(x).icon == SETTINGS && current_menu->list->get(x).color == TFTLIGHTGREY) ? TFT_LIGHTGREY : color;

  #ifdef HAS_FULL_SCREEN
    #ifdef MARAUDER_V8
      this->drawSharkGridButton(b, x, false);
    #else
      display_obj.tft.setFreeFont(MENU_FONT);
      display_obj.key[b].initButton(&display_obj.tft, KEY_X, KEY_Y + b * (KEY_H + KEY_SPACING_Y), KEY_W, KEY_H, TFT_BLACK, TFT_BLACK, color, (char*)"", KEY_TEXTSIZE);
      display_obj.key[b].drawButton(false, current_menu->list->get(x).name);
      if ((current_menu->list->get(x).name != text09) && (current_menu->list->get(x).icon != 255))
            display_obj.tft.drawXBitmap(0,
                                        KEY_Y + (b * (KEY_H + KEY_SPACING_Y)) - (ICON_H / 2),
                                        menu_icons[current_menu->list->get(x).icon],
                                        ICON_W,
                                        ICON_H,
                                        TFT_BLACK,
                                        icon_color);
      display_obj.tft.setFreeFont(NULL);
    #endif
  #endif
}

void MenuFunctions::buttonSelected(int b, int x) {
  if (x == -1)
    x = b;

  // Ensure b is within valid button index range
  b = (x - menu_start_index) % BUTTON_SCREEN_LIMIT;

  uint16_t color = this->getColor(current_menu->list->get(x).color);

  #ifdef HAS_MINI_SCREEN
    this->drawMiniMenuButton(b, x, true);
  #endif

  #ifdef HAS_FULL_SCREEN
    #ifdef MARAUDER_V8
      this->drawSharkGridButton(b, x, true);
    #else
      display_obj.tft.setFreeFont(MENU_FONT);
      if (current_menu->list->get(x).icon == SETTINGS && current_menu->list->get(x).color == TFTLIGHTGREY) {
        uint16_t setting_color = current_menu->list->get(x).selected ? TFT_GREEN : TFT_RED;
        display_obj.key[b].initButton(&display_obj.tft, KEY_X, KEY_Y + b * (KEY_H + KEY_SPACING_Y), KEY_W, KEY_H, TFT_BLACK, TFT_LIGHTGREY, setting_color, (char*)"", KEY_TEXTSIZE);
        display_obj.key[b].drawButton(false, current_menu->list->get(x).name);
        display_obj.tft.drawXBitmap(0,
                                        KEY_Y + (b * (KEY_H + KEY_SPACING_Y)) - (ICON_H / 2),
                                        menu_icons[current_menu->list->get(x).icon],
                                        ICON_W,
                                        ICON_H,
                                        TFT_BLACK,
                                        TFT_LIGHTGREY);
      } else {
        display_obj.key[b].drawButton(true, current_menu->list->get(x).name);
        if ((current_menu->list->get(x).name != text09) && (current_menu->list->get(x).icon != 255))
              display_obj.tft.drawXBitmap(0,
                                          KEY_Y + (b * (KEY_H + KEY_SPACING_Y)) - (ICON_H / 2),
                                          menu_icons[current_menu->list->get(x).icon],
                                          ICON_W,
                                          ICON_H,
                                          TFT_BLACK,
                                          color);
      }
      display_obj.tft.setFreeFont(NULL);
    #endif
  #endif
}

void MenuFunctions::displayMenuButtons() {
  #ifdef HAS_ILI9341
    #ifdef MARAUDER_V8
      const bool has_previous = this->menu_start_index > 0;
      const bool has_next = current_menu && current_menu->list &&
                            this->menu_start_index + BUTTON_SCREEN_LIMIT < current_menu->list->size();
      const uint16_t previous_color = has_previous ? WD_CYAN : WD_DIM;
      const uint16_t next_color = has_next ? WD_CYAN : WD_DIM;
      const int16_t footer_height = TFT_HEIGHT - SHARK_GRID_FOOTER_Y - 7;
      const int16_t footer_mid = SHARK_GRID_FOOTER_Y + footer_height / 2;

      display_obj.tft.fillRect(0,
                               SHARK_GRID_FOOTER_Y - 6,
                               TFT_WIDTH,
                               TFT_HEIGHT - SHARK_GRID_FOOTER_Y + 6,
                               TFT_BLACK);
      wdRule(display_obj.tft, 4, SHARK_GRID_FOOTER_Y - 5, TFT_WIDTH - 8, WD_EDGE);

      wdPanel(display_obj.tft, 4, SHARK_GRID_FOOTER_Y, 92, footer_height,
              WD_SURFACE, WD_EDGE, previous_color);
      wdPanel(display_obj.tft, TFT_WIDTH - 96, SHARK_GRID_FOOTER_Y, 92, footer_height,
              WD_SURFACE, WD_EDGE, next_color);

      display_obj.tft.setFreeFont(NULL);
      display_obj.tft.setTextSize(1);
      display_obj.tft.setTextDatum(MC_DATUM);
      display_obj.tft.setTextColor(previous_color, WD_SURFACE);
      display_obj.tft.drawString("< PREV", 50, footer_mid, 2);
      display_obj.tft.setTextColor(next_color, WD_SURFACE);
      display_obj.tft.drawString("NEXT >", TFT_WIDTH - 50, footer_mid, 2);

      uint16_t page_count = 1;
      uint16_t current_page = 1;
      if (current_menu && current_menu->list && current_menu->list->size() > 0) {
        page_count = (current_menu->list->size() + BUTTON_SCREEN_LIMIT - 1) / BUTTON_SCREEN_LIMIT;
        current_page = this->menu_start_index / BUTTON_SCREEN_LIMIT + 1;
      }

      char page_label[16];
      snprintf(page_label, sizeof(page_label), "%02u/%02u", (unsigned)current_page, (unsigned)page_count);
      display_obj.tft.setTextColor(WD_CYAN, TFT_BLACK);
      display_obj.tft.drawString(page_label, TFT_WIDTH / 2, footer_mid - 6, 1);
      display_obj.tft.setTextColor(WD_DIM, TFT_BLACK);
      display_obj.tft.drawString("PAGE", TFT_WIDTH / 2, footer_mid + 6, 1);
      display_obj.tft.setTextDatum(TL_DATUM);
      return;
    #endif
    // Draw lines to show each menu button
    for (int i = 0; i < 3; i++) {

      #ifdef MARAUDER_V8
        uint16_t zone_color = SHARK_GREEN_DIM;
      #else
        uint16_t zone_color = TFT_FARTGRAY;
      #endif

      // Draw horizontal line on left
      display_obj.tft.drawLine(0, 
                              TFT_HEIGHT / 3 * (i),
                              (TFT_WIDTH / 12) / 2,
                              TFT_HEIGHT / 3 * (i),
                              zone_color);

      // Draw horizontal line on right
      display_obj.tft.drawLine(TFT_WIDTH - 1 - ((TFT_WIDTH / 12) / 2), 
                              TFT_HEIGHT / 3 * (i),
                              TFT_WIDTH,
                              TFT_HEIGHT / 3 * (i),
                              zone_color);

      // Draw vertical line on left
      display_obj.tft.drawLine(0, 
                              (TFT_HEIGHT / 3 * (i)) - ((TFT_WIDTH / 12) / 2),
                              0,
                              (TFT_HEIGHT / 3 * (i)) + ((TFT_WIDTH / 12) / 2),
                              zone_color);

      // Draw vertical line on right
      display_obj.tft.drawLine(TFT_WIDTH - 1, 
                              (TFT_HEIGHT / 3 * (i)) - ((TFT_WIDTH / 12) / 2),
                              TFT_WIDTH - 1,
                              (TFT_HEIGHT / 3 * (i)) + ((TFT_WIDTH / 12) / 2),
                              zone_color);
    }
  #endif
}

// Function to check menu input
  // IQ Family Watch overlay: full-screen take-over for a family device that
  // identified itself on this device's AP. Repaints at ~3 FPS so anything
  // running underneath cannot bleed through, flashes the frame, and the main
  // loop dismisses it on any touch (or after 15 s).
  void MenuFunctions::drawFamilyAlert(uint32_t currentTime) {
    static uint32_t last_paint = 0;
    if (last_paint && currentTime - last_paint < 300) return;
    last_paint = currentTime;
    TFT_eSPI& tft = display_obj.tft;
    const bool flash = (currentTime / 500) % 2 == 0;
    const uint16_t frame = flash ? TFT_CYAN : TFT_WHITE;
    tft.fillScreen(TFT_BLACK);
    tft.drawRoundRect(6, 6, 228, 308, 8, frame);
    tft.drawRoundRect(9, 9, 222, 302, 8, frame);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("// IQ FAMILY WATCH", 120, 38, 2);
    tft.setTextColor(frame, TFT_BLACK);
    tft.drawString("FAMILY", 120, 84, 4);
    tft.drawString("DEVICE", 120, 114, 4);
    tft.drawString("DETECTED", 120, 144, 4);
    String nm = String(wifi_scan_obj.family_name);
    nm.toUpperCase();
    tft.fillRoundRect(48, 176, 144, 26, 5, TFT_CYAN);
    tft.setTextColor(TFT_BLACK, TFT_CYAN);
    tft.drawString(nm, 120, 189, 2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(String((int)wifi_scan_obj.family_rssi) + " dBm  " +
                   wifi_scan_obj.familyProximityWord(), 120, 228, 2);
    // 5-bar proximity meter straight off the measured RSSI.
    const int8_t r = wifi_scan_obj.family_rssi;
    const uint8_t bars = r >= -50 ? 5 : r >= -60 ? 4 : r >= -67 ? 3 : r >= -75 ? 2 : r >= -80 ? 1 : 0;
    for (uint8_t i = 0; i < 5; i++) {
      const uint16_t h = 6 + i * 5;
      tft.fillRect(93 + i * 12, 282 - h, 8, h, i < bars ? TFT_CYAN : TFT_DARKGREY);
    }
    if (flash) {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString("TAP TO DISMISS", 120, 296, 2);
    }
    tft.setTextDatum(TL_DATUM);
  }

  void MenuFunctions::dismissFamilyAlert() {
    wifi_scan_obj.family_alert_showing = false;
    display_obj.tft.fillScreen(TFT_BLACK);
    // Menus redraw immediately; live dashboards repaint on their own tick.
    if (wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF ||
        wifi_scan_obj.currentScanMode == WIFI_CONNECTED) {
      if (current_menu) this->changeMenu(current_menu, true);
    }
    #ifdef MARAUDER_V8
      if (shark_web_obj.active) shark_web_obj.drawCard();
    #endif
  }

void MenuFunctions::main(uint32_t currentTime)
{
  #if defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)
    this->updateKeyboard();
  #endif

  #ifdef MARAUDER_V8
    sharkProfileTick(currentTime);
    // Web Control running in the background: mirror the live screen, run any
    // remote control action, and if the phone launched a tool, start it
    // (StartScan drops the AP first).
    if (shark_web_obj.background) {
      shark_web_obj.capture();
      int web_action = shark_web_obj.takeAction();
      if (web_action >= 0)
        this->webNavAction(web_action);
      int web_mode = shark_web_obj.takePending();
      if (web_mode >= 0)
        this->startWebTool(web_mode);
      else if (web_mode == -3)
        shark_web_obj.completeWiFiJoin();
      else if (web_mode == -4) {
        shark_web_obj.resume_pending = false;   // user closed it on purpose
        shark_web_obj.stop();
      }
    }
  #endif

  // Some function exited and we need to go back to normal
  if (display_obj.exit_draw) {
    if (wifi_scan_obj.currentScanMode != WIFI_CONNECTED)
      wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
    display_obj.exit_draw = false;
    this->orientDisplay();
  }
  if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) ||
      (wifi_scan_obj.currentScanMode == WIFI_CONNECTED) ||
      (wifi_scan_obj.currentScanMode == OTA_UPDATE) ||
      (wifi_scan_obj.currentScanMode == ESP_UPDATE) ||
      (wifi_scan_obj.currentScanMode == SHOW_INFO) ||
      (wifi_scan_obj.currentScanMode == WIFI_SCAN_GPS_DATA) ||
      (wifi_scan_obj.currentScanMode == GPS_POI) ||
      (wifi_scan_obj.currentScanMode == GPS_TRACKER) ||
      (wifi_scan_obj.currentScanMode == WIFI_SCAN_GPS_NMEA)) {
    if (wifi_scan_obj.orient_display) {
      this->orientDisplay();
      wifi_scan_obj.orient_display = false;
    }
  }

  if (currentTime != 0) {
    if (currentTime - initTime >= BANNER_TIME) {
      this->initTime = millis();
      if ((wifi_scan_obj.currentScanMode != LV_JOIN_WIFI) &&
          (wifi_scan_obj.currentScanMode != LV_ADD_SSID)) {
        // V8 redraws after the global display pass so buffered/full-screen
        // tools cannot paint over the HUD. Other targets keep this position.
        #ifndef MARAUDER_V8
          this->updateStatusBar();
        #endif
      }
      
      // Do channel analyzer stuff
      if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ANALYZER) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_ANALYZER)){
        #ifdef HAS_SCREEN
          #ifdef MARAUDER_V8
            if (wifi_scan_obj.currentScanMode == BT_SCAN_ANALYZER)
              this->drawBluetoothAnalyzerUI(false);
            else if (!this->wifi_tool_ui) {
              this->setGraphScale(this->graphScaleCheck(wifi_scan_obj._analyzer_values));
              this->drawGraph(wifi_scan_obj._analyzer_values);
            }
          #else
            this->setGraphScale(this->graphScaleCheck(wifi_scan_obj._analyzer_values));
            this->drawGraph(wifi_scan_obj._analyzer_values);
          #endif
        #endif
      }

      #if defined(MARAUDER_V8) && defined(HAS_BT)
        if (wifi_scan_obj.currentScanMode == BT_SCAN_ALL &&
            !this->bt_sniffer_data_view)
          this->drawBluetoothSnifferUI(false);
        if (wifi_scan_obj.currentScanMode == BT_SCAN_FLIPPER &&
            !this->bt_flipper_data_view)
          this->drawFlipperSnifferUI(false);
        if (wifi_scan_obj.currentScanMode == BT_SCAN_SKIMMERS &&
            !this->bt_skimmer_data_view)
          this->drawCardSkimmerUI(false);
        if (((wifi_scan_obj.currentScanMode == BT_SCAN_AIRTAG) ||
             (wifi_scan_obj.currentScanMode == BT_SCAN_AIRTAG_MON) ||
             (wifi_scan_obj.currentScanMode == BT_SCAN_FLOCK) ||
             (wifi_scan_obj.currentScanMode == BT_SCAN_RAYBAN)) &&
            !this->bt_passive_data_view)
          this->drawPassiveBleDetectorUI(false);
        // GPS Data owns its themed screen; repaint is throttled + change-
        // gated inside drawGpsDataUI itself.
        if (wifi_scan_obj.currentScanMode == WIFI_SCAN_GPS_DATA)
          this->drawGpsDataUI(false);
        if (wifi_scan_obj.currentScanMode == GPS_TRACKER)
          this->drawGpsTrackerUI(false);
        if (wifi_scan_obj.currentScanMode == BT_SCAN_FOX_HUNT)
          this->drawFoxHuntUI(false);
        if (wifi_scan_obj.currentScanMode == BT_ATTACK_SOUR_APPLE ||
            wifi_scan_obj.currentScanMode == BT_ATTACK_SWIFTPAIR_SPAM ||
            wifi_scan_obj.currentScanMode == BT_ATTACK_APPLE_JUICE ||
            wifi_scan_obj.currentScanMode == BT_ATTACK_SAMSUNG_SPAM ||
            wifi_scan_obj.currentScanMode == BT_ATTACK_GOOGLE_SPAM ||
            wifi_scan_obj.currentScanMode == BT_ATTACK_FLIPPER_SPAM ||
            wifi_scan_obj.currentScanMode == BT_ATTACK_SPAM_ALL)
          this->drawBleSpamUI(false);
      #endif

      #ifdef MARAUDER_V8
        if (wifi_scan_obj.currentScanMode == WIFI_SCAN_SIG_STREN)
          this->drawWiFiFoxHuntUI(false);
        // Wi-Fi sniffers using the shared passive-detector dashboard.
        if (this->wifi_passive_ui && wifi_scan_obj.isPassiveWifiMode() &&
            !this->wifi_passive_data_view)
          this->drawPassiveWifiDetectorUI(false);
        if (this->wifi_tool_ui &&
            !this->wifi_packet_target_view &&
            this->isWifiToolUiMode(wifi_scan_obj.currentScanMode))
          this->drawWifiToolUI(false);
        if (this->network_scan_ui)
          this->drawNetworkScannerUI(false);
        if (this->beacon_list_ui)
          this->drawBeaconListUI(false);
        if (this->beacon_spam_ui)
          this->drawBeaconSpamUI(false);
        if (this->probe_flood_ui)
          this->drawProbeFloodUI(false);
        if (this->deauth_flood_ui)
          this->drawDeauthFloodUI(false);
        if (this->attack_dash_ui)
          this->drawAttackDashUI(false);
        if (this->funny_beacon_ui)
          this->drawFunnyBeaconUI(false);
        if (this->rick_roll_ui)
          this->drawRickRollUI(false);
      #endif

      if (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ACT) {
        #ifdef HAS_SCREEN
          #ifdef MARAUDER_V8
          if (!this->wifi_tool_ui) {
          #endif
          this->setGraphScale(this->graphScaleCheckSmall(wifi_scan_obj.channel_activity));

          this->drawGraphSmall(wifi_scan_obj.channel_activity);
          #ifdef MARAUDER_V8
          }
          #endif

        #endif
      }
    }
  }


  boolean pressed = false;
  // This is code from bodmer's keypad example
  uint16_t t_x = 0, t_y = 0; // To store the touch coordinates

  // Get the display buffer out of the way
  if ((wifi_scan_obj.currentScanMode != WIFI_SCAN_OFF ) &&
      (wifi_scan_obj.currentScanMode != WIFI_CONNECTED) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_BEACON_SPAM) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_AP_SPAM) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_CSA) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_QUIET) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_AUTH) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_DEAUTH) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_DEAUTH_MANUAL) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_DEAUTH_TARGETED) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_BAD_MSG_TARGETED) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_BAD_MSG) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_SLEEP) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_SLEEP_TARGETED) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_MIMIC) &&
	  (wifi_scan_obj.currentScanMode != WIFI_ATTACK_FUNNY_BEACON) &&
      #ifdef MARAUDER_V8
      // M5SHARK dashboards own the complete TFT; the legacy scrolling list must
      // not paint over either the detector or shared Wi-Fi monitor canvas.
      (!this->wifi_passive_ui) && (!this->wifi_tool_ui) &&
      (!this->network_scan_ui) &&
      (!this->beacon_list_ui) &&
      (!this->beacon_spam_ui) &&
      (!this->funny_beacon_ui) &&
      (!this->rick_roll_ui) &&
      (!this->probe_flood_ui) &&
      (!this->deauth_flood_ui) &&
      (!this->attack_dash_ui) &&
      #endif
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_RICK_ROLL))
    display_obj.displayBuffer();


  int pre_getTouch = millis();

  #ifdef HAS_ILI9341
    if (!this->disable_touch) {
      #ifdef MARAUDER_V8
        // Shared Wi-Fi dashboards use larger physical controls and a lower
        // pressure threshold so light taps still register through the case.
        if (this->wifi_tool_ui || this->network_scan_ui ||
            this->beacon_list_ui || this->beacon_spam_ui ||
            this->funny_beacon_ui || this->rick_roll_ui)
          pressed = display_obj.updateTouch(&t_x, &t_y, 350);
        else
          pressed = display_obj.updateTouch(&t_x, &t_y);
      #else
        pressed = display_obj.updateTouch(&t_x, &t_y);
      #endif
    }
  #endif

  #ifdef MARAUDER_V8
    // IQ Family Watch take-over: a pending family beacon pops the full-screen
    // alert (never over the join-keyboard, which cannot repaint itself), and
    // while it shows, any touch dismisses it. The dismissing touch is
    // swallowed so it cannot also click whatever runs underneath.
    if (wifi_scan_obj.family_alert_pending && !wifi_scan_obj.family_alert_showing &&
        wifi_scan_obj.currentScanMode != LV_JOIN_WIFI &&
        wifi_scan_obj.currentScanMode != LV_ADD_SSID) {
      wifi_scan_obj.family_alert_pending = false;
      wifi_scan_obj.family_alert_showing = true;
      wifi_scan_obj.family_last_alert_ms = currentTime;
      this->drawFamilyAlert(currentTime);
    }
    if (wifi_scan_obj.family_alert_showing) {
      if (pressed || currentTime - wifi_scan_obj.family_last_alert_ms > 15000UL)
        this->dismissFamilyAlert();
      else
        this->drawFamilyAlert(currentTime);
      pressed = false;
      t_x = 0;
      t_y = 0;
    }
  #endif

  #ifdef MARAUDER_V8
    // Auto-resume Web Control: any tool (a scan, radar, RuView, a Wi-Fi join)
    // tore the AP down because one radio can't host the web server AND hop
    // channels at the same time. As soon as the tool exits and we are back on a
    // menu with the radio free, bring the AP + server straight back -- so from
    // the phone's side the web server just blips and returns, never staying dead.
    // A joined network (WIFI_CONNECTED) is excluded on purpose: restarting the
    // AP would force WIFI_AP mode and drop the very station link the panel's
    // join flow just established. Web Control resumes next time the radio frees.
    if (shark_web_obj.resume_pending && current_menu && current_menu->list &&
        !shark_web_obj.active && !shark_web_obj.background &&
        wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) {
      shark_web_obj.resume_pending = false;
      shark_web_obj.startBackground();
    }

    // Home-screen idle wallpaper: after ~25 s with no touch on the main menu
    // (and nothing else running), show the full-screen wallpaper until a touch.
    if (this->last_activity == 0)
      this->last_activity = currentTime;
    if (pressed)
      this->last_activity = currentTime;
    if (this->idle_wall_on &&
        current_menu == &mainMenu && !pressed && !this->disable_touch &&
        !shark_web_obj.active && !shark_web_obj.background &&
        (wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF ||
         wifi_scan_obj.currentScanMode == WIFI_CONNECTED) &&
        currentTime - this->last_activity > 25000) {
      sharkIdleWallpaper(display_obj.tft);
      this->last_activity = millis();
      this->changeMenu(current_menu, true);
      return;
    }
  #endif


  // Brightness gesture: hold top or bottom zone 1.5s to enter brightness mode
  #ifdef HAS_ILI9341
    if (pressed && (wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF ||
                    wifi_scan_obj.currentScanMode == WIFI_CONNECTED)) {
      uint16_t zoneUp = TFT_HEIGHT * 25 / 100;
      uint16_t zoneDown = TFT_HEIGHT * 75 / 100;
      #ifdef MARAUDER_V8
        const bool brightness_gesture_zone = t_y < STATUS_BAR_WIDTH;
      #else
        const bool brightness_gesture_zone = t_y < zoneUp || t_y >= zoneDown;
      #endif
      if (brightness_gesture_zone) {
        uint32_t hold_start = millis();
        uint16_t hx, hy;
        bool held = false;
        while (display_obj.updateTouch(&hx, &hy)) {
          if (millis() - hold_start >= 1500) {
            held = true;
            break;
          }
          delay(10);
        }
        if (held) {
          // Wait for release before entering brightness mode
          while (display_obj.updateTouch(&hx, &hy)) delay(10);
          this->brightnessMode();
          return;
        }
      }
    }
  #endif

  // POI button interception during wardrive — full width bottom bar.
  // V8 is excluded: its SHARK dashboard owns the bottom row (PAUSE/BACK live
  // there), and wardrive tagging goes through the wide TAG POI key in
  // handleWifiToolTouch instead.
  #if defined(HAS_ILI9341) && !defined(MARAUDER_V8)
    if (pressed &&
        (wifi_scan_obj.currentScanMode == WIFI_SCAN_WAR_DRIVE ||
         wifi_scan_obj.currentScanMode == WIFI_SCAN_STATION_WAR_DRIVE)) {
      if (t_y >= (SCREEN_HEIGHT - 50)) {
        wifi_scan_obj.tagPOI(nullptr);
        // Brief green flash
        display_obj.tft.fillRect(0, SCREEN_HEIGHT - 50, SCREEN_WIDTH, 50, TFT_GREEN);
        display_obj.tft.setTextSize(2);
        #ifdef HAS_GPS
        if (gps_obj.getFixStatus())
          display_obj.tft.setTextColor(TFT_BLACK, TFT_GREEN);
        else
        #endif
          display_obj.tft.setTextColor(TFT_BLACK, TFT_RED);
        String poiFlash = "POI (" + String(wifi_scan_obj.poiCount) + ")";
        int16_t flashWidth = poiFlash.length() * 12;
        display_obj.tft.setCursor((SCREEN_WIDTH - flashWidth) / 2, SCREEN_HEIGHT - 33);
        display_obj.tft.print(poiFlash);
        delay(200);
        x = -1;
        y = -1;
        return;
      }
    }
  #endif

  // GPS info screens (GPS Data / NMEA Stream / GPS Tracker) draw full-screen and
  // are deliberately excluded from the generic scan-stop touch below, while the
  // v8 grid touch handler only runs when no scan is active -- so without this the
  // Back gesture is dead and the user is trapped in the app. Give them an
  // explicit tap-anywhere exit that stops the scan and returns to the GPS menu.
  #if defined(HAS_ILI9341) && defined(HAS_GPS)
    if (pressed &&
        ((wifi_scan_obj.currentScanMode == WIFI_SCAN_GPS_DATA) ||
         (wifi_scan_obj.currentScanMode == WIFI_SCAN_GPS_NMEA) ||
         (wifi_scan_obj.currentScanMode == GPS_TRACKER))) {
      wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
      display_obj.init();
      this->changeMenu(&gpsMenu, true);
      x = -1;
      y = -1;
      return;
    }
  #endif

  #if defined(MARAUDER_V8) && defined(HAS_ILI9341)
    if (pressed && wifi_scan_obj.currentScanMode == WIFI_SCAN_SIG_STREN) {
      uint16_t release_x = t_x, release_y = t_y;
      while (display_obj.updateTouch(&release_x, &release_y)) delay(10);
      this->handleWiFiFoxHuntTouch(t_x, t_y);
      x = -1;
      y = -1;
      return;
    }
  #endif

  // Bluetooth Analyzer has persistent START / STOP / CLEAR / BACK controls.
  // Consume its touch here before the legacy "touch anywhere to exit" path.
  #if defined(MARAUDER_V8) && defined(HAS_ILI9341) && defined(HAS_BT)
    if (pressed && wifi_scan_obj.currentScanMode == BT_SCAN_ANALYZER) {
      uint16_t release_x = t_x, release_y = t_y;
      while (display_obj.updateTouch(&release_x, &release_y)) delay(10);
      this->handleBluetoothAnalyzerTouch(t_x, t_y);
      x = -1;
      y = -1;
      return;
    }
    if (pressed && wifi_scan_obj.currentScanMode == BT_SCAN_ALL) {
      uint16_t release_x = t_x, release_y = t_y;
      while (display_obj.updateTouch(&release_x, &release_y)) delay(10);
      this->handleBluetoothSnifferTouch(t_x, t_y);
      x = -1;
      y = -1;
      return;
    }
    if (pressed && wifi_scan_obj.currentScanMode == BT_SCAN_FLIPPER) {
      uint16_t release_x = t_x, release_y = t_y;
      while (display_obj.updateTouch(&release_x, &release_y)) delay(10);
      this->handleFlipperSnifferTouch(t_x, t_y);
      x = -1;
      y = -1;
      return;
    }
    if (pressed && wifi_scan_obj.currentScanMode == BT_SCAN_SKIMMERS) {
      uint16_t release_x = t_x, release_y = t_y;
      while (display_obj.updateTouch(&release_x, &release_y)) delay(10);
      this->handleCardSkimmerTouch(t_x, t_y);
      x = -1;
      y = -1;
      return;
    }
    if (pressed && ((wifi_scan_obj.currentScanMode == BT_SCAN_AIRTAG) ||
                    (wifi_scan_obj.currentScanMode == BT_SCAN_AIRTAG_MON) ||
                    (wifi_scan_obj.currentScanMode == BT_SCAN_FLOCK) ||
                    (wifi_scan_obj.currentScanMode == BT_SCAN_RAYBAN))) {
      uint16_t release_x = t_x, release_y = t_y;
      while (display_obj.updateTouch(&release_x, &release_y)) delay(10);
      this->handlePassiveBleDetectorTouch(t_x, t_y);
      x = -1;
      y = -1;
      return;
    }
    if (pressed && wifi_scan_obj.currentScanMode == BT_SCAN_FOX_HUNT) {
      uint16_t release_x = t_x, release_y = t_y;
      while (display_obj.updateTouch(&release_x, &release_y)) delay(10);
      this->handleFoxHuntTouch(t_x, t_y);
      x = -1;
      y = -1;
      return;
    }
    if (pressed && (wifi_scan_obj.currentScanMode == BT_ATTACK_SOUR_APPLE ||
                    wifi_scan_obj.currentScanMode == BT_ATTACK_SWIFTPAIR_SPAM ||
                    wifi_scan_obj.currentScanMode == BT_ATTACK_APPLE_JUICE ||
                    wifi_scan_obj.currentScanMode == BT_ATTACK_SAMSUNG_SPAM ||
                    wifi_scan_obj.currentScanMode == BT_ATTACK_GOOGLE_SPAM ||
                    wifi_scan_obj.currentScanMode == BT_ATTACK_FLIPPER_SPAM ||
                    wifi_scan_obj.currentScanMode == BT_ATTACK_SPAM_ALL)) {
      uint16_t release_x = t_x, release_y = t_y;
      while (display_obj.updateTouch(&release_x, &release_y)) delay(10);
      this->handleBleSpamTouch(t_x, t_y);
      x = -1;
      y = -1;
      return;
    }
  #endif

  // Wi-Fi sniffers sharing the passive-detector dashboard.
  #ifdef MARAUDER_V8
    if (this->beacon_list_ui) {
      if ((int32_t)(currentTime - this->beacon_list_touch_ready_at) < 0) {
        x = -1;
        y = -1;
        return;
      }

      const bool beacon_back_touch = pressed &&
          ((t_x >= 176 && t_y >= 238) ||
           (t_y >= STATUS_BAR_WIDTH && t_y < 51));
      if (beacon_back_touch) {
        this->exitBeaconListUI();
        x = -1;
        y = -1;
        return;
      }

      if (!pressed) {
        this->beacon_list_touch_active = false;
        this->beacon_list_touch_region = -1;
      } else {
        int16_t region = -1;
        if (t_y >= 238)
          region = t_x < 70 ? 1 : (t_x < 124 ? 2 :
                   (t_x < 178 ? 3 : 4));
        if (region >= 0 &&
            (!this->beacon_list_touch_active ||
             region != this->beacon_list_touch_region ||
             currentTime - this->beacon_list_touch_ms >= 700)) {
          this->beacon_list_touch_active = true;
          this->beacon_list_touch_region = region;
          this->beacon_list_touch_ms = currentTime;
          this->handleBeaconListTouch(t_x, t_y);
        }
      }
      x = -1;
      y = -1;
      return;
    }

    if (this->beacon_spam_ui) {
      if ((int32_t)(currentTime - this->beacon_spam_touch_ready_at) < 0) {
        x = -1;
        y = -1;
        return;
      }

      const bool spam_back_touch = pressed &&
          ((t_x >= 168 && t_y >= 238) ||
           (t_y >= STATUS_BAR_WIDTH && t_y < 51));
      if (spam_back_touch) {
        this->exitBeaconSpamUI();
        x = -1;
        y = -1;
        return;
      }

      if (!pressed) {
        this->beacon_spam_touch_active = false;
        this->beacon_spam_touch_region = -1;
      } else {
        int16_t region = -1;
        if (t_y >= 238)
          region = t_x < 100 ? 1 : (t_x < 168 ? 2 : 3);
        if (region >= 0 &&
            (!this->beacon_spam_touch_active ||
             region != this->beacon_spam_touch_region ||
             currentTime - this->beacon_spam_touch_ms >= 700)) {
          this->beacon_spam_touch_active = true;
          this->beacon_spam_touch_region = region;
          this->beacon_spam_touch_ms = currentTime;
          this->handleBeaconSpamTouch(t_x, t_y);
        }
      }
      x = -1;
      y = -1;
      return;
    }

    if (this->probe_flood_ui) {
      if ((int32_t)(currentTime - this->probe_flood_touch_ready_at) < 0) {
        x = -1;
        y = -1;
        return;
      }

      const bool probe_back_touch = pressed &&
          ((t_x >= 176 && t_y >= 255) ||
           (t_y >= STATUS_BAR_WIDTH && t_y < 51));
      if (probe_back_touch) {
        this->exitProbeFloodUI();
        x = -1;
        y = -1;
        return;
      }

      if (!pressed) {
        this->probe_flood_touch_active = false;
        this->probe_flood_touch_region = -1;
      } else {
        int16_t region = -1;
        if (t_y >= 255)
          region = t_x < 70 ? 1 : (t_x < 124 ? 2 :
                   (t_x < 178 ? 3 : 4));
        else if (t_y >= 100 && t_y < 120)
          region = 9;  // list-header SCAN chip
        else if (t_y >= 120 && t_y < 243)
          region = 10 + (t_y - 123) / 24;  // one region per visible row
        if (region >= 0 &&
            (!this->probe_flood_touch_active ||
             region != this->probe_flood_touch_region ||
             currentTime - this->probe_flood_touch_ms >= 700)) {
          this->probe_flood_touch_active = true;
          this->probe_flood_touch_region = region;
          this->probe_flood_touch_ms = currentTime;
          this->handleProbeFloodTouch(t_x, t_y);
        }
      }
      x = -1;
      y = -1;
      return;
    }

    if (this->deauth_flood_ui) {
      if ((int32_t)(currentTime - this->deauth_flood_touch_ready_at) < 0) {
        x = -1;
        y = -1;
        return;
      }

      const bool deauth_back_touch = pressed &&
          ((t_x >= 176 && t_y >= 255) ||
           (t_y >= STATUS_BAR_WIDTH && t_y < 51));
      if (deauth_back_touch) {
        this->exitDeauthFloodUI();
        x = -1;
        y = -1;
        return;
      }

      if (!pressed) {
        this->deauth_flood_touch_active = false;
        this->deauth_flood_touch_region = -1;
      } else {
        int16_t region = -1;
        if (t_y >= 255)
          region = t_x < 70 ? 1 : (t_x < 124 ? 2 :
                   (t_x < 178 ? 3 : 4));
        else if (t_y >= 100 && t_y < 120)
          region = 9;  // list-header SCAN chip
        else if (t_y >= 120 && t_y < 243)
          region = 10 + (t_y - 123) / 24;  // one region per visible row
        if (region >= 0 &&
            (!this->deauth_flood_touch_active ||
             region != this->deauth_flood_touch_region ||
             currentTime - this->deauth_flood_touch_ms >= 700)) {
          this->deauth_flood_touch_active = true;
          this->deauth_flood_touch_region = region;
          this->deauth_flood_touch_ms = currentTime;
          this->handleDeauthFloodTouch(t_x, t_y);
        }
      }
      x = -1;
      y = -1;
      return;
    }

    if (this->attack_dash_ui) {
      if ((int32_t)(currentTime - this->attack_dash_touch_ready_at) < 0) {
        x = -1;
        y = -1;
        return;
      }

      const bool attack_back_touch = pressed &&
          ((t_x >= 176 && t_y >= 255) ||
           (t_y >= STATUS_BAR_WIDTH && t_y < 51));
      if (attack_back_touch) {
        this->exitAttackDashUI();
        x = -1;
        y = -1;
        return;
      }

      if (!pressed) {
        this->attack_dash_touch_active = false;
        this->attack_dash_touch_region = -1;
      } else {
        int16_t region = -1;
        if (t_y >= 255)
          region = t_x < 70 ? 1 : (t_x < 124 ? 2 :
                   (t_x < 178 ? 3 : 4));
        else if (t_y >= 100 && t_y < 120)
          region = 9;  // list-header SCAN chip
        else if (t_y >= 120 && t_y < 243)
          region = 10 + (t_y - 123) / 24;  // one region per visible row
        if (region >= 0 &&
            (!this->attack_dash_touch_active ||
             region != this->attack_dash_touch_region ||
             currentTime - this->attack_dash_touch_ms >= 700)) {
          this->attack_dash_touch_active = true;
          this->attack_dash_touch_region = region;
          this->attack_dash_touch_ms = currentTime;
          this->handleAttackDashTouch(t_x, t_y);
        }
      }
      x = -1;
      y = -1;
      return;
    }

    if (this->funny_beacon_ui) {
      if ((int32_t)(currentTime - this->funny_beacon_touch_ready_at) < 0) {
        x = -1;
        y = -1;
        return;
      }

      const bool funny_back_touch = pressed &&
          ((t_x >= 176 && t_y >= 238) ||
           (t_y >= STATUS_BAR_WIDTH && t_y < 51));
      if (funny_back_touch) {
        this->exitFunnyBeaconUI();
        x = -1;
        y = -1;
        return;
      }

      if (!pressed) {
        this->funny_beacon_touch_active = false;
        this->funny_beacon_touch_region = -1;
      } else {
        int16_t region = -1;
        if (t_y >= 238)
          region = t_x < 70 ? 1 : (t_x < 124 ? 2 :
                   (t_x < 178 ? 3 : 4));
        if (region >= 0 &&
            (!this->funny_beacon_touch_active ||
             region != this->funny_beacon_touch_region ||
             currentTime - this->funny_beacon_touch_ms >= 700)) {
          this->funny_beacon_touch_active = true;
          this->funny_beacon_touch_region = region;
          this->funny_beacon_touch_ms = currentTime;
          this->handleFunnyBeaconTouch(t_x, t_y);
        }
      }
      x = -1;
      y = -1;
      return;
    }

    if (this->rick_roll_ui) {
      if ((int32_t)(currentTime - this->rick_roll_touch_ready_at) < 0) {
        x = -1;
        y = -1;
        return;
      }

      const bool rick_back_touch = pressed &&
          ((t_x >= 176 && t_y >= 238) ||
           (t_y >= STATUS_BAR_WIDTH && t_y < 51));
      if (rick_back_touch) {
        this->exitRickRollUI();
        x = -1;
        y = -1;
        return;
      }

      if (!pressed) {
        this->rick_roll_touch_active = false;
        this->rick_roll_touch_region = -1;
      } else {
        int16_t region = -1;
        if (t_y >= 238)
          region = t_x < 70 ? 1 : (t_x < 124 ? 2 :
                   (t_x < 178 ? 3 : 4));
        if (region >= 0 &&
            (!this->rick_roll_touch_active ||
             region != this->rick_roll_touch_region ||
             currentTime - this->rick_roll_touch_ms >= 700)) {
          this->rick_roll_touch_active = true;
          this->rick_roll_touch_region = region;
          this->rick_roll_touch_ms = currentTime;
          this->handleRickRollTouch(t_x, t_y);
        }
      }
      x = -1;
      y = -1;
      return;
    }

    if (this->network_scan_ui) {
      if ((int32_t)(currentTime - this->network_scan_touch_ready_at) < 0) {
        x = -1;
        y = -1;
        return;
      }

      const bool network_back_touch = pressed &&
          ((t_x >= 154 && t_y >= 238) ||
           (t_y >= STATUS_BAR_WIDTH && t_y < 51));
      if (network_back_touch) {
        this->exitNetworkScannerUI();
        x = -1;
        y = -1;
        return;
      }

      if (!pressed) {
        this->network_scan_touch_active = false;
        this->network_scan_touch_region = -1;
      } else {
        int16_t region = -1;
        if (t_y >= 238)
          region = t_x < 56 ? 1 : (t_x < 112 ? 2 :
                   (t_x < 168 ? 3 : 4));
        if (region >= 0 &&
            (!this->network_scan_touch_active ||
             region != this->network_scan_touch_region ||
             currentTime - this->network_scan_touch_ms >= 700)) {
          this->network_scan_touch_active = true;
          this->network_scan_touch_region = region;
          this->network_scan_touch_ms = currentTime;
          this->handleNetworkScannerTouch(t_x, t_y);
        }
      }
      x = -1;
      y = -1;
      return;
    }

    if (pressed && this->wifi_passive_ui && wifi_scan_obj.isPassiveWifiMode()) {
      uint16_t release_x = t_x, release_y = t_y;
      while (display_obj.updateTouch(&release_x, &release_y)) delay(10);
      this->handlePassiveWifiDetectorTouch(t_x, t_y);
      x = -1;
      y = -1;
      return;
    }
    if (this->wifi_tool_ui &&
        this->isWifiToolUiMode(wifi_scan_obj.currentScanMode)) {
      // Discard only the short launch-touch tail. There is deliberately no
      // release gate here: a missed release sample must never freeze the UI.
      if ((int32_t)(currentTime - this->wifi_tool_touch_ready_at) < 0) {
        x = -1;
        y = -1;
        return;
      }

      // BACK never depends on debounce. The whole title strip plus the wide
      // lower-right area are emergency exits.
      const bool wifi_tool_back_touch =
          !this->wifi_packet_target_view && pressed &&
          ((t_x >= 154 && t_y >= 238) ||
           (t_y >= STATUS_BAR_WIDTH && t_y < 51));
      if (wifi_tool_back_touch) {
        this->exitWifiToolUI();
        x = -1;
        y = -1;
        return;
      }
      if (!pressed) {
        this->wifi_tool_touch_active = false;
        this->wifi_tool_touch_region = -1;
      } else {
        int16_t region = -1;
        if (this->wifi_packet_target_view) {
          if (t_y >= 270)
            region = 20 + min((uint8_t)2, (uint8_t)(t_x / 80));
          else if (t_y >= 76 && t_y < 266)
            region = 30 + min((uint8_t)4, (uint8_t)((t_y - 76) / 38));
        } else if (wifi_scan_obj.currentScanMode == WIFI_SCAN_PACKET_RATE &&
                   t_x >= 12 && t_x <= 228 && t_y >= 205 && t_y < 266) {
          region = 10;
        } else if (t_y >= 238) {
          region = t_x < 56 ? 1 : (t_x < 112 ? 2 :
                   (t_x < 154 ? 3 : 4));
        }

        // Only real controls enter debounce. A new region fires immediately;
        // the same region recovers after 700 ms if release was never sampled.
        if (region >= 0 &&
            (!this->wifi_tool_touch_active ||
             region != this->wifi_tool_touch_region ||
             currentTime - this->wifi_tool_touch_ms >= 700)) {
          this->wifi_tool_touch_active = true;
          this->wifi_tool_touch_region = region;
          this->wifi_tool_touch_ms = currentTime;
          this->handleWifiToolTouch(t_x, t_y);
        }
        x = -1;
        y = -1;
        return;
      }
    }
  #endif

  // This is if there are scans/attacks going on
  #ifdef HAS_ILI9341
    if ((wifi_scan_obj.currentScanMode != WIFI_SCAN_OFF) &&
        (pressed) &&
        (wifi_scan_obj.currentScanMode != WIFI_CONNECTED) &&
        (wifi_scan_obj.currentScanMode != OTA_UPDATE) &&
        (wifi_scan_obj.currentScanMode != ESP_UPDATE) &&
        (wifi_scan_obj.currentScanMode != SHOW_INFO) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_GPS_DATA) &&
        (wifi_scan_obj.currentScanMode != GPS_POI) &&
        (wifi_scan_obj.currentScanMode != GPS_TRACKER) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_GPS_NMEA))
    {
      // Stop the current scan
      if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_SAE_COMMIT) ||
          (wifi_scan_obj.currentScanMode == SHARK_RUVIEW_CSI) ||
          (wifi_scan_obj.currentScanMode == SHARK_DRONE_RID_SCAN) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_DETECT_FOLLOW) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_STATION_WAR_DRIVE) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_STATION) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_WAR_DRIVE) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_DISPLAY_AP_INFO) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_EVIL_PORTAL) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_AP_STA) ||
          (wifi_scan_obj.currentScanMode == WIFI_PING_SCAN) ||
          (wifi_scan_obj.currentScanMode == WIFI_ARP_SCAN) ||
          (wifi_scan_obj.currentScanMode == WIFI_PORT_SCAN_ALL) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_SSH) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_TELNET) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_DNS) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_SMTP) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_HTTP) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_HTTPS) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_RDP) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_FTP) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_SMB) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_MQTT) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_MYSQL) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_POSTGRES) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_VNC) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_REDIS) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_MQTTS) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_PWN) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_PINESCAN) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_MULTISSID) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_ESPRESSIF) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_ALL) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BEACON_SPAM) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_AP_SPAM) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_CSA) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_QUIET) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_AUTH) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_DEAUTH) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_DEAUTH_MANUAL) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_DEAUTH_TARGETED) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BAD_MSG_TARGETED) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BAD_MSG) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_SLEEP) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_SLEEP_TARGETED) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_SAE_COMMIT) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_MIMIC) ||
		      (wifi_scan_obj.currentScanMode == WIFI_ATTACK_FUNNY_BEACON) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_RICK_ROLL) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BEACON_LIST) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_ALL) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_FOX_HUNT) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_SIG_STREN) ||
          (wifi_scan_obj.currentScanMode == BT_ATTACK_FINDMY_LIVE) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_RAYBAN) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_AIRTAG) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_AIRTAG_MON) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_FLIPPER) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_SIMPLE) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_SIMPLE_TWO) ||
          (wifi_scan_obj.currentScanMode == BT_ATTACK_SOUR_APPLE) ||
          (wifi_scan_obj.currentScanMode == BT_ATTACK_APPLE_JUICE) ||
          (wifi_scan_obj.currentScanMode == BT_ATTACK_SWIFTPAIR_SPAM) ||
          (wifi_scan_obj.currentScanMode == BT_ATTACK_SPAM_ALL) ||
          (wifi_scan_obj.currentScanMode == BT_ATTACK_SAMSUNG_SPAM) ||
          (wifi_scan_obj.currentScanMode == BT_ATTACK_GOOGLE_SPAM) ||
          (wifi_scan_obj.currentScanMode == BT_ATTACK_FLIPPER_SPAM) ||
          (wifi_scan_obj.currentScanMode == BT_SPOOF_AIRTAG) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_WAR_DRIVE) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_WAR_DRIVE_CONT) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_SKIMMERS) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_ANALYZER))
      {
        wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  
        // If we don't do this, the text and button coordinates will be off
        display_obj.init();
  
        // Take us back to the menu
        changeMenu(current_menu, true);
      }
  
      x = -1;
      y = -1;
  
      return;
    }
  #endif

  #ifdef HAS_BUTTONS

    #if (C_BTN >= 0) && !defined(MARAUDER_CARDPUTER) && !defined(MARAUDER_CARDPUTER_ADV)
      bool c_btn_press = c_btn.justPressed();
    #elif defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)
      bool c_btn_press = this->isKeyPressed('(');
    #endif

    #ifndef HAS_ILI9341
    
      if ((c_btn_press) &&
          (wifi_scan_obj.currentScanMode != WIFI_SCAN_OFF) &&
          (wifi_scan_obj.currentScanMode != WIFI_CONNECTED) &&
          (wifi_scan_obj.currentScanMode != OTA_UPDATE) &&
          (wifi_scan_obj.currentScanMode != ESP_UPDATE) &&
          (wifi_scan_obj.currentScanMode != SHOW_INFO) &&
          (wifi_scan_obj.currentScanMode != WIFI_SCAN_GPS_DATA) &&
          (wifi_scan_obj.currentScanMode != GPS_POI) &&
          (wifi_scan_obj.currentScanMode != GPS_TRACKER) &&
          (wifi_scan_obj.currentScanMode != WIFI_SCAN_GPS_NMEA))
      {
        // Stop the current scan
        if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_PROBE) ||
            (wifi_scan_obj.currentScanMode == SHARK_RUVIEW_CSI) ||
            (wifi_scan_obj.currentScanMode == SHARK_DRONE_RID_SCAN) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_SAE_COMMIT) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_DETECT_FOLLOW) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_STATION_WAR_DRIVE) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_RAW_CAPTURE) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_STATION) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_AP) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_WAR_DRIVE) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_DISPLAY_AP_INFO) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_EVIL_PORTAL) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_SIG_STREN) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_FINDMY_LIVE) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_AP_STA) ||
            (wifi_scan_obj.currentScanMode == WIFI_PING_SCAN) ||
            (wifi_scan_obj.currentScanMode == WIFI_ARP_SCAN) ||
            (wifi_scan_obj.currentScanMode == WIFI_PORT_SCAN_ALL) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_SSH) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_TELNET) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_DNS) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_SMTP) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_HTTP) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_HTTPS) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_RDP) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_FTP) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_SMB) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_MQTT) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_MYSQL) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_POSTGRES) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_VNC) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_REDIS) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_MQTTS) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_PWN) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_PINESCAN) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_MULTISSID) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_ESPRESSIF) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_ALL) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_DEAUTH) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BEACON_SPAM) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_AP_SPAM) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_CSA) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_QUIET) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_AUTH) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_DEAUTH) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_DEAUTH_MANUAL) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_DEAUTH_TARGETED) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BAD_MSG_TARGETED) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BAD_MSG) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_SLEEP) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_SLEEP_TARGETED) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_SAE_COMMIT) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_MIMIC) ||
			      (wifi_scan_obj.currentScanMode == WIFI_ATTACK_FUNNY_BEACON) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_RICK_ROLL) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BEACON_LIST) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_ALL) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_FOX_HUNT) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_RAYBAN) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_AIRTAG) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_AIRTAG_MON) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_FLIPPER) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_FLOCK) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_SIMPLE) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_SIMPLE_TWO) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_SOUR_APPLE) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_APPLE_JUICE) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_SWIFTPAIR_SPAM) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_SPAM_ALL) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_SAMSUNG_SPAM) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_GOOGLE_SPAM) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_FLIPPER_SPAM) ||
            (wifi_scan_obj.currentScanMode == BT_SPOOF_AIRTAG) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_WAR_DRIVE) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_WAR_DRIVE_CONT) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_SKIMMERS) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_EAPOL) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_ACTIVE_EAPOL) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_ACTIVE_LIST_EAPOL) ||
            (wifi_scan_obj.currentScanMode == WIFI_PACKET_MONITOR) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ANALYZER) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ACT) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_PACKET_RATE) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_ANALYZER))
        {
          wifi_scan_obj.StartScan(WIFI_SCAN_OFF);

          // Restore display state without full reinit to avoid screen flash
          #if defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)
            display_obj.tft.setRotation(SCREEN_ORIENTATION);
            display_obj.clearScreen();
          #else
            display_obj.init();
          #endif

          // Take us back to the menu
          changeMenu(current_menu, true);
        }
    
        x = -1;
        y = -1;
    
        return;
      }
    #endif

  #endif


  // Check if any key coordinate boxes contain the touch coordinates
  // This is for when on a menu
  // Make sure to add certain scanning functions here or else
  // menu items will be selected while scans and attacks are running
  #ifdef HAS_ILI9341
    if ((wifi_scan_obj.currentScanMode != WIFI_ATTACK_BEACON_SPAM) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_AP_SPAM) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_CSA) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_QUIET) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_AUTH) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_DEAUTH) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_DEAUTH_MANUAL) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_DEAUTH_TARGETED) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_BAD_MSG_TARGETED) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_BAD_MSG) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_SLEEP) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_SLEEP_TARGETED) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_SAE_COMMIT) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_MIMIC) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_PACKET_RATE) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_RAW_CAPTURE) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_CHAN_ANALYZER) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_CHAN_ACT) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_SIG_STREN) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_AP) &&
        (wifi_scan_obj.currentScanMode != BT_SCAN_FLOCK) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_PROBE) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_DEAUTH) &&
		    (wifi_scan_obj.currentScanMode != WIFI_ATTACK_FUNNY_BEACON) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_EAPOL) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_RICK_ROLL))
    {
      // Need this to set all keys to false
      /*for (uint8_t b = 0; b < BUTTON_ARRAY_LEN; b++) {
        if (pressed && display_obj.key[b].contains(t_x, t_y)) {
          display_obj.key[b].press(true);  // tell the button it is pressed
        } else {
          display_obj.key[b].press(false);  // tell the button it is NOT pressed
        }
      }*/

      #ifdef MARAUDER_V8
      // The v8 grid is direct-touch: each visible tile owns its hit target.
      // Page controls remain explicit in the footer, with no invisible zones.
      // SHOW_INFO must be here too: Device Info draws its infoMenu (whose
      // only item is Back) but sets scan mode SHOW_INFO -- without it in
      // the allow-list every tap on that screen was ignored.
      if (current_menu && current_menu->list &&
          ((wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) ||
           (wifi_scan_obj.currentScanMode == WIFI_CONNECTED) ||
           (wifi_scan_obj.currentScanMode == OTA_UPDATE) ||
           (wifi_scan_obj.currentScanMode == SHOW_INFO))) {
        const uint8_t visible_buttons = min(BUTTON_SCREEN_LIMIT,
                                            current_menu->list->size() - this->menu_start_index);
        int16_t released_node = -1;

        for (uint8_t b = 0; b < visible_buttons; b++) {
          const bool contains_touch = pressed && display_obj.key[b].contains(t_x, t_y);
          display_obj.key[b].press(contains_touch);
        }

        for (uint8_t b = 0; b < visible_buttons; b++) {
          const int16_t node_index = this->menu_start_index + b;
          if (display_obj.key[b].justPressed()) {
            const uint16_t previous = current_menu->selected;
            if (previous != node_index &&
                previous >= this->menu_start_index &&
                previous < this->menu_start_index + visible_buttons) {
              const MenuNode previous_node = current_menu->list->get(previous);
              const bool previous_is_setting = previous_node.icon == SETTINGS && previous_node.color == TFTLIGHTGREY;
              if (!previous_node.selected || previous_is_setting)
                this->buttonNotSelected(previous - this->menu_start_index, previous);
            }
            current_menu->selected = node_index;
            this->buttonSelected(b, node_index);
          }

          if (display_obj.key[b].justReleased() && !pressed)
            released_node = node_index;
        }

        if (released_node >= 0 && released_node < current_menu->list->size()) {
          current_menu->selected = released_node;
          std::function<void()> action = current_menu->list->get(released_node).callable;
          action();
          x = -1;
          y = -1;
          return;
        }

        const int8_t page_button = display_obj.menuButton(&t_x, &t_y, pressed);
        if (page_button >= 0) {
          int16_t new_start = this->menu_start_index;
          if (page_button == UP_BUTTON && this->menu_start_index > 0)
            new_start = max(0, this->menu_start_index - BUTTON_SCREEN_LIMIT);
          else if (page_button == DOWN_BUTTON &&
                   this->menu_start_index + BUTTON_SCREEN_LIMIT < current_menu->list->size())
            new_start = this->menu_start_index + BUTTON_SCREEN_LIMIT;

          if (new_start != this->menu_start_index) {
            current_menu->selected = new_start;
            this->buildButtons(current_menu, new_start);
            this->displayCurrentMenu(new_start);
          }
          else {
            this->displayMenuButtons();
          }
          x = -1;
          y = -1;
          return;
        }
      }
      #else
      // Detect up, down, select
      uint8_t menu_button = display_obj.menuButton(&t_x, &t_y, pressed);

      if (menu_button > -1) {
        if (menu_button == UP_BUTTON) {
          if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) ||
              (wifi_scan_obj.currentScanMode == WIFI_CONNECTED) ||
              (wifi_scan_obj.currentScanMode == OTA_UPDATE)) {
            if (current_menu->selected > 0) {
              current_menu->selected--;
              // Page up
              if (current_menu->selected < this->menu_start_index) {
                this->buildButtons(current_menu, current_menu->selected);
                this->displayCurrentMenu(current_menu->selected);
              }
              this->buttonSelected(current_menu->selected - this->menu_start_index, current_menu->selected);
              if (!current_menu->list->get(current_menu->selected + 1).selected || (current_menu->list->get(current_menu->selected + 1).icon == SETTINGS && current_menu->list->get(current_menu->selected + 1).color == TFTLIGHTGREY))
                this->buttonNotSelected(current_menu->selected + 1 - this->menu_start_index, current_menu->selected + 1);
            }
            // Loop to end
            else {
              current_menu->selected = current_menu->list->size() - 1;
              if (current_menu->selected >= BUTTON_SCREEN_LIMIT) {
                this->buildButtons(current_menu, current_menu->selected + 1 - BUTTON_SCREEN_LIMIT);
                this->displayCurrentMenu(current_menu->selected + 1 - BUTTON_SCREEN_LIMIT);
              }
              this->buttonSelected(current_menu->selected, current_menu->selected);
              if (!current_menu->list->get(0).selected || (current_menu->list->get(0).icon == SETTINGS && current_menu->list->get(0).color == TFTLIGHTGREY))
                this->buttonNotSelected(0, this->menu_start_index);
            }
          }
          else if ((wifi_scan_obj.currentScanMode == WIFI_PACKET_MONITOR) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_EAPOL) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ANALYZER) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_PACKET_RATE) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_RAW_CAPTURE) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_AP) ||
                  (wifi_scan_obj.currentScanMode == BT_SCAN_FLOCK) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_PROBE) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_DEAUTH) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_SIG_STREN)) {
            #ifndef HAS_DUAL_BAND
              if (wifi_scan_obj.set_channel < 14)
                wifi_scan_obj.changeChannel(wifi_scan_obj.set_channel + 1);
              else
                wifi_scan_obj.changeChannel(1);
            #else
              if (wifi_scan_obj.dual_band_channel_index < DUAL_BAND_CHANNELS - 1)
                wifi_scan_obj.dual_band_channel_index++;
              else
                wifi_scan_obj.dual_band_channel_index = 0;

              wifi_scan_obj.changeChannel(wifi_scan_obj.dual_band_channels[wifi_scan_obj.dual_band_channel_index]);
            #endif
          }
          else if (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ACT) {
            #ifndef HAS_DUAL_BAND
              if (wifi_scan_obj.activity_page < MAX_CHANNEL / CHAN_PER_PAGE) {
                wifi_scan_obj.activity_page++;
              }
            #else
              if (wifi_scan_obj.activity_page < DUAL_BAND_CHANNELS / CHAN_PER_PAGE) {
                wifi_scan_obj.activity_page++;
              }
            #endif
            wifi_scan_obj.drawChannelLine();
          }
        }
        if (menu_button == DOWN_BUTTON) {
          if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) ||
              (wifi_scan_obj.currentScanMode == WIFI_CONNECTED) ||
              (wifi_scan_obj.currentScanMode == OTA_UPDATE)) {
            if (current_menu->selected < current_menu->list->size() - 1) {
              current_menu->selected++;
              // Page down
              if (current_menu->selected - this->menu_start_index >= BUTTON_SCREEN_LIMIT) {
                this->buildButtons(current_menu, current_menu->selected + 1 - BUTTON_SCREEN_LIMIT);
                this->displayCurrentMenu(current_menu->selected + 1 - BUTTON_SCREEN_LIMIT);
              }
              else
                this->buttonSelected(current_menu->selected - this->menu_start_index, current_menu->selected);
              if (!current_menu->list->get(current_menu->selected - 1).selected || (current_menu->list->get(current_menu->selected - 1).icon == SETTINGS && current_menu->list->get(current_menu->selected - 1).color == TFTLIGHTGREY))
                this->buttonNotSelected(current_menu->selected - 1 - this->menu_start_index, current_menu->selected - 1);
            }
            // Loop to beginning
            else {
              if (current_menu->selected >= BUTTON_SCREEN_LIMIT) {
                current_menu->selected = 0;
                this->buildButtons(current_menu);
                this->displayCurrentMenu();
                this->buttonSelected(current_menu->selected);
              }
              else {
                current_menu->selected = 0;
                this->buttonSelected(current_menu->selected);
                if (!current_menu->list->get(current_menu->list->size() - 1).selected || (current_menu->list->get(current_menu->list->size() - 1).icon == SETTINGS && current_menu->list->get(current_menu->list->size() - 1).color == TFTLIGHTGREY))
                  this->buttonNotSelected(current_menu->list->size() - 1);
              }
            }
          }
          else if ((wifi_scan_obj.currentScanMode == WIFI_PACKET_MONITOR) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_EAPOL) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ANALYZER) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_PACKET_RATE) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_RAW_CAPTURE) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_AP) ||
                  (wifi_scan_obj.currentScanMode == BT_SCAN_FLOCK) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_PROBE) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_DEAUTH) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_SIG_STREN)) {
            #ifndef HAS_DUAL_BAND
              if (wifi_scan_obj.set_channel > 1)
                wifi_scan_obj.changeChannel(wifi_scan_obj.set_channel - 1);
              else
                wifi_scan_obj.changeChannel(14);
            #else
              if (wifi_scan_obj.dual_band_channel_index > 0)
                wifi_scan_obj.dual_band_channel_index--;
              else
                wifi_scan_obj.dual_band_channel_index = DUAL_BAND_CHANNELS - 1;

              wifi_scan_obj.changeChannel(wifi_scan_obj.dual_band_channels[wifi_scan_obj.dual_band_channel_index]);
            #endif
          }
          else if (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ACT) {
            #ifndef HAS_DUAL_BAND
              if (wifi_scan_obj.activity_page > 1) {
                wifi_scan_obj.activity_page--;
              }
            #else
              if (wifi_scan_obj.activity_page > 0) {
                wifi_scan_obj.activity_page--;
              }
            #endif
            wifi_scan_obj.drawChannelLine();
          }
        }
        if(menu_button == SELECT_BUTTON) {
          current_menu->list->get(current_menu->selected).callable();
        }
        else {
          if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) ||
              (wifi_scan_obj.currentScanMode == WIFI_CONNECTED))
            this->displayMenuButtons();
        }
      }
      #endif
    }
    x = -1;
    y = -1;
  #endif

  // Menu navigation and paging
  #ifdef HAS_BUTTONS
    // Don't do this for touch screens
    #if !(defined(MARAUDER_V6) || defined(MARAUDER_V6_1) || defined(MARAUDER_CYD_MICRO) || defined(MARAUDER_CYD_GUITION) || defined(MARAUDER_CYD_2USB) || defined(MARAUDER_CYD_3_5_INCH))
      #if !defined(MARAUDER_M5STICKC) || defined(MARAUDER_M5STICKCP2)
        #if (U_BTN >= 0 || defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV))
          #if (U_BTN >= 0)
            if (u_btn.justPressed()) {
          #elif defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)
            if (this->isKeyPressed(';')) {
          #endif
              if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) ||
                  (wifi_scan_obj.currentScanMode == WIFI_CONNECTED) ||
                  (wifi_scan_obj.currentScanMode == OTA_UPDATE)) {
                if (current_menu->selected > 0) {
                  current_menu->selected--;
                  // Page up
                  if (current_menu->selected < this->menu_start_index) {
                    this->buildButtons(current_menu, current_menu->selected);
                    this->displayCurrentMenu(current_menu->selected);
                  }
                  this->buttonSelected(current_menu->selected - this->menu_start_index, current_menu->selected);
                  if (!current_menu->list->get(current_menu->selected + 1).selected || (current_menu->list->get(current_menu->selected + 1).icon == SETTINGS && current_menu->list->get(current_menu->selected + 1).color == TFTLIGHTGREY))
                    this->buttonNotSelected(current_menu->selected + 1 - this->menu_start_index, current_menu->selected + 1);
                }
                // Loop to end
                else {
                  current_menu->selected = current_menu->list->size() - 1;
                  if (current_menu->selected >= BUTTON_SCREEN_LIMIT) {
                    this->buildButtons(current_menu, current_menu->selected + 1 - BUTTON_SCREEN_LIMIT);
                    this->displayCurrentMenu(current_menu->selected + 1 - BUTTON_SCREEN_LIMIT);
                  }
                  this->buttonSelected(current_menu->selected, current_menu->selected);
                  if (!current_menu->list->get(0).selected || (current_menu->list->get(0).icon == SETTINGS && current_menu->list->get(0).color == TFTLIGHTGREY))
                    this->buttonNotSelected(0, this->menu_start_index);
                }
              }
              else if ((wifi_scan_obj.currentScanMode == WIFI_PACKET_MONITOR) ||
                      (wifi_scan_obj.currentScanMode == WIFI_SCAN_EAPOL) ||
                      (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ANALYZER) ||
                      (wifi_scan_obj.currentScanMode == WIFI_SCAN_PACKET_RATE) ||
                      (wifi_scan_obj.currentScanMode == WIFI_SCAN_RAW_CAPTURE) ||
                      (wifi_scan_obj.currentScanMode == WIFI_SCAN_AP) ||
                      (wifi_scan_obj.currentScanMode == BT_SCAN_FLOCK) ||
                      (wifi_scan_obj.currentScanMode == WIFI_SCAN_PROBE) ||
                      (wifi_scan_obj.currentScanMode == WIFI_SCAN_DEAUTH) ||
                      (wifi_scan_obj.currentScanMode == WIFI_SCAN_SIG_STREN)) {
                #ifndef HAS_DUAL_BAND
                  if (wifi_scan_obj.set_channel < 14)
                    wifi_scan_obj.changeChannel(wifi_scan_obj.set_channel + 1);
                  else
                    wifi_scan_obj.changeChannel(1);
                #else
                  if (wifi_scan_obj.dual_band_channel_index < DUAL_BAND_CHANNELS - 1)
                    wifi_scan_obj.dual_band_channel_index++;
                  else
                    wifi_scan_obj.dual_band_channel_index = 0;

                  wifi_scan_obj.changeChannel(wifi_scan_obj.dual_band_channels[wifi_scan_obj.dual_band_channel_index]);
                #endif
              }
              else if (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ACT) {
                #ifndef HAS_DUAL_BAND
                  if (wifi_scan_obj.activity_page < MAX_CHANNEL / CHAN_PER_PAGE) {
                    wifi_scan_obj.activity_page++;
                  }
                #else
                  if (wifi_scan_obj.activity_page < DUAL_BAND_CHANNELS / CHAN_PER_PAGE) {
                    wifi_scan_obj.activity_page++;
                  }
                #endif
                wifi_scan_obj.drawChannelLine();
              }
            }
        #endif
      #endif

      #if (D_BTN >= 0 || defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV))
      #if (D_BTN >= 0)
      if (d_btn.justPressed()){
      #elif defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)
      if (this->isKeyPressed('.')){
      #endif
        if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) ||
            (wifi_scan_obj.currentScanMode == WIFI_CONNECTED) ||
            (wifi_scan_obj.currentScanMode == OTA_UPDATE)) {
          if (current_menu->selected < current_menu->list->size() - 1) {
            current_menu->selected++;
            // Page down
            if (current_menu->selected - this->menu_start_index >= BUTTON_SCREEN_LIMIT) {
              this->buildButtons(current_menu, current_menu->selected + 1 - BUTTON_SCREEN_LIMIT);
              this->displayCurrentMenu(current_menu->selected + 1 - BUTTON_SCREEN_LIMIT);
            }
            else
              this->buttonSelected(current_menu->selected - this->menu_start_index, current_menu->selected);
            if (!current_menu->list->get(current_menu->selected - 1).selected || (current_menu->list->get(current_menu->selected - 1).icon == SETTINGS && current_menu->list->get(current_menu->selected - 1).color == TFTLIGHTGREY))
              this->buttonNotSelected(current_menu->selected - 1 - this->menu_start_index, current_menu->selected - 1);
          }
          // Loop to beginning
          else {
            if (current_menu->selected >= BUTTON_SCREEN_LIMIT) {
              current_menu->selected = 0;
              this->buildButtons(current_menu);
              this->displayCurrentMenu();
              this->buttonSelected(current_menu->selected);
            }
            else {
              current_menu->selected = 0;
              this->buttonSelected(current_menu->selected);
              if (!current_menu->list->get(current_menu->list->size() - 1).selected || (current_menu->list->get(current_menu->list->size() - 1).icon == SETTINGS && current_menu->list->get(current_menu->list->size() - 1).color == TFTLIGHTGREY))
                this->buttonNotSelected(current_menu->list->size() - 1);
            }
          }
        }
        else if ((wifi_scan_obj.currentScanMode == WIFI_PACKET_MONITOR) ||
                (wifi_scan_obj.currentScanMode == WIFI_SCAN_EAPOL) ||
                (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ANALYZER) ||
                (wifi_scan_obj.currentScanMode == WIFI_SCAN_PACKET_RATE) ||
                (wifi_scan_obj.currentScanMode == WIFI_SCAN_RAW_CAPTURE) ||
                (wifi_scan_obj.currentScanMode == WIFI_SCAN_AP) ||
                (wifi_scan_obj.currentScanMode == BT_SCAN_FLOCK) ||
                (wifi_scan_obj.currentScanMode == WIFI_SCAN_PROBE) ||
                (wifi_scan_obj.currentScanMode == WIFI_SCAN_DEAUTH) ||
                (wifi_scan_obj.currentScanMode == WIFI_SCAN_SIG_STREN)) {
          #ifndef HAS_DUAL_BAND
            if (wifi_scan_obj.set_channel > 1)
              wifi_scan_obj.changeChannel(wifi_scan_obj.set_channel - 1);
            else
              wifi_scan_obj.changeChannel(14);
          #else
            if (wifi_scan_obj.dual_band_channel_index > 0)
              wifi_scan_obj.dual_band_channel_index--;
            else
              wifi_scan_obj.dual_band_channel_index = DUAL_BAND_CHANNELS - 1;

            wifi_scan_obj.changeChannel(wifi_scan_obj.dual_band_channels[wifi_scan_obj.dual_band_channel_index]);
          #endif
        }
        else if (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ACT) {
          #ifndef HAS_DUAL_BAND
            if (wifi_scan_obj.activity_page > 1) {
              wifi_scan_obj.activity_page--;
            }
          #else
            if (wifi_scan_obj.activity_page > 0) {
              wifi_scan_obj.activity_page--;
            }
          #endif
          wifi_scan_obj.drawChannelLine();
        }
      }
      #endif

      #if (R_BTN >= 0 || defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV))
      #if (R_BTN >= 0)
      if (r_btn.justPressed()) {
      #elif defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)
      if (this->isKeyPressed('/')) {
      #endif
        if (wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) {
          #ifndef HAS_DUAL_BAND
            if (wifi_scan_obj.set_channel < 14)
              wifi_scan_obj.changeChannel(wifi_scan_obj.set_channel + 1);
            else
              wifi_scan_obj.changeChannel(1);
          #else
            if (wifi_scan_obj.dual_band_channel_index < DUAL_BAND_CHANNELS - 1)
              wifi_scan_obj.dual_band_channel_index++;
            else
              wifi_scan_obj.dual_band_channel_index = 0;

            wifi_scan_obj.changeChannel(wifi_scan_obj.dual_band_channels[wifi_scan_obj.dual_band_channel_index]);
          #endif
        }
      }
      #endif

      #if (L_BTN >= 0 || defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV))
      #if (L_BTN >= 0)
      if (l_btn.justPressed()) {
      #elif defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)
      if (this->isKeyPressed(',')) {
      #endif
        if (wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) {
          #ifndef HAS_DUAL_BAND
            if (wifi_scan_obj.set_channel > 1)
              wifi_scan_obj.changeChannel(wifi_scan_obj.set_channel - 1);
            else
              wifi_scan_obj.changeChannel(14);
          #else
            if (wifi_scan_obj.dual_band_channel_index > 0)
              wifi_scan_obj.dual_band_channel_index--;
            else
              wifi_scan_obj.dual_band_channel_index = DUAL_BAND_CHANNELS - 1;

            wifi_scan_obj.changeChannel(wifi_scan_obj.dual_band_channels[wifi_scan_obj.dual_band_channel_index]);
          #endif
        }
      }
      #endif

      #if defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)
      if (this->isKeyPressed('`') || this->isKeyPressed(KEY_BACKSPACE)) {
        if (wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) {
          if (current_menu->parentMenu != NULL) {
            this->changeMenu(current_menu->parentMenu, true);
          }
        }
      }
      #endif

      if(c_btn_press){
        current_menu->list->get(current_menu->selected).callable();
      }

    #endif
  #endif
}

#if BATTERY_ANALOG_ON == 1
byte battery_analog_array[10];
byte battery_count = 0;
byte battery_analog_last = 101;
#define BATTERY_CHECK 50
uint16_t battery_analog = 0;
void MenuFunctions::battery(bool initial)
{
  if (BATTERY_ANALOG_ON) {
    uint8_t n = 0;
    byte battery_analog_sample[10];
    byte deviation;
    if (battery_count == BATTERY_CHECK - 5)  digitalWrite(BATTERY_PIN, HIGH);
    else if (battery_count == 5) digitalWrite(BATTERY_PIN, LOW);
    if (battery_count == 0) {
      battery_analog = 0;
      for (n = 9; n > 0; n--)battery_analog_array[n] = battery_analog_array[n - 1];
      for (n = 0; n < 10; n++) {
        battery_analog_sample[n] = map((analogRead(ANALOG_PIN) * 5), 2400, 4200, 0, 100);
        if (battery_analog_sample[n] > 100) battery_analog_sample[n] = 100;
        else if (battery_analog_sample[n] < 0) battery_analog_sample[n] = 0;
        battery_analog += battery_analog_sample[n];
      }
      battery_analog = battery_analog / 10;
      for (n = 0; n < 10; n++) {
        deviation = abs(battery_analog - battery_analog_sample[n]);
        if (deviation >= 10) battery_analog_sample[n] = battery_analog;
      }
      battery_analog = 0;
      for (n = 0; n < 10; n++) battery_analog += battery_analog_sample[n];
      battery_analog = battery_analog / 10;
      battery_analog_array[0] = battery_analog;
      if (battery_analog_array[9] > 0 ) {
        battery_analog = 0;
        for (n = 0; n < 10; n++) battery_analog += battery_analog_array[n];
        battery_analog = battery_analog / 10;
      }
      battery_count ++;
    }
    else if (battery_count < BATTERY_CHECK) battery_count++;
    else if (battery_count >= BATTERY_CHECK) battery_count = 0;

    if (battery_analog_last != battery_analog) {
      battery_analog_last = battery_analog;
      MenuFunctions::battery2();
    }
  }
}
void MenuFunctions::battery2(bool initial)
{
  uint16_t the_color;
  if ( digitalRead(CHARGING_PIN) == 1) the_color = TFT_BLUE;
  else if (battery_analog < 20) the_color = TFT_RED;
  else if (battery_analog < 40)  the_color = TFT_YELLOW;
  else the_color = TFT_GREEN;

  display_obj.tft.setTextColor(the_color, STATUSBAR_COLOR);
  display_obj.tft.fillRect(SB_TOUCH_X, 0, 50, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
  display_obj.tft.drawXBitmap(SB_TOUCH_X,
                              0,
                              menu_icons[STATUS_BAT],
                              16,
                              16,
                              STATUSBAR_COLOR,
                              the_color);
  display_obj.tft.drawString((String) battery_analog + "%", SB_BAT_X, 0, 2);
}
#else
void MenuFunctions::battery(bool initial)
{
  #ifdef HAS_BATTERY
    uint16_t the_color;
    if (battery_obj.i2c_supported)
    {
      // Could use int compare maybe idk
      if (((String)battery_obj.battery_level != "25") && ((String)battery_obj.battery_level != "0"))
        the_color = TFT_GREEN;
      else
        the_color = TFT_RED;

      if ((battery_obj.battery_level != battery_obj.old_level) || (initial)) {
        battery_obj.old_level = battery_obj.battery_level;
        display_obj.tft.fillRect(204, 0, SCREEN_WIDTH, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
      }

      display_obj.tft.setCursor(0, 1);
      /*if (!this->disable_touch) {
        display_obj.tft.drawXBitmap(SB_TOUCH_X,
                                    0,
                                    menu_icons[STATUS_BAT],
                                    16,
                                    16,
                                    STATUSBAR_COLOR,
                                    the_color);
      }*/
      #if defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)
        display_obj.tft.drawString((String)battery_obj.battery_level + "%", SB_BAT_X, 0, 1);
      #else
        display_obj.tft.drawString((String)battery_obj.battery_level + "%", SB_BAT_X, 0, 2);
      #endif
    }
  #endif
}
void MenuFunctions::battery2(bool initial)
{
  MenuFunctions::battery(initial);
}
#endif

void MenuFunctions::updateStatusBar()
{
  #ifdef MARAUDER_V8
    this->drawSharkTopBar(false);
    return;
  #endif

  display_obj.tft.setTextSize(1);

  bool status_changed = false;
  
  #if defined(MARAUDER_MINI) || defined(MARAUDER_M5STICKC) || defined(MARAUDER_REV_FEATHER) || defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV) || defined(MARAUDER_MINI_V3)
    display_obj.tft.setFreeFont(NULL);
  #endif

  uint16_t the_color; 

  #ifdef HAS_GPS
    if (this->old_gps_sat_count != gps_obj.getNumSats()) {
      this->old_gps_sat_count = gps_obj.getNumSats();
      display_obj.tft.fillRect(0, 0, SCREEN_WIDTH, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
      status_changed = true;
    }
  #endif

  // GPS Stuff
  #ifdef HAS_GPS
    if (gps_obj.getGpsModuleStatus()) {
      if (gps_obj.getFixStatus())
        the_color = TFT_GREEN;
      else
        the_color = TFT_RED;
        
      #ifdef HAS_FULL_SCREEN
        display_obj.tft.drawXBitmap(4,
                                    0,
                                    menu_icons[STATUS_GPS],
                                    16,
                                    16,
                                    STATUSBAR_COLOR,
                                    the_color);
        display_obj.tft.setTextColor(TFT_WHITE, STATUSBAR_COLOR, true);

        display_obj.tft.drawString(gps_obj.getNumSatsString(), 22, 0, 2);
      #elif defined(HAS_SCREEN)
        display_obj.tft.setTextColor(the_color, STATUSBAR_COLOR, true);
        display_obj.tft.drawString("GPS", 0, 0, 1);
      #endif
    }
  #endif

  display_obj.tft.setTextColor(TFT_WHITE, STATUSBAR_COLOR, true);

  // WiFi Channel Stuff
  uint8_t primaryChannel;
  wifi_second_chan_t secondChannel;
  esp_err_t err = esp_wifi_get_channel(&primaryChannel, &secondChannel);

  uint8_t current_channel = wifi_scan_obj.set_channel;

  if (err == ESP_OK)
    current_channel = primaryChannel;

  if ((current_channel != wifi_scan_obj.old_channel) || (status_changed)) {
    wifi_scan_obj.old_channel = current_channel;
    #if defined(MARAUDER_MINI) || defined(MARAUDER_M5STICKC) || defined(MARAUDER_REV_FEATHER) || defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV) || defined(MARAUDER_MINI_V3)
      display_obj.tft.fillRect(TFT_WIDTH/4, 0, CHAR_WIDTH * 6, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
    #elif defined(HAS_DUAL_BAND)
      display_obj.tft.fillRect(50, 0, (CHAR_WIDTH / 2) * 8, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
    #else
      display_obj.tft.fillRect(50, 0, (CHAR_WIDTH / 2) * 7, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
    #endif
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawString("CH: " + (String)wifi_scan_obj.old_channel, 50, 0, 2);
    #endif

    #ifdef HAS_MINI_SCREEN
      display_obj.tft.drawString("CH:" + (String)wifi_scan_obj.old_channel, TFT_WIDTH/4, 0, 1);
    #endif
  }

  // RAM Stuff
  wifi_scan_obj.free_ram = String(esp_get_free_heap_size());
  if ((wifi_scan_obj.free_ram != wifi_scan_obj.old_free_ram) || (status_changed)) {
    wifi_scan_obj.old_free_ram = wifi_scan_obj.free_ram;
    //display_obj.tft.fillRect(SB_MEM_X, 0, 60, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
    #ifdef HAS_FULL_SCREEN
    #ifndef HAS_PSRAM
      display_obj.tft.drawString("D:" + String(getDRAMUsagePercent()) + "%", SB_MEM_X, 0, 2);
    #else
      display_obj.tft.drawString("D:" + String(getDRAMUsagePercent()) + "%", SB_MEM_X, 0, 1);
      display_obj.tft.drawString("P:" + String(getPSRAMUsagePercent()) + "%", SB_MEM_X, 8, 1);
    #endif
  #endif

  #ifdef HAS_MINI_SCREEN
    display_obj.tft.drawString(String(getDRAMUsagePercent()) + "%", TFT_WIDTH/1.75, 0, 1);
  #endif
  }

  // Draw battery info
  MenuFunctions::battery(false);
  display_obj.tft.fillRect(186, 0, 16, STATUS_BAR_WIDTH, STATUSBAR_COLOR);

  // Disable touch stuff
  #ifdef HAS_ILI9341
    #ifdef HAS_BUTTONS
      if (this->disable_touch) {
        display_obj.tft.setCursor(0, 1);
        display_obj.tft.drawXBitmap(SB_TOUCH_X,
                                    0,
                                    menu_icons[DISABLE_TOUCH],
                                    16,
                                    16,
                                    STATUSBAR_COLOR,
                                    TFT_RED);
      }
      else {
        display_obj.tft.setCursor(0, 1);
        display_obj.tft.drawXBitmap(SB_TOUCH_X,
                                    0,
                                    menu_icons[DISABLE_TOUCH],
                                    16,
                                    16,
                                    STATUSBAR_COLOR,
                                    TFT_DARKGREY);
      }
    #endif
  #endif

  // Draw SD info
  #ifdef HAS_SD
    if (sd_obj.supported)
      the_color = TFT_GREEN;
    else
      the_color = TFT_RED;

    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawXBitmap(SB_SD_X,
                                  0,
                                  menu_icons[STATUS_SD],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  the_color);
    #endif
  #endif

  #ifdef HAS_MINI_SCREEN
    display_obj.tft.setTextColor(the_color, STATUSBAR_COLOR, true);
    display_obj.tft.drawString("SD", TFT_WIDTH - 12, 0, 1);
  #endif

  // WiFi connection status stuff
  if (wifi_scan_obj.wifi_connected) {
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawXBitmap(SB_WIFI_X,
                                  0,
                                  menu_icons[JOINED],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  TFT_GREEN);
    #endif
  } else {
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawXBitmap(SB_WIFI_X,
                                  0,
                                  menu_icons[JOINED],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  TFT_DARKGREY);
    #endif
  }

  // Force PMKID stuff
  if ((wifi_scan_obj.force_pmkid) || (wifi_scan_obj.ep_deauth)) {
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawXBitmap(SB_FORCE_X,
                                  0,
                                  menu_icons[FORCE],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  TFT_GREEN);
    #endif
  } else {
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawXBitmap(SB_FORCE_X,
                                  0,
                                  menu_icons[FORCE],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  TFT_DARKGREY);
    #endif
  }
}

#ifdef MARAUDER_V8
void MenuFunctions::setSharkHudActivity(uint8_t activity_mask, bool active)
{
  if (active)
    this->shark_hud_tool_activity |= activity_mask;
  else
    this->shark_hud_tool_activity &= ~activity_mask;

  // Blocking tools do not return to the Arduino loop while they run, so make
  // their real radio state visible immediately instead of waiting for exit.
  this->drawSharkTopBar(true);
}
#endif

void MenuFunctions::drawStatusBar()
{
  #ifdef MARAUDER_V8
    this->drawSharkTopBar(true);
    return;
  #endif

  display_obj.tft.setTextSize(1);
  #ifdef HAS_MINI_SCREEN
    display_obj.tft.setFreeFont(NULL);
  #endif
  display_obj.tft.fillRect(0, 0, SCREEN_WIDTH, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
  display_obj.tft.setTextColor(TFT_WHITE, STATUSBAR_COLOR);

  uint16_t the_color;

  // GPS Stuff
  #ifdef HAS_GPS
    if (gps_obj.getGpsModuleStatus()) {
      if (gps_obj.getFixStatus())
        the_color = TFT_GREEN;
      else
        the_color = TFT_RED;
        
      #ifdef HAS_FULL_SCREEN
        display_obj.tft.drawXBitmap(4,
                                    0,
                                    menu_icons[STATUS_GPS],
                                    16,
                                    16,
                                    STATUSBAR_COLOR,
                                    the_color);
        display_obj.tft.setTextColor(TFT_WHITE, STATUSBAR_COLOR);

        display_obj.tft.drawString(gps_obj.getNumSatsString(), 22, 0, 2);
      #endif
    }
  #endif

  display_obj.tft.setTextColor(TFT_WHITE, STATUSBAR_COLOR);


  // WiFi Channel Stuff
  uint8_t primaryChannel;
  wifi_second_chan_t secondChannel;
  esp_err_t err = esp_wifi_get_channel(&primaryChannel, &secondChannel);

  if (err == ESP_OK)
    wifi_scan_obj.old_channel = primaryChannel;
  else
    wifi_scan_obj.old_channel = wifi_scan_obj.set_channel;

  #ifdef HAS_MINI_SCREEN
    display_obj.tft.fillRect(43, 0, TFT_WIDTH * 0.21, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
  #else
    display_obj.tft.fillRect(50, 0, TFT_WIDTH * 0.21, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
  #endif
  #ifdef HAS_FULL_SCREEN
    display_obj.tft.drawString("CH: " + (String)wifi_scan_obj.old_channel, 50, 0, 2);
  #endif

  #ifdef HAS_MINI_SCREEN
    display_obj.tft.drawString("CH:" + (String)wifi_scan_obj.old_channel, TFT_WIDTH/4, 0, 1);
  #endif

  // RAM Stuff
  wifi_scan_obj.free_ram = String(esp_get_free_heap_size());
  wifi_scan_obj.old_free_ram = wifi_scan_obj.free_ram;
  display_obj.tft.fillRect(100, 0, 60, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
  #ifdef HAS_FULL_SCREEN
    #ifndef HAS_PSRAM
      display_obj.tft.drawString("D:" + String(getDRAMUsagePercent()) + "%", SB_MEM_X, 0, 2);
    #else
      display_obj.tft.drawString("D:" + String(getDRAMUsagePercent()) + "%", SB_MEM_X, 0, 1);
      display_obj.tft.drawString("P:" + String(getPSRAMUsagePercent()) + "%", SB_MEM_X, 8, 1);
    #endif
  #endif

  #ifdef HAS_MINI_SCREEN
    display_obj.tft.drawString(String(getDRAMUsagePercent()) + "%", TFT_WIDTH/1.75, 0, 1);
  #endif


  MenuFunctions::battery(true);
  display_obj.tft.fillRect(186, 0, 16, STATUS_BAR_WIDTH, STATUSBAR_COLOR);


  // Disable touch stuff
  #ifdef HAS_ILI9341
    #ifdef HAS_BUTTONS
      if (this->disable_touch) {
        display_obj.tft.setCursor(0, 1);
        display_obj.tft.drawXBitmap(SB_TOUCH_X,
                                    0,
                                    menu_icons[DISABLE_TOUCH],
                                    16,
                                    16,
                                    STATUSBAR_COLOR,
                                    TFT_RED);
      }
      else {
        display_obj.tft.setCursor(0, 1);
        display_obj.tft.drawXBitmap(SB_TOUCH_X,
                                    0,
                                    menu_icons[DISABLE_TOUCH],
                                    16,
                                    16,
                                    STATUSBAR_COLOR,
                                    TFT_DARKGREY);
      }
    #endif
  #endif

  // Draw SD info
  #ifdef HAS_SD
    if (sd_obj.supported)
      the_color = TFT_GREEN;
    else
      the_color = TFT_RED;
  

    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawXBitmap(SB_SD_X,
                                  0,
                                  menu_icons[STATUS_SD],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  the_color);
    #endif
  #endif

  #ifdef HAS_MINI_SCREEN
    display_obj.tft.setTextColor(the_color, STATUSBAR_COLOR);
    display_obj.tft.drawString("SD", TFT_WIDTH - 12, 0, 1);
  #endif

  // WiFi connection status stuff
  if (wifi_scan_obj.wifi_connected) {
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawXBitmap(SB_WIFI_X,
                                  0,
                                  menu_icons[JOINED],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  TFT_GREEN);
    #endif
  } else {
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawXBitmap(SB_WIFI_X,
                                  0,
                                  menu_icons[JOINED],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  TFT_DARKGREY);
    #endif
  }

  // Force PMKID stuff
  if ((wifi_scan_obj.force_pmkid) || (wifi_scan_obj.ep_deauth)) {
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawXBitmap(SB_FORCE_X,
                                  0,
                                  menu_icons[FORCE],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  TFT_GREEN);
    #endif
  } else {
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawXBitmap(SB_FORCE_X,
                                  0,
                                  menu_icons[FORCE],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  TFT_DARKGREY);
    #endif
  }
}

void MenuFunctions::orientDisplay() {
  display_obj.init();

  display_obj.tft.setRotation(SCREEN_ORIENTATION); // Portrait

  display_obj.tft.setCursor(0, 0);

  #ifdef HAS_ILI9341
    #ifndef HAS_CYD_TOUCH
      display_obj.setCalData();
    #else
      display_obj.touchscreen.setRotation(0);
    #endif
  #endif

  changeMenu(current_menu, true);
}

const char* MenuFunctions::callSetting(const char* key) {
  specSettingMenu.name = key;

  const char* setting_type = settings_obj.getSettingType(key);

  if (setting_type && strcmp(setting_type, "bool") == 0) {
    return "bool";
  }

  return "";
}

/*void MenuFunctions::displaySetting(String key, Menu* menu, int index) {
  specSettingMenu.name = key;

  bool setting_value = settings_obj.loadSetting<bool>(key);

  // Make a local copy of menu node
  MenuNode node = menu->list->get(index);

  display_obj.tft.setTextWrap(false);
  display_obj.tft.setFreeFont(NULL);
  display_obj.tft.setCursor(0, 100);
  display_obj.tft.setTextSize(1);

  // Set local copy value
  if (!setting_value) {
    display_obj.tft.setTextColor(TFT_RED);
    display_obj.tft.println(F(text_table1[4]));
    node.selected = false;
  }
  else {
    display_obj.tft.setTextColor(TFT_GREEN);
    display_obj.tft.println(F(text_table1[5]));
    node.selected = true;
  }

  // Put local copy back into menu
  menu->list->set(index, node);
    
}*/

void MenuFunctions::displaySetting(const char* key, Menu* menu, int index) {
  specSettingMenu.name = String(key);

  bool setting_value = settings_obj.loadSetting<bool>(key);

  // Make a local copy of menu node
  MenuNode node = menu->list->get(index);

  display_obj.tft.setTextWrap(false);
  display_obj.tft.setFreeFont(NULL);
  display_obj.tft.setCursor(0, 100);
  display_obj.tft.setTextSize(1);

  // Set local copy value
  if (!setting_value) {
    display_obj.tft.setTextColor(TFT_RED);
    display_obj.tft.println(F(text_table1[4]));
    node.selected = false;
  } else {
    display_obj.tft.setTextColor(TFT_GREEN);
    display_obj.tft.println(F(text_table1[5]));
    node.selected = true;
  }

  // Put local copy back into menu
  menu->list->set(index, node);
}

#if defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)
void MenuFunctions::updateKeyboard()
{
  M5CardputerKeyboard.updateKeyList();
  M5CardputerKeyboard.updateKeysState();
}

bool MenuFunctions::isKeyPressed(char c)
{
  bool pressed = M5CardputerKeyboard.isKeyPressed(c);

  if (pressed)
    delay(200);

  return pressed;
}
#endif

#ifdef HAS_DIRECT_UPLOAD
  void MenuFunctions::buildUploadFileMenu() {
    if (sd_obj.supported) {
      this->setupSDFileList();

      uploadLogsMenu.list->clear();
      delete uploadLogsMenu.list;
      uploadLogsMenu.list = new LinkedList<MenuNode>();
      uploadLogsMenu.name = "Logs";

      uploadLogsMenu.parentMenu = &wifiGeneralMenu;

      this->addNodes(&uploadLogsMenu, "Back", TFTLIGHTGREY, 0, [this]() {
        this->changeMenu(uploadLogsMenu.parentMenu, true);
      });

      this->addNodes(&uploadLogsMenu, "Delete Wardrive Logs", TFTORANGE, 0, [this]() {
        this->changeMenu(&deleteAllMenu, true);
      });

      this->addNodes(&uploadLogsMenu, "Upload All", TFTGREEN, 0, [this]() {
        this->changeMenu(&uploadAllMenu, true);
        
      });

      for (int i = 0; i < sd_obj.sd_files->size(); i++) {
        File current_file = sd_obj.getFile("/" + sd_obj.sd_files->get(i));
        if (sd_obj.sd_files->get(i).startsWith("wardrive_") || sd_obj.sd_files->get(i).startsWith("wigle-")) {
          if (!sd_obj.sd_files->get(i).endsWith(".wdg") && !sd_obj.sd_files->get(i).endsWith(".wigle") && !sd_obj.sd_files->get(i).endsWith(".gpx")) {
            this->addNodes(&uploadLogsMenu, sd_obj.sd_files->get(i).c_str(), TFTCYAN, 0, [this, i]() {
              sd_obj.selected_file_name = sd_obj.sd_files->get(i);
              Serial.println(sd_obj.sd_files->get(i) + " selected");
              this->changeMenu(&actionMenu, true);
            });
          }
        }
      }

      Serial.println("Built SD file menu with " + (String)sd_obj.sd_files->size() + " files");
    } else {
      Serial.println("SD Card not detected. Skipping menu creation...");
    }
  }
#endif

// Function to build the menus
void MenuFunctions::RunSetup()
{
  extern LinkedList<AccessPoint>* access_points;
  extern LinkedList<Station>* stations;
  extern LinkedList<AirTag>* airtags;
  extern LinkedList<IPAddress>* ipList;
  extern LinkedList<ProbeReqSsid>* probe_req_ssids;
  extern LinkedList<ssid>* ssids;
  extern LinkedList<BleDevice>* ble_devices;
  #ifdef MARAUDER_V8
    sharkProfileBegin();
  #endif

  this->disable_touch = false;

  #if defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)
    M5CardputerKeyboard.begin();
  #endif
   
  // root menu stuff
  mainMenu.list = new LinkedList<MenuNode>(); // Get list in first menu ready

  // Main menu stuff
  wifiMenu.list = new LinkedList<MenuNode>(); // Get list in second menu ready
#ifdef HAS_BT
  bluetoothMenu.list = new LinkedList<MenuNode>(); // Get list in third menu ready
#endif
  deviceMenu.list = new LinkedList<MenuNode>();
  #ifdef MARAUDER_V8
    themeMenu.list = new LinkedList<MenuNode>();
  #endif
  #ifdef HAS_GPS
    if (gps_obj.getGpsModuleStatus()) {
      gpsMenu.list = new LinkedList<MenuNode>();
      gpsInfoMenu.list = new LinkedList<MenuNode>();
    }
  #endif

  // Device menu stuff
  failedUpdateMenu.list = new LinkedList<MenuNode>();
  confirmMenu.list = new LinkedList<MenuNode>();
  updateMenu.list = new LinkedList<MenuNode>();
  settingsMenu.list = new LinkedList<MenuNode>();
  specSettingMenu.list = new LinkedList<MenuNode>();
  infoMenu.list = new LinkedList<MenuNode>();
  // WiFi menu stuff
  wifiSnifferMenu.list = new LinkedList<MenuNode>();
  sharkDefenseMenu.list = new LinkedList<MenuNode>();
  #ifdef MARAUDER_V8
    ruviewMenu.list = new LinkedList<MenuNode>();
    prankMenu.list = new LinkedList<MenuNode>();
    wifiPrankMenu.list = new LinkedList<MenuNode>();
    btPrankMenu.list = new LinkedList<MenuNode>();
  #endif
  wifiScannerMenu.list = new LinkedList<MenuNode>();
  wifiAttackMenu.list = new LinkedList<MenuNode>();
  /*#ifdef HAS_GPS
    wardrivingMenu.list = new LinkedList<MenuNode>();
  #endif*/
  wifiGeneralMenu.list = new LinkedList<MenuNode>();
  wifiAPMenu.list = new LinkedList<MenuNode>();
  wifiIPMenu.list = new LinkedList<MenuNode>();
  apInfoMenu.list = new LinkedList<MenuNode>();
  setMacMenu.list = new LinkedList<MenuNode>();
  genAPMacMenu.list = new LinkedList<MenuNode>();
  wifiStationMenu.list = new LinkedList<MenuNode>();
  selectProbeSSIDsMenu.list = new LinkedList<MenuNode>();

  // WiFi HTML menu stuff
  htmlMenu.list = new LinkedList<MenuNode>();
  miniKbMenu.list = new LinkedList<MenuNode>();
  #ifdef HAS_SD
    sdDeleteMenu.list = new LinkedList<MenuNode>();
  #endif

  // Bluetooth menu stuff
  bluetoothSnifferMenu.list = new LinkedList<MenuNode>();
  bluetoothAttackMenu.list = new LinkedList<MenuNode>();

  // Settings stuff
  generateSSIDsMenu.list = new LinkedList<MenuNode>();
  clearSSIDsMenu.list = new LinkedList<MenuNode>();
  clearAPsMenu.list = new LinkedList<MenuNode>();
  saveFileMenu.list = new LinkedList<MenuNode>();

  #ifdef HAS_DIRECT_UPLOAD
    uploadLogsMenu.list = new LinkedList<MenuNode>();
    uploadAllMenu.list = new LinkedList<MenuNode>();
    deleteAllMenu.list = new LinkedList<MenuNode>();
    actionMenu.list = new LinkedList<MenuNode>();
  #endif

  saveSSIDsMenu.list = new LinkedList<MenuNode>();
  loadSSIDsMenu.list = new LinkedList<MenuNode>();
  saveAPsMenu.list = new LinkedList<MenuNode>();
  loadAPsMenu.list = new LinkedList<MenuNode>();
  saveATsMenu.list = new LinkedList<MenuNode>();
  loadATsMenu.list = new LinkedList<MenuNode>();

  evilPortalMenu.list = new LinkedList<MenuNode>();
  ssidsMenu.list = new LinkedList<MenuNode>();

  #ifdef HAS_GPS
    gpsPOIMenu.list = new LinkedList<MenuNode>();
  #endif

  foxHuntMenu.list = new LinkedList<MenuNode>();

  // Work menu names
  mainMenu.name = text_table1[6];
  wifiMenu.name = text_table1[7];
  deviceMenu.name = text_table1[9];
  #ifdef MARAUDER_V8
    themeMenu.name = "Theme";
  #endif
  failedUpdateMenu.name = text_table1[11];
  confirmMenu.name = text_table1[13];
  updateMenu.name = text_table1[15];
  infoMenu.name = text_table1[17];
  settingsMenu.name = text_table1[18];
  bluetoothMenu.name = text_table1[19];
  wifiSnifferMenu.name = text_table1[20];
  sharkDefenseMenu.name = "Cyber Defense";
  #ifdef MARAUDER_V8
    ruviewMenu.name = "RuView // Connect";
  #endif
  wifiScannerMenu.name = "Scanners";
  wifiAttackMenu.name = text_table1[21];
  wifiGeneralMenu.name = text_table1[22];
  saveFileMenu.name = "Save/Load Files";
  saveSSIDsMenu.name = "Save SSIDs";
  loadSSIDsMenu.name = "Load SSIDs";
  saveAPsMenu.name = "Save APs";
  loadAPsMenu.name = "Load APs";
  saveATsMenu.name = "Save Airtags";
  loadATsMenu.name = "Load Airtags";

  bluetoothSnifferMenu.name = text_table1[23];
  bluetoothAttackMenu.name = "Bluetooth Attacks";
  generateSSIDsMenu.name = text_table1[27];
  clearSSIDsMenu.name = text_table1[28];
  clearAPsMenu.name = text_table1[29];
  wifiAPMenu.name = "Select";
  wifiIPMenu.name = "Active IPs";
  apInfoMenu.name = "AP Info";
  setMacMenu.name = "Set MACs";
  genAPMacMenu.name = "Generate AP MAC";
  wifiStationMenu.name = "Select Stations";

  #ifdef HAS_DIRECT_UPLOAD
    uploadLogsMenu.name = "Upload Logs";
    uploadAllMenu.name = "Upload All?";
    deleteAllMenu.name = "Delete All?";
    actionMenu.name = "Destination";
  #endif

  #ifdef HAS_GPS
    gpsMenu.name = "GPS"; 
    gpsInfoMenu.name = "GPS Data";
    //wardrivingMenu.name = "Wardriving";
  #endif  
  htmlMenu.name = "EP HTML List";
  miniKbMenu.name = "Mini Keyboard";

  #ifdef HAS_SD
    sdDeleteMenu.name = "Delete SD Files";
  #endif

  selectProbeSSIDsMenu.name = "Probe Requests";
  evilPortalMenu.name = "Evil Portal";
  ssidsMenu.name = "SSIDs";

  #ifdef HAS_GPS
    gpsPOIMenu.name = "GPS POI";
  #endif

  foxHuntMenu.name = "Fox Hunt";

  // Build Main Menu
  mainMenu.parentMenu = NULL;
  this->addNodes(&mainMenu, "Cyber Defense", TFTGREEN, SCANNERS, [this]() {
    this->changeMenu(&sharkDefenseMenu, true);
  });
  this->addNodes(&mainMenu, text_table1[7], TFTGREEN, WIFI, [this]() {
    #ifdef MARAUDER_V8
      shark_radar_obj.playWifiIntro();
    #endif
    this->changeMenu(&wifiMenu, true);
  });
  #ifdef HAS_BT
    this->addNodes(&mainMenu, text_table1[19], TFTCYAN, BLUETOOTH, [this]() {
      #ifdef MARAUDER_V8
        shark_radar_obj.playBluetoothIntro();
      #endif
      this->changeMenu(&bluetoothMenu, true);
    });
  #endif
  #ifdef HAS_GPS
	if (gps_obj.getGpsModuleStatus()) {
    	this->addNodes(&mainMenu, text1_66, TFTRED, GPS_MENU, [this]() {
      	#ifdef MARAUDER_V8
      	  shark_radar_obj.playGpsIntro();
      	#endif
      	this->changeMenu(&gpsMenu, true);
    	});
	}
  #endif
  this->addNodes(&mainMenu, text_table1[9], TFTBLUE, DEVICE, [this]() {
    this->changeMenu(&deviceMenu, true);
  });
  #ifdef HAS_BT
    this->addNodes(&mainMenu, "BadUSB Tools", TFTRED, EAPOL, [this]() {
      bad_usb_obj.run();
      this->changeMenu(&mainMenu, true);
    });
  #endif
  this->addNodes(&mainMenu, "Card Control", TFTORANGE, SD_UPDATE, [this]() {
    this->cardControlScreen();
    this->changeMenu(&mainMenu, true);
  });
  #ifdef MARAUDER_V8
    this->addNodes(&mainMenu, "Operator Profile", TFTCYAN, PROFILE_ICON, [this]() {
      this->profileScreen();
      this->changeMenu(&mainMenu, true);
    });
    this->addNodes(&mainMenu, "Prank", TFTMAGENTA, EAPOL, [this]() {
      this->changeMenu(&prankMenu, true);
    });
  #endif
  this->addNodes(&mainMenu, text_table1[30], TFTLIGHTGREY, REBOOT, []() {
    ESP.restart();
  });

  #ifdef MARAUDER_V8
    // Prank section (added; existing menus untouched). Each prank is a
    // self-contained Start/Stop/Back screen that releases its radio on exit.
    prankMenu.name = "Prank";
    prankMenu.parentMenu = &mainMenu;
    this->addNodes(&prankMenu, text09, TFTLIGHTGREY, 0, [this]() {
      this->changeMenu(prankMenu.parentMenu, true);
    });
    this->addNodes(&prankMenu, "WiFi Pranks", TFTGREEN, WIFI, [this]() {
      this->changeMenu(&wifiPrankMenu, true);
    });
    #ifdef HAS_BT
      this->addNodes(&prankMenu, "Bluetooth Pranks", TFTCYAN, BLUETOOTH, [this]() {
        this->changeMenu(&btPrankMenu, true);
      });
    #endif

    wifiPrankMenu.name = "WiFi Pranks";
    wifiPrankMenu.parentMenu = &prankMenu;
    this->addNodes(&wifiPrankMenu, text09, TFTLIGHTGREY, 0, [this]() {
      this->changeMenu(wifiPrankMenu.parentMenu, true);
    });
    this->addNodes(&wifiPrankMenu, "Funny Hotspot", TFTGREEN, WIFI, [this]() {
      shark_prank_obj.funnyHotspot(); this->changeMenu(&wifiPrankMenu, true);
    });
    this->addNodes(&wifiPrankMenu, "SSID Rotator", TFTCYAN, WIFI, [this]() {
      shark_prank_obj.ssidRotator(); this->changeMenu(&wifiPrankMenu, true);
    });
    this->addNodes(&wifiPrankMenu, "Guest Counter", TFTORANGE, WIFI, [this]() {
      shark_prank_obj.guestCounter(); this->changeMenu(&wifiPrankMenu, true);
    });
    this->addNodes(&wifiPrankMenu, "Prank Portal", TFTMAGENTA, WIFI, [this]() {
      shark_prank_obj.prankPortal(); this->changeMenu(&wifiPrankMenu, true);
    });
    this->addNodes(&wifiPrankMenu, "Meme Portal", TFTMAGENTA, WIFI, [this]() {
      shark_prank_obj.memePortal(); this->changeMenu(&wifiPrankMenu, true);
    });
    this->addNodes(&wifiPrankMenu, "Custom Portal", TFTCYAN, WIFI, [this]() {
      shark_prank_obj.customPortal(); this->changeMenu(&wifiPrankMenu, true);
    });
    this->addNodes(&wifiPrankMenu, "Monkey WiFi", TFTMAGENTA, WIFI, [this]() {
      shark_prank_obj.monkeyWifi(); this->changeMenu(&wifiPrankMenu, true);
    });

    #ifdef HAS_BT
      btPrankMenu.name = "Bluetooth Pranks";
      btPrankMenu.parentMenu = &prankMenu;
      this->addNodes(&btPrankMenu, text09, TFTLIGHTGREY, 0, [this]() {
        this->changeMenu(btPrankMenu.parentMenu, true);
      });
      this->addNodes(&btPrankMenu, "BLE Name Broadcast", TFTCYAN, BLUETOOTH, [this]() {
        shark_prank_obj.bleNameBroadcast(); this->changeMenu(&btPrankMenu, true);
      });
      this->addNodes(&btPrankMenu, "BLE Name Rotator", TFTCYAN, BLUETOOTH, [this]() {
        shark_prank_obj.bleNameRotator(); this->changeMenu(&btPrankMenu, true);
      });
      this->addNodes(&btPrankMenu, "BLE Advertiser", TFTGREEN, BLUETOOTH, [this]() {
        shark_prank_obj.bleAdvertiser(); this->changeMenu(&btPrankMenu, true);
      });
      this->addNodes(&btPrankMenu, "BLE Beacon", TFTORANGE, BLUETOOTH, [this]() {
        shark_prank_obj.bleBeacon(); this->changeMenu(&btPrankMenu, true);
      });
      this->addNodes(&btPrankMenu, "BLE Radar", TFTGREEN, BLUETOOTH, [this]() {
        shark_prank_obj.bleRadar(); this->changeMenu(&btPrankMenu, true);
      });
      this->addNodes(&btPrankMenu, "BLE Hunt", TFTMAGENTA, BLUETOOTH, [this]() {
        shark_prank_obj.bleHunt(); this->changeMenu(&btPrankMenu, true);
      });
      // Shark Hunt: two-device chase. One SHARK is the beacon, the other seeks.
      this->addNodes(&btPrankMenu, "Shark Hunt: Beacon", TFTRED, BLUETOOTH, [this]() {
        shark_prank_obj.sharkHuntBeacon(); this->changeMenu(&btPrankMenu, true);
      });
      this->addNodes(&btPrankMenu, "Shark Hunt: Seeker", TFTCYAN, BLUETOOTH, [this]() {
        shark_prank_obj.sharkHuntSeeker(); this->changeMenu(&btPrankMenu, true);
      });
    #endif
  #endif

  // SHARK defensive operations. Every action here is receive-only/passive.
  sharkDefenseMenu.parentMenu = &mainMenu;
  this->addNodes(&sharkDefenseMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(sharkDefenseMenu.parentMenu, true);
  });
  this->addNodes(&sharkDefenseMenu, "RuView CSI", TFTGREEN, SCANNERS, [this]() {
    // RuView needs a live Wi-Fi link to read CSI. If we are not connected, open
    // an in-place picker that connects and drops straight into RuView, so the
    // user never lands on a dead "JOIN WIFI FIRST" screen.
    this->startRuView();
  });
  this->addNodes(&sharkDefenseMenu, "Drone RID Detector", TFTGREEN, BLUETOOTH_SNIFF, [this]() {
    this->startWifiToolUI(SHARK_DRONE_RID_SCAN, TFT_GREEN);
  });
  this->addNodes(&sharkDefenseMenu, "Deauth Guard", TFTRED, DEAUTH_SNIFF, [this]() {
    this->startPassiveWifiToolUI(WIFI_SCAN_DEAUTH, TFT_RED);
  });
  this->addNodes(&sharkDefenseMenu, "Pineapple Watch", TFTYELLOW, PINESCAN_SNIFF, [this]() {
    this->startWifiToolUI(WIFI_SCAN_PINESCAN, TFT_YELLOW);
  });
  this->addNodes(&sharkDefenseMenu, "MultiSSID Watch", TFTORANGE, MULTISSID_SNIFF, [this]() {
    this->startWifiToolUI(WIFI_SCAN_MULTISSID, TFT_ORANGE);
  });
  this->addNodes(&sharkDefenseMenu, "Tracker Monitor", TFTMAGENTA, SCANNERS, [this]() {
    this->startWifiToolUI(WIFI_SCAN_DETECT_FOLLOW, TFT_MAGENTA);
  });
  this->addNodes(&sharkDefenseMenu, "AP + Client Map", TFTLIME, BEACON_SNIFF, [this]() {
    this->startWifiToolUI(WIFI_SCAN_AP_STA, TFT_GREEN);
  });
  this->addNodes(&sharkDefenseMenu, "WiFi Spectrum", TFTCYAN, PACKET_MONITOR, [this]() {
    this->startWifiToolUI(WIFI_SCAN_CHAN_ANALYZER, TFT_CYAN);
  });
  #ifdef HAS_BT
    this->addNodes(&sharkDefenseMenu, "BLE Spectrum", TFTCYAN, PACKET_MONITOR, [this]() {
      display_obj.clearScreen();
      this->drawStatusBar();
      wifi_scan_obj.StartScan(BT_SCAN_ANALYZER, TFT_CYAN);
      this->renderGraphUI(BT_SCAN_ANALYZER);
    });
  #endif
  this->addNodes(&sharkDefenseMenu, "Packet Rate", TFTORANGE, PACKET_MONITOR, [this]() {
    this->startWifiToolUI(WIFI_SCAN_PACKET_RATE, TFT_ORANGE);
  });

  // More passive/defensive detectors. Every one is receive-only.
  this->addNodes(&sharkDefenseMenu, "Handshake Watch", TFTVIOLET, EAPOL, [this]() {
    this->startWifiToolUI(WIFI_SCAN_EAPOL, TFT_VIOLET);
  });
  this->addNodes(&sharkDefenseMenu, "Channel Summary", TFTCYAN, PACKET_MONITOR, [this]() {
    this->startWifiToolUI(WIFI_SCAN_CHAN_ACT, TFT_CYAN);
  });
  this->addNodes(&sharkDefenseMenu, "PCAP Logger", TFTWHITE, PACKET_MONITOR, [this]() {
    this->startWifiToolUI(WIFI_SCAN_RAW_CAPTURE, TFT_WHITE);
  });
  // Deauth Alarm: same passive deauth sniff as Deauth Guard, but the
  // dashboard title strip flashes a red flood alarm when the deauth rate
  // crosses the threshold.
  this->addNodes(&sharkDefenseMenu, "Deauth Alarm", TFTRED, DEAUTH_SNIFF, [this]() {
    this->startPassiveWifiToolUI(WIFI_SCAN_DEAUTH, TFT_RED);
    wifi_scan_obj.deauth_alarm = true;   // set after StartScan (which resets it)
    this->drawPassiveWifiDetectorUI(true);
  });
  // Evil Twin Scan: normal AP scan on the passive detector dashboard, with
  // an amber strip flagging SSIDs advertised by more than one BSSID (the
  // classic hotspot spoof).
  this->addNodes(&sharkDefenseMenu, "Evil Twin Scan", TFTYELLOW, BEACON_SNIFF, [this]() {
    this->startPassiveWifiToolUI(WIFI_SCAN_AP, TFT_YELLOW);
    wifi_scan_obj.evil_twin = true;      // set after StartScan (which resets it)
    this->drawPassiveWifiDetectorUI(true);
  });
  // Wardrive Log: AP + BLE + GPS -> Wigle-format CSV on SD (needs GPS + SD),
  // on the shared Wi-Fi monitor dashboard. Without a GPS module the scan
  // cannot produce coordinates, so say that instead of an idle dashboard.
  this->addNodes(&sharkDefenseMenu, "Wardrive Log", TFTLIME, SCANNERS, [this]() {
    #ifdef HAS_GPS
      if (gps_obj.getGpsModuleStatus()) {
        this->startWifiToolUI(WIFI_SCAN_WAR_DRIVE, TFT_GREEN);
        return;
      }
    #endif
    TFT_eSPI& tft = display_obj.tft;
    tft.fillRoundRect(34, 132, 172, 56, 5, TFT_ORANGE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, TFT_ORANGE);
    tft.drawString("GPS OFFLINE", 120, 152, 2);
    tft.drawString("ENABLE IN GPS MENU", 120, 172, 1);
    tft.setTextDatum(TL_DATUM);
    delay(1100);
    this->changeMenu(&sharkDefenseMenu, true);
  });
  #ifdef HAS_BT
    this->addNodes(&sharkDefenseMenu, "Tracker Detect", TFTWHITE, BLUETOOTH_SNIFF, [this]() {
      display_obj.clearScreen();
      this->drawStatusBar();
      wifi_scan_obj.StartScan(BT_SCAN_AIRTAG_MON, TFT_WHITE);
      this->drawPassiveBleDetectorUI(true);
    });
    this->addNodes(&sharkDefenseMenu, "Card Skimmer", TFTMAGENTA, CC_SKIMMERS, [this]() {
      display_obj.clearScreen();
      this->drawStatusBar();
      wifi_scan_obj.StartScan(BT_SCAN_SKIMMERS, TFT_MAGENTA);
      this->drawCardSkimmerUI(true);
    });
  #endif

  // R111 accountability & guardrails.
  this->addNodes(&sharkDefenseMenu, "Session Report", TFTCYAN, SCANNERS, [this]() {
    this->exportSessionReport();
  });
  this->addNodes(&sharkDefenseMenu, "Attack PIN", TFTYELLOW, KEYBOARD_ICO, [this]() {
    char buf[9] = {0};
    if (!keyboardInput(buf, sizeof(buf), "SET PIN (EMPTY=CLEAR)")) return;
    settings_obj.saveSetting<bool>("AttackPin", String(buf));
    TFT_eSPI& tft = display_obj.tft;
    tft.fillRoundRect(34, 132, 172, 56, 5, buf[0] ? TFT_GREEN : TFT_ORANGE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, buf[0] ? TFT_GREEN : TFT_ORANGE);
    tft.drawString(buf[0] ? "PIN SAVED" : "PIN CLEARED", 120, 160, 2);
    tft.setTextDatum(TL_DATUM);
    delay(1100);
    this->changeMenu(&sharkDefenseMenu, true);
  });
  this->addNodes(&sharkDefenseMenu, "Attack Cap", TFTORANGE, PACKET_MONITOR, [this]() {
    // Cycle OFF -> 5 -> 10 -> 30 -> 60 minutes. The cap pauses every
    // transmit loop when it expires and writes a TX CAP-STOP audit line.
    static const uint16_t steps[] = {0, 5, 10, 30, 60};
    const uint16_t cur = wifi_scan_obj.attackCapMinutes();
    uint8_t next = 0;
    for (uint8_t i = 0; i < 4; i++)
      if (steps[i] == cur) { next = i + 1; break; }
    const uint16_t mins = steps[next];
    settings_obj.saveSetting<bool>("AttackCap", mins ? String(mins) : String(""));
    TFT_eSPI& tft = display_obj.tft;
    tft.fillRoundRect(34, 132, 172, 56, 5, TFT_CYAN);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, TFT_CYAN);
    tft.drawString(mins ? ("CAP " + String(mins) + " MIN") : String("CAP OFF"), 120, 160, 2);
    tft.setTextDatum(TL_DATUM);
    delay(1100);
    this->changeMenu(&sharkDefenseMenu, true);
  });

  // IQ Family Watch: family devices (SharkDeck OS) announce themselves the
  // moment they join this device's access point; ON pops the full-screen
  // FAMILY DEVICE DETECTED alert with the station's measured RSSI.
  wifi_scan_obj.family_watch_enabled =
    settings_obj.loadSetting<String>("FamilyWatch") != "0";
  this->addNodes(&sharkDefenseMenu, "IQ Family Watch", TFTCYAN, WIFI, [this]() {
    const bool on = !wifi_scan_obj.family_watch_enabled;
    wifi_scan_obj.family_watch_enabled = on;
    settings_obj.saveSetting<bool>("FamilyWatch", on ? String("1") : String("0"));
    TFT_eSPI& tft = display_obj.tft;
    tft.fillRoundRect(34, 132, 172, 56, 5, on ? TFT_CYAN : TFT_ORANGE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, on ? TFT_CYAN : TFT_ORANGE);
    tft.drawString(on ? "IQ WATCH ON" : "IQ WATCH OFF", 120, 152, 2);
    tft.drawString(on ? "DECK ALERTS + RSSI" : "BEACONS IGNORED", 120, 172, 1);
    tft.setTextDatum(TL_DATUM);
    delay(1100);
    this->changeMenu(&sharkDefenseMenu, true);
  });

  // Web Control: local AP + app dashboard with a join QR. run() returns a scan
  // mode to launch, -1 (stopped), or -2 (sent to background, AP stays up).
  this->addNodes(&sharkDefenseMenu, "Web Control", TFTCYAN, WIFI, [this]() {
    int mode = shark_web_obj.run();
    if (mode == -2 || mode == -1) {
      this->changeMenu(&sharkDefenseMenu, true);
      return;
    }
    this->startWebTool(mode);
  });
  this->addNodes(&sharkDefenseMenu, "WiFi Radar", TFTGREEN, SCANNERS, [this]() {
    shark_radar_obj.runWifi();
    this->changeMenu(&sharkDefenseMenu, true);
  });

  // Build WiFi Menu
  wifiMenu.parentMenu = &mainMenu; // Main Menu is second menu parent
  this->addNodes(&wifiMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(wifiMenu.parentMenu, true);
  });
  this->addNodes(&wifiMenu, text_table1[31], TFTYELLOW, SNIFFERS, [this]() {
    this->changeMenu(&wifiSnifferMenu, true);
  });
  this->addNodes(&wifiMenu, "Scanners", TFTORANGE, SCANNERS, [this]() {
    this->changeMenu(&wifiScannerMenu, true);
  });
  /*#ifdef HAS_GPS
    this->addNodes(&wifiMenu, "Wardriving", TFTGREEN, NULL, BEACON_SNIFF, [this]() {
      this->changeMenu(&wardrivingMenu, true);
    });
  #endif*/
  this->addNodes(&wifiMenu, text_table1[32], TFTRED, ATTACKS, [this]() {
    this->changeMenu(&wifiAttackMenu, true);
  });
  this->addNodes(&wifiMenu, text_table1[33], TFTPURPLE, GENERAL_APPS, [this]() {
    this->changeMenu(&wifiGeneralMenu, true);
  });

  // Build WiFi scanner Menu
  wifiScannerMenu.parentMenu = &wifiMenu; // Main Menu is second menu parent
  this->addNodes(&wifiScannerMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(wifiScannerMenu.parentMenu, true);
  });
  this->addNodes(&wifiScannerMenu, "Ping Scan", TFTGREEN, PING_SCAN_ICON, [this]() {
    this->startNetworkScannerUI(WIFI_PING_SCAN, TFT_CYAN);
  });
  #ifndef HAS_DUAL_BAND
    this->addNodes(&wifiScannerMenu, "ARP Scan", TFTCYAN, ARP_SCAN_ICON, [this]() {
      display_obj.clearScreen();
      this->drawStatusBar();
      wifi_scan_obj.StartScan(WIFI_ARP_SCAN, TFT_CYAN);
    });
  #endif
  this->addNodes(&wifiScannerMenu, "Port Scan All", TFTMAGENTA, PORT_SCAN_ICON, [this](){
    if (WiFi.status() != WL_CONNECTED) {
      this->startNetworkScannerUI(WIFI_PORT_SCAN_ALL, TFT_MAGENTA);
      return;
    }
    // Add the back button
    wifiIPMenu.list->clear();
      this->addNodes(&wifiIPMenu, text09, TFTLIGHTGREY, 0, [this]() {
      this->changeMenu(wifiIPMenu.parentMenu, true);
    });

    // A full scan is always usable after joining Wi-Fi, even before running
    // discovery. The gateway is the safe, explicit default target.
    if (wifi_scan_obj.gateway != IPAddress(0, 0, 0, 0)) {
      String gateway_label = "Gateway " + wifi_scan_obj.gateway.toString();
      this->addNodes(&wifiIPMenu, gateway_label.c_str(), TFTCYAN, 255, [this](){
        wifi_scan_obj.current_scan_ip = wifi_scan_obj.gateway;
        this->startNetworkScannerUI(WIFI_PORT_SCAN_ALL, TFT_MAGENTA);
      });
    }

    // Populate the menu with buttons
    for (int i = 0; i < ipList->size(); i++) {
      // This is the menu node
      this->addNodes(&wifiIPMenu, ipList->get(i).toString().c_str(), TFTBLUE, 255, [this, i](){
        Serial.println("Selected: " + ipList->get(i).toString());
        wifi_scan_obj.current_scan_ip = ipList->get(i);
        this->startNetworkScannerUI(WIFI_PORT_SCAN_ALL, TFT_MAGENTA);
      });
    }
    this->changeMenu(&wifiIPMenu, true);
  });
  this->addNodes(&wifiScannerMenu, "SSH Scan", TFTORANGE, SSH_SCAN_ICON, [this]() {
    this->startNetworkScannerUI(WIFI_SCAN_SSH, TFT_ORANGE);
  });
  this->addNodes(&wifiScannerMenu, "Telnet Scan", TFTRED, TELNET_SCAN_ICON, [this]() {
    this->startNetworkScannerUI(WIFI_SCAN_TELNET, TFT_RED);
  });
  this->addNodes(&wifiScannerMenu, "SMTP Scan", TFTWHITE, SMTP_SCAN_ICON, [this]() {
    this->startNetworkScannerUI(WIFI_SCAN_SMTP, TFT_WHITE);
  });
  this->addNodes(&wifiScannerMenu, "DNS Scan", TFTLIME, DNS_SCAN_ICON, [this]() {
    this->startNetworkScannerUI(WIFI_SCAN_DNS, TFT_GREEN);
  });
  this->addNodes(&wifiScannerMenu, "HTTP Scan", TFTSKYBLUE, HTTP_SCAN_ICON, [this]() {
    this->startNetworkScannerUI(WIFI_SCAN_HTTP, TFT_CYAN);
  });
  this->addNodes(&wifiScannerMenu, "HTTPS Scan", TFTYELLOW, HTTPS_SCAN_ICON, [this]() {
    this->startNetworkScannerUI(WIFI_SCAN_HTTPS, TFT_YELLOW);
  });
  this->addNodes(&wifiScannerMenu, "RDP Scan", TFTPURPLE, RDP_SCAN_ICON, [this]() {
    this->startNetworkScannerUI(WIFI_SCAN_RDP, TFT_PURPLE);
  });
  this->addNodes(&wifiScannerMenu, "FTP Scan", TFTCYAN, FTP_SCAN_ICON, [this]() {
    this->startNetworkScannerUI(WIFI_SCAN_FTP, TFT_CYAN);
  });
  this->addNodes(&wifiScannerMenu, "SMB Scan", TFTSKYBLUE, SMB_SCAN_ICON, [this]() {
    this->startNetworkScannerUI(WIFI_SCAN_SMB, TFT_CYAN);
  });
  this->addNodes(&wifiScannerMenu, "MQTT Scan", TFTGREEN, MQTT_SCAN_ICON, [this]() {
    this->startNetworkScannerUI(WIFI_SCAN_MQTT, TFT_GREEN);
  });
  this->addNodes(&wifiScannerMenu, "MySQL Scan", TFTORANGE, MYSQL_SCAN_ICON, [this]() {
    this->startNetworkScannerUI(WIFI_SCAN_MYSQL, TFT_ORANGE);
  });
  this->addNodes(&wifiScannerMenu, "Postgres Scan", TFTPURPLE, POSTGRES_SCAN_ICON, [this]() {
    this->startNetworkScannerUI(WIFI_SCAN_POSTGRES, TFT_PURPLE);
  });
  this->addNodes(&wifiScannerMenu, "VNC Scan", TFTYELLOW, VNC_SCAN_ICON, [this]() {
    this->startNetworkScannerUI(WIFI_SCAN_VNC, TFT_YELLOW);
  });
  this->addNodes(&wifiScannerMenu, "Redis Scan", TFTRED, REDIS_SCAN_ICON, [this]() {
    this->startNetworkScannerUI(WIFI_SCAN_REDIS, TFT_RED);
  });
  this->addNodes(&wifiScannerMenu, "MQTT TLS Scan", TFTWHITE, MQTTS_SCAN_ICON, [this]() {
    this->startNetworkScannerUI(WIFI_SCAN_MQTTS, TFT_WHITE);
  });

  // Build WiFi sniffer Menu
  wifiSnifferMenu.parentMenu = &wifiMenu; // Main Menu is second menu parent
  this->addNodes(&wifiSnifferMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(wifiSnifferMenu.parentMenu, true);
  });
  // These Wi-Fi sniffers use the same detector dashboard as the BLE FindMy /
  // Flock / Meta tools: StartScan, then arm the passive detector and draw it.
  this->addNodes(&wifiSnifferMenu, text_table1[42], TFTCYAN, PROBE_SNIFF, [this]() {
    this->startPassiveWifiToolUI(WIFI_SCAN_PROBE, TFT_CYAN);
  });
  this->addNodes(&wifiSnifferMenu, text_table1[43], TFTMAGENTA, BEACON_SNIFF, [this]() {
    this->startPassiveWifiToolUI(WIFI_SCAN_AP, TFT_MAGENTA);
  });
  this->addNodes(&wifiSnifferMenu, text_table1[44], TFTRED, DEAUTH_SNIFF, [this]() {
    this->startPassiveWifiToolUI(WIFI_SCAN_DEAUTH, TFT_RED);
  });
  this->addNodes(&wifiSnifferMenu, "Packet Count", TFTORANGE, PACKET_MONITOR, [this]() {
    this->startWifiToolUI(WIFI_SCAN_PACKET_RATE, TFT_ORANGE);
  });
  #ifdef HAS_ILI9341
    this->addNodes(&wifiSnifferMenu, text_table1[46], TFTVIOLET, EAPOL, [this]() {
      this->startWifiToolUI(WIFI_SCAN_EAPOL, TFT_VIOLET);
    });
    this->addNodes(&wifiSnifferMenu, text_table1[45], TFTBLUE, PACKET_MONITOR, [this]() {
      this->startWifiToolUI(WIFI_PACKET_MONITOR, TFT_BLUE);
    });
  #else // No touch
    this->addNodes(&wifiSnifferMenu, text_table1[46], TFTVIOLET, EAPOL, [this]() {
      this->startWifiToolUI(WIFI_SCAN_EAPOL, TFT_VIOLET);
    });
    this->addNodes(&wifiSnifferMenu, text_table1[45], TFTBLUE, PACKET_MONITOR, [this]() {
      this->startWifiToolUI(WIFI_PACKET_MONITOR, TFT_BLUE);
    });
  #endif
  this->addNodes(&wifiSnifferMenu, "Channel Analyzer", TFTCYAN, PACKET_MONITOR, [this]() {
    this->startWifiToolUI(WIFI_SCAN_CHAN_ANALYZER, TFT_CYAN);
  });
  this->addNodes(&wifiSnifferMenu, "Channel Summary", TFTORANGE, PACKET_MONITOR, [this]() {
    this->startWifiToolUI(WIFI_SCAN_CHAN_ACT, TFT_CYAN);
  });

  this->addNodes(&wifiSnifferMenu, text_table1[58], TFTWHITE, PACKET_MONITOR, [this]() {
    this->startWifiToolUI(WIFI_SCAN_RAW_CAPTURE, TFT_WHITE);
  });

  this->addNodes(&wifiSnifferMenu, text_table1[47], TFTRED, PWNAGOTCHI, [this]() {
    this->startWifiToolUI(WIFI_SCAN_PWN, TFT_RED);
  });
  
  this->addNodes(&wifiSnifferMenu, text_table1[63], TFTYELLOW, PINESCAN_SNIFF, [this]() {
    this->startWifiToolUI(WIFI_SCAN_PINESCAN, TFT_YELLOW);
  });

  this->addNodes(&wifiSnifferMenu, text_table1[64], TFTORANGE, MULTISSID_SNIFF, [this]() {
    this->startWifiToolUI(WIFI_SCAN_MULTISSID, TFT_ORANGE);
  });
  this->addNodes(&wifiSnifferMenu, "Scan AP/STA", TFTLIME, BEACON_SNIFF, [this]() {
    this->startWifiToolUI(WIFI_SCAN_AP_STA, 0x97e0);
  });
  /*this->addNodes(&wifiSnifferMenu, "Fox Hunt", TFTCYAN, PACKET_MONITOR, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_SIG_STREN, TFT_CYAN);
  });*/
  this->addNodes(&wifiSnifferMenu, "Fox Hunt", TFTCYAN, SCANNERS, [this]() {
    foxHuntMenu.list->clear();

    // Wi-Fi Fox Hunt target picker
    foxHuntMenu.parentMenu = &wifiSnifferMenu; // Second Menu is third menu parent
    this->addNodes(&foxHuntMenu, text09, TFTLIGHTGREY, 0, [this]() {
      this->changeMenu(foxHuntMenu.parentMenu, true);
    });
    
    // Order APs strongest-signal first (stable) -- upstream v1.15.0 fox-hunt
    // sort, kept in the SHARK list UI. Snapshot RSSI once.
    {
      int n = access_points->size();
      std::vector<int> order(n);
      std::vector<int> rssis(n);
      for (int i = 0; i < n; i++) { order[i] = i; rssis[i] = access_points->get(i).rssi; }
      std::stable_sort(order.begin(), order.end(),
                       [&rssis](int a, int b) { return rssis[a] > rssis[b]; });
      for (int k = 0; k < n; k++) {
        int i = order[k];
        AccessPoint access_point = access_points->get(i);
        access_point.selected = false;
        access_points->set(i, access_point);
        uint8_t node_color = rssiToMenuColor(access_points->get(i).rssi);
        String node_name = String(access_points->get(i).rssi) + " " + access_points->get(i).essid;
        this->addNodes(&foxHuntMenu, node_name.c_str(), node_color, 255, [this, i](){
          AccessPoint access_point = access_points->get(i);
          access_point.selected = true;
          access_points->set(i, access_point);
          display_obj.clearScreen();
          this->drawStatusBar();
          wifi_scan_obj.StartScan(WIFI_SCAN_SIG_STREN, TFT_CYAN);
          this->drawWiFiFoxHuntUI(true);
        });
      }
    }
    this->changeMenu(&foxHuntMenu, true);
  });
  this->addNodes(&wifiSnifferMenu, "MAC Monitor", TFTMAGENTA, SCANNERS, [this]() {
    this->startWifiToolUI(WIFI_SCAN_DETECT_FOLLOW, TFT_MAGENTA);
  });
  this->addNodes(&wifiSnifferMenu, "SAE Commit", TFTLIME, EAPOL, [this]() {
    this->startWifiToolUI(WIFI_SCAN_SAE_COMMIT, TFT_GREEN);
  });

  // Build Wardriving menu
  #ifdef HAS_GPS
    /*wardrivingMenu.parentMenu = &wifiMenu; // Main Menu is second menu parent
    this->addNodes(&wardrivingMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
      this->changeMenu(wardrivingMenu.parentMenu, true);
    });*/
    if (gps_obj.getGpsModuleStatus()) {
      this->addNodes(&wifiSnifferMenu, "Wardrive", TFTGREEN, BEACON_SNIFF, [this]() {
        this->startWifiToolUI(WIFI_SCAN_WAR_DRIVE, TFT_GREEN);
      });
    }
  #endif
  /*#ifdef HAS_GPS
    if (gps_obj.getGpsModuleStatus()) {
      this->addNodes(&wardrivingMenu, "Station Wardrive", TFTORANGE, NULL, PROBE_SNIFF, [this]() {
        display_obj.clearScreen();
        this->drawStatusBar();
        wifi_scan_obj.StartScan(WIFI_SCAN_STATION_WAR_DRIVE, TFT_ORANGE);
      });
    }
  #endif*/

  // Build WiFi attack menu
  wifiAttackMenu.parentMenu = &wifiMenu; // Main Menu is second menu parent
  this->addNodes(&wifiAttackMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(wifiAttackMenu.parentMenu, true);
  });
  this->addNodes(&wifiAttackMenu, text_table1[50], TFTRED,
                 BEACON_LIST_ATTACK_ICON, [this]() {
    this->startBeaconListUI();
  });
  this->addNodes(&wifiAttackMenu, text_table1[51], TFTORANGE,
                 BEACON_SPAM_ATTACK_ICON, [this]() {
    this->startBeaconSpamUI();
  });
  this->addNodes(&wifiAttackMenu, text1_67, TFTCYAN,
                 FUNNY_BEACON_ATTACK_ICON, [this]() {
    this->startFunnyBeaconUI();
  });
  this->addNodes(&wifiAttackMenu, text_table1[52], TFTYELLOW,
                 RICK_ROLL_BEACON_ICON, [this]() {
    this->startRickRollUI();
  });
  this->addNodes(&wifiAttackMenu, text_table1[53], TFTRED,
                 PROBE_FLOOD_ATTACK_ICON, [this]() {
    this->startProbeFloodUI();
  });
  this->addNodes(&wifiAttackMenu, "Evil Portal", TFTORANGE, BEACON_SNIFF, [this]() {

    wifiAPMenu.list->clear();
    ssidsMenu.list->clear();

    wifiAPMenu.parentMenu = &evilPortalMenu;
    ssidsMenu.parentMenu = &evilPortalMenu;

    this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, 0, [this]() {
      this->changeMenu(wifiAPMenu.parentMenu, true);
    });
    this->addNodes(&ssidsMenu, text09, TFTLIGHTGREY, 0, [this]() {
      this->changeMenu(ssidsMenu.parentMenu, true);
    });

    // Get AP list ready
    for (int i = 0; i < access_points->size(); i++) {
      // This is the menu node
      this->addNodes(&wifiAPMenu, access_points->get(i).essid.c_str(), TFTCYAN, 255, [this, i](){
        if (evil_portal_obj.setAP(access_points->get(i).essid)) {
          AccessPoint new_ap = access_points->get(i);
          new_ap.selected = true;
          access_points->set(i, new_ap);

          evil_portal_obj.ap_index = i;

          display_obj.clearScreen();
          this->drawStatusBar();
          wifi_scan_obj.StartScan(WIFI_SCAN_EVIL_PORTAL, TFT_ORANGE);
          wifi_scan_obj.setMac();
        }
        else
          this->changeMenu(&evilPortalMenu, true);
      });
    }

    for (int i = 0; i < ssids->size(); i++) {
      // This is the menu node
      this->addNodes(&ssidsMenu, ssids->get(i).essid.c_str(), TFTCYAN, 255, [this, i](){
        if (evil_portal_obj.setAP(ssids->get(i).essid)) {
          display_obj.clearScreen();
          this->drawStatusBar();
          wifi_scan_obj.StartScan(WIFI_SCAN_EVIL_PORTAL, TFT_ORANGE);
          wifi_scan_obj.setMac();
        }
        else
          this->changeMenu(&evilPortalMenu, true);
      });
    }
    this->changeMenu(&evilPortalMenu, true);
  });
  this->addNodes(&wifiAttackMenu, text_table1[54], TFTRED,
                 DEAUTH_FLOOD_ATTACK_ICON, [this]() {
    this->startDeauthFloodUI();
  });
  this->addNodes(&wifiAttackMenu, text_table1[57], TFTMAGENTA, BEACON_LIST, [this]() {
    this->startAttackDashUI(0);
  });
  this->addNodes(&wifiAttackMenu, text_table1[62], TFTRED, DEAUTH_SNIFF, [this]() {
    this->startAttackDashUI(1);
  });

  this->addNodes(&wifiAttackMenu, "Karma", TFTORANGE, KEYBOARD_ICO, [this](){
    // Add the back button
    selectProbeSSIDsMenu.list->clear();
    this->addNodes(&selectProbeSSIDsMenu, text09, TFTLIGHTGREY, 0, [this]() {
      this->changeMenu(&wifiAttackMenu, true);
    });

    // Populate the menu with buttons
    for (int i = 0; i < probe_req_ssids->size(); i++) {
      // This is the menu node
      this->addNodes(&selectProbeSSIDsMenu, probe_req_ssids->get(i).essid.c_str(), TFTCYAN, 255, [this, i](){
        if (evil_portal_obj.setAP(probe_req_ssids->get(i).essid)) {
          display_obj.clearScreen();
          this->drawStatusBar();
          wifi_scan_obj.StartScan(WIFI_SCAN_EVIL_PORTAL, TFT_ORANGE);
          wifi_scan_obj.setMac();
        }
        else
          this->changeMenu(&wifiAttackMenu, true);
      });
    }
    this->changeMenu(&selectProbeSSIDsMenu, true);
  });

  this->addNodes(&wifiAttackMenu, "Bad Msg", TFTRED, DEAUTH_SNIFF, [this]() {
    this->startAttackDashUI(2);
  });
  this->addNodes(&wifiAttackMenu, "Bad Msg Targeted", TFTYELLOW, DEAUTH_SNIFF, [this]() {
    this->startAttackDashUI(3);
  });
  this->addNodes(&wifiAttackMenu, "Assoc Sleep", TFTRED, DEAUTH_SNIFF, [this]() {
    this->startAttackDashUI(4);
  });
  this->addNodes(&wifiAttackMenu, "Assoc Sleep Targ", TFTMAGENTA, DEAUTH_SNIFF, [this]() {
    this->startAttackDashUI(5);
  });
  this->addNodes(&wifiAttackMenu, "SAE Commit Flood", TFTLIME, EAPOL, [this]() {
    this->startAttackDashUI(6);
  });
  this->addNodes(&wifiAttackMenu, "Channel Switch", TFTORANGE, BEACON_LIST, [this]() {
    this->startAttackDashUI(7);
  });
  this->addNodes(&wifiAttackMenu, "Quiet Time", TFTRED, BEACON_LIST, [this]() {
    this->startAttackDashUI(8);
  });
  // R105 attack apps.
  this->addNodes(&wifiAttackMenu, "Auth Rush", TFTLIME, EAPOL, [this]() {
    this->startAttackDashUI(9);
  });
  this->addNodes(&wifiAttackMenu, "Mimic Clone", TFTCYAN, BEACON_LIST, [this]() {
    this->startAttackDashUI(10);
  });
  this->addNodes(&wifiAttackMenu, "Handshake Harvest", TFTVIOLET, EAPOL, [this]() {
    // Capture-only app: the Wi-Fi monitor dashboard already renders live
    // EAPOL/complete-handshake totals for this mode, and it never transmits
    // unless the optional deauth assist setting is enabled.
    this->startWifiToolUI(WIFI_SCAN_ACTIVE_LIST_EAPOL, TFT_VIOLET);
  });

  evilPortalMenu.parentMenu = &wifiAttackMenu;
  this->addNodes(&evilPortalMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(evilPortalMenu.parentMenu, true);
  });
  this->addNodes(&evilPortalMenu, "Access Points", TFTGREEN, BEACON_SNIFF, [this]() {
    this->changeMenu(&wifiAPMenu, true);
  });
  this->addNodes(&evilPortalMenu, "User SSIDs", TFTCYAN, PROBE_SNIFF, [this]() {
    this->changeMenu(&ssidsMenu, true);
  });

  // Build WiFi General menu
  wifiGeneralMenu.parentMenu = &wifiMenu;
  this->addNodes(&wifiGeneralMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(wifiGeneralMenu.parentMenu, true);
  });
  this->addNodes(&wifiGeneralMenu, text_table1[27], TFTSKYBLUE, GENERATE, [this]() {
    this->ssidStudio();
    this->changeMenu(&wifiGeneralMenu, true);
  });
  // Active AP survey with full per-network detail; selections become the
  // shared attack targets.
  this->addNodes(&wifiGeneralMenu, "Scan AP", TFTCYAN, SCANNERS, [this]() {
    this->scanApStudio();
    this->changeMenu(&wifiGeneralMenu, true);
  });

	//Add Select probe ssid
  this->addNodes(&wifiGeneralMenu, text_table1[65], TFTCYAN, KEYBOARD_ICO, [this]() {
    selectProbeSSIDsMenu.list->clear();

    // Add the back button
    this->addNodes(&selectProbeSSIDsMenu, text09, TFTLIGHTGREY, 0, [this]() {
      this->changeMenu(&wifiGeneralMenu, true);

      // TODO: TBD - Should probe_req_ssids have it´s own life and override ap.config and/or ssids -list for EP?
      // If so, then we should not add selected ssids to ssids list

      // Add selected ssid names to ssids list when clicking back button
      if (probe_req_ssids->size() > 0) {

        //TODO: TBD - Clear ssids list before adding new ones??

        for (int i = 0; i < probe_req_ssids->size(); i++) {
          ProbeReqSsid cur_probe_ssid = probe_req_ssids->get(i);
          if (cur_probe_ssid.selected) {
            bool ssidExists = false;
            for (int i = 0; i < ssids->size(); i++) {
              if (ssids->get(i).essid == cur_probe_ssid.essid) {
                ssidExists = true;
                break;
              }
            }
            if (!ssidExists) {
              wifi_scan_obj.addSSID(cur_probe_ssid.essid);
          }
        }
      }
    }
    });

    // Populate the menu with buttons
    for (int i = 0; i < probe_req_ssids->size(); i++) {
      ProbeReqSsid cur_ssid = probe_req_ssids->get(i);
      // This is the menu node
      String button_name = "[" + String(cur_ssid.requests) + "]" + cur_ssid.essid;
      this->addNodes(
        &selectProbeSSIDsMenu,
        button_name.c_str(),
        TFTCYAN,
        255,
        [this, i]() {
          ProbeReqSsid new_ssid = probe_req_ssids->get(i);
          new_ssid.selected = !probe_req_ssids->get(i).selected;

          // Change selection status of menu node
          MenuNode new_node = current_menu->list->get(i + 1);
          new_node.selected = !current_menu->list->get(i + 1).selected;
          current_menu->list->set(i + 1, new_node);

          probe_req_ssids->set(i, new_ssid);
        },
        probe_req_ssids->get(i).selected);
    }
    this->changeMenu(&selectProbeSSIDsMenu, true);
  });

  clearSSIDsMenu.parentMenu = &wifiGeneralMenu;

  #ifdef HAS_ILI9341
    this->addNodes(&wifiGeneralMenu, text_table1[1], TFTNAVY, KEYBOARD_ICO, [this](){
      char ssidBuf[64] = {0};
      bool keep_going = true;
      while (keep_going) {
        display_obj.clearScreen(); 
        if (keyboardInput(ssidBuf, sizeof(ssidBuf), "Enter SSID")) {
          if (ssidBuf[0] != 0)
            wifi_scan_obj.addSSID(String(ssidBuf));
          for (int i = 0; i < 64; i++)
            ssidBuf[i] = 0;
        }
        else
          keep_going = false;
      }

      this->changeMenu(current_menu);
    });
  #endif
  #if (!defined(HAS_ILI9341) && defined(HAS_BUTTONS))
    this->addNodes(&wifiGeneralMenu, text_table1[1], TFTNAVY, KEYBOARD_ICO, [this](){
      this->changeMenu(&miniKbMenu, true);
      #ifdef HAS_MINI_KB
        this->miniKeyboard(&miniKbMenu);
      #endif
    });
  #endif
  this->addNodes(&wifiGeneralMenu, text_table1[28], TFTSILVER, CLEAR_ICO, [this]() {
    this->sharkNotice("SSID LIST CLEARED",
                      String(wifi_scan_obj.clearList(CLEAR_SSID)) + " entries removed");
    this->changeMenu(&wifiGeneralMenu, true);
  });
  this->addNodes(&wifiGeneralMenu, text_table1[29], TFTDARKGREY, CLEAR_ICO, [this]() {
    const int aps = wifi_scan_obj.clearList(CLEAR_APS);
    wifi_scan_obj.clearList(CLEAR_STA);
    this->sharkNotice("ACCESS POINTS CLEARED", String(aps) + " APs removed");
    this->changeMenu(&wifiGeneralMenu, true);
  });
  this->addNodes(&wifiGeneralMenu, text_table1[60], TFTBLUE, CLEAR_ICO, [this]() {
    this->sharkNotice("STATIONS CLEARED",
                      String(wifi_scan_obj.clearList(CLEAR_STA)) + " stations removed");
    this->changeMenu(&wifiGeneralMenu, true);
  });
  //#else // Mini EP HTML select
    this->addNodes(&wifiGeneralMenu, "Select EP HTML File", TFTCYAN, KEYBOARD_ICO, [this](){
      // Add the back button
      htmlMenu.list->clear();
        this->addNodes(&htmlMenu, text09, TFTLIGHTGREY, 0, [this]() {
        this->changeMenu(htmlMenu.parentMenu, true);
      });

      // Populate the menu with buttons
      for (int i = 0; i < evil_portal_obj.html_files->size(); i++) {
        // This is the menu node
        this->addNodes(&htmlMenu, evil_portal_obj.html_files->get(i).c_str(), TFTCYAN, 255, [this, i](){
          evil_portal_obj.selected_html_index = i;
          evil_portal_obj.target_html_name = evil_portal_obj.html_files->get(evil_portal_obj.selected_html_index);
          Serial.println("Set Evil Portal HTML as " + evil_portal_obj.target_html_name);
          evil_portal_obj.using_serial_html = false;
          this->changeMenu(htmlMenu.parentMenu, true);
          return;
        });
      }
      this->changeMenu(&htmlMenu, true);
    });

    //#if (!defined(HAS_ILI9341) && defined(HAS_BUTTONS))
      miniKbMenu.parentMenu = &wifiGeneralMenu;
      #if !defined(MARAUDER_CARDPUTER) && !defined(MARAUDER_CARDPUTER_ADV)
        this->addNodes(&miniKbMenu, "a", TFTCYAN, 0, [this]() {
          this->changeMenu(miniKbMenu.parentMenu, true);
        });
      #endif
    //#endif

    htmlMenu.parentMenu = &wifiGeneralMenu;
    this->addNodes(&htmlMenu, text09, TFTLIGHTGREY, 0, [this]() {
      this->changeMenu(htmlMenu.parentMenu, true);
    });

    // Select APs on Mini
    this->addNodes(&wifiGeneralMenu, "Select APs", TFTNAVY, KEYBOARD_ICO, [this](){
      wifiAPMenu.parentMenu = &wifiGeneralMenu;
      // Add the back button
      wifiAPMenu.list->clear();
        this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, 0, [this]() {
        this->changeMenu(wifiAPMenu.parentMenu, true);
      });

      this->addNodes(&wifiAPMenu, "Select ALL", TFTGREEN, 255, [this](){

        for (int x = 0; x < access_points->size(); x++) {
          AccessPoint new_ap = access_points->get(x);
          new_ap.selected = !access_points->get(x).selected;
          access_points->set(x, new_ap);

          MenuNode new_node = current_menu->list->get(x + 2);
          new_node.selected = !current_menu->list->get(x + 2).selected;
          current_menu->list->set(x + 2, new_node);
        }

        this->changeMenu(current_menu, true);

      });

      // Populate the menu with buttons
      for (int i = 0; i < access_points->size(); i++) {
        // This is the menu node
        this->addNodes(&wifiAPMenu, access_points->get(i).essid.c_str(), TFTCYAN, 255, [this, i](){
        AccessPoint new_ap = access_points->get(i);
        new_ap.selected = !access_points->get(i).selected;

        // Change selection status of menu node
        MenuNode new_node = current_menu->list->get(i + 2);
        new_node.selected = !current_menu->list->get(i + 2).selected;
        current_menu->list->set(i + 2, new_node);

        access_points->set(i, new_ap);
        }, access_points->get(i).selected);
      }
      this->changeMenu(&wifiAPMenu, true);
    });

    this->addNodes(&wifiGeneralMenu, "View AP Info", TFTCYAN, KEYBOARD_ICO, [this](){
      wifiAPMenu.parentMenu = &wifiGeneralMenu;
      
      // Add the back button
      wifiAPMenu.list->clear();
        this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, 0, [this]() {
        this->changeMenu(wifiAPMenu.parentMenu, true);
      });

      // Populate the menu with buttons
      for (int i = 0; i < access_points->size(); i++) {
        // This is the menu node
        this->addNodes(&wifiAPMenu, access_points->get(i).essid.c_str(), TFTCYAN, 255, [this, i](){
          this->changeMenu(&apInfoMenu, true);
          wifi_scan_obj.RunAPInfo(i);
        });
      }
      this->changeMenu(&wifiAPMenu, true);
    });

    apInfoMenu.parentMenu = &wifiAPMenu;
    this->addNodes(&apInfoMenu, text09, TFTLIGHTGREY, 0, [this]() {
      this->changeMenu(apInfoMenu.parentMenu, true);
    });

    wifiAPMenu.parentMenu = &wifiGeneralMenu;
    this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, 0, [this]() {
      this->changeMenu(wifiAPMenu.parentMenu, true);
    });

    wifiIPMenu.parentMenu = &wifiScannerMenu;
    this->addNodes(&wifiIPMenu, text09, TFTLIGHTGREY, 0, [this]() {
      this->changeMenu(wifiIPMenu.parentMenu, true);
    });


    // Select Stations on Mini v2
    this->addNodes(&wifiGeneralMenu, "Select Stations", TFTCYAN, KEYBOARD_ICO, [this](){
      wifiAPMenu.parentMenu = &wifiGeneralMenu;

      wifiAPMenu.list->clear();
        this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, 0, [this]() {
        this->changeMenu(wifiAPMenu.parentMenu, true);
      });

      int menu_limit = access_points->size();


      for (int i = 0; i < menu_limit; i++) {
        wifiStationMenu.list->clear();
        this->addNodes(&wifiAPMenu, access_points->get(i).essid.c_str(), TFTCYAN, 255, [this, i](){

          wifiStationMenu.list->clear();

          wifiStationMenu.parentMenu = &wifiAPMenu;

          // Add back button to the APs
          this->addNodes(&wifiStationMenu, text09, TFTLIGHTGREY, 0, [this]() {
            this->changeMenu(wifiStationMenu.parentMenu, true);
          });

          this->addNodes(&wifiStationMenu, "Select ALL", TFTGREEN, 255, [this, i](){

            for (int y = 0; y < access_points->get(i).stations->size(); y++) {
              int cur_ap_sta_inx = access_points->get(i).stations->get(y);
              Station new_sta = stations->get(cur_ap_sta_inx);
              new_sta.selected = !stations->get(cur_ap_sta_inx).selected;

              // Change selection status of menu node
              MenuNode new_node = current_menu->list->get(y + 2);
              new_node.selected = !current_menu->list->get(y + 2).selected;
              current_menu->list->set(y + 2, new_node);

              stations->set(cur_ap_sta_inx, new_sta);
            }

            this->changeMenu(current_menu, true);

          });

          // Add the AP's stations to the specific AP menu
          for (int x = 0; x < access_points->get(i).stations->size(); x++) {
            int cur_ap_sta = access_points->get(i).stations->get(x);

            this->addNodes(&wifiStationMenu, macToString(stations->get(cur_ap_sta)).c_str(), TFTCYAN, 255, [this, i, cur_ap_sta, x](){
            Station new_sta = stations->get(cur_ap_sta);
            new_sta.selected = !stations->get(cur_ap_sta).selected;

            // Change selection status of menu node
            MenuNode new_node = current_menu->list->get(x + 2);
            new_node.selected = !current_menu->list->get(x + 2).selected;
            current_menu->list->set(x + 2, new_node);

            stations->set(cur_ap_sta, new_sta);
            }, stations->get(cur_ap_sta).selected);
          }

          // Final change menu to the menu of Stations
          this->changeMenu(&wifiStationMenu, true);
          
        }, false);
      }
      this->changeMenu(&wifiAPMenu, true);
    });

    this->addNodes(&wifiGeneralMenu, "Join WiFi", TFTWHITE, KEYBOARD_ICO, [this](){

      wifiAPMenu.parentMenu = &wifiGeneralMenu;

      // Add the back button
      wifiAPMenu.list->clear();
        this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, 0, [this]() {
        this->changeMenu(wifiAPMenu.parentMenu, true);
      });

      // Populate the menu with buttons
      for (int i = 0; i < access_points->size(); i++) {
        // This is the menu node
        this->addNodes(&wifiAPMenu, access_points->get(i).essid.c_str(), TFTCYAN, 255, [this, i](){
          // Join WiFi using mini keyboard
          #ifdef HAS_MINI_KB
            this->changeMenu(&miniKbMenu, true);
            String password = this->miniKeyboard(&miniKbMenu, true);
            if (password != "") {
              Serial.println("Using SSID: " + (String)access_points->get(i).essid + " Password: " + (String)password);
              wifi_scan_obj.currentScanMode = LV_JOIN_WIFI;
              wifi_scan_obj.StartScan(LV_JOIN_WIFI, TFT_YELLOW); 
              wifi_scan_obj.joinWiFi(access_points->get(i).essid, password);
              this->changeMenu(current_menu, true);
            }
          #endif

          // Join WiFi using touch screen keyboard
          #ifdef HAS_TOUCH
            char passwordBuf[64] = {0};  // or prefill with existing SSID
            if (keyboardInput(passwordBuf, sizeof(passwordBuf), "Enter Password")) {
              wifi_scan_obj.joinWiFi(access_points->get(i).essid, String(passwordBuf), true);
            }

            this->changeMenu(&wifiGeneralMenu, true);
          #endif
        });
      }
      this->changeMenu(&wifiAPMenu, true);
    });

    this->addNodes(&wifiGeneralMenu, "Join Saved WiFi", TFTWHITE, KEYBOARD_ICO, [this](){
      String ssid = settings_obj.loadSetting<String>("ClientSSID");
      String pw = settings_obj.loadSetting<String>("ClientPW");

      if ((ssid != "") && (pw != "")) {
        wifi_scan_obj.joinWiFi(ssid, pw, false);
        this->changeMenu(&wifiGeneralMenu, true);
      }
      else {
        wifiAPMenu.parentMenu = &wifiGeneralMenu;

        // Add the back button
        wifiAPMenu.list->clear();
          this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, 0, [this]() {
          this->changeMenu(wifiAPMenu.parentMenu, true);
        });

        // Populate the menu with buttons
        for (int i = 0; i < access_points->size(); i++) {
          // This is the menu node
          this->addNodes(&wifiAPMenu, access_points->get(i).essid.c_str(), TFTCYAN, 255, [this, i](){
            // Join WiFi using mini keyboard
            #ifdef HAS_MINI_KB
              this->changeMenu(&miniKbMenu, true);
              String password = this->miniKeyboard(&miniKbMenu, true);
              if (password != "") {
                Serial.println("Using SSID: " + (String)access_points->get(i).essid + " Password: " + (String)password);
                wifi_scan_obj.currentScanMode = LV_JOIN_WIFI;
                wifi_scan_obj.StartScan(LV_JOIN_WIFI, TFT_YELLOW); 
                wifi_scan_obj.joinWiFi(access_points->get(i).essid, password);
                this->changeMenu(current_menu, true);
              }
            #endif

            // Join WiFi using touch screen keyboard
            #ifdef HAS_TOUCH
              char passwordBuf[64] = {0};  // or prefill with existing SSID
              if (keyboardInput(passwordBuf, sizeof(passwordBuf), "Enter Password")) {
                wifi_scan_obj.joinWiFi(access_points->get(i).essid, String(passwordBuf), true);
              }

              this->changeMenu(&wifiGeneralMenu, true);
            #endif
          });
        }
        this->changeMenu(&wifiAPMenu, true);
      }
    });

    this->addNodes(&wifiGeneralMenu, "Start AP", TFTGREEN, KEYBOARD_ICO, [this](){
      ssidsMenu.parentMenu = &wifiGeneralMenu;

      // Add the back button
      ssidsMenu.list->clear();
        this->addNodes(&ssidsMenu, text09, TFTLIGHTGREY, 0, [this]() {
        this->changeMenu(ssidsMenu.parentMenu, true);
      });

      // Populate the menu with buttons
      for (int i = 0; i < ssids->size(); i++) {
        // This is the menu node
        this->addNodes(&ssidsMenu, ssids->get(i).essid.c_str(), TFTCYAN, 255, [this, i](){
          // Join WiFi using mini keyboard
          #ifdef HAS_MINI_KB
            this->changeMenu(&miniKbMenu, true);
            String password = this->miniKeyboard(&miniKbMenu, true);
            if (password != "") {
              Serial.println("Using SSID: " + (String)ssids->get(i).essid + " Password: " + (String)password);
              wifi_scan_obj.currentScanMode = LV_JOIN_WIFI;
              wifi_scan_obj.StartScan(LV_JOIN_WIFI, TFT_YELLOW); 
              wifi_scan_obj.startWiFi(ssids->get(i).essid, password);
              this->changeMenu(current_menu, true);
            }
          #endif

          // Join WiFi using touch screen keyboard
          #ifdef HAS_TOUCH
            char passwordBuf[64] = {0};  // or prefill with existing SSID
            if (keyboardInput(passwordBuf, sizeof(passwordBuf), "Enter Password")) {
              Serial.println("Using SSID: " + (String)ssids->get(i).essid + " Password: " + String(passwordBuf));
              wifi_scan_obj.startWiFi(ssids->get(i).essid, String(passwordBuf));
            }

            this->changeMenu(&wifiGeneralMenu, false);
          #endif
        });
      }
      this->changeMenu(&ssidsMenu, true);
    });

    this->addNodes(&wifiGeneralMenu, "Host AP Info", TFTGREEN, BEACON_SNIFF, [this]() {
      display_obj.clearScreen();
      this->drawStatusBar();
      wifi_scan_obj.StartScan(WIFI_SCAN_DISPLAY_AP_INFO, TFT_GREEN);
    });

    wifiStationMenu.parentMenu = &ssidsMenu;
    this->addNodes(&wifiStationMenu, text09, TFTLIGHTGREY, 0, [this]() {
      this->changeMenu(wifiStationMenu.parentMenu, true);
    });

  this->addNodes(&wifiGeneralMenu, "Set MACs", TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(&setMacMenu, true);
  });

  this->addNodes(&wifiGeneralMenu, "Shutdown WiFi", TFTRED, 0, [this]() {
    WiFi.softAPdisconnect(true); // Also shut down the SoftAP if it is running
	WiFi.disconnect(true);
    delay(100);
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF, TFT_RED);
    this->changeMenu(current_menu, true);
  });
  
  #ifdef HAS_DIRECT_UPLOAD
    this->addNodes(&wifiGeneralMenu, "Upload Wardrive Logs", TFTGREEN, 0, [this]() {
      display_obj.clearScreen();
      display_obj.tft.setTextWrap(false);
      display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
      display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
      display_obj.tft.println("Loading...");

      this->buildUploadFileMenu();

      this->changeMenu(&uploadLogsMenu, true);
    });

    uploadAllMenu.parentMenu = &uploadLogsMenu;
    this->addNodes(&uploadAllMenu, text09, TFTLIGHTGREY, 0, [this]() {
      this->changeMenu(uploadAllMenu.parentMenu, true);
    });
    this->addNodes(&uploadAllMenu, "WiGLE", TFTLIGHTGREY, 0, [this]() {
      display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);

      String ssid = settings_obj.loadSetting<String>("ClientSSID");
      String pw = settings_obj.loadSetting<String>("ClientPW");

      if ((ssid == "") && (pw == "")) {
        display_obj.clearScreen();
        display_obj.tft.setTextWrap(true);
        display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
        display_obj.tft.println("WiFi Credentials Empty.");
        display_obj.tft.println("Returning...");
        display_obj.tft.setTextWrap(false);
      }
      else {
        display_obj.clearScreen();
        display_obj.showCenterText(String("Connecting to " + ssid).c_str(), TFT_HEIGHT / 2, true);
        if (!wifi_scan_obj.joinWiFi(ssid, pw, false)) {
          display_obj.clearScreen();
          display_obj.tft.setTextWrap(true);
          display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
          display_obj.tft.println("Could not connect to WiFi.");
          display_obj.tft.println("Returning...");
          display_obj.tft.setTextWrap(false);
        }
        else {
          delay(1000);
          for (int i = 0; i < sd_obj.sd_files->size(); i++) {
            if (sd_obj.sd_files->get(i).startsWith("wardrive_") || sd_obj.sd_files->get(i).startsWith("wigle-")) {
              if (!sd_obj.sd_files->get(i).endsWith(".wigle") && !sd_obj.sd_files->get(i).endsWith(".wdg") && !sd_obj.sd_files->get(i).endsWith(".gpx")) {
                Serial.println("Uploading " + sd_obj.sd_files->get(i) + "...");
                if (wifi_scan_obj.uploadFile("/" + sd_obj.sd_files->get(i), true, WIGLE_UPLOAD)) {
                  display_obj.clearScreen();
                  display_obj.showCenterText("WiGLE OK", TFT_HEIGHT / 2);
                } else {
                  display_obj.clearScreen();
                  display_obj.showCenterText("WiGLE failed", TFT_HEIGHT / 2);
                }
              }
            }
          }
          WiFi.disconnect(true);
          delay(100);
          wifi_scan_obj.StartScan(WIFI_SCAN_OFF, TFT_RED);
        }
      }

      delay(2000);

      this->changeMenu(uploadAllMenu.parentMenu, true);
    });
    this->addNodes(&uploadAllMenu, "WDGWars", TFTLIGHTGREY, 0, [this]() {
      String ssid = settings_obj.loadSetting<String>("ClientSSID");
      String pw = settings_obj.loadSetting<String>("ClientPW");

      display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);

      if ((ssid == "") && (pw == "")) {
        display_obj.clearScreen();
        display_obj.tft.setTextWrap(true);
        display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
        display_obj.tft.println("WiFi Credentials Empty.");
        display_obj.tft.println("Returning...");
        display_obj.tft.setTextWrap(false);
      }
      else {
        display_obj.clearScreen();
        display_obj.showCenterText(String("Connecting to " + ssid).c_str(), TFT_HEIGHT / 2, true);
        if (!wifi_scan_obj.joinWiFi(ssid, pw, false)) {
          display_obj.clearScreen();
          display_obj.tft.setTextWrap(true);
          display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
          display_obj.tft.println("Could not connect to WiFi.");
          display_obj.tft.println("Returning...");
          display_obj.tft.setTextWrap(false);
        }
        else {
          delay(1000);
          for (int i = 0; i < sd_obj.sd_files->size(); i++) {
            if (sd_obj.sd_files->get(i).startsWith("wardrive_") || sd_obj.sd_files->get(i).startsWith("wigle-")) {
              if (!sd_obj.sd_files->get(i).endsWith(".wigle") && !sd_obj.sd_files->get(i).endsWith(".wdg") && !sd_obj.sd_files->get(i).endsWith(".gpx")) {
                Serial.println("Uploading " + sd_obj.sd_files->get(i) + "...");
                if (wifi_scan_obj.uploadFile("/" + sd_obj.sd_files->get(i), true, WDG_UPLOAD)) {
                  display_obj.clearScreen();
                  display_obj.showCenterText("WDG OK", TFT_HEIGHT / 2);
                } else {
                  display_obj.clearScreen();
                  display_obj.showCenterText("WDG failed", TFT_HEIGHT / 2);
                }
              }
            }
          }
          WiFi.disconnect(true);
          delay(100);
          wifi_scan_obj.StartScan(WIFI_SCAN_OFF, TFT_RED);
        }
      }

      delay(2000);

      this->changeMenu(uploadAllMenu.parentMenu, true);
    });
    this->addNodes(&uploadAllMenu, "Both", TFTLIGHTGREY, 0, [this]() {
      String ssid = settings_obj.loadSetting<String>("ClientSSID");
      String pw = settings_obj.loadSetting<String>("ClientPW");

      display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);

      if ((ssid == "") && (pw == "")) {
        display_obj.clearScreen();
        display_obj.tft.setTextWrap(true);
        display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
        display_obj.tft.println("WiFi Credentials Empty.");
        display_obj.tft.println("Returning...");
        display_obj.tft.setTextWrap(false);
      }
      else {
        display_obj.clearScreen();
        display_obj.showCenterText(String("Connecting to " + ssid).c_str(), TFT_HEIGHT / 2, true);
        if (!wifi_scan_obj.joinWiFi(ssid, pw, false)) {
          display_obj.clearScreen();
          display_obj.tft.setTextWrap(true);
          display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
          display_obj.tft.println("Could not connect to WiFi.");
          display_obj.tft.println("Returning...");
          display_obj.tft.setTextWrap(false);
        }
        else {
          delay(1000);
          for (int i = 0; i < sd_obj.sd_files->size(); i++) {
            if (sd_obj.sd_files->get(i).startsWith("wardrive_") || sd_obj.sd_files->get(i).startsWith("wigle-")) {
              if (!sd_obj.sd_files->get(i).endsWith(".wigle") && !sd_obj.sd_files->get(i).endsWith(".wdg") && !sd_obj.sd_files->get(i).endsWith(".gpx")) {
                Serial.println("Uploading " + sd_obj.sd_files->get(i) + "...");
                if (wifi_scan_obj.uploadFile("/" + sd_obj.sd_files->get(i), true, BOTH_UPLOAD)) {
                  display_obj.clearScreen();
                  display_obj.showCenterText("Upload OK", TFT_HEIGHT / 2);
                } else {
                  display_obj.clearScreen();
                  display_obj.showCenterText("Upload failed", TFT_HEIGHT / 2);
                }
              }
            }
          }
          WiFi.disconnect(true);
          delay(100);
          wifi_scan_obj.StartScan(WIFI_SCAN_OFF, TFT_RED);
        }
      }

      delay(2000);

      this->changeMenu(uploadAllMenu.parentMenu, true);
    });

    deleteAllMenu.parentMenu = &uploadLogsMenu;
    this->addNodes(&deleteAllMenu, "No", TFTLIGHTGREY, 0, [this]() {
      this->changeMenu(deleteAllMenu.parentMenu, true);
    });
    this->addNodes(&deleteAllMenu, "Yes", TFTRED, 0, [this]() {
      display_obj.tft.setTextColor(TFT_ORANGE, TFT_BLACK);

      display_obj.clearScreen();

      display_obj.showCenterText("Deleting logs...", TFT_HEIGHT / 2, true);

      for (int i = 0; i < sd_obj.sd_files->size(); i++) {
        if (sd_obj.sd_files->get(i).startsWith("wardrive_") || sd_obj.sd_files->get(i).startsWith("wigle-")) {
          if (sd_obj.removeFile("/" + sd_obj.sd_files->get(i))) {
            Serial.println("Removed file: " + sd_obj.sd_files->get(i));
            sd_obj.removeFile("/" + sd_obj.sd_files->get(i) + ".wdg");
            sd_obj.removeFile("/" + sd_obj.sd_files->get(i) + ".wigle");
          }
          else {
            Serial.println("Could not remove file: " + sd_obj.sd_files->get(i));
          }
        }
      }
      display_obj.clearScreen();

      display_obj.showCenterText("Logs removed", TFT_HEIGHT / 2, true);

      delay(2000);

      this->buildUploadFileMenu();

      this->changeMenu(&uploadLogsMenu, true);
    });

    actionMenu.parentMenu = &uploadLogsMenu;
    this->addNodes(&actionMenu, text09, TFTLIGHTGREY, 0, [this]() {
      this->changeMenu(actionMenu.parentMenu, true);
    });
    this->addNodes(&actionMenu, "WiGLE", TFTLIGHTGREY, 0, [this]() {
      String ssid = settings_obj.loadSetting<String>("ClientSSID");
      String pw = settings_obj.loadSetting<String>("ClientPW");

      display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);

      if ((ssid == "") && (pw == "")) {
        display_obj.clearScreen();
        display_obj.tft.setTextWrap(true);
        display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
        display_obj.tft.println("WiFi Credentials Empty.");
        display_obj.tft.println("Returning...");
        display_obj.tft.setTextWrap(false);
      }
      else {
        display_obj.clearScreen();
        display_obj.showCenterText(String("Connecting to " + ssid).c_str(), TFT_HEIGHT / 2, true);
        if (!wifi_scan_obj.joinWiFi(ssid, pw, false)) {
          display_obj.clearScreen();
          display_obj.tft.setTextWrap(true);
          display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
          display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
          display_obj.tft.println("Could not connect to WiFi.");
          display_obj.tft.println("Returning...");
          display_obj.tft.setTextWrap(false);
        }
        else {
          delay(1000);
          Serial.println("Uploading " + sd_obj.selected_file_name + "...");
          if (wifi_scan_obj.uploadFile("/" + sd_obj.selected_file_name, true, WIGLE_UPLOAD)) {
            display_obj.clearScreen();
            display_obj.showCenterText("WiGLE OK", TFT_HEIGHT / 2, true);
          } else {
            display_obj.clearScreen();
            display_obj.showCenterText("WiGLE failed", TFT_HEIGHT / 2, true);
          }

          WiFi.disconnect(true);
          delay(100);
          wifi_scan_obj.StartScan(WIFI_SCAN_OFF, TFT_RED);
        }
      }

      delay(2000);

      this->changeMenu(&actionMenu, true);
    });
    this->addNodes(&actionMenu, "WDGWars", TFTLIGHTGREY, 0, [this]() {
      String ssid = settings_obj.loadSetting<String>("ClientSSID");
      String pw = settings_obj.loadSetting<String>("ClientPW");

      display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);

      if ((ssid == "") && (pw == "")) {
        display_obj.clearScreen();
        display_obj.tft.setTextWrap(true);
        display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
        display_obj.tft.println("WiFi Credentials Empty.");
        display_obj.tft.println("Returning...");
        display_obj.tft.setTextWrap(false);
      }
      else {
        display_obj.clearScreen();
        display_obj.showCenterText(String("Connecting to " + ssid).c_str(), TFT_HEIGHT / 2, true);
        if (!wifi_scan_obj.joinWiFi(ssid, pw, false)) {
          display_obj.clearScreen();
          display_obj.tft.setTextWrap(true);
          display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
          display_obj.tft.println("Could not connect to WiFi.");
          display_obj.tft.println("Returning...");
          display_obj.tft.setTextWrap(false);
        }
        else {
          delay(1000);
          Serial.println("Uploading " + sd_obj.selected_file_name + "...");
          if (wifi_scan_obj.uploadFile("/" + sd_obj.selected_file_name, true, WDG_UPLOAD)) {
            display_obj.clearScreen();
            display_obj.showCenterText("WDG OK", TFT_HEIGHT / 2, true);
          } else {
            display_obj.clearScreen();
            display_obj.showCenterText("WDG failed", TFT_HEIGHT / 2, true);
          }

          WiFi.disconnect(true);
        delay(100);
        wifi_scan_obj.StartScan(WIFI_SCAN_OFF, TFT_RED);
        }
      }

      delay(2000);

      this->changeMenu(&actionMenu, true);
    });
    this->addNodes(&actionMenu, "Both", TFTLIGHTGREY, 0, [this]() {
      String ssid = settings_obj.loadSetting<String>("ClientSSID");
      String pw = settings_obj.loadSetting<String>("ClientPW");

      display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);

      if ((ssid == "") && (pw == "")) {
        display_obj.clearScreen();
        display_obj.tft.setTextWrap(true);
        display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
        display_obj.tft.println("WiFi Credentials Empty.");
        display_obj.tft.println("Returning...");
        display_obj.tft.setTextWrap(false);
      }
      else {
        display_obj.clearScreen();
        display_obj.showCenterText(String("Connecting to " + ssid).c_str(), TFT_HEIGHT / 2, true);
        if (!wifi_scan_obj.joinWiFi(ssid, pw, false)) {
          display_obj.clearScreen();
          display_obj.tft.setTextWrap(true);
          display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
          display_obj.tft.println("Could not connect to WiFi.");
          display_obj.tft.println("Returning...");
          display_obj.tft.setTextWrap(false);
        }
        else {
          delay(1000);
          Serial.println("Uploading " + sd_obj.selected_file_name + "...");
          if (wifi_scan_obj.uploadFile("/" + sd_obj.selected_file_name, true, BOTH_UPLOAD)) {
            display_obj.clearScreen();
            display_obj.showCenterText("Upload OK", TFT_HEIGHT / 2, true);
          } else {
            display_obj.clearScreen();
            display_obj.showCenterText("Upload failed", TFT_HEIGHT / 2, true);
          }

          WiFi.disconnect(true);
          delay(100);
          wifi_scan_obj.StartScan(WIFI_SCAN_OFF, TFT_RED);
        }
      }

      delay(2000);

      this->changeMenu(&actionMenu, true);
    });
  #endif


  // Menu for generating and setting MAC addrs for AP and STA
  setMacMenu.parentMenu = &wifiGeneralMenu;
  this->addNodes(&setMacMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(setMacMenu.parentMenu, true);
  });

  // Generate random MAC for AP
  this->addNodes(&setMacMenu, "Generate AP MAC", TFTLIME, 0, [this]() {
    this->changeMenu(&genAPMacMenu, true);
    wifi_scan_obj.RunGenerateRandomMac(true);
  });

  // Generate random MAC for AP
  this->addNodes(&setMacMenu, "Generate STA MAC", TFTCYAN, 0, [this]() {
    this->changeMenu(&genAPMacMenu, true);
    wifi_scan_obj.RunGenerateRandomMac(false);
  });

  // Clone AP MAC to ESP32 for button folks
  //#ifndef HAS_ILI9341
    this->addNodes(&setMacMenu, "Clone AP MAC", TFTRED, CLEAR_ICO, [this](){
      wifiAPMenu.parentMenu = &wifiGeneralMenu;

      // Add the back button
      wifiAPMenu.list->clear();
        this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, 0, [this]() {
        this->changeMenu(wifiAPMenu.parentMenu, true);
      });

      // Populate the menu with buttons
      for (int i = 0; i < access_points->size(); i++) {
        // This is the menu node
        this->addNodes(&wifiAPMenu, access_points->get(i).essid.c_str(), TFTLIME, 255, [this, i](){
          this->changeMenu(&genAPMacMenu, true);
          wifi_scan_obj.RunSetMac(access_points->get(i).bssid, true);
        });
      }
      this->changeMenu(&wifiAPMenu, true);
    });

    this->addNodes(&setMacMenu, "Clone STA MAC", TFTMAGENTA, CLEAR_ICO, [this](){
      wifiAPMenu.parentMenu = &wifiGeneralMenu;

      // Add the back button
      wifiAPMenu.list->clear();
        this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, 0, [this]() {
        this->changeMenu(wifiAPMenu.parentMenu, true);
      });

      // Populate the menu with buttons
      for (int i = 0; i < stations->size(); i++) {
        // This is the menu node
        this->addNodes(&wifiAPMenu, macToString(stations->get(i).mac).c_str(), TFTMAGENTA, 255, [this, i](){
          this->changeMenu(&genAPMacMenu, true);
          wifi_scan_obj.RunSetMac(stations->get(i).mac, false);
        });
      }
      this->changeMenu(&wifiAPMenu, true);
    });
  //#endif

  // Menu for generating and setting access point MAC (just goes bacK)
  genAPMacMenu.parentMenu = &wifiGeneralMenu;
  this->addNodes(&genAPMacMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(genAPMacMenu.parentMenu, true);
  });

  // Build generate ssids menu
  generateSSIDsMenu.parentMenu = &wifiGeneralMenu;
  this->addNodes(&generateSSIDsMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(generateSSIDsMenu.parentMenu, true);
  });

  // Build clear ssids menu
  
  this->addNodes(&clearSSIDsMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(clearSSIDsMenu.parentMenu, true);
  });
  clearAPsMenu.parentMenu = &wifiGeneralMenu;
  this->addNodes(&clearAPsMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(clearAPsMenu.parentMenu, true);
  });

#ifdef HAS_BT
  // Build Bluetooth Menu
  bluetoothMenu.parentMenu = &mainMenu; // Second Menu is third menu parent
  this->addNodes(&bluetoothMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(bluetoothMenu.parentMenu, true);
  });
  this->addNodes(&bluetoothMenu, text_table1[31], TFTYELLOW, SNIFFERS, [this]() {
    this->changeMenu(&bluetoothSnifferMenu, true);
  });
  this->addNodes(&bluetoothMenu, "Bluetooth Attacks", TFTRED, ATTACKS, [this]() {
    this->changeMenu(&bluetoothAttackMenu, true);
  });

  // Build bluetooth sniffer Menu
  bluetoothSnifferMenu.parentMenu = &bluetoothMenu; // Second Menu is third menu parent
  this->addNodes(&bluetoothSnifferMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(bluetoothSnifferMenu.parentMenu, true);
  });
  this->addNodes(&bluetoothSnifferMenu, text_table1[34], TFTGREEN, BLUETOOTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_SCAN_ALL, TFT_GREEN);
    this->drawBluetoothSnifferUI(true);
  });
  this->addNodes(&bluetoothSnifferMenu, "Flipper Sniff", TFTORANGE, FLIPPER, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_SCAN_FLIPPER, TFT_ORANGE);
    this->drawFlipperSnifferUI(true);
  });
  this->addNodes(&bluetoothSnifferMenu, "FindMy Sniff", TFTWHITE, BLUETOOTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_SCAN_AIRTAG, TFT_WHITE);
    this->drawPassiveBleDetectorUI(true);
  });
  this->addNodes(&bluetoothSnifferMenu, "FindMy Monitor", TFTWHITE, BLUETOOTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_SCAN_AIRTAG_MON, TFT_WHITE);
    this->drawPassiveBleDetectorUI(true);
  });
  this->addNodes(&bluetoothSnifferMenu, text_table1[35], TFTMAGENTA, CC_SKIMMERS, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_SCAN_SKIMMERS, TFT_MAGENTA);
    this->drawCardSkimmerUI(true);
  });
  this->addNodes(&bluetoothSnifferMenu, "Bluetooth Analyzer", TFTCYAN, PACKET_MONITOR, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_SCAN_ANALYZER, TFT_CYAN);
    this->renderGraphUI(BT_SCAN_ANALYZER);
  });
  this->addNodes(&bluetoothSnifferMenu, "Flock Sniff", TFTORANGE, FLOCK, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_SCAN_FLOCK, TFT_ORANGE);
    this->drawPassiveBleDetectorUI(true);
  });
  this->addNodes(&bluetoothSnifferMenu, "Meta Detect", TFTWHITE, BLUETOOTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_SCAN_RAYBAN, TFT_CYAN);
    this->drawPassiveBleDetectorUI(true);
  });
  this->addNodes(&bluetoothSnifferMenu, "Fox Hunt", TFTCYAN, SCANNERS, [this]() {
    foxHuntMenu.list->clear();

    // Bluetooth Fox Hunt Menu
    foxHuntMenu.parentMenu = &bluetoothSnifferMenu; // Second Menu is third menu parent
    this->addNodes(&foxHuntMenu, text09, TFTLIGHTGREY, 0, [this]() {
      this->changeMenu(foxHuntMenu.parentMenu, true);
    });
    
    // Order targets strongest-signal first (stable) so the closest device to
    // hunt sits at the top of the list -- upstream v1.15.0 fox-hunt sort, kept
    // in the SHARK list UI. Snapshot RSSI once to avoid repeated list walks.
    {
      int n = ble_devices->size();
      std::vector<int> order(n);
      std::vector<int> rssis(n);
      for (int i = 0; i < n; i++) { order[i] = i; rssis[i] = ble_devices->get(i).rssi; }
      std::stable_sort(order.begin(), order.end(),
                       [&rssis](int a, int b) { return rssis[a] > rssis[b]; });
      for (int k = 0; k < n; k++) {
        int i = order[k];
        BleDevice ble_device = ble_devices->get(i);
        ble_device.selected = false;
        ble_devices->set(i, ble_device);
        uint8_t node_color = rssiToMenuColor(ble_devices->get(i).rssi);
        String node_name = String(ble_devices->get(i).rssi) + " " + ble_devices->get(i).name;
        this->addNodes(&foxHuntMenu, node_name.c_str(), node_color, 255, [this, i](){
          BleDevice ble_device = ble_devices->get(i);
          ble_device.selected = true;
          ble_devices->set(i, ble_device);
          display_obj.clearScreen();
          this->drawStatusBar();
          wifi_scan_obj.StartScan(BT_SCAN_FOX_HUNT, TFT_CYAN);
          this->drawFoxHuntUI(true);
        });
      }
    }
    this->changeMenu(&foxHuntMenu, true);
  });

  // Bluetooth Attack menu
  bluetoothAttackMenu.parentMenu = &bluetoothMenu; // Second Menu is third menu parent
  this->addNodes(&bluetoothAttackMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(bluetoothAttackMenu.parentMenu, true);
  });
  this->addNodes(&bluetoothAttackMenu, "Sour Apple", TFTGREEN, DEAUTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_ATTACK_SOUR_APPLE, TFT_GREEN);
    this->drawBleSpamUI(true);
  });
  this->addNodes(&bluetoothAttackMenu, "Apple Juice", TFTYELLOW, DEAUTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_ATTACK_APPLE_JUICE, TFT_YELLOW);
    this->drawBleSpamUI(true);
  });
  this->addNodes(&bluetoothAttackMenu, "Swiftpair Spam", TFTCYAN, KEYBOARD_ICO, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_ATTACK_SWIFTPAIR_SPAM, TFT_CYAN);
    this->drawBleSpamUI(true);
  });
  this->addNodes(&bluetoothAttackMenu, "Samsung BLE Spam", TFTRED, GENERAL_APPS, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_ATTACK_SAMSUNG_SPAM, TFT_RED);
    this->drawBleSpamUI(true);
  });
  this->addNodes(&bluetoothAttackMenu, "Google BLE Spam", TFTPURPLE, LANGUAGE, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_ATTACK_GOOGLE_SPAM, TFT_PURPLE);
    this->drawBleSpamUI(true);
  });
  this->addNodes(&bluetoothAttackMenu, "Flipper BLE Spam", TFTORANGE, FLIPPER, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_ATTACK_FLIPPER_SPAM, TFT_ORANGE);
    this->drawBleSpamUI(true);
  });
  this->addNodes(&bluetoothAttackMenu, "BLE Spam All", TFTMAGENTA, DEAUTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_ATTACK_SPAM_ALL, TFT_MAGENTA);
    this->drawBleSpamUI(true);
  });

#endif

  //#ifndef HAS_ILI9341
    #ifdef HAS_BT
      this->addNodes(&bluetoothAttackMenu, "Spoof Airtag", TFTWHITE, ATTACKS, [this](){
          wifiAPMenu.parentMenu = &bluetoothAttackMenu;

          // Clear nodes and add back button
          wifiAPMenu.list->clear();
          this->addNodes(&wifiAPMenu, text09, TFT_LIGHTGREY, 0, [this]() {
          this->changeMenu(wifiAPMenu.parentMenu, true);
        });

        // Add buttons for all airtags
        // Find out how big our menu is going to be
        int menu_limit;
        if (airtags->size() <= BUTTON_ARRAY_LEN)
          menu_limit = airtags->size();
        else
          menu_limit = BUTTON_ARRAY_LEN;

        // Create the menu nodes for all of the list items
        for (int i = 0; i < menu_limit; i++) {
          this->addNodes(&wifiAPMenu, airtags->get(i).mac.c_str(), TFTWHITE, BLUETOOTH, [this, i](){
            AirTag new_at = airtags->get(i);
            new_at.selected = true;

            airtags->set(i, new_at);

            // Set all other airtags to "Not Selected"
            for (int x = 0; x < airtags->size(); x++) {
              if (x != i) {
                AirTag new_atx = airtags->get(x);
                new_atx.selected = false;
                airtags->set(x, new_atx);
              }
            }

            // Start the spoof
            display_obj.clearScreen();
            this->drawStatusBar();
            wifi_scan_obj.StartScan(BT_SPOOF_AIRTAG, TFT_WHITE);

          });
        }
        this->changeMenu(&wifiAPMenu, true);
      });

      #ifdef HAS_NIMBLE_2
      this->addNodes(&bluetoothAttackMenu, "FindMy Sound", TFTCYAN, ATTACKS, [this](){
          wifiAPMenu.parentMenu = &bluetoothAttackMenu;

          // Clear nodes and add back button
          wifiAPMenu.list->clear();
          this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, 0, [this]() {
          this->changeMenu(wifiAPMenu.parentMenu, true);
        });

        /*this->addNodes(&wifiAPMenu, "Live", TFTMAGENTA, 0, [this]() {
          display_obj.clearScreen();
          this->drawStatusBar();
          wifi_scan_obj.StartScan(BT_ATTACK_FINDMY_LIVE, TFT_RED);
        });*/

        int menu_limit = airtags->size();

        // Create the menu nodes for all of the list items
        for (int i = 0; i < menu_limit; i++) {
          uint8_t node_color = rssiToMenuColor(airtags->get(i).rssi);
          String node_name = String(airtags->get(i).rssi) + " " + airtags->get(i).mac;
          this->addNodes(&wifiAPMenu, node_name.c_str(), node_color, BLUETOOTH, [this, i](){
            AirTag new_at = airtags->get(i);
            new_at.selected = true;
            new_at.connectable = true;

            airtags->set(i, new_at);

            // Set all other airtags to "Not Selected"
            for (int x = 0; x < airtags->size(); x++) {
              if (x != i) {
                AirTag new_atx = airtags->get(x);
                new_atx.selected = false;
                airtags->set(x, new_atx);
              }
            }

            // Start the spoof
            display_obj.clearScreen();
            this->drawStatusBar();
            wifi_scan_obj.executeFindMySound(true);
            delay(2000);
            this->changeMenu(&wifiAPMenu, true);
          });
        }
        this->changeMenu(&wifiAPMenu, true);
      });
      #endif

      wifiAPMenu.parentMenu = &bluetoothAttackMenu;
      this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, 0, [this]() {
        this->changeMenu(wifiAPMenu.parentMenu, true);
      });

      wifiAPMenu.parentMenu = &bluetoothAttackMenu;
      this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, 0, [this]() {
        this->changeMenu(wifiAPMenu.parentMenu, true);
      });
    #endif

  //#endif

  // Device menu
  deviceMenu.parentMenu = &mainMenu;
  this->addNodes(&deviceMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(deviceMenu.parentMenu, true);
  });

  #ifdef HAS_SD
    if (sd_obj.supported) {

      sdDeleteMenu.parentMenu = &deviceMenu;

      this->addNodes(&deviceMenu, "Update Firmware", TFTORANGE, SD_UPDATE, [this]() {
        display_obj.clearScreen();
        display_obj.tft.setTextWrap(false);
        display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
        display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
        display_obj.tft.println("Loading...");

        // Clear menu and lists
        this->buildSDFileMenu(true);

        this->changeMenu(&sdDeleteMenu, true);
      });
    }
  #endif

  this->addNodes(&deviceMenu, "Save/Load Files", TFTCYAN, SD_UPDATE, [this]() {
    this->changeMenu(&saveFileMenu, true);
  });

  #ifndef HAS_MINI_SCREEN
    this->addNodes(&deviceMenu, "Brightness", TFTYELLOW, BRIGHTNESS, [this]() {
      this->brightnessMode();
    });
  #endif

  #ifdef MARAUDER_V8
    this->addNodes(&deviceMenu, "Theme", TFTCYAN, DRAW, [this]() {
      this->changeMenu(&themeMenu, true);
    });
    #ifdef HAS_BT
      // BadUSB over BLE. Active/offensive, so it lives here rather than in the
      // receive-only Cyber Defense menu.
      this->addNodes(&deviceMenu, "BadUSB Tools", TFTRED, EAPOL, [this]() {
        bad_usb_obj.run();
        this->changeMenu(&deviceMenu, true);
      });
      // Chameleon Ultra remote: BLE central link with auto-connect.
      this->addNodes(&deviceMenu, "Chameleon Ultra", TFTGREEN, BLUETOOTH_SNIFF, [this]() {
        shark_chameleon_obj.run();
        this->changeMenu(&deviceMenu, true);
      });
    #endif
    // Stops a Web Control AP that was left running in the background.
    this->addNodes(&deviceMenu, "Stop Web Control", TFTORANGE, WIFI, [this]() {
      shark_web_obj.resume_pending = false;   // explicit stop cancels auto-resume
      shark_web_obj.stop();
      this->drawStatusBar();
      this->changeMenu(&deviceMenu, true);
    });
  #endif

  this->addNodes(&deviceMenu, text_table1[17], TFTWHITE, DEVICE_INFO, [this]() {
    wifi_scan_obj.currentScanMode = SHOW_INFO;
    this->changeMenu(&infoMenu, true);
    wifi_scan_obj.RunInfo();
  });
  this->addNodes(&deviceMenu, text08, TFTBLUE, SETTINGS, [this]() {
    this->changeMenu(&settingsMenu, true);
  });

  #ifdef HAS_SD
    if (sd_obj.supported) {

      sdDeleteMenu.parentMenu = &deviceMenu;

      this->addNodes(&deviceMenu, "Delete SD Files", TFTCYAN, SD_UPDATE, [this]() {
        display_obj.clearScreen();
        display_obj.tft.setTextWrap(false);
        display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
        display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
        display_obj.tft.println("Loading...");

        // Clear menu and lists
        this->buildSDFileMenu();

        this->changeMenu(&sdDeleteMenu, true);
      });
    }
  #endif

  // Save Files Menu
  saveFileMenu.parentMenu = &deviceMenu;
  this->addNodes(&saveFileMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(saveFileMenu.parentMenu, true);
  });
  this->addNodes(&saveFileMenu, "Save SSIDs", TFTCYAN, SD_UPDATE, [this]() {
    this->changeMenu(&saveSSIDsMenu, true);
    wifi_scan_obj.RunSaveSSIDList(true);
  });
  this->addNodes(&saveFileMenu, "Load SSIDs", TFTSKYBLUE, SD_UPDATE, [this]() {
    this->changeMenu(&loadSSIDsMenu, true);
    wifi_scan_obj.RunLoadSSIDList();
  });
  this->addNodes(&saveFileMenu, "Save APs", TFTNAVY, SD_UPDATE, [this]() {
    this->changeMenu(&saveAPsMenu, true);
    wifi_scan_obj.RunSaveAPList();
  });
  this->addNodes(&saveFileMenu, "Load APs", TFTBLUE, SD_UPDATE, [this]() {
    this->changeMenu(&loadAPsMenu, true);
    wifi_scan_obj.RunLoadAPList();
  });
  this->addNodes(&saveFileMenu, "Save Airtags", TFTWHITE, SD_UPDATE, [this]() {
    this->changeMenu(&saveAPsMenu, true);
    wifi_scan_obj.RunSaveATList();
  });
  this->addNodes(&saveFileMenu, "Load Airtags", TFTWHITE, SD_UPDATE, [this]() {
    this->changeMenu(&loadAPsMenu, true);
    wifi_scan_obj.RunLoadATList();
  });

  saveSSIDsMenu.parentMenu = &saveFileMenu;
  this->addNodes(&saveSSIDsMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(saveSSIDsMenu.parentMenu, true);
  });

  loadSSIDsMenu.parentMenu = &saveFileMenu;
  this->addNodes(&loadSSIDsMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(loadSSIDsMenu.parentMenu, true);
  });

  saveAPsMenu.parentMenu = &saveFileMenu;
  this->addNodes(&saveAPsMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(saveAPsMenu.parentMenu, true);
  });

  loadAPsMenu.parentMenu = &saveFileMenu;
  this->addNodes(&loadAPsMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(loadAPsMenu.parentMenu, true);
  });

  saveATsMenu.parentMenu = &saveFileMenu;
  this->addNodes(&saveATsMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(saveATsMenu.parentMenu, true);
  });

  loadATsMenu.parentMenu = &saveFileMenu;
  this->addNodes(&loadATsMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(loadATsMenu.parentMenu, true);
  });

  // GPS Menu
  #ifdef HAS_GPS
    if (gps_obj.getGpsModuleStatus()) {
      gpsMenu.parentMenu = &mainMenu; // Main Menu is second menu parent

      this->addNodes(&gpsMenu, text09, TFTLIGHTGREY, 0, [this]() {
        this->changeMenu(gpsMenu.parentMenu, true);
      });

      this->addNodes(&gpsMenu, "GPS Data", TFTRED, GPS_MENU, [this]() {
        wifi_scan_obj.currentScanMode = WIFI_SCAN_GPS_DATA;
        this->changeMenu(&gpsInfoMenu, true);
        wifi_scan_obj.StartScan(WIFI_SCAN_GPS_DATA, TFT_CYAN);
        this->drawGpsDataUI(true);
      });

      #ifdef MARAUDER_V8
        this->addNodes(&gpsMenu, "IceNav", TFTCYAN, ICENAV, [this]() {
          shark_icenav_obj.run();
          this->changeMenu(&gpsMenu, true);
        });

        this->addNodes(&gpsMenu, "Nav Compass", TFTCYAN, NAV_COMPASS_ICON, [this]() {
          shark_icenav_obj.runCompass();
          this->changeMenu(&gpsMenu, true);
        });

        this->addNodes(&gpsMenu, "Satellite Info", TFTWHITE, SAT_INFO_ICON, [this]() {
          shark_icenav_obj.runSatelliteInfo();
          this->changeMenu(&gpsMenu, true);
        });

        this->addNodes(&gpsMenu, "Add WPT", TFTORANGE, ADD_WPT_ICON, [this]() {
          shark_icenav_obj.runAddWaypoint();
          this->changeMenu(&gpsMenu, true);
        });

        this->addNodes(&gpsMenu, "Sensor Radar", TFTCYAN, SENSOR_RADAR_ICON, [this]() {
          shark_icenav_obj.runSensorRadar();
          this->changeMenu(&gpsMenu, true);
        });

        this->addNodes(&gpsMenu, "GPS Radar", TFTGREEN, GPS_MENU, [this]() {
          shark_radar_obj.runGps();
          this->changeMenu(&gpsMenu, true);
        });
      #endif

      this->addNodes(&gpsMenu, "NMEA Stream", TFTORANGE, GPS_MENU, [this]() {
        wifi_scan_obj.currentScanMode = WIFI_SCAN_GPS_NMEA;
        this->changeMenu(&gpsInfoMenu, true);
        wifi_scan_obj.StartScan(WIFI_SCAN_GPS_NMEA, TFT_ORANGE);
      });

      this->addNodes(&gpsMenu, "GPS Tracker", TFTGREEN, GPS_MENU, [this]() {
        wifi_scan_obj.currentScanMode = GPS_TRACKER;
        this->changeMenu(&gpsInfoMenu, true);
        wifi_scan_obj.StartScan(GPS_TRACKER, TFT_CYAN);
        this->drawGpsTrackerUI(true);
      });

      this->addNodes(&gpsMenu, "GPS POI", TFTCYAN, GPS_MENU, [this]() {
        wifi_scan_obj.StartScan(GPS_POI, TFT_CYAN);
        wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
        this->changeMenu(&gpsPOIMenu, true);
      });

      // Pure-GPS tools. Each is a self-contained blocking screen that pumps the
      // GPS parser itself and exits on touch, then returns to this menu.
      #ifdef MARAUDER_V8
        this->addNodes(&gpsMenu, "Speedometer", TFTGREEN, GPS_MENU, [this]() {
          shark_radar_obj.runSpeed();
          this->changeMenu(&gpsMenu, true);
        });

        this->addNodes(&gpsMenu, "Compass", TFTCYAN, GPS_MENU, [this]() {
          shark_radar_obj.runCompass();
          this->changeMenu(&gpsMenu, true);
        });

        this->addNodes(&gpsMenu, "GPS Clock", TFTORANGE, GPS_MENU, [this]() {
          shark_radar_obj.runClock();
          this->changeMenu(&gpsMenu, true);
        });

        this->addNodes(&gpsMenu, "Waypoint", TFTWHITE, GPS_MENU, [this]() {
          shark_radar_obj.runWaypoint();
          this->changeMenu(&gpsMenu, true);
        });
      #endif

      // GPS POI Menu
      gpsPOIMenu.parentMenu = &gpsMenu;
      this->addNodes(&gpsPOIMenu, text09, TFTLIGHTGREY, 0, [this]() {
        wifi_scan_obj.currentScanMode = GPS_POI;
        wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
        this->changeMenu(gpsPOIMenu.parentMenu, true);
      });
      this->addNodes(&gpsPOIMenu, "Mark POI", TFTCYAN, GPS_MENU, [this]() {
        wifi_scan_obj.currentScanMode = GPS_POI;
        display_obj.tft.setCursor(0, TFT_HEIGHT / 2);
        display_obj.clearScreen();
        if (wifi_scan_obj.RunGPSInfo(true, false, true))
          display_obj.showCenterText("POI Logged", TFT_HEIGHT / 2);
        else
          display_obj.showCenterText("POI Log Failed", TFT_HEIGHT / 2);
        wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
        delay(2000);
        this->changeMenu(&gpsPOIMenu, true);
      });

      // GPS Info Menu
      gpsInfoMenu.parentMenu = &gpsMenu;
      this->addNodes(&gpsInfoMenu, text09, TFTLIGHTGREY, 0, [this]() {
        if(wifi_scan_obj.currentScanMode != GPS_TRACKER)
          wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
        wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
        this->changeMenu(gpsInfoMenu.parentMenu, true);
      }); 
    }
  #endif

  // Settings menu
  // Device menu
  #ifdef MARAUDER_V8
    // Theme menu. Selecting an entry repaints the whole interface at once and
    // stores the choice in NVS, so the boot art comes up in the same theme.
    themeMenu.parentMenu = &deviceMenu;
    this->addNodes(&themeMenu, text09, TFTLIGHTGREY, 0, [this]() {
      this->changeMenu(themeMenu.parentMenu, true);
    });

    for (uint8_t theme = 0; theme < SHARK_THEME_COUNT; theme++) {
      this->addNodes(&themeMenu,
                     shark_themes[theme].name,
                     TFTCYAN,
                     DRAW,
                     [this, theme]() {
                       sharkThemeSet(theme);
                       this->markActiveTheme();
                       this->changeMenu(&themeMenu, true);
                     },
                     shark_theme_index == theme);
    }
  #endif

  settingsMenu.parentMenu = &deviceMenu;
  this->addNodes(&settingsMenu, text09, TFTLIGHTGREY, 0, [this]() {
    changeMenu(settingsMenu.parentMenu, true);
  });
  for (int i = 0; i < settings_obj.getNumberSettings(); i++) {
    String settingName = settings_obj.setting_index_to_name(i);
    const char* type = this->callSetting(settingName.c_str());
    if (type && strcmp(type, "bool") == 0) {
      this->addNodes(&settingsMenu, settingName.c_str(), TFTLIGHTGREY, SETTINGS, [this, i, settingName]() {
          settings_obj.toggleSetting(settingName.c_str());
          this->callSetting(settingName.c_str());
          this->changeMenu(&specSettingMenu, true);
          this->displaySetting(settingName.c_str(), &settingsMenu, i + 1);
          wifi_scan_obj.force_pmkid = settings_obj.loadSetting<bool>(text_table4[5]);
          wifi_scan_obj.force_probe = settings_obj.loadSetting<bool>(text_table4[6]);
          wifi_scan_obj.save_pcap = settings_obj.loadSetting<bool>(text_table4[7]);
          wifi_scan_obj.ep_deauth = settings_obj.loadSetting<bool>("EPDeauth");
          wifi_scan_obj.channel_hop = settings_obj.loadSetting<bool>("ChanHop");
      }, settings_obj.loadSetting<bool>(settingName.c_str()));
    }
  }

  #ifdef MARAUDER_V8
    // Idle wallpaper toggle (NVS-backed, default OFF). A dedicated node so it
    // works regardless of the persisted settings file, and reflects its state.
    this->idle_wall_on = sharkLoadIdleWall();
    this->addNodes(&settingsMenu, "Idle Wallpaper", TFTCYAN, SETTINGS, [this]() {
      this->idle_wall_on = !this->idle_wall_on;
      sharkSaveIdleWall(this->idle_wall_on);
      display_obj.clearScreen();
      this->drawStatusBar();
      display_obj.tft.setTextColor(this->idle_wall_on ? WD_CYAN : WD_DIM, TFT_BLACK);
      display_obj.showCenterText(this->idle_wall_on ? "IDLE WALLPAPER: ON"
                                                    : "IDLE WALLPAPER: OFF",
                                 TFT_HEIGHT / 2);
      delay(1100);
      this->changeMenu(&settingsMenu, true);
    }, this->idle_wall_on);
  #endif

  Serial.println("Finished settings nodes");

  // Specific setting menu
  specSettingMenu.parentMenu = &settingsMenu;
  addNodes(&specSettingMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(specSettingMenu.parentMenu, true);
  });

  // Web Update
  updateMenu.parentMenu = &deviceMenu;

  // Failed update menu
  failedUpdateMenu.parentMenu = &deviceMenu;
  this->addNodes(&failedUpdateMenu, text09, TFTLIGHTGREY, 0, [this]() {
    wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
    this->changeMenu(failedUpdateMenu.parentMenu, true);
  });

  // Device info menu
  infoMenu.parentMenu = &deviceMenu;
  this->addNodes(&infoMenu, text09, TFTLIGHTGREY, 0, [this]() {
    wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
    this->changeMenu(infoMenu.parentMenu, true);
  });

  Serial.println("Changing to main menu...");

  // Set the current menu to the mainMenu
  this->changeMenu(&mainMenu, true);

  this->initTime = millis();
}

//#if (!defined(HAS_ILI9341) && defined(HAS_BUTTONS))
#ifdef HAS_MINI_KB
  String MenuFunctions::miniKeyboard(Menu * targetMenu, bool do_pass) {
    // Prepare a char array and reset temp SSID string
    extern LinkedList<ssid>* ssids;

    String ret_val = "";

    bool pressed = true;

    wifi_scan_obj.current_mini_kb_ssid = "";

    #ifdef HAS_MINI_KB
      if (c_btn.isHeld()) {
        while (!c_btn.justReleased())
          delay(1);
      }
    #endif

    int str_len = wifi_scan_obj.alfa.length() + 1; 

    char char_array[str_len];

    wifi_scan_obj.alfa.toCharArray(char_array, str_len);

    #ifdef HAS_TOUCH
      uint16_t t_x = 0, t_y = 0;

    #endif

    // Button loop until hold center button
    #ifdef HAS_BUTTONS
      //#if !(defined(MARAUDER_V6) || defined(MARAUDER_V6_1) || defined(MARAUDER_CYD_MICRO))
        while(true) {
          // Keyboard functions for switch hardware
          #ifdef HAS_MINI_KB
            // Cycle char previous
            #ifdef HAS_L
              if ((l_btn.justPressed()) || (l_btn.isHeld())) {
                pressed = true;
                if (this->mini_kb_index > 0)
                  this->mini_kb_index--;
                else
                  this->mini_kb_index = str_len - 2;

                targetMenu->list->set(0, MenuNode{String(char_array[this->mini_kb_index]).c_str(), false, TFTCYAN, 0, true, NULL});
                this->buildButtons(targetMenu);

                while (!l_btn.justReleased()) {
                  l_btn.justPressed();
                  if (!l_btn.isHeld())
                    delay(1);
                  else
                    break;
                }
              }
            #endif

            // Cycle char next
            #ifdef HAS_R
              if ((r_btn.justPressed()) || (r_btn.isHeld())) {
                pressed = true;
                if (this->mini_kb_index < str_len - 2)
                  this->mini_kb_index++;
                else
                  this->mini_kb_index = 0;

                targetMenu->list->set(0, MenuNode{String(char_array[this->mini_kb_index]).c_str(), false, TFTCYAN, 0, true, NULL});
                this->buildButtons(targetMenu, 0, &char_array[this->mini_kb_index]);
                
                while (!r_btn.justReleased()) {
                  r_btn.justPressed();
                  if (!r_btn.isHeld())
                    delay(1);
                  else
                    break;
                }
              }
            #endif

            //// 5-WAY SWITCH STUFF
            // Add character
            #if (defined(HAS_D) && defined(HAS_R))
              if (d_btn.justPressed()) {
                pressed = true;
                wifi_scan_obj.current_mini_kb_ssid.concat(String(char_array[this->mini_kb_index]).c_str());
                while (!d_btn.justReleased())
                  delay(1);
              }
            #endif

            // Remove character
            #if (defined(HAS_U) && defined(HAS_L))
              if (u_btn.justPressed()) {
                pressed = true;
                wifi_scan_obj.current_mini_kb_ssid.remove(wifi_scan_obj.current_mini_kb_ssid.length() - 1);
                while (!u_btn.justReleased())
                  delay(1);
              }
            #endif

            //// PARTIAL SWITCH STUFF
            // Advance char or add char
            #if (defined(HAS_D) && !defined(HAS_R))
              if (d_btn.justPressed()) {
                bool was_held = false;
                pressed = true;
                while(!d_btn.justReleased()) {
                  d_btn.justPressed();

                  // Add letter to string
                  if (d_btn.isHeld()) {
                    wifi_scan_obj.current_mini_kb_ssid.concat(String(char_array[this->mini_kb_index]).c_str());
                    was_held = true;
                    break;
                  }
                }
                if (!was_held) {
                  if (this->mini_kb_index < str_len - 2)
                    this->mini_kb_index++;
                  else
                    this->mini_kb_index = 0;

                  targetMenu->list->set(0, MenuNode{String(char_array[this->mini_kb_index]).c_str(), false, TFTCYAN, 0, true, NULL});
                  this->buildButtons(targetMenu, 0, &char_array[this->mini_kb_index]);
                }
              }
            #endif

            // Prev char or remove char
            #if (defined(HAS_U) && !defined(HAS_L))
              if (u_btn.justPressed()) {
                bool was_held = false;
                pressed = true;
                while(!u_btn.justReleased()) {
                  u_btn.justPressed();

                  // Remove letter from string
                  if (u_btn.isHeld()) {
                    wifi_scan_obj.current_mini_kb_ssid.remove(wifi_scan_obj.current_mini_kb_ssid.length() - 1);
                    was_held = true;
                    break;
                  }
                }
                if (!was_held) {
                  if (this->mini_kb_index > 0)
                    this->mini_kb_index--;
                  else
                    this->mini_kb_index = str_len - 2;

                  targetMenu->list->set(0, MenuNode{String(char_array[this->mini_kb_index]).c_str(), false, TFTCYAN, 0, true, NULL});
                  this->buildButtons(targetMenu);
                }
              }
            #endif

            // Add SSID
            #if defined(HAS_C) && !defined(MARAUDER_CARDPUTER) && !defined(MARAUDER_CARDPUTER_ADV)
              if (c_btn.justPressed()) {
                while (!c_btn.justReleased()) {
                  c_btn.justPressed(); // Need to continue updating button hold status. My shitty library.

                  // Exit
                  if (c_btn.isHeld()) {
                    this->changeMenu(targetMenu->parentMenu);
                    return wifi_scan_obj.current_mini_kb_ssid;
                  }
                  delay(1);
                }

                if (!do_pass) {
                // If we have a string, add it to list of SSIDs
                  if (wifi_scan_obj.current_mini_kb_ssid != "") {
                    pressed = true;
                    ssid s = {wifi_scan_obj.current_mini_kb_ssid, random(1, 12), {random(256), random(256), random(256), random(256), random(256), random(256)}, false};
                    ssids->unshift(s);
                    wifi_scan_obj.current_mini_kb_ssid = "";
                  }
                }
              }
            #endif
          #endif

          #if defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)
            for (int i = 0; i < 95; i++) {
              if ((M5CardputerKeyboard._ascii_list[i] != '(') &&
                  (M5CardputerKeyboard._ascii_list[i] != '`')) {
                if (this->isKeyPressed(M5CardputerKeyboard._ascii_list[i])) {
                  pressed = true;
                  wifi_scan_obj.current_mini_kb_ssid.concat(M5CardputerKeyboard._ascii_list[i]);
                }
                if (this->isKeyPressed(KEY_BACKSPACE)) {
                  pressed = true;
                  wifi_scan_obj.current_mini_kb_ssid.remove(wifi_scan_obj.current_mini_kb_ssid.length() - 1);
                }
              }
            }

            if (!do_pass) {
              if (this->isKeyPressed('`')) {
                this->changeMenu(targetMenu->parentMenu, true);
                return wifi_scan_obj.current_mini_kb_ssid;
              }

              if (this->isKeyPressed('(')) {
                if (!do_pass) {
                  if (wifi_scan_obj.current_mini_kb_ssid != "") {
                    pressed = true;
                    ssid s = {wifi_scan_obj.current_mini_kb_ssid, random(1, 12), {random(256), random(256), random(256), random(256), random(256), random(256)}, false};
                    ssids->unshift(s);
                    wifi_scan_obj.current_mini_kb_ssid = "";
                  }
                }
              }
            }
            else {
              if (this->isKeyPressed('(')) {
                this->changeMenu(targetMenu->parentMenu, true);
                return wifi_scan_obj.current_mini_kb_ssid;
              }

              if (this->isKeyPressed('`')) {
                this->changeMenu(targetMenu->parentMenu, true);
                return "";
              }
            }
            
          #endif

          // Keyboard functions for touch hardware
          #ifdef HAS_TOUCH
            bool touched = display_obj.updateTouch(&t_x, &t_y);

            uint8_t menu_button = display_obj.menuButton(&t_x, &t_y, touched);

            // Cycle char previous
            if (menu_button == UP_BUTTON) {
              pressed = true;
              if (this->mini_kb_index > 0)
                this->mini_kb_index--;
              else
                this->mini_kb_index = str_len - 2;

              targetMenu->list->set(0, MenuNode{String(char_array[this->mini_kb_index]).c_str(), false, TFTCYAN, 0, true, NULL});
              this->buildButtons(targetMenu);
              while (display_obj.updateTouch(&t_x, &t_y) > 0)
                delay(1);
              display_obj.menuButton(&t_x, &t_y, display_obj.updateTouch(&t_x, &t_y));
            }

            // Cycle char next
            if (menu_button == DOWN_BUTTON) {
              pressed = true;
              if (this->mini_kb_index < str_len - 2)
                this->mini_kb_index++;
              else
                this->mini_kb_index = 0;

              targetMenu->list->set(0, MenuNode{String(char_array[this->mini_kb_index]).c_str(), false, TFTCYAN, 0, true, NULL});
              this->buildButtons(targetMenu, 0, &char_array[this->mini_kb_index]);
              while (display_obj.updateTouch(&t_x, &t_y) > 0)
                delay(1);
              display_obj.menuButton(&t_x, &t_y, display_obj.updateTouch(&t_x, &t_y));
            }

            //// 5-WAY SWITCH STUFF
            // Add character when select button is pressed
            if (menu_button == SELECT_BUTTON) {
              pressed = true;
              wifi_scan_obj.current_mini_kb_ssid.concat(String(char_array[this->mini_kb_index]).c_str());
              while (display_obj.updateTouch(&t_x, &t_y) > 0)
                delay(1);
              display_obj.menuButton(&t_x, &t_y, display_obj.updateTouch(&t_x, &t_y));
            }

            // Remove character when select button is held
            if ((display_obj.isTouchHeld()) && (display_obj.menuButton(&t_x, &t_y, touched, true) == SELECT_BUTTON)) {
              pressed = true;
              wifi_scan_obj.current_mini_kb_ssid.remove(wifi_scan_obj.current_mini_kb_ssid.length() - 1);
              while (display_obj.menuButton(&t_x, &t_y, display_obj.updateTouch(&t_x, &t_y)) < 0)
                delay(1);
            }

            //// PARTIAL SWITCH STUFF
            // Advance char or add char
            #if (defined(HAS_D) && !defined(HAS_R))
              if (d_btn.justPressed()) {
                bool was_held = false;
                pressed = true;
                while(!d_btn.justReleased()) {
                  d_btn.justPressed();

                  // Add letter to string
                  if (d_btn.isHeld()) {
                    wifi_scan_obj.current_mini_kb_ssid.concat(String(char_array[this->mini_kb_index]).c_str());
                    was_held = true;
                    break;
                  }
                }
                if (!was_held) {
                  if (this->mini_kb_index < str_len - 2)
                    this->mini_kb_index++;
                  else
                    this->mini_kb_index = 0;

                  targetMenu->list->set(0, MenuNode{String(char_array[this->mini_kb_index]).c_str(), false, TFTCYAN, 0, true, NULL});
                  this->buildButtons(targetMenu, 0, &char_array[this->mini_kb_index]);
                }
              }
            #endif

            // Prev char or remove char
            #if (defined(HAS_U) && !defined(HAS_L))
              if (u_btn.justPressed()) {
                bool was_held = false;
                pressed = true;
                while(!u_btn.justReleased()) {
                  u_btn.justPressed();

                  // Remove letter from string
                  if (u_btn.isHeld()) {
                    wifi_scan_obj.current_mini_kb_ssid.remove(wifi_scan_obj.current_mini_kb_ssid.length() - 1);
                    was_held = true;
                    break;
                  }
                }
                if (!was_held) {
                  if (this->mini_kb_index > 0)
                    this->mini_kb_index--;
                  else
                    this->mini_kb_index = str_len - 2;

                  targetMenu->list->set(0, MenuNode{String(char_array[this->mini_kb_index]).c_str(), false, TFTCYAN, 0, true, NULL});
                  this->buildButtons(targetMenu);
                }
              }
            #endif

            // Exit if UP button is held
            if ((display_obj.isTouchHeld()) && (display_obj.menuButton(&t_x, &t_y, touched, true) == UP_BUTTON)) {
              display_obj.clearScreen();
              while (display_obj.menuButton(&t_x, &t_y, display_obj.updateTouch(&t_x, &t_y)) < 0)
                delay(1);

              // Reset the touch keys so we don't activate the keys when we go back
              display_obj.menuButton(&t_x, &t_y, display_obj.updateTouch(&t_x, &t_y));
              this->changeMenu(targetMenu->parentMenu, true);
              return wifi_scan_obj.current_mini_kb_ssid;
            }

            // If the screen is touched but none of the keys are used, don't refresh display
            if (menu_button < 0)
              pressed = false;

          #endif

          // Display info on screen
          if (pressed) {
            this->displayCurrentMenu();
            display_obj.tft.setTextWrap(false);
            display_obj.tft.fillRect(0, SCREEN_HEIGHT / 3, SCREEN_WIDTH, STATUS_BAR_WIDTH, TFT_BLACK);
            display_obj.tft.fillRect(0, SCREEN_HEIGHT / 3 + TEXT_HEIGHT * 2, SCREEN_WIDTH, STATUS_BAR_WIDTH, TFT_BLACK);
            display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
            display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
            display_obj.tft.println(wifi_scan_obj.current_mini_kb_ssid + "\n");
            display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);

            display_obj.tft.println(ssids->get(0).essid);

            display_obj.tft.setTextColor(TFT_ORANGE, TFT_BLACK);
            #ifdef HAS_MINI_KB
              #if !defined(MARAUDER_CARDPUTER) && !defined(MARAUDER_CARDPUTER_ADV)
              display_obj.tft.println("U/D - Rem/Add Char");
              display_obj.tft.println("L/R - Prev/Nxt Char");
              #endif
              if (!do_pass) {
                #if defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)
                  display_obj.tft.println("Enter - Save");
                  display_obj.tft.println("Esc - Exit");
                #else
                  display_obj.tft.println("C - Save");
                  display_obj.tft.println("C(Hold) - Exit");
                #endif
              }
              else {
                #if defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)
                  display_obj.tft.println("Enter - Enter");
                #else
                  display_obj.tft.println("C(Hold) - Enter");
                #endif
              }
            #endif

            #ifdef HAS_TOUCH
              display_obj.tft.println("U/D - Prev/Nxt Char");
              display_obj.tft.println("C - Add Char");
              display_obj.tft.println("C(Hold) - Rem Char");
              display_obj.tft.println("U(Hold) - Enter");
            #endif
            pressed = false;
          }
        }
      //#endif
    #endif
  }
#endif

void MenuFunctions::setupSDFileList(bool update) {
  sd_obj.sd_files->clear();

  delete sd_obj.sd_files;

  sd_obj.sd_files = new LinkedList<String>();

  if (!update)
    sd_obj.listDirToLinkedList(sd_obj.sd_files);
  else
    sd_obj.listDirToLinkedList(sd_obj.sd_files, "/", ".bin");
}

void MenuFunctions::buildSDFileMenu(bool update) {
  this->setupSDFileList(update);

  sdDeleteMenu.list->clear();
  delete sdDeleteMenu.list;
  sdDeleteMenu.list = new LinkedList<MenuNode>();

  if (!update)
    sdDeleteMenu.name = "SD Files";
  else
    sdDeleteMenu.name = "Bin Files";

  this->addNodes(&sdDeleteMenu, text09, TFTLIGHTGREY, 0, [this]() {
    this->changeMenu(sdDeleteMenu.parentMenu, true);
  });

  if (!update) {
    this->addNodes(&sdDeleteMenu, "Delete Selected", TFTORANGE, 0, [this]() {
      for (int x = 0; x < sd_obj.sd_files->size(); x++) {
        if (current_menu->list->get(x + 2).selected) {
          if (sd_obj.removeFile("/" + sd_obj.sd_files->get(x))) {
            Serial.println("Deleted /" + sd_obj.sd_files->get(x));
            display_obj.clearScreen();
            display_obj.tft.setTextWrap(false);
            display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
            display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
            display_obj.tft.println("Deleting /" + sd_obj.sd_files->get(x) + "...");
          }
        }
      }
      this->buildSDFileMenu();
      this->changeMenu(&sdDeleteMenu, true);
    });
  }

  if (!update) {
    for (int x = 0; x < sd_obj.sd_files->size(); x++) {
      this->addNodes(&sdDeleteMenu, sd_obj.sd_files->get(x).c_str(), TFTCYAN, SD_UPDATE, [this, x]() {
        // Change selection status of menu node
        MenuNode new_node = current_menu->list->get(x + 2);
        new_node.selected = !current_menu->list->get(x + 2).selected;
        current_menu->list->set(x + 2, new_node);
      });
    }
  }
  else {
    for (int x = 0; x < sd_obj.sd_files->size(); x++) {
      this->addNodes(&sdDeleteMenu, sd_obj.sd_files->get(x).c_str(), TFTCYAN, SD_UPDATE, [this, x]() {
        wifi_scan_obj.currentScanMode = OTA_UPDATE;
        this->changeMenu(&failedUpdateMenu, true);
        sd_obj.runUpdate("/" + sd_obj.sd_files->get(x));
      });
    }
  }
}


// Function to add MenuNodes to a menu
void MenuFunctions::addNodes(Menu * menu, const char* name, uint8_t color, int place, std::function<void()> callable, bool selected)
{
  //Serial.println("Building node: " + name);
  menu->list->add(MenuNode{String(name), false, color, place, selected, callable});
}

void MenuFunctions::setGraphScale(float scale) {
  this->_graph_scale = scale;
}

float MenuFunctions::calculateGraphScale(uint8_t value) {
  if ((value * this->_graph_scale < GRAPH_VERT_LIM) && (value * this->_graph_scale > GRAPH_VERT_LIM * 0.75)) {
    return this->_graph_scale;  // No scaling needed if the value is within the limit
  }

  if (value < GRAPH_VERT_LIM)
    return 1.0;

  // Calculate the multiplier proportionally
  return (0.75 * GRAPH_VERT_LIM) / value;
}

float MenuFunctions::calculateGraphScale(int16_t value) {
  if ((value * this->_graph_scale < GRAPH_VERT_LIM) && (value * this->_graph_scale > GRAPH_VERT_LIM * 0.75)) {
    return this->_graph_scale;  // No scaling needed if the value is within the limit
  }

  if (value < GRAPH_VERT_LIM)
    return 1.0;

  // Calculate the multiplier proportionally
  return (0.75 * GRAPH_VERT_LIM) / value;
}

float MenuFunctions::graphScaleCheck(const int16_t array[SCREEN_WIDTH]) {
  int16_t maxValue = 0;

  // Iterate through the array to find the highest value
  for (int16_t i = 0; i < SCREEN_WIDTH; i++) {
    if (array[i] > maxValue) {
      maxValue = array[i];
    }
  }

  // If the highest value exceeds GRAPH_VERT_LIM, call calculateMultiplier
  if (maxValue > GRAPH_VERT_LIM) {
    return this->calculateGraphScale(maxValue);
  }

  // If the highest value does not exceed GRAPH_VERT_LIM, return 1.0
  return 1.0;
}

float MenuFunctions::graphScaleCheckSmall(const uint8_t array[CHAN_PER_PAGE]) {
  uint8_t maxValue = 0;

  // Iterate through the array to find the highest value
  for (uint8_t i = 0; i < CHAN_PER_PAGE; i++) {
    if (array[i] > maxValue) {
      maxValue = array[i];
    }
  }

  // If the highest value exceeds GRAPH_VERT_LIM, call calculateMultiplier
  if (maxValue > GRAPH_VERT_LIM) {
    return this->calculateGraphScale(maxValue);
  }

  // If the highest value does not exceed GRAPH_VERT_LIM, return 1.0
  return 1.0;
}

void MenuFunctions::drawMaxLine(int16_t value, uint16_t color) {
  display_obj.tft.drawLine(0, TFT_HEIGHT - (value * this->_graph_scale), TFT_WIDTH, TFT_HEIGHT - (value * this->_graph_scale), color);
  display_obj.tft.setCursor(0, TFT_HEIGHT - (value * this->_graph_scale));
  display_obj.tft.setTextColor(color, TFT_BLACK);
  display_obj.tft.setTextSize(1);
  display_obj.tft.println((String)(value / BASE_MULTIPLIER));
}

void MenuFunctions::drawMaxLine(uint8_t value, uint16_t color) {
  //display_obj.tft.drawLine(0, TFT_HEIGHT - (value * this->_graph_scale), TFT_WIDTH, TFT_HEIGHT - (value * this->_graph_scale), color);
  display_obj.tft.setCursor(0, TFT_HEIGHT - (value * this->_graph_scale));
  display_obj.tft.setTextColor(color, TFT_BLACK);
  display_obj.tft.setTextSize(1);
  display_obj.tft.println((String)value);
}

void MenuFunctions::drawGraphSmall(uint8_t *values) {
  uint8_t maxValue = 0;
  //(i + (CHAN_PER_PAGE * (this->activity_page - 1)))

  int bar_width = SCREEN_WIDTH / (CHAN_PER_PAGE * 2);
  //display_obj.tft.fillRect(0, TFT_HEIGHT / 2 + 1, SCREEN_WIDTH, (TFT_HEIGHT / 2) + 1, TFT_BLACK);

  #ifndef HAS_DUAL_BAND
    for (int i = 1; i < CHAN_PER_PAGE + 1; i++) {
      int targ_val = i + (CHAN_PER_PAGE * (wifi_scan_obj.activity_page - 1)) - 1;
      int x_mult = (i * 2) - 1;
      int x_coord = (SCREEN_WIDTH / (CHAN_PER_PAGE * 2)) * (x_mult - 1);

      if (values[targ_val] > maxValue) {
        maxValue = values[targ_val];
      }

      if (values[targ_val] * this->_graph_scale <= GRAPH_VERT_LIM) {
        display_obj.tft.fillRect(x_coord, SCREEN_HEIGHT / 2 + 1, bar_width, SCREEN_HEIGHT / 2 + 1, TFT_BLACK);
        display_obj.tft.fillRect(x_coord, SCREEN_HEIGHT - (values[targ_val] * this->_graph_scale), bar_width, values[targ_val] * this->_graph_scale, TFT_CYAN);
      }

      display_obj.tft.drawLine(x_coord - 2, SCREEN_HEIGHT - GRAPH_VERT_LIM - (CHAR_WIDTH * 2), x_coord - 2, SCREEN_HEIGHT, TFT_WHITE);
    }
  #else
    for (int i = 1; i < CHAN_PER_PAGE + 1; i++) {
      int targ_val = i + (CHAN_PER_PAGE * (wifi_scan_obj.activity_page - 1)) - 1;
      int x_mult = (i * 2) - 1;
      int x_coord = (SCREEN_WIDTH / (CHAN_PER_PAGE * 2)) * (x_mult - 1);

      if (values[targ_val] > maxValue) {
        maxValue = values[targ_val];
      }

      if (values[targ_val] * this->_graph_scale <= GRAPH_VERT_LIM) {
        display_obj.tft.fillRect(x_coord, SCREEN_HEIGHT / 2 + 1, bar_width + 3, SCREEN_HEIGHT / 2 + 1, TFT_BLACK);
        display_obj.tft.fillRect(x_coord, SCREEN_HEIGHT - (values[targ_val] * this->_graph_scale), bar_width, values[targ_val] * this->_graph_scale, TFT_CYAN);
      }

      display_obj.tft.drawLine(x_coord - 2, SCREEN_HEIGHT - GRAPH_VERT_LIM - (CHAR_WIDTH * 2), x_coord - 2, SCREEN_HEIGHT, TFT_WHITE);
    }
  #endif

  this->drawMaxLine(maxValue, TFT_GREEN); // Draw max
}

void MenuFunctions::drawGraph(int16_t *values) {
  #if !defined(MARAUDER_CARDPUTER) && !defined(MARAUDER_CARDPUTER_ADV)
    int width = TFT_WIDTH;
  #else
    int width = SCREEN_WIDTH;
  #endif

  int16_t maxValue = 0;
  int total = 0;
  for (int i = width - 1; i >= 0; i--) {
    if (values[i] >= 0) {
      total = total + values[i];
      if (values[i] > maxValue) {
        maxValue = values[i];
      }
      #if !defined(MARAUDER_CARDPUTER) && !defined(MARAUDER_CARDPUTER_ADV)
        display_obj.tft.drawLine(i, TFT_HEIGHT, i, TFT_HEIGHT - GRAPH_VERT_LIM, TFT_BLACK);
        display_obj.tft.drawLine(i, TFT_HEIGHT, i, TFT_HEIGHT - (values[i] * this->_graph_scale), TFT_CYAN);
      #else
        display_obj.tft.drawLine(i, TFT_WIDTH, i, TFT_WIDTH - GRAPH_VERT_LIM, TFT_BLACK);
        display_obj.tft.drawLine(i, TFT_WIDTH, i, TFT_WIDTH - (values[i] * this->_graph_scale), TFT_CYAN);
        display_obj.tft.setCursor(0, 0);
        display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
      #endif
    }
    else {
      int16_t ch_val = values[i] * -1;
      #if !defined(MARAUDER_CARDPUTER) && !defined(MARAUDER_CARDPUTER_ADV)
        display_obj.tft.drawLine(i, TFT_HEIGHT, i, TFT_HEIGHT - GRAPH_VERT_LIM, TFT_BLACK);
        display_obj.tft.drawLine(i, TFT_HEIGHT, i, TFT_HEIGHT - GRAPH_VERT_LIM, TFT_RED);
        display_obj.tft.setCursor(i, TFT_HEIGHT - GRAPH_VERT_LIM);
      #else
        display_obj.tft.drawLine(i, TFT_WIDTH, i, TFT_WIDTH - GRAPH_VERT_LIM, TFT_BLACK);
        display_obj.tft.drawLine(i, TFT_WIDTH, i, TFT_WIDTH - GRAPH_VERT_LIM, TFT_RED);
        display_obj.tft.setCursor(i, TFT_WIDTH - GRAPH_VERT_LIM);
      #endif
      display_obj.tft.setTextColor(TFT_BLACK, TFT_RED);
      display_obj.tft.setTextSize(1);
      display_obj.tft.println((String)ch_val);
    }
  }

  this->drawMaxLine(maxValue, TFT_GREEN); // Draw max
  this->drawMaxLine((int16_t)(total / TFT_WIDTH), TFT_ORANGE); // Draw average
}

void MenuFunctions::renderGraphUI(uint8_t scan_mode) {
  #ifdef MARAUDER_V8
    if (scan_mode == BT_SCAN_ANALYZER) {
      this->drawBluetoothAnalyzerUI(true);
      return;
    }
  #endif
  display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (scan_mode == WIFI_SCAN_CHAN_ANALYZER)
    display_obj.tft.drawCentreString("Frames/" + (String)BANNER_TIME + "ms", SCREEN_WIDTH / 2, SCREEN_HEIGHT - GRAPH_VERT_LIM - (CHAR_WIDTH * 2), 1);
  else if (scan_mode == BT_SCAN_ANALYZER)
    display_obj.tft.drawCentreString("BLE Beacons/" + (String)BANNER_TIME + "ms", SCREEN_WIDTH / 2, SCREEN_HEIGHT - GRAPH_VERT_LIM - (CHAR_WIDTH * 2), 1);
  display_obj.tft.drawLine(0, SCREEN_HEIGHT - GRAPH_VERT_LIM - 1, SCREEN_WIDTH, SCREEN_HEIGHT - GRAPH_VERT_LIM - 1, TFT_WHITE);
  display_obj.tft.setCursor(0, SCREEN_HEIGHT - GRAPH_VERT_LIM - (CHAR_WIDTH * 8));
  display_obj.tft.setTextSize(1);
  display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
  display_obj.tft.println("Max");
  display_obj.tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  display_obj.tft.println("Average");
  display_obj.tft.setTextColor(TFT_RED, TFT_BLACK);
  if (scan_mode != BT_SCAN_ANALYZER)
    display_obj.tft.println("Channel Marker");
}

uint16_t MenuFunctions::getColor(uint16_t color) {
  #ifdef MARAUDER_V8
    // ctOS hierarchy: cyan carries the system, amber warns, red marks the
    // offensive tooling, bone is reserved for navigation entries.
    if (color == TFTRED)
      return WD_RED;
    if (color == TFTORANGE || color == TFTYELLOW)
      return WD_AMBER;
    if (color == TFTLIGHTGREY || color == TFTGREY || color == TFTGRAY ||
        color == TFTSILVER || color == TFTDARKGREY || color == TFTWHITE)
      return WD_BONE;
    return WD_CYAN;
  #endif
  if (color == TFTWHITE) return TFT_WHITE;
  else if (color == TFTCYAN) return TFT_CYAN;
  else if (color == TFTBLUE) return TFT_BLUE;
  else if (color == TFTRED) return TFT_RED;
  else if (color == TFTGREEN) return TFT_GREEN;
  else if (color == TFTGREY) return TFT_LIGHTGREY;
  else if (color == TFTGRAY) return TFT_LIGHTGREY;
  else if (color == TFTMAGENTA) return TFT_MAGENTA;
  else if (color == TFTVIOLET) return TFT_VIOLET;
  else if (color == TFTORANGE) return TFT_ORANGE;
  else if (color == TFTYELLOW) return TFT_YELLOW;
  else if (color == TFTLIGHTGREY) return TFT_LIGHTGREY;
  else if (color == TFTPURPLE) return TFT_PURPLE;
  else if (color == TFTNAVY) return TFT_NAVY;
  else if (color == TFTSILVER) return TFT_SILVER;
  else if (color == TFTDARKGREY) return TFT_DARKGREY;
  else if (color == TFTSKYBLUE) return TFT_SKYBLUE;
  else if (color == TFTLIME) return 0x97e0;
  else return color;
}

// Function to change menu
void MenuFunctions::changeMenu(Menu* menu, bool simple_change) {
  if (!simple_change) {
    //display_obj.initScrollValues();
    //display_obj.setupScrollArea(TOP_FIXED_AREA, BOT_FIXED_AREA);
    display_obj.init();

    #ifdef HAS_ILI9341
      extern void backlightOn();
	  backlightOn();
    #endif
  }
  current_menu = menu;

  current_menu->selected = 0;

  buildButtons(menu);

  displayCurrentMenu();

  //#ifdef MARAUDER_V8
  //  digitalWrite(TFT_BL, HIGH);
  //#endif
}

void MenuFunctions::buildButtons(Menu *menu, int starting_index, const char* button_name) {
  if (menu->list == NULL || menu->list->size() == 0)
      return;

  if (starting_index >= menu->list->size())
    starting_index = menu->list->size() - BUTTON_SCREEN_LIMIT;
  if (starting_index < 0)
    starting_index = 0;

  this->menu_start_index = starting_index;

  uint8_t visible_buttons = min(BUTTON_SCREEN_LIMIT, menu->list->size() - starting_index);

  for (uint8_t i = 0; i < visible_buttons; i++) {
    MenuNode node = menu->list->get(starting_index + i);
    uint16_t color = (node.icon == SETTINGS && node.color == TFTLIGHTGREY) ? (node.selected ? TFT_GREEN : TFT_RED) : this->getColor(node.color);

    char buf[64];

    if (button_name != nullptr && button_name[0] != '\0') {
      strncpy(buf, button_name, sizeof(buf));
      buf[sizeof(buf) - 1] = '\0';
    } else {
      node.name.toCharArray(buf, sizeof(buf));
    }

    #ifdef MARAUDER_V8
      const int16_t column = i % SHARK_GRID_COLUMNS;
      const int16_t row = i / SHARK_GRID_COLUMNS;
      const int16_t center_x = SHARK_GRID_LEFT + column * (SHARK_GRID_CELL_W + SHARK_GRID_GAP_X) + SHARK_GRID_CELL_W / 2;
      const int16_t center_y = SHARK_GRID_TOP + row * (SHARK_GRID_CELL_H + SHARK_GRID_GAP_Y) + SHARK_GRID_CELL_H / 2;
      display_obj.key[i].initButton(&display_obj.tft,
                                    center_x,
                                    center_y,
                                    SHARK_GRID_CELL_W,
                                    SHARK_GRID_CELL_H,
                                    SHARK_GREEN_DIM,
                                    TFT_BLACK,
                                    SHARK_GREEN,
                                    buf,
                                    1);
    #else
      display_obj.key[i].initButton(&display_obj.tft,
                                    KEY_X,
                                    KEY_Y + i * (KEY_H + KEY_SPACING_Y),
                                    KEY_W,
                                    KEY_H,
                                    TFT_BLACK,
                                    TFT_BLACK,
                                    color,
                                    buf,
                                    KEY_TEXTSIZE);

      #if defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)
        display_obj.key[i].setLabelDatum(BUTTON_PADDING - (KEY_W / 2), 4, ML_DATUM);
      #else
        display_obj.key[i].setLabelDatum(BUTTON_PADDING - (KEY_W / 2), 2, ML_DATUM);
      #endif
    #endif
  }

  #ifdef MARAUDER_V8
  const int16_t pager_height = TFT_HEIGHT - SHARK_GRID_FOOTER_Y - 7;
  display_obj.key[BUTTON_ARRAY_LEN + UP_BUTTON].initButton(&display_obj.tft,
                                                           51,
                                                           SHARK_GRID_FOOTER_Y + pager_height / 2,
                                                           94,
                                                           pager_height,
                                                           SHARK_GREEN_DIM,
                                                           TFT_BLACK,
                                                           SHARK_GREEN,
                                                           (char*)"",
                                                           1);
  display_obj.key[BUTTON_ARRAY_LEN + DOWN_BUTTON].initButton(&display_obj.tft,
                                                             TFT_WIDTH - 51,
                                                             SHARK_GRID_FOOTER_Y + pager_height / 2,
                                                             94,
                                                             pager_height,
                                                             SHARK_GREEN_DIM,
                                                             TFT_BLACK,
                                                             SHARK_GREEN,
                                                             (char*)"",
                                                             1);
  display_obj.key[BUTTON_ARRAY_LEN + SELECT_BUTTON].initButton(&display_obj.tft,
                                                               -8,
                                                               -8,
                                                               1,
                                                               1,
                                                               TFT_BLACK,
                                                               TFT_BLACK,
                                                               TFT_BLACK,
                                                               (char*)"",
                                                               1);
  #else
  for (int i = BUTTON_ARRAY_LEN; i < BUTTON_ARRAY_LEN + 3; i++) {
    uint16_t x = TFT_WIDTH / 2;
    uint16_t y = TFT_HEIGHT / 3 * (i - BUTTON_ARRAY_LEN) + ((TFT_HEIGHT / 3) / 2);
    uint16_t w = TFT_WIDTH;
    uint16_t h = TFT_HEIGHT / 3 - 1;

    display_obj.key[i].initButton(&display_obj.tft,
                                  x,
                                  y,
                                  w,
                                  h,
                                  TFT_LIGHTGREY,
                                  TFT_BLACK,
                                  TFT_BLACK,
                                  "Chicken",
                                  1);
  }
  #endif
}

void MenuFunctions::displayCurrentMenu(int start_index)
{
  //Serial.println(F("Displaying current menu..."));
  display_obj.clearScreen();
  display_obj.updateBanner(current_menu->name);
  display_obj.tft.setTextColor(TFT_LIGHTGREY, TFT_DARKGREY);
  this->drawStatusBar();

  if (current_menu->list != NULL)
  {
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.setFreeFont(MENU_FONT);
    #endif

    #ifdef HAS_MINI_SCREEN
      display_obj.tft.setFreeFont(NULL);
      display_obj.tft.setTextSize(1);
    #endif

    for (uint16_t i = start_index; i < min(start_index + BUTTON_SCREEN_LIMIT, current_menu->list->size()); i++)
    {
      if (!current_menu || !current_menu->list || i >= current_menu->list->size())
        continue;
      uint16_t color = this->getColor(current_menu->list->get(i).color);
      #ifdef HAS_FULL_SCREEN
        #ifdef MARAUDER_V8
          bool is_setting_node = (current_menu->list->get(i).icon == SETTINGS && current_menu->list->get(i).color == TFTLIGHTGREY);
          bool is_selected = current_menu->selected == i ||
                             (!is_setting_node && current_menu->list->get(i).selected);
          this->drawSharkGridButton(i - start_index, i, is_selected);
        #else
        bool is_setting_node = (current_menu->list->get(i).icon == SETTINGS && current_menu->list->get(i).color == TFTLIGHTGREY);
        if (is_setting_node && current_menu->selected == i) {
          uint16_t setting_color = current_menu->list->get(i).selected ? TFT_GREEN : TFT_RED;
          display_obj.key[i - start_index].initButton(&display_obj.tft, KEY_X, KEY_Y + (i - start_index) * (KEY_H + KEY_SPACING_Y), KEY_W, KEY_H, TFT_BLACK, TFT_LIGHTGREY, setting_color, (char*)"", KEY_TEXTSIZE);
          display_obj.key[i - start_index].drawButton(false, current_menu->list->get(i).name);
          display_obj.tft.drawXBitmap(0,
                                      KEY_Y + (i - start_index) * (KEY_H + KEY_SPACING_Y) - (ICON_H / 2),
                                      menu_icons[current_menu->list->get(i).icon],
                                      ICON_W,
                                      ICON_H,
                                      TFT_BLACK,
                                      TFT_LIGHTGREY);
        } else if ((!is_setting_node && current_menu->list->get(i).selected) || (current_menu->selected == i)) {
          display_obj.key[i - start_index].drawButton(true, current_menu->list->get(i).name);
          if ((current_menu->list->get(i).name != text09) && (current_menu->list->get(i).icon != 255))
            display_obj.tft.drawXBitmap(0,
                                        KEY_Y + (i - start_index) * (KEY_H + KEY_SPACING_Y) - (ICON_H / 2),
                                        menu_icons[current_menu->list->get(i).icon],
                                        ICON_W,
                                        ICON_H,
                                        TFT_BLACK,
                                        color);
        } else {
          display_obj.key[i - start_index].drawButton(false, current_menu->list->get(i).name);
          if ((current_menu->list->get(i).name != text09) && (current_menu->list->get(i).icon != 255))
            display_obj.tft.drawXBitmap(0,
                                        KEY_Y + (i - start_index) * (KEY_H + KEY_SPACING_Y) - (ICON_H / 2),
                                        menu_icons[current_menu->list->get(i).icon],
                                        ICON_W,
                                        ICON_H,
                                        TFT_BLACK,
                                        is_setting_node ? TFT_LIGHTGREY : color);
        }
        #endif

      #endif

      #ifdef HAS_MINI_SCREEN
        if ((current_menu->selected == i) || ((current_menu->list->get(i).icon != SETTINGS || current_menu->list->get(i).color != TFTLIGHTGREY) && current_menu->list->get(i).selected))
          this->drawMiniMenuButton(i - start_index, i, true);
        else 
          this->drawMiniMenuButton(i - start_index, i, false);
      #endif
    }
    display_obj.tft.setFreeFont(NULL);
  }

  this->displayMenuButtons();
}

// ============================================================
// BRIGHTNESS ADJUSTMENT MODE
// Hold top/bottom zone 1.5s to enter. TAP TOP = brighter, TAP BOTTOM = dimmer.
// TAP MIDDLE or wait 3s = save & exit.
// ============================================================
#ifndef HAS_MINI_SCREEN
  void MenuFunctions::brightnessMode() {
    extern void brightnessSave(uint8_t level);
    extern uint8_t getBrightnessLevel();

    const uint8_t levels[] = {26, 51, 77, 102, 128, 153, 179, 204, 230, 255};
    const uint8_t numLevels = 10;
    uint8_t level = getBrightnessLevel();

    // LEDC write compatibility (2.x vs 3.x board package)
    #if ESP_ARDUINO_VERSION_MAJOR >= 3
      #define BL_PREVIEW(duty) ledcWrite(TFT_BL, (duty))
    #else
      #define BL_PREVIEW(duty) ledcWrite(0, (duty))
    #endif

    display_obj.tft.fillScreen(TFT_BLACK);

    #ifdef MARAUDER_V8
      const uint16_t bright_title = WD_CYAN;
      const uint16_t bright_hint = WD_DIM;
      const uint16_t bright_save = WD_AMBER;
      const uint16_t bright_fill = WD_CYAN;
      const uint16_t bright_edge = WD_EDGE;
      const uint16_t bright_text = WD_WHITE;
      wdCornerTicks(display_obj.tft, 4, 4, TFT_WIDTH - 8, TFT_HEIGHT - 8, 12, WD_CYAN_DIM);
      display_obj.tft.setTextColor(bright_title, TFT_BLACK);
      display_obj.tft.drawCentreString("// BRIGHTNESS", TFT_WIDTH/2, 30, 2);
    #else
      const uint16_t bright_title = TFT_CYAN;
      const uint16_t bright_hint = TFT_DARKGREY;
      const uint16_t bright_save = TFT_RED;
      const uint16_t bright_fill = TFT_CYAN;
      const uint16_t bright_edge = TFT_WHITE;
      const uint16_t bright_text = TFT_WHITE;
      display_obj.tft.setTextColor(bright_title, TFT_BLACK);
      display_obj.tft.drawCentreString("BRIGHTNESS", TFT_WIDTH/2, 30, 2);
    #endif

    display_obj.tft.setTextColor(bright_hint, TFT_BLACK);
    display_obj.tft.drawCentreString("TAP TOP = BRIGHTER", TFT_WIDTH/2, 10, 1);
    display_obj.tft.drawCentreString("TAP BOTTOM = DIMMER", TFT_WIDTH/2, TFT_HEIGHT - 20, 1);
    display_obj.tft.setTextColor(bright_save, TFT_BLACK);
    display_obj.tft.drawCentreString("TAP MIDDLE or WAIT 3s = SAVE", TFT_WIDTH/2, TFT_HEIGHT/2 + 50, 1);

    auto drawBar = [&]() {
      uint16_t barX = 30, barY = TFT_HEIGHT/2 - 25, barW = TFT_WIDTH - 60, barH = 30;
      display_obj.tft.drawRect(barX, barY, barW, barH, bright_edge);
      uint16_t fillW = (barW - 4) * (level + 1) / numLevels;
      display_obj.tft.fillRect(barX + 2, barY + 2, barW - 4, barH - 4, TFT_BLACK);
      display_obj.tft.fillRect(barX + 2, barY + 2, fillW, barH - 4, bright_fill);
      #ifdef MARAUDER_V8
        wdCornerTicks(display_obj.tft, barX, barY, barW, barH, 6, WD_CYAN);
      #endif
      display_obj.tft.fillRect(0, barY + barH + 5, TFT_WIDTH, 20, TFT_BLACK);
      display_obj.tft.setTextColor(bright_text, TFT_BLACK);
      String pct = String(levels[level] * 100 / 255) + "%";
      display_obj.tft.drawCentreString(pct, TFT_WIDTH/2, barY + barH + 8, 2);
    };
    drawBar();

    uint16_t zoneUp = TFT_HEIGHT * 25 / 100;
    uint16_t zoneDown = TFT_HEIGHT * 75 / 100;
    uint32_t lastTouch = millis();

    while (true) {
      // Auto-save after 3s of no touch
      if (millis() - lastTouch >= 3000) {
        brightnessSave(level);
        break;
      }

      uint16_t tx, ty;
      if (display_obj.updateTouch(&tx, &ty)) {
        lastTouch = millis();
        // Wait for release
        while (display_obj.updateTouch(&tx, &ty)) delay(10);

        if (ty < zoneUp) {
          if (level < numLevels - 1) {
            level++;
            BL_PREVIEW(levels[level]);
            drawBar();
          }
        } else if (ty >= zoneDown) {
          if (level > 0) {
            level--;
            BL_PREVIEW(levels[level]);
            drawBar();
          }
        } else {
          // Middle = save now
          brightnessSave(level);
          break;
        }
        delay(150);
      }
      delay(30);
    }

    #undef BL_PREVIEW
    this->changeMenu(current_menu, true);
  }
#endif

#endif
