#pragma once

#include <Arduino.h>
#include "configs.h"

#if defined(HAS_SCREEN) && defined(MARAUDER_V8)

// Web Control: a local SoftAP with an app-style dashboard. A phone that joins
// the SHARK access point gets the live menu remote, passive app launcher,
// Wi-Fi onboarding, SD card manager, BadUSB script lab, theme system, display
// controls, and controller-side UI/UX preferences.
//
// Radio note: the ESP32 cannot run the SoftAP and promiscuous monitor mode at
// once. So the launcher is a remote trigger: tapping a tool records it, run()
// tears the AP down and returns the mode, and the caller starts it on the
// device. "Run in Background" keeps the AP + server up while you use the menus;
// starting any radio tool automatically stops the AP first.

struct SharkWebTool {
  const char* name;
  const char* cat;   // "wifi" | "ble" | "gps" | "defense"
  int mode;
  uint16_t color;
};

// Sentinel "modes" for web-launched apps that are on-device screens rather than
// WiFiScan modes. Kept well above any real scan-mode id.
#define SHARK_WEB_APP_WIFI_RADAR 900
#define SHARK_WEB_APP_GPS_RADAR  901
// Prank launchers (blocking on-device prank screens started from the web app).
#define SHARK_WEB_APP_MONKEY        902
#define SHARK_WEB_APP_FUNNY_HOTSPOT 903
#define SHARK_WEB_APP_SSID_ROTATOR  904
#define SHARK_WEB_APP_GUEST_COUNTER 905
#define SHARK_WEB_APP_PRANK_PORTAL  906
#define SHARK_WEB_APP_MEME_PORTAL   907
#define SHARK_WEB_APP_CUSTOM_PORTAL 908
#define SHARK_WEB_APP_BLE_NAME      909
#define SHARK_WEB_APP_BLE_ROTATOR   910
#define SHARK_WEB_APP_BLE_ADVERT    911
#define SHARK_WEB_APP_BLE_BEACON    912
#define SHARK_WEB_APP_BLE_RADAR     913
#define SHARK_WEB_APP_BLE_HUNT      914

class SharkWeb {
  public:
    bool active = false;       // AP + server up, foreground card shown
    bool background = false;    // AP + server up, device using the menus

    // Set true when a web-launched radio tool or Wi-Fi join tears Web Control
    // down. The main menu loop watches this and restarts the AP + server once
    // the radio is free again, so the phone panel is never left permanently
    // dead after RuView / a scan / a join. Cleared on manual open or exit.
    volatile bool resume_pending = false;

    // Foreground: brings up the AP + server, draws the on-device card with the
    // join QR, and blocks. Returns:
    //   >= 0  a scan mode the phone asked to launch (AP already down)
    //   -1    the user stopped it by touching the bottom
    //   -2    the user sent it to the background (AP stays up, background=true)
    int run();

    // Non-blocking: start the AP + server and return immediately. The async
    // server self-serves; the device keeps using its menus.
    void startBackground();

    // Poll from the foreground touch loop or on demand. Returns a pending web
    // launch (>=0), -3 for Wi-Fi join, -4 for exit, else -1.
    int takePending();

    // Called on the main Arduino task after takePending() returns -3. Stops the
    // control AP, persists the submitted credentials, and runs the normal join
    // UI so networking is never mutated from the async HTTP task.
    void completeWiFiJoin();

    // Pending remote menu-control action from the panel (>=0), else -1.
    int takeAction();

    // Tear the AP + server down. Safe to call when nothing is up.
    void stop();

    // Reads the live framebuffer (downscaled) into the shadow buffer that
    // /api/screen serves. Cheap no-op if Web Control is not up. Call it from
    // the foreground loop and, when backgrounded, from the menu loop.
    void capture();

    bool isActive() { return active || background; }

    // Repaints the foreground Web Control card. Public so the IQ family
    // alert can restore the screen after its full-screen take-over.
    void drawCard();

  private:
    void startAP();
    void registerRoutes();
    void drawQr(int16_t x, int16_t y, int16_t scale);

    String statusJson();
    String themeListJson();
    String toolListJson();
    String cardJson();
    String wifiScanJson();
    void refreshScanCache();

    volatile int pending_mode = -1;
    volatile int pending_action = -1;
    volatile bool pending_join = false;
    volatile bool pending_exit = false;
    String join_ssid;
    String join_password;
    uint16_t last_clients = 0xFFFF;
    uint32_t last_refresh = 0;
    uint32_t last_capture = 0;
    bool routes_ready = false;
    // Join-list cache: the endpoint serves this instantly; the async scan is
    // kicked and collected on the main task (capture), so no WiFi call ever
    // runs on the HTTP task. Stores just the networks JSON array.
    String scan_cache = "[]";
    volatile bool scan_request = false;
    bool scan_await = false;
    uint32_t scan_kick = 0;
    uint32_t scan_age = 0;
    // Live screen JSON, computed on the main task by capture() and read under a
    // mutex by the async status handler, so the two tasks never touch the menu
    // structures at the same time.
    String screen_cache = "\"title\":\"SHARK\",\"scan\":0,\"page\":1,\"pages\":1,\"tiles\":[]";
};

extern SharkWeb shark_web_obj;

#endif
