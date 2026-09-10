#include "SharkWeb.h"

#if defined(HAS_SCREEN) && defined(MARAUDER_V8)

#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "ESPAsyncWebServer.h"
#include "SharkTheme.h"
#include "SharkWebPage.h"
#include "SharkUI.h"
#include "SharkPrank.h"
#include "SharkQR.h"
#ifdef HAS_SD
  #include <SD.h>
#endif
#include "Display.h"
#include "WiFiScan.h"
#include "SDInterface.h"
#include "BatteryInterface.h"
#include "MenuFunctions.h"

extern Display display_obj;
extern WiFiScan wifi_scan_obj;
extern SDInterface sd_obj;
extern BatteryInterface battery_obj;
extern MenuFunctions menu_function_obj;
extern void brightnessSave(uint8_t level);
extern uint8_t getBrightnessLevel();

namespace {
  AsyncWebServer shark_server(80);
  const char SHARK_AP_SSID[] = "M5-SHARK-CTRL";
  const char SHARK_AP_PASS[] = "sharkc0ntrol";
  const IPAddress SHARK_AP_IP(172, 16, 0, 1);
  SemaphoreHandle_t screen_mtx = nullptr;
  DNSServer dns_server;   // captive-portal DNS: resolves any host to the AP

  const char* const THEME_KEYS[SHARK_THEME_COUNT] = {
    "watch", "matrix", "cyber", "spider"
  };

  const SharkWebTool SHARK_WEB_TOOLS[] = {
    {"Wi-Fi Spectrum", "wifi", WIFI_SCAN_CHAN_ANALYZER, 0x07FF},
    {"Channel Summary", "wifi", WIFI_SCAN_CHAN_ACT, 0x07FF},
    {"Packet Rate", "wifi", WIFI_SCAN_PACKET_RATE, 0xFD20},
    {"Deauth Guard", "defense", WIFI_SCAN_DEAUTH, 0xF800},
    {"Handshake Watch", "defense", WIFI_SCAN_EAPOL, 0x881F},
    {"PCAP Logger", "defense", WIFI_SCAN_RAW_CAPTURE, 0xFFFF},
    {"Drone RID Detector", "defense", SHARK_DRONE_RID_SCAN, 0x07E0},
    #ifdef HAS_BT
      {"Tracker Detect", "ble", BT_SCAN_AIRTAG_MON, 0xFFFF},
      {"Card Skimmer", "ble", BT_SCAN_SKIMMERS, 0xF81F},
    #endif
    {"WiFi Radar", "wifi", SHARK_WEB_APP_WIFI_RADAR, 0x07E0},
    {"RuView CSI", "defense", SHARK_RUVIEW_CSI, 0x07E0},
    #ifdef HAS_GPS
      {"GPS Sky View", "gps", SHARK_WEB_APP_GPS_RADAR, 0x07E0},
    #endif
    // Pranks (launch the on-device prank screen; the AP drops while it runs).
    {"Monkey WiFi", "prank", SHARK_WEB_APP_MONKEY, 0xF81F},
    {"Funny Hotspot", "prank", SHARK_WEB_APP_FUNNY_HOTSPOT, 0x07E0},
    {"SSID Rotator", "prank", SHARK_WEB_APP_SSID_ROTATOR, 0x07FF},
    {"Guest Counter", "prank", SHARK_WEB_APP_GUEST_COUNTER, 0xFD20},
    {"Prank Portal", "prank", SHARK_WEB_APP_PRANK_PORTAL, 0xF81F},
    {"Meme Portal", "prank", SHARK_WEB_APP_MEME_PORTAL, 0xF81F},
    {"Custom Portal", "prank", SHARK_WEB_APP_CUSTOM_PORTAL, 0x07FF},
    #ifdef HAS_BT
      {"BLE Name Broadcast", "prank", SHARK_WEB_APP_BLE_NAME, 0x07FF},
      {"BLE Name Rotator", "prank", SHARK_WEB_APP_BLE_ROTATOR, 0x07FF},
      {"BLE Advertiser", "prank", SHARK_WEB_APP_BLE_ADVERT, 0x07E0},
      {"BLE Beacon", "prank", SHARK_WEB_APP_BLE_BEACON, 0xFD20},
      {"BLE Radar", "prank", SHARK_WEB_APP_BLE_RADAR, 0x07E0},
      {"BLE Hunt", "prank", SHARK_WEB_APP_BLE_HUNT, 0xF81F},
    #endif
  };
  const uint8_t SHARK_WEB_TOOL_COUNT = sizeof(SHARK_WEB_TOOLS) / sizeof(SHARK_WEB_TOOLS[0]);

  // Same order as SHARK_WEB_TOOLS so modeForApp() indexing matches.
  const char* const APP_KEYS[] = {
    "wifi-spectrum", "channel-summary", "packet-rate", "deauth-guard",
    "handshake-watch", "pcap-logger", "drone-rid",
    #ifdef HAS_BT
      "tracker-detect", "card-skimmer",
    #endif
    "wifi-radar", "ruview-csi",
    #ifdef HAS_GPS
      "gps-radar",
    #endif
    "monkey-wifi", "funny-hotspot", "ssid-rotator", "guest-counter",
    "prank-portal", "meme-portal", "custom-portal",
    #ifdef HAS_BT
      "ble-name", "ble-rotator", "ble-advertiser", "ble-beacon",
      "ble-radar", "ble-hunt",
    #endif
  };

  bool upload_ok = false;
  String upload_path;
  #ifdef HAS_SD
    File wallpaper_file;
    bool wallpaper_ok = false;
    File monkey_file;         // admin Monkey-image upload target
    bool monkey_ok = false;
  #endif

  String hexColor(uint16_t c) {
    uint8_t r = ((c >> 11) & 0x1F) * 255 / 31;
    uint8_t g = ((c >> 5) & 0x3F) * 255 / 63;
    uint8_t b = (c & 0x1F) * 255 / 31;
    char buf[8];
    snprintf(buf, sizeof(buf), "#%02x%02x%02x", r, g, b);
    return String(buf);
  }

  String jsonEscape(const String& s) {
    String out;
    out.reserve(s.length() + 8);
    for (size_t i = 0; i < s.length(); i++) {
      char c = s.charAt(i);
      if (c == '"' || c == '\\') out += '\\';
      if (c == '\n') { out += "\\n"; continue; }
      if (c == '\r') continue;
      out += c;
    }
    return out;
  }

  String formValue(AsyncWebServerRequest* req, const char* name) {
    if (req->hasParam(name, true)) return req->getParam(name, true)->value();
    if (req->hasParam(name)) return req->getParam(name)->value();
    return String();
  }

  bool controlAllowed(AsyncWebServerRequest* req) {
    return req->hasHeader("X-Shark-Control") && req->getHeader("X-Shark-Control")->value() == "1";
  }

  bool rejectControl(AsyncWebServerRequest* req) {
    if (controlAllowed(req)) return false;
    req->send(403, "application/json", "{\"ok\":false,\"error\":\"control header required\"}");
    return true;
  }

  bool allowedChannel(int channel) {
    const uint8_t channels[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,32,36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96,100,104,108,112,116,120,124,128,132,136,140,144,149,153,157,161,165,169,173,177};
    for (uint8_t c : channels) if (channel == c) return true;
    return false;
  }

  int modeForApp(const String& app) {
    for (uint8_t i = 0; i < SHARK_WEB_TOOL_COUNT; i++) if (app == APP_KEYS[i]) return SHARK_WEB_TOOLS[i].mode;
    return -1;
  }

  String safePath(String path) {
    path.replace("\\", "/");
    if (!path.startsWith("/")) path = "/" + path;
    if (path.length() < 2 || path.length() > 96 || path.indexOf("..") >= 0 || path.indexOf("//") >= 0) return String();
    int next_slash = path.indexOf('/', 1);
    if (next_slash >= 0 && !path.startsWith("/SCRIPTS/")) return String();
    if (path.startsWith("/SCRIPTS/") && path.indexOf('/', 9) >= 0) return String();
    return path;
  }

  bool protectedPath(String path) {
    path.toLowerCase();
    return path == "/update.bin" || path == "/firmware.bin";
  }

  String cleanFilename(String name) {
    name.replace("\\", "/");
    int slash = name.lastIndexOf('/');
    if (slash >= 0) name = name.substring(slash + 1);
    name.trim();
    if (!name.length() || name.length() > 64 || name.indexOf("..") >= 0) return String();
    for (size_t i = 0; i < name.length(); i++) {
      char c = name.charAt(i);
      if (!(isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == ' ')) return String();
    }
    return name;
  }
}

SharkWeb shark_web_obj;

String SharkWeb::themeListJson() {
  String out = "[";
  for (uint8_t i = 0; i < SHARK_THEME_COUNT; i++) {
    if (i) out += ',';
    out += "{\"i\":" + String(i) + ",\"key\":\"" + THEME_KEYS[i] + "\"" +
           ",\"name\":\"" + jsonEscape(shark_themes[i].name) + "\"" +
           ",\"tag\":\"" + jsonEscape(shark_themes[i].tag) + "\"" +
           ",\"accent\":\"" + hexColor(shark_themes[i].accent) + "\"}";
  }
  return out + "]";
}

String SharkWeb::toolListJson() {
  String out = "[";
  for (uint8_t i = 0; i < SHARK_WEB_TOOL_COUNT; i++) {
    if (i) out += ',';
    out += "{\"mode\":" + String(SHARK_WEB_TOOLS[i].mode) + ",\"cat\":\"" + SHARK_WEB_TOOLS[i].cat + "\"" +
           ",\"c\":\"" + hexColor(SHARK_WEB_TOOLS[i].color) + "\",\"name\":\"" +
           jsonEscape(SHARK_WEB_TOOLS[i].name) + "\"}";
  }
  return out + "]";
}

String SharkWeb::statusJson() {
  #ifdef HAS_SD
    const bool sd_ready = sd_obj.supported;
  #else
    const bool sd_ready = false;
  #endif
  #ifdef HAS_BATTERY
    const int battery = battery_obj.battery_level;
  #else
    const int battery = -1;
  #endif
  String out = "{";
  out += "\"name\":\"" SHARK_UI_NAME "\"";
  out += ",\"version\":\"" + jsonEscape(display_obj.version_number) + "\"";
  out += ",\"theme\":" + String(shark_theme_index) + ",\"themeName\":\"" + jsonEscape(shark_theme->name) + "\"";
  out += ",\"themeKey\":\"" + String(THEME_KEYS[shark_theme_index]) + "\",\"tag\":\"" + jsonEscape(shark_theme->tag) + "\"";
  out += ",\"accent\":\"" + hexColor(shark_theme->accent) + "\",\"channel\":" + String(wifi_scan_obj.set_channel);
  out += ",\"brightness\":" + String(getBrightnessLevel()) + ",\"sd\":" + String(sd_ready ? "true" : "false");
  out += ",\"wifi\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",\"battery\":" + String(battery);
  out += ",\"clients\":" + String(WiFi.softAPgetStationNum()) + ",\"uptime\":" + String(millis() / 1000);
  out += ",\"freeHeap\":" + String(ESP.getFreeHeap()) + ",\"usbHid\":false,\"bleHid\":true";
  String sc;
  if (screen_mtx && xSemaphoreTake(screen_mtx, pdMS_TO_TICKS(20)) == pdTRUE) {
    sc = screen_cache;
    xSemaphoreGive(screen_mtx);
  } else sc = screen_cache;
  out += "," + sc + "}";
  return out;
}

String SharkWeb::wifiScanJson() {
  // Serve the cached list immediately and let the main task refresh it (see
  // capture()). Never scan synchronously here: that blocks the async HTTP
  // task for seconds and drags the SoftAP across channels, dropping the
  // phone that is looking at the panel. The cache is written on the main
  // task under the same mutex the status snapshot uses. "scanning" tells the
  // page a fresh sweep is in flight so it keeps polling for the update.
  String networks;
  if (screen_mtx && xSemaphoreTake(screen_mtx, pdMS_TO_TICKS(20)) == pdTRUE) {
    networks = scan_cache;
    xSemaphoreGive(screen_mtx);
  } else {
    networks = scan_cache;
  }
  return String("{\"networks\":") + networks +
         ",\"scanning\":" + (scan_await ? "true" : "false") + "}";
}

String SharkWeb::cardJson() {
  #ifdef HAS_SD
    if (!sd_obj.supported) return "{\"ready\":false,\"total\":0,\"used\":0,\"files\":[]}";
    String out = "{\"ready\":true,\"total\":" + String((uint64_t)SD.totalBytes()) +
                 ",\"used\":" + String((uint64_t)SD.usedBytes()) + ",\"files\":[";
    bool first = true;
    uint8_t count = 0;
    const char* dirs[] = {"/", "/SCRIPTS"};
    for (uint8_t d = 0; d < 2 && count < 64; d++) {
      File dir = SD.open(dirs[d]);
      if (!dir || !dir.isDirectory()) { if (dir) dir.close(); continue; }
      while (count < 64) {
        File entry = dir.openNextFile();
        if (!entry) break;
        if (!entry.isDirectory()) {
          String name = entry.name();
          int slash = name.lastIndexOf('/');
          String base = slash >= 0 ? name.substring(slash + 1) : name;
          if (!name.startsWith("/")) name = d == 1 ? "/SCRIPTS/" + base : "/" + base;
          else if (d == 1 && !name.startsWith("/SCRIPTS/")) name = "/SCRIPTS/" + base;
          if (!first) out += ',';
          first = false;
          out += "{\"path\":\"" + jsonEscape(name) + "\",\"name\":\"" + jsonEscape(base) +
                 "\",\"size\":" + String((uint32_t)entry.size()) + ",\"protected\":" +
                 String(protectedPath(name) ? "true" : "false") + ",\"script\":" + String(d == 1 ? "true" : "false") + "}";
          count++;
        }
        entry.close();
      }
      dir.close();
    }
    return out + "]}";
  #else
    return "{\"ready\":false,\"total\":0,\"used\":0,\"files\":[]}";
  #endif
}

void SharkWeb::registerRoutes() {
  if (routes_ready) return;
  shark_server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    AsyncWebServerResponse* response = req->beginResponse_P(
      200, "text/html; charset=utf-8", SHARK_WEB_PAGE_GZ, SHARK_WEB_PAGE_GZ_LEN);
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "no-store");
    req->send(response);
  });
  shark_server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) { req->send(200, "application/json", statusJson()); });
  // IQ Family Watch: family devices announce themselves here right after
  // joining this AP (SharkDeck OS ships a NetworkManager dispatcher that
  // calls it). The station's live RSSI is read from the AP association
  // list, so proximity is measured, not assumed.
  shark_server.on("/shark/family", HTTP_GET, [this](AsyncWebServerRequest* req) {
    String id = req->hasParam("id") ? req->getParam("id")->value() : String("FAMILY");
    if (id.length() == 0 || id.length() > 16) id = "FAMILY";
    int8_t best = -128;
    wifi_sta_list_t sta;
    if (esp_wifi_ap_get_sta_list(&sta) == ESP_OK) {
      for (int i = 0; i < sta.num; i++)
        if (sta.sta[i].rssi > best) best = sta.sta[i].rssi;
    }
    wifi_scan_obj.familyBeacon(id.c_str(), best);
    req->send(200, "text/plain", "M5SHARK");
  });
  shark_server.on("/api/themes", HTTP_GET, [this](AsyncWebServerRequest* req) { req->send(200, "application/json", themeListJson()); });
  shark_server.on("/api/tools", HTTP_GET, [this](AsyncWebServerRequest* req) { req->send(200, "application/json", toolListJson()); });
  shark_server.on("/api/card", HTTP_GET, [this](AsyncWebServerRequest* req) { req->send(200, "application/json", cardJson()); });
  shark_server.on("/api/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest* req) {
    scan_request = true;   // main task refreshes the cache; answer is instant
    req->send(200, "application/json", wifiScanJson());
  });

  shark_server.on("/api/theme", HTTP_POST, [this](AsyncWebServerRequest* req) {
    if (rejectControl(req)) return;
    int i = formValue(req, "i").toInt();
    if (i < 0 || i >= SHARK_THEME_COUNT) { req->send(400, "application/json", "{\"ok\":false}"); return; }
    sharkThemeSet((uint8_t)i);
    if (active && !background) drawCard();
    req->send(200, "application/json", statusJson());
  });
  shark_server.on("/api/brightness", HTTP_POST, [this](AsyncWebServerRequest* req) {
    if (rejectControl(req)) return;
    int level = formValue(req, "level").toInt();
    if (level < 0 || level > 9) { req->send(400, "application/json", "{\"ok\":false}"); return; }
    brightnessSave((uint8_t)level);
    req->send(200, "application/json", statusJson());
  });
  shark_server.on("/api/channel", HTTP_POST, [this](AsyncWebServerRequest* req) {
    if (rejectControl(req)) return;
    int channel = formValue(req, "channel").toInt();
    if (!allowedChannel(channel)) { req->send(400, "application/json", "{\"ok\":false}"); return; }
    wifi_scan_obj.set_channel = (uint8_t)channel;
    req->send(200, "application/json", statusJson());
  });
  shark_server.on("/api/nav", HTTP_POST, [this](AsyncWebServerRequest* req) {
    if (rejectControl(req)) return;
    int action = formValue(req, "a").toInt();
    if (action >= 0 && action <= 17) pending_action = action;
    req->send(200, "application/json", "{\"ok\":true}");
  });
  shark_server.on("/api/launch", HTTP_POST, [this](AsyncWebServerRequest* req) {
    if (rejectControl(req)) return;
    int mode = modeForApp(formValue(req, "app"));
    if (mode < 0) { req->send(400, "application/json", "{\"ok\":false}"); return; }
    pending_mode = mode;
    req->send(200, "application/json", "{\"ok\":true}");
  });
  shark_server.on("/api/wifi/join", HTTP_POST, [this](AsyncWebServerRequest* req) {
    if (rejectControl(req)) return;
    String ssid = formValue(req, "ssid");
    String password = formValue(req, "password");
    if (!ssid.length() || ssid.length() > 32 || password.length() > 63) {
      req->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid credentials\"}"); return;
    }
    join_ssid = ssid;
    join_password = password;
    pending_join = true;
    req->send(202, "application/json", "{\"ok\":true}");
  });
  shark_server.on("/api/file/delete", HTTP_POST, [](AsyncWebServerRequest* req) {
    if (rejectControl(req)) return;
    String path = safePath(formValue(req, "path"));
    if (!sd_obj.supported || !path.length() || protectedPath(path)) { req->send(400, "application/json", "{\"ok\":false}"); return; }
    bool ok = SD.remove(path);
    req->send(ok ? 200 : 404, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
  });
  shark_server.on("/api/file", HTTP_GET, [](AsyncWebServerRequest* req) {
    String path = safePath(formValue(req, "path"));
    if (!sd_obj.supported || !path.length() || !SD.exists(path)) { req->send(404); return; }
    req->send(SD, path, "application/octet-stream", true);
  });
  shark_server.on("/api/script", HTTP_GET, [](AsyncWebServerRequest* req) {
    String path = safePath(formValue(req, "path"));
    if (!sd_obj.supported || !path.startsWith("/SCRIPTS/") || !SD.exists(path)) { req->send(404); return; }
    File f = SD.open(path, FILE_READ);
    String body;
    body.reserve(min((size_t)8192, f.size()));
    while (f.available() && body.length() < 8192) body += (char)f.read();
    f.close();
    req->send(200, "text/plain; charset=utf-8", body);
  });
  shark_server.on("/api/upload", HTTP_POST,
    [](AsyncWebServerRequest* req) {
      if (rejectControl(req)) return;
      req->send(upload_ok ? 200 : 400, "application/json", upload_ok ? "{\"ok\":true}" : "{\"ok\":false}");
    },
    [](AsyncWebServerRequest* req, String filename, size_t index, uint8_t* data, size_t len, bool final) {
      if (!controlAllowed(req) || !sd_obj.supported) return;
      const bool scripts = formValue(req, "target") == "scripts";
      const size_t limit = scripts ? 65536 : 2097152;
      if (index == 0) {
        upload_ok = false;
        String name = cleanFilename(filename);
        if (!name.length()) return;
        if (scripts) SD.mkdir("/SCRIPTS");
        upload_path = String(scripts ? "/SCRIPTS/" : "/") + name;
        if (protectedPath(upload_path)) { upload_path = ""; return; }
        if (SD.exists(upload_path)) SD.remove(upload_path);
        req->_tempFile = SD.open(upload_path, FILE_WRITE);
      }
      if (!req->_tempFile || index + len > limit) {
        if (req->_tempFile) req->_tempFile.close();
        if (upload_path.length()) SD.remove(upload_path);
        upload_path = "";
        return;
      }
      if (len) req->_tempFile.write(data, len);
      if (final) { req->_tempFile.close(); upload_ok = true; }
    });
  shark_server.on("/api/exit", HTTP_POST, [this](AsyncWebServerRequest* req) {
    if (rejectControl(req)) return;
    pending_exit = true;
    req->send(202, "application/json", "{\"ok\":true}");
  });
  shark_server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest* req) {
    if (rejectControl(req)) return;
    req->send(202, "application/json", "{\"ok\":true}");
    delay(160);
    ESP.restart();
  });
  #ifdef HAS_SD
    // Home wallpaper: the browser sends a 240x320 RGB565 blob (already resized
    // and compressed client-side); we stream it straight to SD.
    shark_server.on("/api/wallpaper", HTTP_POST,
      [](AsyncWebServerRequest* req) {
        req->send(wallpaper_ok ? 200 : 400, "application/json",
                  wallpaper_ok ? "{\"ok\":true}" : "{\"ok\":false}");
      },
      NULL,
      [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
        if (index == 0) {
          wallpaper_ok = false;
          if (wallpaper_file)          // close any handle left open by an abort
            wallpaper_file.close();
          if (!sd_obj.supported || total != (size_t)SHARK_WALL_BYTES)
            return;
          if (!SD.exists("/shark"))
            SD.mkdir("/shark");
          wallpaper_file = SD.open(SHARK_WALL_PATH, FILE_WRITE);
        }
        if (wallpaper_file) {
          wallpaper_file.write(data, len);
          if (index + len >= total) {
            wallpaper_file.close();
            wallpaper_ok = (index + len == total);
          }
        }
      });
    shark_server.on("/api/wallpaper/clear", HTTP_POST, [](AsyncWebServerRequest* req) {
      if (rejectControl(req)) return;
      if (sd_obj.supported && SD.exists(SHARK_WALL_PATH))
        SD.remove(SHARK_WALL_PATH);
      req->send(200, "application/json", "{\"ok\":true}");
    });

    // --- Admin Monkey WiFi image management (behind the control header) -------
    // Upload a new captive-portal image (?ext=jpg|jpeg|png|gif|webp). Streamed
    // to SD /monkey/active.<ext>; validated by magic bytes and capped in size.
    shark_server.on("/api/monkey/upload", HTTP_POST,
      [this](AsyncWebServerRequest* req) {
        if (rejectControl(req)) return;                 // 403 if not admin
        req->send(monkey_ok ? 200 : 400, "application/json",
                  monkey_ok ? "{\"ok\":true}" : "{\"ok\":false}");
      },
      NULL,
      [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
        static const size_t MONKEY_MAX = 512 * 1024;   // 512 KB cap for the C5
        if (index == 0) {
          monkey_ok = false;
          if (monkey_file) monkey_file.close();
          if (!controlAllowed(req)) return;             // admin only (no send here)
          if (!sd_obj.supported || total == 0 || total > MONKEY_MAX) return;
          // Validate the image by magic bytes.
          bool ok_type = (len >= 4) && (
            (data[0] == 0xFF && data[1] == 0xD8) ||                                  // JPEG
            (data[0] == 0x89 && data[1] == 0x50 && data[2] == 0x4E && data[3] == 0x47) || // PNG
            (data[0] == 'G' && data[1] == 'I' && data[2] == 'F') ||                  // GIF
            (data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F')); // WEBP(RIFF)
          if (!ok_type) return;
          String ext = "jpg";
          if (req->hasParam("ext")) ext = req->getParam("ext")->value();
          ext.toLowerCase();
          if (ext != "jpg" && ext != "jpeg" && ext != "png" && ext != "gif" && ext != "webp")
            ext = "jpg";
          if (!SD.exists("/monkey")) SD.mkdir("/monkey");
          const char* all[5] = {"jpg", "jpeg", "png", "gif", "webp"};
          for (int i = 0; i < 5; i++) {                 // keep only one active image
            String p = String("/monkey/active.") + all[i];
            if (SD.exists(p)) SD.remove(p);
          }
          monkey_file = SD.open(String("/monkey/active.") + ext, FILE_WRITE);
        }
        if (monkey_file) {
          monkey_file.write(data, len);
          if (index + len >= total) {
            monkey_file.close();
            monkey_ok = (index + len == total);
          }
        }
      });

    shark_server.on("/api/monkey/restore", HTTP_POST, [](AsyncWebServerRequest* req) {
      if (rejectControl(req)) return;
      SharkPrank::monkeyRestoreDefault();
      req->send(200, "application/json", "{\"ok\":true}");
    });
  #endif

  // Preview the active Monkey image, and report its source. Admin only.
  shark_server.on("/api/monkey/image", HTTP_GET, [this](AsyncWebServerRequest* req) {
    if (rejectControl(req)) return;
    SharkPrank::serveMonkeyImage(req);
  });
  shark_server.on("/api/monkey/source", HTTP_GET, [this](AsyncWebServerRequest* req) {
    if (rejectControl(req)) return;
    req->send(200, "application/json",
              String("{\"src\":\"") + SharkPrank::monkeyImageSource() + "\"}");
  });
  // Any unknown path (including OS captive-portal checks such as
  // /generate_204, /hotspot-detect.html, /connecttest.txt) serves the app, so
  // joining the AP pops the sign-in page and every hostname opens the panel.
  shark_server.onNotFound([](AsyncWebServerRequest* req) {
    if (req->method() != HTTP_GET) { req->send(404, "text/plain", "not found"); return; }
    // Captive portal: every unknown host/path (the phone's connectivity probe
    // included) gets a 302 to the portal's IP root. A redirect is what reliably
    // makes Android/iOS/Windows pop the "sign in to Wi-Fi" sheet, and pointing
    // at the raw IP means .local / mDNS never has to resolve. The "/" route is
    // registered separately, so this can never loop back onto itself.
    AsyncWebServerResponse* response = req->beginResponse(302, "text/plain", "");
    response->addHeader("Location", "http://172.16.0.1/");
    response->addHeader("Cache-Control", "no-store");
    req->send(response);
  });
  routes_ready = true;
}

void SharkWeb::startAP() {
  if (!screen_mtx) screen_mtx = xSemaphoreCreateMutex();

  // Leave any scan/monitor state and clear stale WiFi config so the AP is a
  // clean, reliably joinable network. A left-over promiscuous mode or an STA
  // trying to reconnect are the usual reasons the SoftAP won't accept clients.
  esp_wifi_set_promiscuous(false);
  WiFi.persistent(false);
  WiFi.disconnect(true, true);
  WiFi.softAPdisconnect(true);
  delay(150);

  // AP-only on a fixed channel is the most reliable configuration to join.
  WiFi.mode(WIFI_AP);
  delay(50);
  WiFi.softAPConfig(SHARK_AP_IP, SHARK_AP_IP, IPAddress(255, 255, 255, 0));
  bool ap_ok = WiFi.softAP(SHARK_AP_SSID, SHARK_AP_PASS, 1 /*ch*/, 0 /*visible*/, 4 /*max*/);
  if (!ap_ok) {                       // one retry if the first bring-up failed
    delay(200);
    WiFi.softAP(SHARK_AP_SSID, SHARK_AP_PASS, 1, 0, 4);
  }
  delay(300);
  registerRoutes();
  shark_server.begin();
  // Reachable as http://shark.local (mDNS/Bonjour) as well as the IP.
  MDNS.end();
  if (MDNS.begin("shark"))
    MDNS.addService("http", "tcp", 80);
  // Captive-portal DNS: every hostname resolves to the AP, so joining the AP
  // pops the "sign in" page and any typed URL (incl. shark.local) opens here.
  dns_server.setErrorReplyCode(DNSReplyCode::NoError);
  dns_server.start(53, "*", SHARK_AP_IP);
}

void SharkWeb::stop() {
  if (!active && !background && !pending_exit && !pending_join) return;
  dns_server.stop();
  MDNS.end();
  shark_server.end();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WiFi.status() == WL_CONNECTED ? WIFI_STA : WIFI_OFF);
  active = false;
  background = false;
  pending_mode = -1;
  pending_action = -1;
  pending_exit = false;
  scan_await = false;   // any kicked join-list scan died with the radio
}

int SharkWeb::takePending() {
  if (pending_join) return -3;
  if (pending_exit) return -4;
  int mode = pending_mode;
  pending_mode = -1;
  return mode;
}

void SharkWeb::completeWiFiJoin() {
  if (!pending_join) return;
  String ssid = join_ssid;
  String password = join_password;
  pending_join = false;
  join_ssid = "";
  join_password = "";
  stop();
  wifi_scan_obj.joinWiFi(ssid, password, true);
  // Joining takes the radio, so Web Control had to go down. Ask the menu loop to
  // bring it back once we are home again -- the panel should not stay dead.
  resume_pending = true;
}

int SharkWeb::takeAction() {
  int action = pending_action;
  pending_action = -1;
  return action;
}

// Rebuild the join-list cache from the finished scan result. Main task only:
// every WiFi call in this class happens here or at panel start/stop, never on
// the async HTTP task.
void SharkWeb::refreshScanCache() {
  // Cache stores just the JSON array of networks; wifiScanJson() wraps it
  // with the live "scanning" flag.
  String out = "[";
  const int found = WiFi.scanComplete();
  if (found > 0) {
    for (int i = 0; i < found && i < 30; i++) {
      if (i) out += ',';
      out += "{\"ssid\":\"" + jsonEscape(WiFi.SSID(i)) + "\",\"rssi\":" + String(WiFi.RSSI(i)) +
             ",\"channel\":" + String(WiFi.channel(i)) + ",\"secure\":" +
             String(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "false" : "true") + "}";
    }
    WiFi.scanDelete();
  } else {
    if (found == 0) WiFi.scanDelete();   // finished empty: release the result
  }
  out += "]";
  if (screen_mtx && xSemaphoreTake(screen_mtx, pdMS_TO_TICKS(20)) == pdTRUE) {
    scan_cache = out;
    xSemaphoreGive(screen_mtx);
  } else {
    scan_cache = out;
  }
  scan_age = millis();
}

void SharkWeb::capture() {
  if (!active && !background) return;
  dns_server.processNextRequest();   // service the captive DNS every tick
  // Join-list cache: when a kicked scan has finished, collect it; when the
  // panel asked for a refresh (or the cache went stale), kick a new async
  // scan. The async scan never blocks this task and the endpoint always
  // answers instantly from the cache. The stale window is long on purpose:
  // an STA scan time-slices the SoftAP's channel, so it should only run when
  // actually asked for (the panel's join dialog) or rarely in the background.
  if (scan_await && WiFi.scanComplete() != WIFI_SCAN_RUNNING) {
    refreshScanCache();
    scan_await = false;
  }
  if (!scan_await && (scan_request || millis() - scan_age > 60000) &&
      millis() - scan_kick > 10000) {
    WiFi.scanNetworks(true, true);
    scan_await = true;
    scan_kick = millis();
    scan_request = false;
  }
  if (millis() - last_capture < 300) return;
  last_capture = millis();
  String state = menu_function_obj.screenStateJson();
  if (screen_mtx && xSemaphoreTake(screen_mtx, pdMS_TO_TICKS(20)) == pdTRUE) {
    screen_cache = state;
    xSemaphoreGive(screen_mtx);
  }
}

void SharkWeb::startBackground() {
  if (!active && !background) {
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
    startAP();
  }
  active = false;
  background = true;
  pending_mode = -1;
}

void SharkWeb::drawQr(int16_t x, int16_t y, int16_t scale) {
  const int16_t n = SHARK_QR_MODULES;
  const int16_t byte_w = (n + 7) / 8;
  const int16_t quiet = 2 * scale;
  display_obj.tft.fillRect(x - quiet, y - quiet, n * scale + quiet * 2, n * scale + quiet * 2, TFT_WHITE);
  for (int16_t row = 0; row < n; row++) for (int16_t col = 0; col < n; col++) {
    const uint8_t bits = pgm_read_byte(&shark_wifi_qr[row * byte_w + (col >> 3)]);
    if (bits & (128 >> (col & 7))) display_obj.tft.fillRect(x + col * scale, y + row * scale, scale, scale, TFT_BLACK);
  }
}

void SharkWeb::drawCard() {
  TFT_eSPI& tft = display_obj.tft;
  const int16_t w = tft.width();
  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(NULL);
  tft.setTextWrap(false);
  tft.setTextSize(1);
  // Same branded top bar as every other screen, then a matching accent title.
  menu_function_obj.drawStatusBar();
  tft.setTextDatum(ML_DATUM); tft.setTextColor(WD_CYAN, TFT_BLACK); tft.drawString("// WEB CONTROL", 6, 27, 1);
  tft.setTextDatum(TL_DATUM);
  const int16_t scale = 3;
  const int16_t qr_px = SHARK_QR_MODULES * scale;
  drawQr((w - qr_px) / 2, 34, scale);
  tft.setTextDatum(MC_DATUM); tft.setTextColor(WD_CYAN, TFT_BLACK); tft.drawString("SCAN TO JOIN", w / 2, 34 + qr_px + 12, 2);
  const int16_t cy = 34 + qr_px + 26;
  wdPanel(tft, 10, cy, w - 20, 78, WD_SURFACE, WD_EDGE, WD_CYAN);
  tft.setTextDatum(TL_DATUM); tft.setTextColor(WD_DIM, WD_SURFACE);
  tft.drawString("SSID", 20, cy + 8, 1); tft.drawString("PASS", 20, cy + 30, 1); tft.drawString("URL", 20, cy + 52, 1);
  tft.setTextColor(WD_WHITE, WD_SURFACE); tft.drawString(SHARK_AP_SSID, 60, cy + 6, 2);
  tft.setTextColor(WD_AMBER, WD_SURFACE); tft.drawString(SHARK_AP_PASS, 60, cy + 28, 2);
  tft.setTextColor(WD_CYAN, WD_SURFACE); tft.drawString("http://172.16.0.1", 60, cy + 50, 2);
  const int16_t ly = cy + 86;
  tft.setTextDatum(ML_DATUM); tft.setTextColor(WD_DIM, TFT_BLACK); tft.drawString("CLIENTS", 12, ly + 6, 1);
  uint16_t clients = WiFi.softAPgetStationNum();
  tft.setTextColor(clients ? WD_CYAN : WD_DIM, TFT_BLACK); tft.drawString(String(clients), 62, ly, 4);
  last_clients = clients;
  wdPanel(tft, 10, ly + 34, (w - 24) / 2, 24, WD_SURFACE, WD_EDGE, WD_CYAN);
  wdPanel(tft, w / 2 + 2, ly + 34, (w - 24) / 2, 24, WD_SURFACE, WD_EDGE, WD_AMBER);
  tft.setTextDatum(MC_DATUM); tft.setTextColor(WD_CYAN, WD_SURFACE); tft.drawString("BACKGROUND", 10 + (w - 24) / 4, ly + 46, 2);
  tft.setTextColor(WD_AMBER, WD_SURFACE); tft.drawString("STOP", w / 2 + 2 + (w - 24) / 4, ly + 46, 2);
  tft.setTextDatum(TL_DATUM);
}

int SharkWeb::run() {
  wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  pending_mode = -1;
  pending_join = false;
  pending_exit = false;
  resume_pending = false;   // opening the panel by hand cancels any auto-resume
  if (!background) startAP();
  active = true;
  background = false;
  drawCard();
  const int16_t split_y = 34 + SHARK_QR_MODULES * 3 + 26 + 78 + 20;
  uint16_t tx, ty;
  while (true) {
    if (pending_join) { completeWiFiJoin(); return -1; }
    if (pending_exit) break;
    // The first remote-menu command moves the panel into background mode so
    // the normal menu loop can execute it on the Arduino main task.
    if (pending_action >= 0) { active = false; background = true; return -2; }
    if (pending_mode >= 0) { int result = pending_mode; stop(); return result; }
    if (display_obj.updateTouch(&tx, &ty)) {
      bool go_bg = ty >= split_y && tx < display_obj.tft.width() / 2;
      while (display_obj.updateTouch(&tx, &ty)) delay(10);
      if (go_bg) { active = false; background = true; return -2; }
      break;
    }
    if (millis() - last_refresh >= 750) {
      last_refresh = millis();
      if (WiFi.softAPgetStationNum() != last_clients) drawCard();
    }
    capture();
    delay(15);
  }
  stop();
  return -1;
}

#endif
