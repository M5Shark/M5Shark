#pragma once

#ifndef MenuFunctions_h
#define MenuFunctions_h

#include "configs.h"

#if defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)
  #include "Keyboard.h"
#endif

#ifdef HAS_TOUCH
  #include "TouchKeyboard.h"
#endif

#ifdef HAS_SCREEN

#define BATTERY_ANALOG_ON 0

#include "WiFiScan.h"
#include "BatteryInterface.h"
#include "SDInterface.h"
#include "settings.h"

#ifdef HAS_BUTTONS
  #include "Switches.h"
  #if (U_BTN >= 0)
    extern Switches u_btn;
  #endif
  #if (D_BTN >= 0)
    extern Switches d_btn;
  #endif
  #if (L_BTN >= 0)
    extern Switches l_btn;
  #endif
  #if (R_BTN >= 0)
    extern Switches r_btn;
  #endif
  #if (C_BTN >= 0)
    extern Switches c_btn;
  #endif
#endif

extern WiFiScan wifi_scan_obj;
extern SDInterface sd_obj;
// #ifdef HAS_BATTERY
extern BatteryInterface battery_obj;
// #endif
extern Settings settings_obj;

#define FLASH_BUTTON 0

#if BATTERY_ANALOG_ON == 1
#define BATTERY_PIN 13
#define ANALOG_PIN 34
#define CHARGING_PIN 27
#endif

// Icon definitions
#define ATTACKS 0
#define BEACON_SNIFF 1
#define BLUETOOTH 2
#define BLUETOOTH_SNIFF 3
#define DEAUTH_SNIFF 4
#define DRAW 5
#define PACKET_MONITOR 6
#define PROBE_SNIFF 7
#define SCANNERS 8
#define CC_SKIMMERS 9
#define SNIFFERS 10
#define WIFI 11
#define BEACON_SPAM 12
#define RICK_ROLL 13
#define REBOOT 14
#define GENERAL_APPS 15
#define UPDATE 16
#define DEVICE 17
#define DEVICE_INFO 18
#define SD_UPDATE 19
#define WEB_UPDATE 20
#define EAPOL 21
#define STATUS_BAT 22
#define STATUS_SD 23
#define PWNAGOTCHI 24
#define SHUTDOWN 25
#define BEACON_LIST 26
#define GENERATE 27
#define CLEAR_ICO 28
#define KEYBOARD_ICO 29
#define JOIN_WIFI 30
#define LANGUAGE 31
#define STATUS_GPS 32
#define GPS_MENU 33
#define DISABLE_TOUCH 34
#define FLIPPER 35
#define BLANK 36
#define PINESCAN_SNIFF 37 // Use blanks icon
#define MULTISSID_SNIFF 37 // Use blanks icon
#define JOINED 38
#define FORCE 39
#define FUNNY_BEACON 40
#define FLOCK 41
#define BRIGHTNESS 42
#define SETTINGS 43
#define ICENAV 44
#define NAV_COMPASS_ICON 45
#define SAT_INFO_ICON 46
#define ADD_WPT_ICON 47
#define SENSOR_RADAR_ICON 48
#define PROFILE_ICON 49
#define PING_SCAN_ICON 50
#define PORT_SCAN_ICON 51
#define SSH_SCAN_ICON 52
#define TELNET_SCAN_ICON 53
#define SMTP_SCAN_ICON 54
#define DNS_SCAN_ICON 55
#define HTTP_SCAN_ICON 56
#define HTTPS_SCAN_ICON 57
#define RDP_SCAN_ICON 58
#define FTP_SCAN_ICON 59
#define SMB_SCAN_ICON 60
#define MQTT_SCAN_ICON 61
#define MYSQL_SCAN_ICON 62
#define POSTGRES_SCAN_ICON 63
#define VNC_SCAN_ICON 64
#define REDIS_SCAN_ICON 65
#define MQTTS_SCAN_ICON 66
#define ARP_SCAN_ICON 67
#define BEACON_LIST_ATTACK_ICON 68
#define BEACON_SPAM_ATTACK_ICON 69
#define FUNNY_BEACON_ATTACK_ICON 70
#define RICK_ROLL_BEACON_ICON 71
#define PROBE_FLOOD_ATTACK_ICON 72
#define DEAUTH_FLOOD_ATTACK_ICON 73

#ifdef MARAUDER_V8
  // Explicit activity supplied by blocking SHARK tools that operate outside
  // WiFiScan::currentScanMode (for example the standalone Wi-Fi radar).
  #define SHARK_HUD_WIFI_ACTIVITY 0x01
  #define SHARK_HUD_BLE_ACTIVITY  0x02
  #define SHARK_HUD_GPS_ACTIVITY  0x04
#endif

struct Menu;

// Individual Nodes of a menu

struct MenuNode {
  String name;
  bool command;
  uint8_t color;
  uint8_t icon;
  bool selected;
  std::function<void()> callable;
};

// Full Menus
struct Menu {
  String name;
  LinkedList<MenuNode>* list;
  Menu                * parentMenu;
  uint16_t               selected = 0;
};


class MenuFunctions
{
  private:

    String u_result = "";


    float _graph_scale = 1.0;
    uint32_t initTime = 0;
    int menu_start_index = 0;
    uint32_t last_activity = 0;   // for the home-screen idle wallpaper
    bool idle_wall_on = false;    // home-screen idle wallpaper enabled (NVS-backed, default OFF)
    #ifdef MARAUDER_V8
      uint8_t shark_hud_tool_activity = 0;
      bool bt_sniffer_data_view = false;
      bool bt_sniffer_resume_after_data = false;
      uint16_t bt_sniffer_data_page = 0;
      bool bt_flipper_data_view = false;
      bool bt_flipper_resume_after_data = false;
      uint16_t bt_flipper_data_page = 0;
      bool bt_skimmer_data_view = false;
      bool bt_skimmer_resume_after_data = false;
      uint16_t bt_skimmer_data_page = 0;
      bool bt_passive_data_view = false;
      bool bt_passive_resume_after_data = false;
      uint16_t bt_passive_data_page = 0;
      bool wifi_passive_data_view = false;
      bool wifi_passive_resume_after_data = false;
      uint16_t wifi_passive_data_page = 0;
      // True while a Wi-Fi sniffer is showing the detector dashboard. Kept apart
      // from wifi_passive_running so the panel keeps refreshing when paused.
      bool wifi_passive_ui = false;
      // Cached dashboard state keeps the Probe/Beacon/Deauth screens from
      // repainting unchanged regions on every 100 ms display tick.
      bool wifi_passive_draw_valid = false;
      uint8_t wifi_passive_draw_mode = 0xFF;
      bool wifi_passive_draw_running = false;
      uint16_t wifi_passive_draw_rate = 0;
      uint16_t wifi_passive_draw_unique = 0;
      uint32_t wifi_passive_draw_total = 0;
      int16_t wifi_passive_draw_last_rssi = -128;
      String wifi_passive_draw_last_name = "";
      String wifi_passive_draw_last_mac = "";
      bool wifi_passive_graph_valid = false;
      uint8_t wifi_passive_graph_bars[216] = {0};
      // Cyber Defense overlay strips (Deauth Alarm / Evil Twin) painted over
      // the passive dashboard's title zone; the title redraws when they clear.
      bool wifi_passive_strip_on = false;
      bool wifi_passive_alarm_flash = false;
      uint32_t wifi_passive_alarm_ms = 0;
      // Shared M5SHARK canvas for the remaining Wi-Fi monitor/sniffer tools.
      // It is armed before StartScan so no legacy header can flash first.
    bool wifi_tool_ui = false;
    // Region-aware debounce for the shared Wi-Fi toolbar. Invalid/lingering
    // launch touches never lock the toolbar, and a missed release sample can
    // recover by moving to another control or waiting for the repeat timeout.
    bool wifi_tool_touch_active = false;
    int16_t wifi_tool_touch_region = -1;
    uint32_t wifi_tool_touch_ms = 0;
    uint32_t wifi_tool_touch_ready_at = 0;
      bool wifi_tool_draw_valid = false;
      uint8_t wifi_tool_draw_mode = 0xFF;
      bool wifi_tool_draw_running = false;
      uint32_t wifi_tool_draw_stats[6] = {0};
      String wifi_tool_draw_signature = "";
      // Wardrive refresh limiter: survey state changes on every scan pass,
      // so the live dashboard repaints at most ~1 Hz instead of flickering.
      uint32_t wifi_tool_wardrive_ms = 0;
      // GPS Data themed screen: refresh limiter + change signatures.
      uint32_t gps_data_ui_ms = 0;
      bool gps_data_ui_valid = false;
      String gps_data_ui_sig = "";
      String gps_data_time_sig = "";
      String gps_data_text_sig = "";
      // GPS Tracker themed screen (same pattern).
      uint32_t gps_trk_ui_ms = 0;
      bool gps_trk_ui_valid = false;
      String gps_trk_ui_sig = "";
      uint8_t wifi_tool_graph_bars[216] = {0};
      // Packet Count owns an in-theme AP/client picker. The radio is paused
      // while rows are toggled so the callback never races list updates.
      bool wifi_packet_target_view = false;
      bool wifi_packet_target_resume_after_select = false;
      bool wifi_packet_target_scan = false;
      uint16_t wifi_packet_target_page = 0;

      // Connected-LAN scanners share one stable dashboard with explicit
      // restart, pause/resume, clear and back controls.
      bool network_scan_ui = false;
      uint8_t network_scan_requested_mode = WIFI_PING_SCAN;
      bool network_scan_draw_valid = false;
      uint32_t network_scan_draw_checked = 0;
      uint32_t network_scan_draw_hits = 0;
      uint8_t network_scan_draw_findings = 0;
      bool network_scan_draw_running = false;
      bool network_scan_draw_complete = false;
      String network_scan_draw_cursor = "";
      bool network_scan_touch_active = false;
      int16_t network_scan_touch_region = -1;
      uint32_t network_scan_touch_ms = 0;
      uint32_t network_scan_touch_ready_at = 0;

      // Custom Beacon List owns a paged, paused-first dashboard. Its touch
      // state follows the same release-independent controls as the scanners.
      bool beacon_list_ui = false;
      bool beacon_list_draw_valid = false;
      bool beacon_list_draw_running = false;
      uint16_t beacon_list_draw_rate = 0;
      uint32_t beacon_list_draw_total = 0;
      uint16_t beacon_list_draw_count = 0;
      uint16_t beacon_list_page = 0;
      uint16_t beacon_list_draw_page = 0xFFFF;
      bool beacon_list_touch_active = false;
      int16_t beacon_list_touch_region = -1;
      uint32_t beacon_list_touch_ms = 0;
      uint32_t beacon_list_touch_ready_at = 0;

      // Random Beacon Spam owns a paused-first live-rate dashboard. The graph
      // stores one real one-second transmitter sample per column.
      bool beacon_spam_ui = false;
      bool beacon_spam_draw_valid = false;
      bool beacon_spam_draw_running = false;
      uint16_t beacon_spam_draw_rate = 0;
      uint32_t beacon_spam_draw_total = 0;
      uint8_t beacon_spam_draw_channel = 0;
      uint8_t beacon_spam_graph_bars[54] = {0};
      bool beacon_spam_touch_active = false;
      int16_t beacon_spam_touch_region = -1;
      uint32_t beacon_spam_touch_ms = 0;
      uint32_t beacon_spam_touch_ready_at = 0;

      // Probe Request Flood owns a paged, paused-first dashboard. Its list rows
      // pick target access points directly (tap toggles selection), so no
      // separate target menu is needed. Live rate/total come from real frames.
      bool probe_flood_ui = false;
      bool probe_flood_draw_valid = false;
      bool probe_flood_draw_running = false;
      uint16_t probe_flood_draw_rate = 0;
      uint32_t probe_flood_draw_total = 0;
      uint16_t probe_flood_draw_count = 0;
      uint16_t probe_flood_draw_selected = 0xFFFF;
      uint16_t probe_flood_page = 0;
      uint16_t probe_flood_draw_page = 0xFFFF;
      bool probe_flood_touch_active = false;
      int16_t probe_flood_touch_region = -1;
      uint32_t probe_flood_touch_ms = 0;
      uint32_t probe_flood_touch_ready_at = 0;

      // Deauth Flood mirrors the Probe Request Flood dashboard: paused-first,
      // paged direct target selection (tap toggles selection), and real
      // transmitter telemetry.
      bool deauth_flood_ui = false;
      bool deauth_flood_draw_valid = false;
      bool deauth_flood_draw_running = false;
      uint16_t deauth_flood_draw_rate = 0;
      uint32_t deauth_flood_draw_total = 0;
      uint16_t deauth_flood_draw_count = 0;
      uint16_t deauth_flood_draw_selected = 0xFFFF;
      uint16_t deauth_flood_page = 0;
      uint16_t deauth_flood_draw_page = 0xFFFF;
      bool deauth_flood_touch_active = false;
      int16_t deauth_flood_touch_region = -1;
      uint32_t deauth_flood_touch_ms = 0;
      uint32_t deauth_flood_touch_ready_at = 0;

      // Generic attack dashboard (R104): one paused-first dashboard drives
      // every remaining Wi-Fi transmit attack (see attackDashDefs). Targets
      // are access points or stations depending on the attack's own loop.
      bool attack_dash_ui = false;
      bool attack_dash_draw_valid = false;
      bool attack_dash_draw_running = false;
      uint16_t attack_dash_draw_rate = 0;
      uint32_t attack_dash_draw_total = 0;
      uint16_t attack_dash_draw_count = 0;
      uint16_t attack_dash_draw_selected = 0xFFFF;
      uint16_t attack_dash_page = 0;
      uint16_t attack_dash_draw_page = 0xFFFF;
      bool attack_dash_touch_active = false;
      int16_t attack_dash_touch_region = -1;
      uint32_t attack_dash_touch_ms = 0;
      uint32_t attack_dash_touch_ready_at = 0;
      uint8_t attack_dash_index = 0;
      // R106: mirrors the in-dashboard scan flag for dirty-region redraws.
      // Shared by the generic and both flood dashboards; only one is open.
      bool attack_dash_draw_scanning = false;

      // Funny SSID Beacon owns a paged, paused-first dashboard for its fixed
      // name set, plus real transmitter telemetry.
      bool funny_beacon_ui = false;
      bool funny_beacon_draw_valid = false;
      bool funny_beacon_draw_running = false;
      uint16_t funny_beacon_draw_rate = 0;
      uint32_t funny_beacon_draw_total = 0;
      uint8_t funny_beacon_draw_channel = 0;
      uint8_t funny_beacon_page = 0;
      uint8_t funny_beacon_draw_page = 0xFF;
      bool funny_beacon_touch_active = false;
      int16_t funny_beacon_touch_region = -1;
      uint32_t funny_beacon_touch_ms = 0;
      uint32_t funny_beacon_touch_ready_at = 0;

      // Rick Roll Beacon owns a paged, paused-first dashboard for its eight
      // existing SSID lines, plus genuine transmitter telemetry.
      bool rick_roll_ui = false;
      bool rick_roll_draw_valid = false;
      bool rick_roll_draw_running = false;
      uint16_t rick_roll_draw_rate = 0;
      uint32_t rick_roll_draw_total = 0;
      uint8_t rick_roll_draw_channel = 0;
      uint8_t rick_roll_page = 0;
      uint8_t rick_roll_draw_page = 0xFF;
      bool rick_roll_touch_active = false;
      int16_t rick_roll_touch_region = -1;
      uint32_t rick_roll_touch_ms = 0;
      uint32_t rick_roll_touch_ready_at = 0;
    #endif
    uint8_t mini_kb_index = 0;
    uint8_t old_gps_sat_count = 0;
    uint8_t max_graph_value = 0;

    // Main menu stuff
    Menu mainMenu;

    Menu wifiMenu;
    Menu bluetoothMenu;
    #ifdef HAS_GPS
      Menu gpsMenu;   // H4W9 Added GPS Menu option to Main Menu
    #endif
    Menu badusbMenu;
    Menu deviceMenu;
    #ifdef MARAUDER_V8
      Menu themeMenu;
    #endif

    // Device menu stuff
    //Menu whichUpdateMenu;
    Menu failedUpdateMenu;
    Menu confirmMenu;
    Menu updateMenu;
    Menu settingsMenu;
    Menu specSettingMenu;
    //Menu languageMenu;
    Menu sdDeleteMenu;

    // WiFi menu stuff
    Menu wifiSnifferMenu;
    Menu sharkDefenseMenu;
    #ifdef MARAUDER_V8
      Menu ruviewMenu;   // in-place Wi-Fi picker that lands back in RuView CSI
      Menu prankMenu;        // Prank section (added, does not alter existing menus)
      Menu wifiPrankMenu;    // WiFi pranks submenu
      Menu btPrankMenu;      // Bluetooth pranks submenu
    #endif
    Menu wifiScannerMenu;
    Menu wifiAttackMenu;
    /*#ifdef HAS_GPS
      Menu wardrivingMenu;
    #endif*/
    Menu wifiGeneralMenu;
    Menu wifiAPMenu;
    Menu wifiIPMenu;
    Menu ssidsMenu;
    //#ifdef HAS_BT
    //  Menu airtagMenu;
    //#endif
    //#ifndef HAS_ILI9341
      Menu wifiStationMenu;
    //#endif

    // WiFi General Menu
    Menu htmlMenu;
    Menu miniKbMenu;
    Menu saveFileMenu;
    Menu genAPMacMenu;
    Menu cloneAPMacMenu;
    Menu setMacMenu;
    Menu selectProbeSSIDsMenu;

    // Bluetooth menu stuff
    Menu bluetoothSnifferMenu;
    Menu bluetoothAttackMenu;

    // Settings things menus
    Menu generateSSIDsMenu;

    Menu evilPortalMenu;

    Menu foxHuntMenu;

    #ifdef HAS_DIRECT_UPLOAD
      Menu deleteAllMenu;
      Menu uploadAllMenu;
    #endif

    //static void lv_tick_handler();

    // Menu icons

    void buildUploadFileMenu();
    void setupSDFileList(bool update = false);
    void buildSDFileMenu(bool update = false);
    void displayMenuButtons();
    uint16_t getColor(uint16_t color);
    void drawAvgLine(int16_t value);
    void drawMaxLine(int16_t value, uint16_t color);
    void drawMaxLine(uint8_t value, uint16_t color);
    float calculateGraphScale(int16_t value);
    float calculateGraphScale(uint8_t value);
    float graphScaleCheck(const int16_t array[TFT_WIDTH]);
    #ifndef HAS_DUAL_BAND
      float graphScaleCheckSmall(const uint8_t array[MAX_CHANNEL]);
    #else
      float graphScaleCheckSmall(const uint8_t array[DUAL_BAND_CHANNELS]);
    #endif
    void drawGraph(int16_t *values);
    void drawGraphSmall(uint8_t *values);
    void renderGraphUI(uint8_t scan_mode = 0);
    void addNodes(Menu* menu, const char* name, uint8_t color, int place, std::function<void()> callable, bool selected = false);
    void battery(bool initial = false);
    void battery2(bool initial = false);
    const char* callSetting(const char* key);
    void displaySetting(const char* key, Menu* menu, int index);
    void buttonSelected(int b, int x = -1);
    void buttonNotSelected(int b, int x = -1);
    #ifdef MARAUDER_V8
      void drawSharkGridButton(int visible_index, int node_index, bool selected);
      void drawSharkTopBar(bool force = false);
      void drawBluetoothAnalyzerUI(bool full_redraw = false);
      void handleBluetoothAnalyzerTouch(uint16_t touch_x, uint16_t touch_y);
      void drawBluetoothSnifferUI(bool full_redraw = false);
      void drawBluetoothSnifferDataUI(bool full_redraw = false);
      void handleBluetoothSnifferTouch(uint16_t touch_x, uint16_t touch_y);
      void drawFlipperSnifferUI(bool full_redraw = false);
      void drawFlipperSnifferDataUI(bool full_redraw = false);
      void handleFlipperSnifferTouch(uint16_t touch_x, uint16_t touch_y);
      void drawCardSkimmerUI(bool full_redraw = false);
      void drawCardSkimmerDataUI(bool full_redraw = false);
      void handleCardSkimmerTouch(uint16_t touch_x, uint16_t touch_y);
      void drawPassiveBleDetectorUI(bool full_redraw = false);
      void drawPassiveBleDetectorDataUI(bool full_redraw = false);
      void handlePassiveBleDetectorTouch(uint16_t touch_x, uint16_t touch_y);
      // Same dashboard for the Wi-Fi sniffers (Probe/Beacon/Deauth/Pineapple/
      // MultiSSID), so WiFi tools match the BLE detector UI/UX.
      void drawPassiveWifiDetectorUI(bool full_redraw = false);
      void drawPassiveWifiDetectorDataUI(bool full_redraw = false);
      void handlePassiveWifiDetectorTouch(uint16_t touch_x, uint16_t touch_y);
      // IQ Family Watch: full-screen take-over shown when a family device
      // (SharkDeck OS) identifies itself while joined to this device's AP.
      void drawFamilyAlert(uint32_t currentTime);
      void dismissFamilyAlert();
      bool isWifiToolUiMode(uint8_t mode) const;
      void startWifiToolUI(uint8_t mode, uint16_t color);
      void startPassiveWifiToolUI(uint8_t mode, uint16_t color);
      void drawWifiToolUI(bool full_redraw = false);
      void drawGpsDataUI(bool full_redraw = false);
      void drawGpsTrackerUI(bool full_redraw = false);
      void handleWifiToolTouch(uint16_t touch_x, uint16_t touch_y);
      void exitWifiToolUI();
      void openPacketTargetPicker();
      void drawPacketTargetPickerUI(bool full_redraw = false);
      void handlePacketTargetPickerTouch(uint16_t touch_x, uint16_t touch_y);
      void resetPacketTargetCounters();
      void startNetworkScannerUI(uint8_t mode, uint16_t color);
      void drawNetworkScannerUI(bool full_redraw = false);
      void handleNetworkScannerTouch(uint16_t touch_x, uint16_t touch_y);
      void exitNetworkScannerUI();
      const char* networkScannerTitle(uint8_t mode) const;
      uint8_t networkScannerIcon(uint8_t mode) const;
      void startBeaconListUI();
      void drawBeaconListUI(bool full_redraw = false);
      void handleBeaconListTouch(uint16_t touch_x, uint16_t touch_y);
      void exitBeaconListUI();
      void startBeaconSpamUI();
      void drawBeaconSpamUI(bool full_redraw = false);
      void handleBeaconSpamTouch(uint16_t touch_x, uint16_t touch_y);
      void exitBeaconSpamUI();
      void startProbeFloodUI();
      void drawProbeFloodUI(bool full_redraw = false);
      void handleProbeFloodTouch(uint16_t touch_x, uint16_t touch_y);
      void exitProbeFloodUI();
      void startDeauthFloodUI();
      void drawDeauthFloodUI(bool full_redraw = false);
      void handleDeauthFloodTouch(uint16_t touch_x, uint16_t touch_y);
      void exitDeauthFloodUI();
      void startAttackDashUI(uint8_t def_index);
      void toggleAttackDashScan(uint8_t rearm_mode, bool station_targets);
      // R111 guardrails + accountability on the device.
      bool attackPinOk();
      void attackPinDenied();
      void exportSessionReport();
      void drawAttackDashUI(bool full_redraw = false);
      void handleAttackDashTouch(uint16_t touch_x, uint16_t touch_y);
      void exitAttackDashUI();
      void startFunnyBeaconUI();
      void drawFunnyBeaconUI(bool full_redraw = false);
      void handleFunnyBeaconTouch(uint16_t touch_x, uint16_t touch_y);
      void exitFunnyBeaconUI();
      void startRickRollUI();
      void drawRickRollUI(bool full_redraw = false);
      void handleRickRollTouch(uint16_t touch_x, uint16_t touch_y);
      void exitRickRollUI();
      void drawFoxHuntUI(bool full_redraw = false);
      void handleFoxHuntTouch(uint16_t touch_x, uint16_t touch_y);
      void drawWiFiFoxHuntUI(bool full_redraw = false);
      void handleWiFiFoxHuntTouch(uint16_t touch_x, uint16_t touch_y);
      void drawBleSpamUI(bool full_redraw = false);
      void handleBleSpamTouch(uint16_t touch_x, uint16_t touch_y);
      void profileScreen();
      void markActiveTheme();
      void startRuView();          // connect if needed, then run RuView CSI
      void buildRuViewConnect();   // scan APs and show the in-place picker
      void ruViewJoinAndRun(const String& ssid, const String& password);
      void startWebTool(int mode); // launch a scan requested from Web Control
      void webNavAction(int code); // remote menu control from Web Control
      void cardControlScreen();    // local SD summary + web-control handoff
      void ssidStudio();           // themed SSID generator (WiFi General)
      void scanApStudio();         // themed active AP survey (WiFi General)
      void sharkNotice(const char* title, const String& line);  // themed result screen
    public:
      String screenStateJson();    // live screen contents for Web Control mirror
    private:
    #endif
    #ifdef HAS_MINI_SCREEN
      void drawMiniMenuButton(int b, int x, bool selected);
    #endif
    //#if (!defined(HAS_ILI9341) && defined(HAS_BUTTONS))
    #ifdef HAS_MINI_KB
      String miniKeyboard(Menu * targetMenu, bool do_pass = false);
    #endif
    //#endif

    #if defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)
      Keyboard_Class M5CardputerKeyboard = Keyboard_Class();
      void updateKeyboard();
      bool isKeyPressed(char c);
    #endif

  public:
    Menu* current_menu;
    Menu clearSSIDsMenu;
    Menu clearAPsMenu;
    
    // Save Files Menu
    Menu saveSSIDsMenu;
    Menu loadSSIDsMenu;
    Menu saveAPsMenu;
    Menu loadAPsMenu;
    Menu saveATsMenu;
    Menu loadATsMenu;

    #ifdef HAS_GPS
      // GPS Menu
      Menu gpsInfoMenu;
      Menu gpsPOIMenu;
    #endif

    Menu infoMenu;
    Menu apInfoMenu;

    #ifdef HAS_DIRECT_UPLOAD
      Menu uploadLogsMenu;
      Menu actionMenu;
    #endif

    //Ticker tick;

    uint16_t x = -1, y = -1;
    boolean pressed = false;

    bool disable_touch;

    String loaded_file = "";

    void setGraphScale(float scale);
    void updateStatusBar();
    #ifdef MARAUDER_V8
      void setSharkHudActivity(uint8_t activity_mask, bool active);
    #endif
    void buildButtons(Menu* menu, int starting_index = 0, const char* button_name = nullptr);
    void changeMenu(Menu* menu, bool simple_change = false);
    void drawStatusBar();
    void displayCurrentMenu(int start_index = 0);
    #ifndef HAS_MINI_SCREEN
      void brightnessMode();
    #endif
    void main(uint32_t currentTime);
    void RunSetup();
    void orientDisplay();
};


#endif
#endif


