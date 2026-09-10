#include "Display.h"
#include "DisplayLine.h"
#include "lang_var.h"

#ifdef MARAUDER_V8
  #include "SharkTheme.h"
#endif

#ifdef HAS_SCREEN

Display::Display()
#ifdef HAS_CYD_TOUCH
  : touchscreenSPI(VSPI),
    touchscreen(XPT2046_CS, XPT2046_IRQ)
#endif
{
}

int8_t Display::menuButton(uint16_t *x, uint16_t *y, bool pressed, bool check_hold) {
  #ifdef HAS_ILI9341
    for (uint8_t b = BUTTON_ARRAY_LEN; b < BUTTON_ARRAY_LEN + 3; b++) {
      if (pressed && this->key[b].contains(*x, *y)) {
        this->key[b].press(true);  // tell the button it is pressed
      } else {
        this->key[b].press(false);  // tell the button it is NOT pressed
      }
    }

    for (uint8_t b = BUTTON_ARRAY_LEN; b < BUTTON_ARRAY_LEN + 3; b++) {
      if (!check_hold) {
        if ((this->key[b].justReleased()) && (!pressed)) {
          return b - BUTTON_ARRAY_LEN;
        }
      }
      else {
        if ((this->key[b].isPressed())) {
          return b - BUTTON_ARRAY_LEN;
        }
      }
    }

  #endif

  return -1;
}

uint8_t Display::updateTouch(uint16_t *x, uint16_t *y, uint16_t threshold) {
  #ifdef HAS_ILI9341
    if (!this->headless_mode) {
      #ifdef HAS_CAP_TOUCH
        // FT6336 capacitive touch: rotation-aware + edge exclusion
        {
          uint16_t raw_x, raw_y;
          if (!ft6336_read_raw(&raw_x, &raw_y)) return 0;

          // Discard touches within PANCAKE_TOUCH_MARGIN pixels of any panel edge
          #define PANCAKE_PANEL_W TFT_WIDTH
          #define PANCAKE_PANEL_H TFT_HEIGHT
          #define PANCAKE_TOUCH_MARGIN 5
          if (raw_x < PANCAKE_TOUCH_MARGIN || raw_x >= (PANCAKE_PANEL_W - PANCAKE_TOUCH_MARGIN)) return 0;
          if (raw_y < PANCAKE_TOUCH_MARGIN || raw_y >= (PANCAKE_PANEL_H - PANCAKE_TOUCH_MARGIN)) return 0;

          // Transform panel-native portrait coords to screen coords per rotation
          uint8_t rot = this->tft.getRotation();
          switch (rot) {
            case 0: // Portrait
              *x = raw_x;
              *y = raw_y;
              break;
            case 1: // Landscape 90 CW
              *x = raw_y;
              *y = (PANCAKE_PANEL_W - 1) - raw_x;
              break;
            case 2: // Portrait 180
              *x = (PANCAKE_PANEL_W - 1) - raw_x;
              *y = (PANCAKE_PANEL_H - 1) - raw_y;
              break;
            case 3: // Landscape 270 CW
              *x = (PANCAKE_PANEL_H - 1) - raw_y;
              *y = raw_x;
              break;
            default:
              *x = raw_x;
              *y = raw_y;
              break;
          }
          return 1;
        }
      #elif !defined(HAS_CYD_TOUCH)
        return this->tft.getTouch(x, y, threshold);
      #else
        if (this->touchscreen.tirqTouched() && this->touchscreen.touched()) {
          TS_Point p = this->touchscreen.getPoint();

          //*x = map(p.x, 200, 3700, 1, TFT_WIDTH);
          //*y = map(p.y, 240, 3800, 1, TFT_HEIGHT);

          uint8_t rot = this->tft.getRotation();

          //#ifdef HAS_CYD_PORTRAIT
          //  rot = 0;
          //#endif

          switch (rot) {
            case 0: // Standard Protrait
              *x = map(p.x, 200, 3700, 1, TFT_WIDTH);
              *y = map(p.y, 240, 3800, 1, TFT_HEIGHT);
              break;
            case 1:
              *x = map(p.y, 143, 3715, 0, TFT_HEIGHT);     // Horizontal (Y axis in touch, X on screen)
              *y = map(p.x, 3786, 216, 0, TFT_WIDTH);    // Vertical (X axis in touch, Y on screen)
              break;
            case 2:
              *x = map(p.x, 3700, 200, 1, TFT_WIDTH);
              *y = map(p.y, 3800, 240, 1, TFT_HEIGHT);
              break;
            case 3:
              *x = map(p.y, 3800, 240, 1, TFT_WIDTH);
              *y = map(p.x, 200, 3700, 1, TFT_HEIGHT);
              break;
          }
          return 1;
        }
        else
          return 0;
      #endif
    } else {
      return !this->headless_mode;
    }
  #endif

  return 0;
}

bool Display::isTouchHeld(uint16_t threshold) {
  static unsigned long touchStartTime = 0;
  static bool touchHeld = false;
  uint16_t x, y;

  if (this->updateTouch(&x, &y, threshold)) {
    // Touch detected
    if (touchStartTime == 0) {
      touchStartTime = millis();  // First touch timestamp
    } else if (!touchHeld && millis() - touchStartTime >= 1000) {
      touchHeld = true;  // Held for at least 1000ms
      return true;
    }
  } else {
    // Touch released
    touchStartTime = 0;
    touchHeld = false;
  }

  return false;
}

void Display::init() {
  tft.init();

  #if defined(HAS_DUAL_BAND) && !defined(MARAUDER_MINI_V3)
    digitalWrite(TFT_BL, HIGH);
  #endif
}

void Display::setCalData(bool landscape) {
  #if !defined(HAS_CYD_TOUCH) && !defined(HAS_CAP_TOUCH)
    if (!landscape) {
      #ifdef TFT_SHIELD
        uint16_t calData[5] = { 275, 3494, 361, 3528, 4 }; // tft.setRotation(0); // Portrait with TFT Shield
      #elif defined(MARAUDER_CYD_3_5_INCH)
        uint16_t calData[5] = { 239, 3560, 262, 3643, 4 };
      #elif defined(MARAUDER_V8)
        //uint16_t calData[5] = { 351, 3279, 214, 3394, 2 };
        uint16_t calData[5] = { 312, 3431, 191, 3456, 2 };
      #elif defined(TFT_DIY)
        uint16_t calData[5] = { 339, 3470, 237, 3438, 2 }; // tft.setRotation(0); // Portrait with DIY TFT
      #endif
      #ifdef HAS_ILI9341
        tft.setTouch(calData);
      #endif
    }
    else {
      #ifdef TFT_SHIELD
        uint16_t calData[5] = { 391, 3491, 266, 3505, 7 }; // Landscape TFT Shield
      #elif defined(MARAUDER_CYD_3_5_INCH)
        uint16_t calData[5] = { 272, 3648, 234, 3565, 7 };
      #elif defined(MARAUDER_V8)
        uint16_t calData[5] = { 213, 3396, 350, 3275, 1 };
      #else if defined(TFT_DIY)
        uint16_t calData[5] = { 213, 3469, 320, 3446, 1 }; // Landscape TFT DIY
      #endif
      #ifdef HAS_ILI9341
        tft.setTouch(calData);
      #endif
    }
  #endif
}

// Function to prepare the display and the menus
void Display::RunSetup() {
  run_setup = false;

  // Need to declare new
  display_buffer = new LinkedList<String>();

  #ifdef SCREEN_BUFFER
    screen_buffer = new LinkedList<String>();
  #endif

  #ifdef HAS_CYD_TOUCH
    this->touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    this->touchscreen.begin(touchscreenSPI);
    this->touchscreen.setRotation(0);
  #endif

  #ifdef HAS_CAP_TOUCH
    ft6336_init();
  #endif
  
  tft.init();

  tft.setRotation(SCREEN_ORIENTATION);

  #ifdef MARAUDER_V8
    // The WATCH_DOGS UI draws CP437 block glyphs, which TFT_eSPI otherwise
    // shifts by one character above code 175.
    tft.setAttribute(CP437_SWITCH, 1);
  #endif

  tft.setCursor(0, 0);

  #ifdef HAS_ILI9341

    #if !defined(HAS_CYD_TOUCH) && !defined(HAS_CAP_TOUCH)
      this->setCalData();
    #endif

  #endif

  clearScreen();

  #ifdef KIT
    pinMode(KIT_LED_BUILTIN, OUTPUT);
  #endif

  #ifdef MARAUDER_REV_FEATHER
    pinMode(7, OUTPUT);

    delay(10);

    digitalWrite(7, HIGH);
  #endif
}

void Display::tftDrawGraphObjects(byte x_scale)
{
  //draw the graph objects
  tft.fillRect(11, 5, x_scale+1, PKT_HALF, TFT_BLACK); // positive start point
  tft.fillRect(11, PKT_HALF + 1, x_scale+1, PKT_HALF - 1, TFT_BLACK); // negative start point
  tft.drawFastVLine(10, 5, PKT_HALF * 2 - 10, TFT_WHITE); // y axis
  tft.drawFastHLine(10, HEIGHT_1 - 1, PKT_AXIS_W, TFT_WHITE); // x axis
  tft.setTextColor(TFT_YELLOW); tft.setTextSize(1); // set parameters for y axis labels
  tft.setCursor(3, 6); tft.print("+"); // '+' at top of y axis
  tft.setCursor(3, PKT_HALF * 2 - 12); tft.print("0"); // "0" near baseline
}

void Display::tftDrawEapolColorKey(bool filter)
{
  //Display color key
  tft.setTextSize(1); tft.setTextColor(TFT_WHITE);
  tft.fillRect(14, 0, 15, 8, TFT_CYAN); tft.setCursor(30, 0); tft.println(" - EAPOL"); 
  if (filter) {
    uint16_t y = tft.getCursorY();
    tft.setCursor(14, y);
    tft.println("Filter Active");
  }
}

void Display::tftDrawColorKey()
{
  //Display color key
  tft.setTextSize(1); tft.setTextColor(TFT_WHITE);
  tft.fillRect(14, 0, 15, 8, TFT_GREEN); tft.setCursor(30, 0); tft.print(" - Beacons"); 
  tft.fillRect(14, 8, 15, 8, TFT_RED); tft.setCursor(30, 8); tft.print(" - Deauths");
  tft.fillRect(14, 16, 15, 8, TFT_BLUE); tft.setCursor(30, 16); tft.print(" - Probes");
}

void Display::tftDrawXScaleButtons(byte x_scale) {
  tft.drawFastVLine(234, 0, 20, TFT_WHITE);
  tft.setCursor(208, 21); tft.setTextColor(TFT_WHITE); tft.setTextSize(1); tft.print("X Scale:"); tft.print(x_scale);

  key[X_MINUS_INDEX].initButton(&tft, // x - box
                        220,
                        10, // x, y, w, h, outline, fill, text
                        20,
                        20,
                        TFT_BLACK, // Outline
                        TFT_CYAN, // Fill
                        TFT_BLACK, // Text
                        "-",
                        2);
  key[X_PLUS_INDEX].initButton(&tft, // x + box
                        249,
                        10, // x, y, w, h, outline, fill, text
                        20,
                        20,
                        TFT_BLACK, // Outline
                        TFT_CYAN, // Fill
                        TFT_BLACK, // Text
                        "+",
                        2);

  key[X_PLUS_INDEX].setLabelDatum(1, 5, MC_DATUM);
  key[X_MINUS_INDEX].setLabelDatum(1, 5, MC_DATUM);

  key[X_PLUS_INDEX].drawButton();
  key[X_MINUS_INDEX].drawButton();
}

void Display::tftDrawYScaleButtons(byte y_scale)
{
  tft.drawFastVLine(290, 0, 20, TFT_WHITE);
  tft.setCursor(265, 21); tft.setTextColor(TFT_WHITE); tft.setTextSize(1); tft.print("Y Scale:"); tft.print(y_scale);

  key[Y_MINUS_INDEX].initButton(&tft, // y - box
                        276,
                        10, // x, y, w, h, outline, fill, text
                        20,
                        20,
                        TFT_BLACK, // Outline
                        TFT_MAGENTA, // Fill
                        TFT_BLACK, // Text
                        "-",
                        2);
  key[Y_PLUS_INDEX].initButton(&tft, // y + box
                        305,
                        10, // x, y, w, h, outline, fill, text
                        20,
                        20,
                        TFT_BLACK, // Outline
                        TFT_MAGENTA, // Fill
                        TFT_BLACK, // Text
                        "+",
                        2);

  key[Y_MINUS_INDEX].setLabelDatum(1, 5, MC_DATUM);
  key[Y_PLUS_INDEX].setLabelDatum(1, 5, MC_DATUM);

  key[Y_MINUS_INDEX].drawButton();
  key[Y_PLUS_INDEX].drawButton();
}

#ifdef MARAUDER_V8
void Display::drawSharkKey(int16_t cx, int16_t cy, int16_t w, int16_t h,
                          const char* label, uint16_t fill, uint16_t edge,
                          uint16_t txt, uint8_t font) {
  const int16_t x = cx - w / 2, y = cy - h / 2;
  const int16_t cut = 5;
  tft.fillRect(x - 1, y - 1, w + 2, h + 2, TFT_BLACK);   // clear footprint
  tft.fillRect(x, y, w, h, fill);
  // Knock the top-right and bottom-left corners out for the cyberpunk cut.
  tft.fillTriangle(x + w - cut, y, x + w, y, x + w, y + cut, TFT_BLACK);
  tft.fillTriangle(x, y + h - cut, x, y + h, x + cut, y + h, TFT_BLACK);
  // Outline the cut-corner silhouette.
  tft.drawLine(x, y, x + w - cut, y, edge);
  tft.drawLine(x + w - cut, y, x + w, y + cut, edge);
  tft.drawLine(x + w, y + cut, x + w, y + h, edge);
  tft.drawLine(x + w, y + h, x + cut, y + h, edge);
  tft.drawLine(x + cut, y + h, x, y + h - cut, edge);
  tft.drawLine(x, y + h - cut, x, y, edge);
  // Corner accent notch (top-left).
  tft.drawFastHLine(x + 3, y + 3, 4, edge);
  tft.drawFastVLine(x + 3, y + 3, 4, edge);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(txt, fill);
  tft.drawString(label, cx, cy, font);
  tft.setTextDatum(TL_DATUM);
}
#endif

void Display::tftDrawChannelScaleButtons(int set_channel, bool lnd_an) {
  #ifdef MARAUDER_PANCAKE
    TOP_FIXED_AREA_2 = lnd_an ? 48 : 64;
  #endif
  // Themed scan-control buttons on v8; classic colors elsewhere.
  #ifdef MARAUDER_V8
    const uint16_t k_out = WD_CYAN, k_fill = WD_SURFACE, k_txt = WD_CYAN;
  #else
    const uint16_t k_out = TFT_BLACK, k_fill = TFT_BLUE, k_txt = TFT_BLACK;
  #endif
  if (lnd_an) {
    tft.drawFastVLine(178, 0, 20, TFT_WHITE);
    tft.setCursor(145, 21); tft.setTextColor(TFT_WHITE); tft.setTextSize(1); tft.print(text10); tft.print(set_channel);

    key[CHAN_MINUS_INDEX].initButton(&tft, // channel - box
                          164,
                          10, // x, y, w, h, outline, fill, text
                          EXT_BUTTON_WIDTH - (EXT_BUTTON_WIDTH * 0.33),
                          EXT_BUTTON_WIDTH - (EXT_BUTTON_WIDTH * 0.33),
                          k_out, // Outline
                          k_fill, // Fill
                          k_txt, // Text
                          "-",
                          2);
    key[CHAN_PLUS_INDEX].initButton(&tft, // channel + box
                          193,
                          10, // x, y, w, h, outline, fill, text
                          EXT_BUTTON_WIDTH - (EXT_BUTTON_WIDTH * 0.33),
                          EXT_BUTTON_WIDTH - (EXT_BUTTON_WIDTH * 0.33),
                          k_out, // Outline
                          k_fill, // Fill
                          k_txt, // Text
                          "+",
                          2);
  }

  else {
    key[CHAN_MINUS_INDEX].initButton(&tft, // channel - box
                          (EXT_BUTTON_WIDTH / 2) * 6,
                          (STATUS_BAR_WIDTH * 2) + (EXT_BUTTON_WIDTH / 2), // x, y, w, h, outline, fill, text
                          EXT_BUTTON_WIDTH,
                          EXT_BUTTON_WIDTH,
                          k_out, // Outline
                          k_fill, // Fill
                          k_txt, // Text
                          "-",
                          1);
    key[CHAN_PLUS_INDEX].initButton(&tft, // channel + box
                          (EXT_BUTTON_WIDTH / 2) * 10,
                          (STATUS_BAR_WIDTH * 2) + (EXT_BUTTON_WIDTH / 2), // x, y, w, h, outline, fill, text
                          EXT_BUTTON_WIDTH,
                          EXT_BUTTON_WIDTH,
                          k_out, // Outline
                          k_fill, // Fill
                          k_txt, // Text
                          "+",
                          1);
  }

  key[CHAN_MINUS_INDEX].setLabelDatum(1, 5, MC_DATUM);
  key[CHAN_PLUS_INDEX].setLabelDatum(1, 5, MC_DATUM);

  #ifdef MARAUDER_V8
    if (!lnd_an) {
      const int16_t bw = EXT_BUTTON_WIDTH;
      const int16_t by = (STATUS_BAR_WIDTH * 2) + (EXT_BUTTON_WIDTH / 2);
      drawSharkKey((EXT_BUTTON_WIDTH / 2) * 6,  by, bw, bw, "-", WD_SURFACE, WD_CYAN, WD_CYAN, 2);
      drawSharkKey((EXT_BUTTON_WIDTH / 2) * 10, by, bw, bw, "+", WD_SURFACE, WD_CYAN, WD_CYAN, 2);
    } else {
      key[CHAN_MINUS_INDEX].drawButton();
      key[CHAN_PLUS_INDEX].drawButton();
    }
  #else
    key[CHAN_MINUS_INDEX].drawButton();
    key[CHAN_PLUS_INDEX].drawButton();
  #endif
}

void Display::tftDrawChanHopButton(bool lnd_an, bool en) {
  #ifdef MARAUDER_PANCAKE
    TOP_FIXED_AREA_2 = lnd_an ? 48 : 64;
  #endif
  #ifdef MARAUDER_V8
    const uint16_t x_out = WD_AMBER, x_fill = WD_RED, x_txt = TFT_BLACK;
    const uint16_t on_out = WD_CYAN, on_fill = WD_CYAN, on_txt = TFT_BLACK;
  #else
    const uint16_t x_out = TFT_ORANGE, x_fill = TFT_RED, x_txt = TFT_BLACK;
    const uint16_t on_out = TFT_WHITE, on_fill = TFT_GREEN, on_txt = TFT_BLACK;
  #endif
  if (lnd_an) {
    if (!en) {
      key[CHAN_HOP_INDEX].initButton(&tft, // Exit box
                        137,
                        10, // x, y, w, h, outline, fill, text
                        EXT_BUTTON_WIDTH,
                        EXT_BUTTON_WIDTH,
                        x_out, // Outline
                        x_fill, // Fill
                        x_txt, // Text
                        "X",
                        2);
    } else {
      key[CHAN_HOP_INDEX].initButton(&tft, // Exit box
                        137,
                        10, // x, y, w, h, outline, fill, text
                        EXT_BUTTON_WIDTH,
                        EXT_BUTTON_WIDTH,
                        on_out, // Outline
                        on_fill, // Fill
                        on_txt, // Text
                        "O",
                        2);
    }
  }

  else {
    if (!en) {
      key[CHAN_HOP_INDEX].initButton(&tft, // Exit box
                        (EXT_BUTTON_WIDTH / 2) * 14,
                        (STATUS_BAR_WIDTH * 2) + (EXT_BUTTON_WIDTH / 2), // x, y, w, h, outline, fill, text
                        EXT_BUTTON_WIDTH,
                        EXT_BUTTON_WIDTH,
                        x_out, // Outline
                        x_fill, // Fill
                        x_txt, // Text
                        "HOP",
                        1);
    } else {
      key[CHAN_HOP_INDEX].initButton(&tft, // Exit box
                        (EXT_BUTTON_WIDTH / 2) * 14,
                        (STATUS_BAR_WIDTH * 2) + (EXT_BUTTON_WIDTH / 2), // x, y, w, h, outline, fill, text
                        EXT_BUTTON_WIDTH,
                        EXT_BUTTON_WIDTH,
                        on_out, // Outline
                        on_fill, // Fill
                        on_txt, // Text
                        "HOP",
                        1);
    }
  }

  key[CHAN_HOP_INDEX].setLabelDatum(1, 5, MC_DATUM);

  #ifdef MARAUDER_V8
    if (!lnd_an) {
      const int16_t bw = EXT_BUTTON_WIDTH;
      const int16_t bx = (EXT_BUTTON_WIDTH / 2) * 14;
      const int16_t by = (STATUS_BAR_WIDTH * 2) + (EXT_BUTTON_WIDTH / 2);
      if (en)  // channel hopping active: bright filled accent key
        drawSharkKey(bx, by, bw, bw, "HOP", WD_CYAN, WD_CYAN, TFT_BLACK, 1);
      else     // idle: dim inactive key
        drawSharkKey(bx, by, bw, bw, "HOP", WD_SURFACE, WD_DIM, WD_DIM, 1);
    } else {
      key[CHAN_HOP_INDEX].drawButton();
    }
  #else
    key[CHAN_HOP_INDEX].drawButton();
  #endif
}

void Display::tftDrawExitScaleButtons(bool lnd_an) {
  #ifdef MARAUDER_PANCAKE
    TOP_FIXED_AREA_2 = lnd_an ? 48 : 64;
  #endif
  #ifdef MARAUDER_V8
    const uint16_t x_out = WD_AMBER, x_fill = WD_RED, x_txt = TFT_BLACK;
  #else
    const uint16_t x_out = TFT_ORANGE, x_fill = TFT_RED, x_txt = TFT_BLACK;
  #endif
  //tft.drawFastVLine(178, 0, 20, TFT_WHITE);
  //tft.setCursor(145, 21); tft.setTextColor(TFT_WHITE); tft.setTextSize(1); tft.print("Channel:"); tft.print(set_channel);

  if (lnd_an) {

    key[EXIT_BUTTON_INDEX].initButton(&tft, // Exit box
                      137,
                      10, // x, y, w, h, outline, fill, text
                      EXT_BUTTON_WIDTH - (EXT_BUTTON_WIDTH * 0.33),
                      EXT_BUTTON_WIDTH - (EXT_BUTTON_WIDTH * 0.33),
                      x_out, // Outline
                      x_fill, // Fill
                      x_txt, // Text
                      "X",
                      2);
  }

  else {
    key[EXIT_BUTTON_INDEX].initButton(&tft, // Exit box
                      EXT_BUTTON_WIDTH,
                      (STATUS_BAR_WIDTH * 2) + (EXT_BUTTON_WIDTH / 2), // x, y, w, h, outline, fill, text
                      EXT_BUTTON_WIDTH,
                      EXT_BUTTON_WIDTH,
                      x_out, // Outline
                      x_fill, // Fill
                      x_txt, // Text
                      "X",
                      1);
  }

  key[EXIT_BUTTON_INDEX].setLabelDatum(1, 5, MC_DATUM);

  #ifdef MARAUDER_V8
    if (!lnd_an) {
      const int16_t bw = EXT_BUTTON_WIDTH;
      const int16_t bx = EXT_BUTTON_WIDTH;
      const int16_t by = (STATUS_BAR_WIDTH * 2) + (EXT_BUTTON_WIDTH / 2);
      drawSharkKey(bx, by, bw, bw, "X", WD_SURFACE, WD_RED, WD_RED, 2);
    } else {
      key[EXIT_BUTTON_INDEX].drawButton();
    }
  #else
    key[EXIT_BUTTON_INDEX].drawButton();
  #endif
}

void Display::twoPartDisplay(String center_text)
{
  tft.setTextColor(TFT_BLACK, TFT_YELLOW);
  tft.fillRect(0,16,HEIGHT_1,144, TFT_YELLOW);
  //tft.drawCentreString(center_text,120,82,1);
  tft.setTextWrap(true);
  tft.setFreeFont(NULL);
  //showCenterText(center_text, 82);
  //tft.drawCentreString(center_text,120,82,1);
  tft.setCursor(0, 82);
  tft.println(center_text);
  tft.setFreeFont(MENU_FONT);
  tft.setTextWrap(false);
}

void Display::touchToExit()
{
  #ifdef MARAUDER_V8
    // Themed exit control to match the rest of the app: a surface bar with an
    // accent top rule and edge underline, accent label. Follows the theme.
    tft.fillRect(0, 32, HEIGHT_1, 16, WD_SURFACE);
    tft.drawFastHLine(0, 32, HEIGHT_1, WD_CYAN);
    tft.drawFastHLine(0, 47, HEIGHT_1, WD_EDGE);
    tft.setTextColor(WD_CYAN, WD_SURFACE);
    tft.drawCentreString(text11, TFT_WIDTH / 2, 33, 2);
  #else
    tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    tft.fillRect(0,32,HEIGHT_1,16, TFT_LIGHTGREY);
    tft.drawCentreString(text11,TFT_WIDTH / 2,32,2);
  #endif
}


// Function to just draw the screen black
void Display::clearScreen()
{
  #ifdef MARAUDER_PANCAKE
    TOP_FIXED_AREA_2 = 48;
  #endif
  //Serial.println(F("clearScreen()"));
  #ifndef MARAUDER_V7
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
  #elif defined(MARAUDER_MINI) || defined(MARAUDER_MINI_V3)
    tft.fillRect(0, 0, TFT_WIDTH, TFT_HEIGHT, TFT_BLACK);
    tft.setCursor(0, 0);
  #else
    tft.fillRect(0, 0, TFT_WIDTH, TFT_HEIGHT, TFT_BLACK);
    tft.setCursor(0, 0);
  #endif
}

#ifdef SCREEN_BUFFER
void Display::scrollScreenBuffer(bool down) {
  // Scroll screen normal direction (Up)
  if (!down) {
    this->screen_buffer->shift();
  }
}
#endif

void Display::processAndPrintString(TFT_eSPI& tft, const String& originalString) {
  // Scan-log palette. On v8 the console lines follow the active theme so every
  // sniffer/scan body reads as the same UI as the rest of the app; other boards
  // keep the classic Marauder colors.
  #ifdef MARAUDER_V8
    const uint16_t c_default = WD_BONE, c_red = WD_RED, c_green = WD_CYAN,
                   c_cyan = WD_CYAN, c_white = WD_WHITE, c_magenta = WD_AMBER;
  #else
    const uint16_t c_default = TFT_GREEN, c_red = TFT_RED, c_green = TFT_GREEN,
                   c_cyan = TFT_CYAN, c_white = TFT_WHITE, c_magenta = TFT_MAGENTA;
  #endif
  uint16_t text_color = c_default;       // Default text color
  uint16_t background_color = TFT_BLACK; // Default background color

  String new_string = originalString;

  // Check for color macros at the start of the string
  if (new_string.startsWith(";")) {
    if (new_string.startsWith(RED_KEY)) {
      text_color = c_red;
      new_string.remove(0, strlen(RED_KEY)); // Remove the macro
    } else if (new_string.startsWith(GREEN_KEY)) {
      text_color = c_green;
      new_string.remove(0, strlen(GREEN_KEY)); // Remove the macro
    } else if (new_string.startsWith(CYAN_KEY)) {
      text_color = c_cyan;
      new_string.remove(0, strlen(CYAN_KEY)); // Remove the macro
    } else if (new_string.startsWith(WHITE_KEY)) {
      text_color = c_white;
      new_string.remove(0, strlen(WHITE_KEY)); // Remove the macro
    } else if (new_string.startsWith(MAGENTA_KEY)) {
      text_color = c_magenta;
      new_string.remove(0, strlen(MAGENTA_KEY)); // Remove the macro
    }
  }

  // Scan output uses the built-in 6-pixel font. CHAR_WIDTH describes larger
  // menu/layout cells on full-size displays, so using it here cut their line
  // capacity in half and truncated values such as MAC addresses.
  char line[STANDARD_FONT_CHAR_LIMIT + 1];

  #ifdef MARAUDER_V8
    // List UI: a dashed accent rail marks every row, the text is indented past
    // it, and "A -> B" rows (sniffers, MAC -> SSID, etc.) are split into a
    // neutral identifier, a dim arrow, and an accent name so the list reads as
    // structured records instead of a flat green log.
    const int16_t row_y = tft.getCursorY();
    tft.drawFastVLine(1, row_y, 8, WD_CYAN_DIM);
    tft.setCursor(6, row_y);

    int sep = new_string.indexOf(" -> ");
    int sep_len = 4;
    if (sep < 0) { sep = new_string.indexOf("->"); sep_len = 2; }
    if (sep > 0 && text_color == c_default) {
      char lbuf[STANDARD_FONT_CHAR_LIMIT + 1], rbuf[STANDARD_FONT_CHAR_LIMIT + 1];
      fitDisplayLine(lbuf, sizeof(lbuf), new_string.substring(0, sep).c_str());
      fitDisplayLine(rbuf, sizeof(rbuf), new_string.substring(sep + sep_len).c_str());
      tft.setTextColor(WD_BONE, background_color);  tft.print(lbuf);
      tft.setTextColor(WD_DIM, background_color);   tft.print(" > ");
      tft.setTextColor(WD_CYAN, background_color);  tft.print(rbuf);
    } else {
      fitDisplayLine(line, sizeof(line), new_string.c_str());
      tft.setTextColor(text_color, background_color);
      tft.print(line);
    }
    return;
  #endif

  fitDisplayLine(line, sizeof(line), new_string.c_str()); // GCOVR_EXCL_LINE

  // Set text color and print the string
  tft.setTextColor(text_color, background_color);
  tft.print(line);
}

#ifdef MARAUDER_V8
// Stable per-device accent picked from the identifier, so each MAC keeps its
// own colour chip across the feed.
static uint16_t sharkDeviceColor(const String& key) {
  uint32_t hsh = 2166136261u;
  for (uint16_t i = 0; i < key.length(); i++) { hsh ^= (uint8_t)key[i]; hsh *= 16777619u; }
  const uint16_t pal[6] = {WD_CYAN, WD_AMBER, WD_WHITE, WD_GREY, WD_CYAN_SOFT, WD_RED};
  return pal[hsh % 6];
}

void Display::drawScanCard(int16_t x, int16_t y, int16_t w, int16_t h,
                          const String& raw, int index, bool newest) {
  // Strip any leading color macro and let it tint the card accent.
  String s = raw;
  uint16_t tone = WD_CYAN;
  if      (s.startsWith(RED_KEY))     { tone = WD_RED;   s.remove(0, strlen(RED_KEY)); }
  else if (s.startsWith(GREEN_KEY))   { tone = WD_CYAN;  s.remove(0, strlen(GREEN_KEY)); }
  else if (s.startsWith(CYAN_KEY))    { tone = WD_CYAN;  s.remove(0, strlen(CYAN_KEY)); }
  else if (s.startsWith(WHITE_KEY))   { tone = WD_WHITE; s.remove(0, strlen(WHITE_KEY)); }
  else if (s.startsWith(MAGENTA_KEY)) { tone = WD_AMBER; s.remove(0, strlen(MAGENTA_KEY)); }

  // Newest record pulses so the live feed reads as animated.
  const bool pulse = newest && ((millis() / 160) & 1);
  const uint16_t border = newest ? WD_CYAN : WD_EDGE;
  const uint16_t body = (index & 1) ? WD_PANEL : WD_SURFACE;   // alternating rows
  const uint16_t rail = newest ? (pulse ? WD_WHITE : WD_CYAN) : tone;
  const int16_t cut = 6;

  // Cut-corner card body (top-right sliced) to match the control keys.
  tft.fillRect(x, y, w, h, body);
  tft.fillTriangle(x + w - cut, y, x + w, y, x + w, y + cut, TFT_BLACK);
  tft.drawFastVLine(x, y, h, border);
  tft.drawFastHLine(x, y + h - 1, w, border);
  tft.drawFastVLine(x + w - 1, y + cut, h - cut, border);
  tft.drawLine(x, y, x + w - cut, y, border);
  tft.drawLine(x + w - cut, y, x + w - 1, y + cut, border);
  // Accent rail.
  tft.fillRect(x + 2, y + 3, 3, h - 6, rail);

  tft.setTextWrap(false);
  int sep = s.indexOf(" -> ");
  int sl = 4;
  if (sep < 0) { sep = s.indexOf("->"); sl = 2; }

  if (sep > 0) {
    String id = s.substring(0, sep);      id.trim();
    String nm = s.substring(sep + sl);    nm.trim();
    // Per-device colour chip keyed on the identifier.
    const int16_t chip = 8, chx = x + 9, chy = y + (h - chip) / 2;
    tft.fillRect(chx, chy, chip, chip, sharkDeviceColor(id));
    tft.drawRect(chx, chy, chip, chip, WD_EDGE);

    const int16_t textx = x + 22, rightpad = 16;
    while (id.length() > 3 && tft.textWidth(id, 1) > w - textx - rightpad) id.remove(id.length() - 1);
    while (nm.length() > 1 && tft.textWidth(nm, 2) > w - textx - rightpad) nm.remove(nm.length() - 1);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_DIM, body);                       // MAC: quiet, on top
    tft.drawString(id, textx, y + 3, 1);
    tft.setTextColor(newest ? WD_WHITE : WD_CYAN, body);  // name: the hero line
    tft.drawString(nm, textx, y + 11, 2);
  } else {
    const int16_t textx = x + 12;
    while (s.length() > 3 && tft.textWidth(s, 1) > w - textx - 16) s.remove(s.length() - 1);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(WD_BONE, body);
    tft.drawString(s, textx, y + h / 2, 1);
  }

  // Right marker: a pulsing dot for the live newest row, a chevron otherwise.
  if (newest) {
    tft.fillCircle(x + w - 10, y + h / 2, 3, pulse ? WD_WHITE : WD_CYAN);
  } else {
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WD_DIM, body);
    tft.drawString(">", x + w - 9, y + h / 2, 1);
  }
  tft.setTextDatum(TL_DATUM);
}
#endif

void Display::displayBuffer(bool do_clear)
{
  if (this->display_buffer->size() > 0)
  {
    if (display_buffer->size() > DISPLAY_BUFFER_LIMIT)
      this->printing = true;

    #ifdef MARAUDER_V8
    {
      // v8 scan output is rendered as a scrolling stack of themed cards. Drain
      // the incoming lines into the visible buffer (newest last), then repaint.
      const int CARD_CAP = 8;
      const int old_n = this->screen_buffer->size();
      bool scrolled = false;
      while (this->display_buffer->size() > 0) {
        if (this->screen_buffer->size() >= CARD_CAP) {
          this->scrollScreenBuffer();
          scrolled = true;
        }
        this->screen_buffer->add(this->display_buffer->shift());
      }
      const int16_t CARD_X = 4, CARD_W = SCREEN_WIDTH - 8;
      const int16_t CARD_H = 27, PITCH = 30, CARD_Y0 = 58;
      const int n = this->screen_buffer->size();
      // On a scroll every card moved; otherwise only the new cards (and the
      // previous newest, to drop its highlight) need repainting -- less flicker.
      int start = scrolled ? 0 : (old_n > 0 ? old_n - 1 : 0);
      for (int i = start; i < n; i++)
        this->drawScanCard(CARD_X, CARD_Y0 + i * PITCH, CARD_W, CARD_H,
                           this->screen_buffer->get(i), i, i == n - 1);
      this->printing = false;
      return;
    }
    #endif

    int print_count = 2;

    while ((display_buffer->size() > 0) && (print_count > 0))
    {
      // Freeze adding to display buffer
      if (display_buffer->size() > DISPLAY_BUFFER_LIMIT)
        this->printing = true;

      /*#ifndef SCREEN_BUFFER
        xPos = 0;
        if ((display_buffer->size() > 0) && (!loading))
        {
          //printing = true;
          delay(print_delay_1);
          yDraw = scroll_line(TFT_RED);
          tft.setCursor(xPos, yDraw);
          tft.setTextColor(TFT_GREEN, TFT_BLACK);
          tft.print(display_buffer->shift());
          //printing = false;
          delay(print_delay_2);
        }
        if (!tteBar)
          blank[(18+(yStart - TOP_FIXED_AREA) / TEXT_HEIGHT)%19] = xPos;
        else
          blank[(18+(yStart - TOP_FIXED_AREA_2) / TEXT_HEIGHT)%19] = xPos;
      #else*/
        xPos = 0;
        if (this->screen_buffer->size() >= MAX_SCREEN_BUFFER)
          this->scrollScreenBuffer();

        screen_buffer->add(display_buffer->shift());

        for (int i = 0; i < this->screen_buffer->size(); i++) {
		  #ifdef MARAUDER_PANCAKE
			tft.setCursor(xPos, (i * TEXT_HEIGHT) + TOP_FIXED_AREA_2);
		  #else
			#ifdef HAS_TOUCH
			  tft.setCursor(xPos, (i * 12) + ((TFT_HEIGHT / 6) * 1.3));
			#else
			  tft.setCursor(xPos, (i * 12) + (TFT_HEIGHT / 6));
			#endif
		  #endif

          this->processAndPrintString(tft, this->screen_buffer->get(i));
        }
      //#endif

      print_count--;
    }

    this->printing = false;
  }
}

void Display::showCenterText(const char* text, int y, bool small_pp, uint8_t text_size)
{
  if (!text)
    text = "";

  // Centering already assumes either the 1x bitmap font or text_size scaling.
  // Apply that size here as well so callers never inherit a previous UI's
  // text scale (for example, the 2x upload percentage display).
  const uint8_t effective_text_size = resolveDisplayTextSize(small_pp, text_size);
  tft.setTextSize(effective_text_size);

  size_t len = strlen(text);

  if (!small_pp)
    tft.setCursor((SCREEN_WIDTH - (len * (6 * effective_text_size))) / 2, y);
  else
    tft.setCursor((SCREEN_WIDTH - (len * 6)) / 2, y);

  tft.println(text);
}


void Display::updateBanner(String msg)
{
  this->buildBanner(msg, current_banner_pos);
}


void Display::buildBanner(String msg, int xpos)
{
  int h = TEXT_HEIGHT;

  #if defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)
    int banner_y = STATUS_BAR_WIDTH + 8;
  #else
    int banner_y = STATUS_BAR_WIDTH;
  #endif
  this->tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, TEXT_HEIGHT + (banner_y - STATUS_BAR_WIDTH), TFT_BLACK);
  this->tft.setFreeFont(NULL);

  #ifdef MARAUDER_V8
    // ctOS section header: a cyan slash marker, the section in upper case, and
    // the system tag on the right. Branding and device state live in the top
    // bar, so the title never competes with status information.
    const int16_t title_mid = banner_y + TEXT_HEIGHT / 2;
    const int16_t tag_width = (int16_t)strlen(SHARK_UI_TAG) * 6;

    msg.toUpperCase();
    this->tft.setTextSize(1);
    this->tft.setTextDatum(ML_DATUM);
    this->tft.setTextColor(WD_CYAN, TFT_BLACK);
    this->tft.drawString("//", 6, title_mid, 2);

    this->tft.setTextColor(WD_WHITE, TFT_BLACK);
    while (msg.length() > 3 &&
           this->tft.textWidth(msg, 2) > SCREEN_WIDTH - 34 - tag_width)
      msg.remove(msg.length() - 1);
    this->tft.drawString(msg, 26, title_mid, 2);

    this->tft.setTextDatum(MR_DATUM);
    this->tft.setTextColor(WD_DIM, TFT_BLACK);
    this->tft.drawString(SHARK_UI_TAG, SCREEN_WIDTH - 6, title_mid, 1);
    this->tft.setTextDatum(TL_DATUM);

    this->tft.drawFastHLine(0, banner_y + TEXT_HEIGHT - 1, SCREEN_WIDTH, WD_EDGE);
    this->tft.drawFastHLine(0, banner_y + TEXT_HEIGHT - 1, 46, WD_CYAN);
  #else
    this->tft.setTextSize(BANNER_TEXT_SIZE);
    this->tft.setTextColor(TFT_WHITE, TFT_BLACK);
    this->showCenterText(msg.c_str(), banner_y);
  #endif
}

#endif
