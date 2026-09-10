#include "SharkPrank.h"

#if defined(HAS_SCREEN) && defined(MARAUDER_V8)

#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include "SharkTheme.h"
#include "Display.h"
#include "MenuFunctions.h"
#include "WiFiScan.h"
#include "SharkMonkeyImg.h"
#ifdef HAS_SD
  #include <SD.h>
  #include "SDInterface.h"
  extern SDInterface sd_obj;
#endif
#if defined(HAS_BT)
  #include <NimBLEDevice.h>
#endif

extern Display display_obj;
extern MenuFunctions menu_function_obj;
extern WiFiScan wifi_scan_obj;

SharkPrank shark_prank_obj;

namespace {
  const IPAddress PRANK_AP_IP(192, 168, 4, 1);
  AsyncWebServer prank_server(80);
  DNSServer prank_dns;
  bool prank_routes = false;   // handlers registered once for the process
  bool prank_dns_on = false;
  String g_prank_page;         // the page the (single) portal handler serves

  // Cut-corner control key, matching the scan-control button style.
  void drawKey(int16_t cx, int16_t cy, int16_t w, int16_t h, const char* label,
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

  // Full-screen prank-image portal page: black, centred, aspect-preserved,
  // responsive, no forms. References the image at /img.
  const char* MONKEY_HTML =
    "<!doctype html><html><head><meta charset=utf-8><meta name=viewport "
    "content='width=device-width,initial-scale=1,maximum-scale=1'><title>FREE WIFI"
    "</title><style>html,body{margin:0;width:100%;height:100%;background:#000;"
    "overflow:hidden}img{position:fixed;top:0;left:0;right:0;bottom:0;margin:auto;"
    "max-width:100%;max-height:100%;width:auto;height:auto;object-fit:contain}"
    "</style></head><body><img src='/img'></body></html>";

  // Where the active Monkey image lives on SD (admin override of the default).
  const char* MONKEY_EXTS[5] = {"jpg", "jpeg", "png", "gif", "webp"};
  const char* MONKEY_MIME[5] = {"image/jpeg", "image/jpeg", "image/png",
                                "image/gif", "image/webp"};

  int monkeyActiveExt() {   // returns index into MONKEY_EXTS, or -1 for default
    #ifdef HAS_SD
      if (sd_obj.supported)
        for (int i = 0; i < 5; i++)
          if (SD.exists(String("/monkey/active.") + MONKEY_EXTS[i])) return i;
    #endif
    return -1;
  }

  // Registers the captive-portal handlers once for the process: the page (served
  // from g_prank_page), the Monkey image at /img, and a catch-all. These are the
  // ONLY routes on the prank AP -- no admin/upload/settings endpoints, so FREE
  // WIFI clients can never reach administrative functions.
  // 302 to the portal root. Answering the OS "is there internet?" probe with a
  // redirect (rather than the page body) is what makes BOTH iOS and Android
  // auto-pop their captive browser straight onto the portal, instead of Android
  // just showing a silent "Sign in to network" notification.
  void portalRedirect(AsyncWebServerRequest* r) {
    r->redirect("http://192.168.4.1/");
  }

  void ensurePortalRoutes() {
    if (prank_routes) return;
    // The portal page itself and its image.
    prank_server.on("/", HTTP_GET, [](AsyncWebServerRequest* r) {
      r->send(200, "text/html; charset=utf-8", g_prank_page);
    });
    prank_server.on("/img", HTTP_GET, [](AsyncWebServerRequest* r) {
      SharkPrank::serveMonkeyImage(r);
    });
    // Known captive-portal detection endpoints -> redirect so the phone opens
    // its captive browser automatically the moment it joins.
    const char* probes[] = {
      "/generate_204", "/gen_204",             // Android
      "/hotspot-detect.html",                  // iOS / macOS
      "/library/test/success.html",            // iOS / macOS
      "/ncsi.txt", "/connecttest.txt",         // Windows
      "/redirect", "/canonical.html",          // Windows / Firefox
      "/success.txt", "/check_network_status.txt"
    };
    for (uint8_t i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
      prank_server.on(probes[i], HTTP_GET,
                      [](AsyncWebServerRequest* r) { portalRedirect(r); });
    prank_server.onNotFound([](AsyncWebServerRequest* r) {   // captive catch-all
      portalRedirect(r);
    });
    prank_server.begin();
    prank_routes = true;
  }

  const char* PRANK_PAGE =
    "<!doctype html><html><head><meta name=viewport content='width=device-width,"
    "initial-scale=1'><title>ACCESS GRANTED</title><style>body{background:#050505;"
    "color:#00ffcc;font-family:monospace;text-align:center;padding:40px}h1{font-size:"
    "2em;text-shadow:0 0 8px #00ffcc}.b{border:1px solid #00ffcc;padding:20px;margin"
    ":20px auto;max-width:340px}</style></head><body><div class=b><h1>YOU GOT"
    " PRANKED</h1><p>// DEDSEC WAS HERE</p><p>This network is a joke. Your device is"
    " fine. Have a nice day. :)</p></div></body></html>";

  const char* MEME_PAGE =
    "<!doctype html><html><head><meta name=viewport content='width=device-width,"
    "initial-scale=1'><title>MEME NODE</title><style>body{background:#050505;color:"
    "#faff00;font-family:monospace;text-align:center;padding:30px}pre{color:#00ffcc;"
    "font-size:12px}.b{border:1px solid #ff0055;padding:16px;margin:16px auto;max-"
    "width:360px}</style></head><body><div class=b><h2>[ CERTIFIED MEME DELIVERY ]"
    "</h2><pre>  (o_o)   \n /|___|\\  \n  /   \\   </pre><p>404: seriousness not found."
    "</p><p>// powered by SHARK</p></div></body></html>";

  String customPortalPage() {
    #ifdef HAS_SD
      if (sd_obj.supported && SD.exists("/prank/portal.html")) {
        File f = SD.open("/prank/portal.html", FILE_READ);
        if (f) {
          String html = f.readString();
          f.close();
          if (html.length()) return html;
        }
      }
    #endif
    return String("<!doctype html><meta name=viewport content='width=device-width'>"
                  "<body style='background:#050505;color:#00ffcc;font-family:monospace;"
                  "text-align:center;padding:40px'><h2>CUSTOM PORTAL</h2><p>Put your page"
                  " at SD /prank/portal.html</p></body>");
  }
}

void SharkPrank::apDown() {
  if (prank_dns_on) { prank_dns.stop(); prank_dns_on = false; }
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
}

void SharkPrank::apUp(const char* ssid) {
  esp_wifi_set_promiscuous(false);
  WiFi.persistent(false);
  WiFi.disconnect(true, true);
  WiFi.softAPdisconnect(true);
  delay(120);
  WiFi.mode(WIFI_AP);
  delay(40);
  WiFi.softAPConfig(PRANK_AP_IP, PRANK_AP_IP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, NULL, 1, 0, 8);   // open network, channel 1, up to 8 clients
  delay(200);
}

void SharkPrank::drawControls(bool running) {
  // drawKey takes CENTER coordinates. Three 72x34 keys centered at
  // x=40/120/200 put their rects at 4..76, 84..156, 164..236 -- fully
  // on the 240px panel, and each inside its touch third in control().
  const int16_t y = 302, h = 34, w = 72;
  drawKey(40,  y, w, h, "BACK", WD_SURFACE, WD_CYAN, WD_CYAN);
  drawKey(120, y, w, h, "START", running ? WD_SURFACE : WD_CYAN,
          running ? WD_DIM : WD_CYAN, running ? WD_DIM : TFT_BLACK);
  drawKey(200, y, w, h, "STOP", running ? WD_RED : WD_SURFACE,
          running ? WD_AMBER : WD_DIM, running ? TFT_BLACK : WD_DIM);
}

void SharkPrank::frame(const char* title, bool running) {
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
  drawControls(running);
}

void SharkPrank::status(const char* l1, const char* l2, const char* l3, uint16_t col) {
  TFT_eSPI& tft = display_obj.tft;
  const int16_t w = tft.width();
  tft.fillRect(0, 46, w, 234, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  if (l1 && l1[0]) { tft.setTextColor(col, TFT_BLACK);    tft.drawString(l1, w / 2, 96, 4); }
  if (l2 && l2[0]) { tft.setTextColor(WD_BONE, TFT_BLACK); tft.drawString(l2, w / 2, 150, 2); }
  if (l3 && l3[0]) { tft.setTextColor(WD_GREY, TFT_BLACK); tft.drawString(l3, w / 2, 190, 2); }
  tft.setTextDatum(TL_DATUM);
}

int SharkPrank::control() {
  uint16_t tx, ty;
  // 350 threshold: the same value the shared dashboards use so light
  // taps register through the case (the default 600 eats them).
  if (!display_obj.updateTouch(&tx, &ty, 350)) return -1;
  while (display_obj.updateTouch(&tx, &ty, 350)) delay(10);
  // Bottom strip = three equal thirds; each key is drawn inside its third.
  if (ty < 278) return -1;
  if (tx < 80) return 0;
  if (tx < 162) return 1;
  return 2;
}

// --- Individual pranks -------------------------------------------------------

void SharkPrank::funnyHotspot() {
  const char* ssid = "FBI Surveillance Van 3";
  bool running = false;
  frame("FUNNY HOTSPOT", running);
  status("READY", ssid, "tap START to broadcast", WD_DIM);
  uint32_t last = 0;
  while (true) {
    int c = control();
    if (c == 0) { apDown(); return; }
    if (c == 1 && !running) { apUp(ssid); running = true; frame("FUNNY HOTSPOT", running); }
    if (c == 2 && running)  { apDown(); running = false; frame("FUNNY HOTSPOT", running);
                              status("STOPPED", ssid, "", WD_AMBER); }
    if (running && millis() - last > 500) {
      last = millis();
      status("ON AIR", ssid, (String("clients: ") + WiFi.softAPgetStationNum()).c_str(), WD_CYAN);
    }
    delay(20);
  }
}

void SharkPrank::ssidRotator() {
  static const char* names[6] = {
    "Pretty Fly for a WiFi", "Hide Yo Kids Hide Yo WiFi", "Drop It Like Its Hotspot",
    "Loading...", "Virus.exe", "Get Off My LAN"
  };
  bool running = false;
  uint8_t idx = 0;
  frame("SSID ROTATOR", running);
  status("READY", names[0], "tap START to rotate", WD_DIM);
  uint32_t last = 0;
  while (true) {
    int c = control();
    if (c == 0) { apDown(); return; }
    if (c == 1 && !running) { idx = 0; apUp(names[idx]); running = true;
                              frame("SSID ROTATOR", running); last = millis();
                              status("ON AIR", names[idx], "rotating...", WD_CYAN); }
    if (c == 2 && running)  { apDown(); running = false; frame("SSID ROTATOR", running);
                              status("STOPPED", names[idx], "", WD_AMBER); }
    if (running && millis() - last > 5000) {
      last = millis();
      idx = (idx + 1) % 6;
      apUp(names[idx]);
      status("ON AIR", names[idx], (String("clients: ") + WiFi.softAPgetStationNum()).c_str(), WD_CYAN);
    }
    delay(20);
  }
}

void SharkPrank::guestCounter() {
  const char* ssid = "Free Public WiFi";
  bool running = false;
  frame("GUEST COUNTER", running);
  status("READY", ssid, "tap START to open AP", WD_DIM);
  uint32_t last = 0;
  while (true) {
    int c = control();
    if (c == 0) { apDown(); return; }
    if (c == 1 && !running) { apUp(ssid); running = true; frame("GUEST COUNTER", running); }
    if (c == 2 && running)  { apDown(); running = false; frame("GUEST COUNTER", running);
                              status("STOPPED", ssid, "", WD_AMBER); }
    if (running && millis() - last > 400) {
      last = millis();
      TFT_eSPI& tft = display_obj.tft;
      tft.fillRect(0, 46, tft.width(), 234, TFT_BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(WD_GREY, TFT_BLACK);
      tft.drawString("CONNECTED GUESTS", tft.width() / 2, 80, 2);
      tft.setTextColor(WD_CYAN, TFT_BLACK);
      tft.drawString(String(WiFi.softAPgetStationNum()), tft.width() / 2, 150, 7);
      tft.setTextColor(WD_DIM, TFT_BLACK);
      tft.drawString(ssid, tft.width() / 2, 210, 2);
      tft.setTextDatum(TL_DATUM);
    }
    delay(20);
  }
}

// Captive-portal pranks. A single set of handlers (registered once) serves the
// current page held in g_prank_page, so switching pranks never stacks handlers.
void SharkPrank::runPortal(const char* title, const char* ssid_label,
                           const char* page, int kind) {
  const char* ssid = "Free WiFi";
  bool running = false;
  frame(title, running);
  status("READY", ssid_label, "tap START to host", WD_DIM);
  uint32_t last = 0;
  while (true) {
    int c = control();
    if (c == 0) { apDown(); return; }
    if (c == 1 && !running) {
      g_prank_page = (kind == 2) ? customPortalPage() : String(page);
      apUp(ssid);
      ensurePortalRoutes();
      prank_dns.setErrorReplyCode(DNSReplyCode::NoError);
      prank_dns.start(53, "*", PRANK_AP_IP);
      prank_dns_on = true;
      running = true;
      frame(title, running);
      status("HOSTING", ssid_label, "join 'Free WiFi'", WD_CYAN);
    }
    if (c == 2 && running) {
      apDown(); running = false;
      frame(title, running);
      status("STOPPED", ssid_label, "", WD_AMBER);
    }
    if (running) {
      prank_dns.processNextRequest();
      if (millis() - last > 600) {
        last = millis();
        status("HOSTING", ssid_label,
               (String("clients: ") + WiFi.softAPgetStationNum()).c_str(), WD_CYAN);
      }
    }
    delay(15);
  }
}

void SharkPrank::prankPortal() { runPortal("PRANK PORTAL", "You Have Been Pranked", PRANK_PAGE, 0); }
void SharkPrank::memePortal()  { runPortal("MEME PORTAL",  "Meme Delivery Node",   MEME_PAGE, 1); }
void SharkPrank::customPortal(){ runPortal("CUSTOM PORTAL","SD /prank/portal.html", NULL, 2); }

// --- Monkey WiFi -------------------------------------------------------------

void SharkPrank::serveMonkeyImage(AsyncWebServerRequest* req) {
  #ifdef HAS_SD
    int i = monkeyActiveExt();
    if (i >= 0) {
      AsyncWebServerResponse* r =
        req->beginResponse(SD, String("/monkey/active.") + MONKEY_EXTS[i], MONKEY_MIME[i]);
      r->addHeader("Cache-Control", "no-store");
      req->send(r);
      return;
    }
  #endif
  AsyncWebServerResponse* r =
    req->beginResponse_P(200, "image/jpeg", MONKEY_DEFAULT_JPG, MONKEY_DEFAULT_JPG_LEN);
  r->addHeader("Cache-Control", "no-store");
  req->send(r);
}

const char* SharkPrank::monkeyImageSource() {
  return (monkeyActiveExt() >= 0) ? "SD" : "DEFAULT";
}

void SharkPrank::monkeyRestoreDefault() {
  #ifdef HAS_SD
    if (sd_obj.supported)
      for (int i = 0; i < 5; i++) {
        String p = String("/monkey/active.") + MONKEY_EXTS[i];
        if (SD.exists(p)) SD.remove(p);
      }
  #endif
}

void SharkPrank::monkeyWifi() {
  const char* ssid = "FREE WIFI";
  TFT_eSPI& tft = display_obj.tft;
  bool running = false;

  auto chrome = [&]() {
    tft.fillScreen(TFT_BLACK);
    menu_function_obj.drawStatusBar();
    const int16_t y0 = STATUS_BAR_WIDTH + 1;
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString("MONKEY WIFI", 6, y0 + 8, 2);
    tft.drawFastHLine(0, y0 + 16, tft.width(), WD_EDGE);
    tft.drawFastHLine(0, y0 + 16, 46, WD_CYAN);
    tft.setTextDatum(TL_DATUM);
    const int16_t ky = 300, kh = 30, kw = 56;
    drawKey(32,  ky, kw, kh, "START", running ? WD_SURFACE : WD_CYAN,
            running ? WD_DIM : WD_CYAN, running ? WD_DIM : TFT_BLACK, 1);
    drawKey(92,  ky, kw, kh, "STOP",  running ? WD_RED : WD_SURFACE,
            running ? WD_AMBER : WD_DIM, running ? TFT_BLACK : WD_DIM, 1);
    drawKey(152, ky, kw, kh, "SET",  WD_SURFACE, WD_CYAN, WD_CYAN, 1);
    drawKey(212, ky, kw, kh, "BACK", WD_SURFACE, WD_CYAN, WD_CYAN, 1);
  };
  auto stat = [&]() {
    tft.fillRect(0, 46, tft.width(), 232, TFT_BLACK);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(WD_DIM, TFT_BLACK);   tft.drawString("SSID", 20, 84, 2);
    tft.setTextColor(WD_WHITE, TFT_BLACK); tft.drawString(ssid, 120, 84, 2);
    tft.setTextColor(WD_DIM, TFT_BLACK);   tft.drawString("STATUS", 20, 124, 2);
    tft.setTextColor(running ? WD_CYAN : WD_AMBER, TFT_BLACK);
    tft.drawString(running ? "ACTIVE" : "OFF", 120, 124, 2);
    tft.setTextColor(WD_DIM, TFT_BLACK);   tft.drawString("CONNECTED", 20, 168, 2);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString(String(running ? WiFi.softAPgetStationNum() : 0), 150, 160, 6);
    tft.setTextColor(WD_GREY, TFT_BLACK);
    tft.drawString(String("IMAGE: ") + monkeyImageSource(), 20, 226, 1);
    tft.setTextDatum(TL_DATUM);
  };

  chrome(); stat();
  uint32_t last = 0;
  while (true) {
    uint16_t tx, ty;
    if (display_obj.updateTouch(&tx, &ty)) {
      while (display_obj.updateTouch(&tx, &ty)) delay(10);
      if (ty >= 282) {
        if (tx < 62) {                       // START
          if (!running) {
            g_prank_page = MONKEY_HTML;
            apUp(ssid);
            ensurePortalRoutes();
            prank_dns.setErrorReplyCode(DNSReplyCode::NoError);
            prank_dns.start(53, "*", PRANK_AP_IP);
            prank_dns_on = true;
            running = true; chrome(); stat();
          }
        } else if (tx < 122) {               // STOP
          if (running) { apDown(); running = false; chrome(); stat(); }
        } else if (tx < 182) {               // SETTINGS (image / restore default)
          bool leave = false;
          while (!leave) {
            tft.fillScreen(TFT_BLACK);
            menu_function_obj.drawStatusBar();
            tft.setTextDatum(ML_DATUM);
            tft.setTextColor(WD_CYAN, TFT_BLACK);
            tft.drawString("MONKEY SETTINGS", 6, STATUS_BAR_WIDTH + 9, 2);
            tft.setTextDatum(MC_DATUM);
            tft.setTextColor(WD_GREY, TFT_BLACK);
            tft.drawString("ACTIVE IMAGE", tft.width() / 2, 90, 2);
            tft.setTextColor(WD_CYAN, TFT_BLACK);
            tft.drawString(monkeyImageSource(), tft.width() / 2, 130, 4);
            tft.setTextColor(WD_DIM, TFT_BLACK);
            tft.drawString("upload/manage via Web Control", tft.width() / 2, 175, 1);
            drawKey(70, 300, 120, 30, "RESTORE", WD_SURFACE, WD_AMBER, WD_AMBER, 1);
            drawKey(190, 300, 90, 30, "BACK", WD_SURFACE, WD_CYAN, WD_CYAN, 1);
            tft.setTextDatum(TL_DATUM);
            while (true) {
              if (running) prank_dns.processNextRequest();
              uint16_t sx, sy;
              if (display_obj.updateTouch(&sx, &sy)) {
                while (display_obj.updateTouch(&sx, &sy)) delay(10);
                if (sy >= 282) {
                  if (sx < 135) { monkeyRestoreDefault(); }   // RESTORE, redraw
                  else leave = true;
                }
                break;
              }
              delay(15);
            }
          }
          chrome(); stat();
        } else {                             // BACK
          apDown(); return;
        }
      }
    }
    if (running) {
      prank_dns.processNextRequest();
      if (millis() - last > 500) { last = millis(); stat(); }
    }
    delay(15);
  }
}

// --- Bluetooth (BLE) pranks --------------------------------------------------
#if defined(HAS_BT)
namespace {
  bool ble_up = false;

  void bleStop() {
    if (!ble_up) return;
    // Soft stop: a full NimBLEDevice::deinit asserts inside the C5's ROM
    // controller teardown when events are queued, so the stack stays
    // resident (see WiFiScan::bleSoftStop for the full story).
    wifi_scan_obj.bleSoftStop();
    wifi_scan_obj.ble_initialized = false;
    ble_up = false;
  }

  // Bring BLE up and advertise the given name (fresh init each time so the
  // advertised device name changes cleanly). Routes through the WiFiScan
  // crash guard: if BLE start-up has ever aborted the device, the guard
  // refuses here instead of crashing, and the caller shows a status line.
  bool bleAdvertiseName(const char* name) {
    bleStop();
    if (!wifi_scan_obj.ensureBLE(name)) return false;
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    NimBLEAdvertisementData d;
    d.setFlags(0x06);
    d.setName(std::string(name));
    adv->setAdvertisementData(d);
    adv->start();
    ble_up = true;
    return true;
  }
}

void SharkPrank::bleNameBroadcast() {
  const char* name = "Definitely Not a Bug";
  bool running = false;
  frame("BLE NAME BROADCAST", running);
  status("READY", name, "tap START to advertise", WD_DIM);
  while (true) {
    int c = control();
    if (c == 0) { bleStop(); return; }
    if (c == 1 && !running) {
      if (!bleAdvertiseName(name)) {
        status("BT LOCKED", "crash guard active", "", WD_AMBER);
      } else {
        running = true;
        frame("BLE NAME BROADCAST", running);
        status("ON AIR", name, "visible to BLE scanners", WD_CYAN);
      }
    }
    if (c == 2 && running)  { bleStop(); running = false;
                              frame("BLE NAME BROADCAST", running);
                              status("STOPPED", name, "", WD_AMBER); }
    delay(20);
  }
}

void SharkPrank::bleNameRotator() {
  static const char* names[6] = {
    "iPhone (17)", "Samsung Fridge", "Your Mom's AirPods",
    "FBI_Bug_02", "Loading Device...", "Press F to Pay"
  };
  bool running = false;
  uint8_t idx = 0;
  frame("BLE NAME ROTATOR", running);
  status("READY", names[0], "tap START to rotate", WD_DIM);
  uint32_t last = 0;
  while (true) {
    int c = control();
    if (c == 0) { bleStop(); return; }
    if (c == 1 && !running) {
      if (!bleAdvertiseName(names[idx])) {
        status("BT LOCKED", "crash guard active", "", WD_AMBER);
      } else {
        running = true;
        frame("BLE NAME ROTATOR", running); last = millis();
        status("ON AIR", names[idx], "rotating...", WD_CYAN);
      }
    }
    if (c == 2 && running)  { bleStop(); running = false;
                              frame("BLE NAME ROTATOR", running);
                              status("STOPPED", names[idx], "", WD_AMBER); }
    if (running && millis() - last > 4000) {
      last = millis(); idx = (idx + 1) % 6;
      if (!bleAdvertiseName(names[idx])) { bleStop(); running = false; }
      else status("ON AIR", names[idx], "rotating...", WD_CYAN);
    }
    delay(20);
  }
}

void SharkPrank::bleAdvertiser() {
  bool running = false;
  frame("BLE ADVERTISER", running);
  status("READY", "benign mfr data", "tap START to broadcast", WD_DIM);
  while (true) {
    int c = control();
    if (c == 0) { bleStop(); return; }
    if (c == 1 && !running) {
      bleStop();
      if (!wifi_scan_obj.ensureBLE("SHARK-ADV")) {
        status("BT LOCKED", "crash guard active", "", WD_AMBER);
      } else {
        NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
        NimBLEAdvertisementData d;
        d.setFlags(0x06);
        d.setName("SHARK-ADV");
        const uint8_t mfr[7] = {0xFF, 0xFF, 'S', 'H', 'A', 'R', 'K'};  // 0xFFFF = test id
        d.setManufacturerData(mfr, sizeof(mfr));
        adv->setAdvertisementData(d);
        adv->start();
        ble_up = true;
        running = true; frame("BLE ADVERTISER", running);
        status("ON AIR", "mfr: FFFF SHARK", "custom advert data", WD_CYAN);
      }
    }
    if (c == 2 && running) { bleStop(); running = false;
                             frame("BLE ADVERTISER", running);
                             status("STOPPED", "", "", WD_AMBER); }
    delay(20);
  }
}

void SharkPrank::bleBeacon() {
  bool running = false;
  frame("BLE BEACON", running);
  status("READY", "iBeacon format", "tap START to broadcast", WD_DIM);
  while (true) {
    int c = control();
    if (c == 0) { bleStop(); return; }
    if (c == 1 && !running) {
      bleStop();
      if (!wifi_scan_obj.ensureBLE("SHARK-BEACON")) {
        status("BT LOCKED", "crash guard active", "", WD_AMBER);
      } else {
        NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
        // iBeacon: Apple company 0x004C, type 0x02 len 0x15, 16B UUID, major, minor, tx.
        uint8_t ib[25] = {0x4C, 0x00, 0x02, 0x15,
                          0x53, 0x48, 0x41, 0x52, 0x4B, 0x53, 0x48, 0x41,
                          0x52, 0x4B, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                          0x00, 0x01, 0x00, 0x2A, 0xC5};
        NimBLEAdvertisementData d;
        d.setFlags(0x06);
        d.setManufacturerData(ib, sizeof(ib));
        adv->setAdvertisementData(d);
        adv->start();
        ble_up = true;
        running = true; frame("BLE BEACON", running);
        status("ON AIR", "iBeacon 1/42", "major 1 minor 42", WD_CYAN);
      }
    }
    if (c == 2 && running) { bleStop(); running = false;
                             frame("BLE BEACON", running);
                             status("STOPPED", "", "", WD_AMBER); }
    delay(20);
  }
}

void SharkPrank::bleRadar() {
  TFT_eSPI& tft = display_obj.tft;
  bool running = false;
  uint32_t last_draw = 0, last_restart = 0;
  frame("BLE RADAR", running);
  status("READY", "scan nearby BLE", "tap START to scan", WD_DIM);
  while (true) {
    int c = control();
    if (c == 0) { bleStop(); return; }
    if (c == 1 && !running) {
      bleStop();
      if (!wifi_scan_obj.ensureBLE("")) {
        status("BT LOCKED", "crash guard active", "", WD_AMBER);
      } else {
        NimBLEScan* scan = NimBLEDevice::getScan();
        scan->setActiveScan(true);
        // Both params must be set: the constructor default window is 0 and a
        // zero radio window scans nothing. Same proven pair as the BT tools.
        scan->setInterval(50);
        scan->setWindow(30);
        scan->clearResults();
        // Continuous, NON-blocking scan: the loop below keeps polling touch,
        // so STOP/BACK react the instant they are tapped.
        scan->start(0, false, false);
        ble_up = true; last_restart = millis();
        running = true; frame("BLE RADAR", running);
      }
    }
    if (c == 2 && running) { bleStop(); running = false; frame("BLE RADAR", running);
                             status("STOPPED", "", "", WD_AMBER); }
    if (running) {
      NimBLEScan* scan = NimBLEDevice::getScan();
      const uint32_t now = millis();
      // Refresh the scan every 8 s so the list and RSSIs stay current.
      if (now - last_restart > 8000) {
        last_restart = now;
        scan->stop();
        scan->clearResults();
        scan->start(0, false, false);
      }
      if (now - last_draw < 400) { delay(20); continue; }
      last_draw = now;
      NimBLEScanResults res = scan->getResults();
      tft.fillRect(0, 46, tft.width(), 234, TFT_BLACK);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(WD_GREY, TFT_BLACK);
      tft.drawString(String("DEVICES: ") + res.getCount(), 8, 52, 2);
      int shown = res.getCount() < 9 ? res.getCount() : 9;
      for (int i = 0; i < shown; i++) {
        const NimBLEAdvertisedDevice* dev = res.getDevice(i);
        int y = 76 + i * 22;
        int rssi = dev->getRSSI();
        String nm = dev->getName().length() ? String(dev->getName().c_str())
                                            : String(dev->getAddress().toString().c_str());
        while (nm.length() > 3 && tft.textWidth(nm, 1) > 150) nm.remove(nm.length() - 1);
        tft.setTextColor(rssi > -60 ? WD_CYAN : rssi > -80 ? WD_BONE : WD_DIM, TFT_BLACK);
        tft.drawString(nm, 8, y, 1);
        tft.setTextDatum(TR_DATUM);
        tft.drawString(String(rssi) + "dBm", tft.width() - 8, y, 1);
        tft.setTextDatum(TL_DATUM);
      }
    }
    delay(20);
  }
}

void SharkPrank::bleHunt() {
  TFT_eSPI& tft = display_obj.tft;
  bool running = false;
  NimBLEAddress target;
  String target_name;
  bool have_target = false;
  uint32_t last_draw = 0, last_restart = 0;
  frame("BLE HUNT", running);
  status("READY", "locks strongest device", "tap START to hunt", WD_DIM);
  while (true) {
    int c = control();
    if (c == 0) { bleStop(); return; }
    if (c == 1 && !running) {
      bleStop();
      if (!wifi_scan_obj.ensureBLE("")) {
        status("BT LOCKED", "crash guard active", "", WD_AMBER);
      } else {
        // interval + window pair; window alone 0 would scan nothing.
        NimBLEDevice::getScan()->setActiveScan(true);
        NimBLEDevice::getScan()->setInterval(50);
        NimBLEDevice::getScan()->setWindow(30);
        NimBLEDevice::getScan()->clearResults();
        // Continuous, NON-blocking scan keeps the buttons responsive.
        NimBLEDevice::getScan()->start(0, false, false);
        ble_up = true; have_target = false; last_restart = millis();
        running = true; frame("BLE HUNT", running);
        status("SCANNING", "locking target...", "", WD_AMBER);
      }
    }
    if (c == 2 && running) { bleStop(); running = false; have_target = false;
                             frame("BLE HUNT", running); status("STOPPED", "", "", WD_AMBER); }
    if (running) {
      NimBLEScan* scan = NimBLEDevice::getScan();
      const uint32_t now = millis();
      if (now - last_restart > 8000) {
        last_restart = now;
        scan->stop();
        scan->clearResults();
        scan->start(0, false, false);
      }
      if (now - last_draw < 400) { delay(20); continue; }
      last_draw = now;
      NimBLEScanResults res = scan->getResults();
      int best_rssi = -127;
      if (!have_target) {
        // Lock onto the strongest advertiser.
        for (int i = 0; i < res.getCount(); i++) {
          const NimBLEAdvertisedDevice* d = res.getDevice(i);
          if (d->getRSSI() > best_rssi) {
            best_rssi = d->getRSSI();
            target = d->getAddress();
            target_name = d->getName().length() ? String(d->getName().c_str())
                                                 : String(d->getAddress().toString().c_str());
          }
        }
        if (best_rssi > -127) have_target = true;
      } else {
        // Track the locked target's RSSI.
        best_rssi = -127;
        for (int i = 0; i < res.getCount(); i++) {
          const NimBLEAdvertisedDevice* d = res.getDevice(i);
          if (d->getAddress() == target) best_rssi = d->getRSSI();
        }
      }

      const char* state; uint16_t col;
      if (!have_target || best_rssi <= -127) { state = "SEARCHING"; col = WD_DIM; }
      else if (best_rssi > -45)              { state = "FOUND";     col = WD_CYAN; }
      else if (best_rssi > -60)              { state = "HOT";       col = WD_RED; }
      else if (best_rssi > -75)              { state = "WARM";      col = WD_AMBER; }
      else                                    { state = "COLD";      col = WD_GREY; }

      tft.fillRect(0, 46, tft.width(), 234, TFT_BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(WD_GREY, TFT_BLACK);
      String tn = target_name;
      while (tn.length() > 3 && tft.textWidth(tn, 2) > tft.width() - 20) tn.remove(tn.length() - 1);
      tft.drawString(have_target ? tn : "no target yet", tft.width() / 2, 80, 2);
      tft.setTextColor(col, TFT_BLACK);
      tft.drawString(state, tft.width() / 2, 140, 6);
      if (have_target && best_rssi > -127) {
        tft.setTextColor(WD_BONE, TFT_BLACK);
        tft.drawString(String(best_rssi) + " dBm", tft.width() / 2, 200, 4);
      }
      tft.setTextDatum(TL_DATUM);
    }
    delay(20);
  }
}

// Shark Hunt: a two-device chase. One SHARK runs BEACON (advertises the name
// "SHARK-HUNT"); the other runs SEEKER, which locks that name and turns its RSSI
// into a COLD/WARM/HOT/FOUND proximity read plus a HOTTER/COLDER trend.
void SharkPrank::sharkHuntBeacon() {
  const char* name = "SHARK-HUNT";
  bool running = false;
  frame("SHARK HUNT: BEACON", running);
  status("READY", "you are the target", "tap START to broadcast", WD_DIM);
  while (true) {
    int c = control();
    if (c == 0) { bleStop(); return; }
    if (c == 1 && !running) {
      if (!bleAdvertiseName(name)) {
        status("BT LOCKED", "crash guard active", "", WD_AMBER);
      } else {
        running = true;
        frame("SHARK HUNT: BEACON", running);
        status("ON AIR", "name: SHARK-HUNT", "let them find you", WD_CYAN);
      }
    }
    if (c == 2 && running)  { bleStop(); running = false;
                              frame("SHARK HUNT: BEACON", running);
                              status("STOPPED", "", "", WD_AMBER); }
    delay(20);
  }
}

void SharkPrank::sharkHuntSeeker() {
  TFT_eSPI& tft = display_obj.tft;
  const String target_name = "SHARK-HUNT";
  bool running = false;
  int last_rssi = -127;
  uint32_t last_draw = 0, last_restart = 0;
  frame("SHARK HUNT: SEEKER", running);
  status("READY", "find the SHARK beacon", "tap START to hunt", WD_DIM);
  while (true) {
    int c = control();
    if (c == 0) { bleStop(); return; }
    if (c == 1 && !running) {
      bleStop();
      if (!wifi_scan_obj.ensureBLE("")) {
        status("BT LOCKED", "crash guard active", "", WD_AMBER);
      } else {
        // interval + window pair; window alone 0 would scan nothing.
        NimBLEDevice::getScan()->setActiveScan(true);
        NimBLEDevice::getScan()->setInterval(50);
        NimBLEDevice::getScan()->setWindow(30);
        NimBLEDevice::getScan()->clearResults();
        // Continuous, NON-blocking scan keeps the buttons responsive.
        NimBLEDevice::getScan()->start(0, false, false);
        ble_up = true; running = true; last_rssi = -127; last_restart = millis();
        frame("SHARK HUNT: SEEKER", running);
        status("HUNTING", "listening...", "", WD_AMBER);
      }
    }
    if (c == 2 && running) { bleStop(); running = false;
                             frame("SHARK HUNT: SEEKER", running);
                             status("STOPPED", "", "", WD_AMBER); }
    if (running) {
      NimBLEScan* scan = NimBLEDevice::getScan();
      const uint32_t now = millis();
      if (now - last_restart > 8000) {
        last_restart = now;
        scan->stop();
        scan->clearResults();
        scan->start(0, false, false);
      }
      if (now - last_draw < 400) { delay(20); continue; }
      last_draw = now;
      NimBLEScanResults res = scan->getResults();
      int rssi = -127;
      for (int i = 0; i < res.getCount(); i++) {
        const NimBLEAdvertisedDevice* d = res.getDevice(i);
        if (d->getName().length() && target_name == String(d->getName().c_str())) {
          if (d->getRSSI() > rssi) rssi = d->getRSSI();
        }
      }

      const char* state; uint16_t col;
      if (rssi <= -127)    { state = "NO SIGNAL"; col = WD_DIM; }
      else if (rssi > -45) { state = "FOUND";     col = WD_CYAN; }
      else if (rssi > -60) { state = "HOT";       col = WD_RED; }
      else if (rssi > -75) { state = "WARM";      col = WD_AMBER; }
      else                 { state = "COLD";      col = WD_GREY; }

      const char* trend = "";
      if (rssi > -127 && last_rssi > -127) {
        if (rssi >= last_rssi + 3)      trend = "HOTTER >>";
        else if (rssi <= last_rssi - 3) trend = "COLDER <<";
        else                            trend = "steady";
      }
      if (rssi > -127) last_rssi = rssi;

      tft.fillRect(0, 46, tft.width(), 234, TFT_BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(WD_GREY, TFT_BLACK);
      tft.drawString("target: SHARK-HUNT", tft.width() / 2, 74, 2);
      tft.setTextColor(col, TFT_BLACK);
      tft.drawString(state, tft.width() / 2, 132, 6);
      if (rssi > -127) {
        tft.setTextColor(WD_BONE, TFT_BLACK);
        tft.drawString(String(rssi) + " dBm", tft.width() / 2, 188, 4);
        tft.setTextColor(col, TFT_BLACK);
        tft.drawString(trend, tft.width() / 2, 224, 4);
      } else {
        tft.setTextColor(WD_DIM, TFT_BLACK);
        tft.drawString("run BEACON on the other SHARK", tft.width() / 2, 200, 2);
      }
      tft.setTextDatum(TL_DATUM);
    }
    delay(20);
  }
}
#endif  // HAS_BT

#endif
