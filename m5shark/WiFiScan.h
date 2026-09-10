#pragma once

#ifndef WiFiScan_h
#define WiFiScan_h

#include "configs.h"
#include "utils.h"

#include <ArduinoJson.h>
#include <algorithm>
#include <vector>

#ifdef HAS_BT
  #include <NimBLEDevice.h> // 1.3.8, 2.3.2
#endif

/*#ifdef HAS_IDF_3
  extern "C" {
    #include "esp_netif.h"
    #include "esp_netif_net_stack.h"
  }
#endif*/

//#include <WiFi.h>
#include <ESP32Ping.h>
#include "EvilPortal.h"
#include <math.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include <esp_timer.h>
#include "mbedtls/entropy.h"
#include "mbedtls/bignum.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecp.h"
#ifndef HAS_IDF_3
  #include <lwip/etharp.h>
  #include <lwip/ip_addr.h>
#endif
#ifdef HAS_IDF_3
  #include "esp_system.h"
  #include "esp_mac.h"
#endif
#if defined(HAS_BT) && !defined(HAS_NIMBLE_2)
  #include "esp_bt.h"
#endif
#ifdef HAS_SCREEN
  #include "Display.h"
#endif
#ifdef HAS_SD
  #include "SDInterface.h"
#endif
#include "Buffer.h"
#ifdef HAS_BATTERY
  #include "BatteryInterface.h"
#endif
#ifdef HAS_GPS
  #include "GpsInterface.h"
#endif
#include "settings.h"
#include "Assets.h"
#ifdef HAS_FLIPPER_LED
  #include "flipperLED.h"
#elif defined(XIAO_ESP32_S3)
  #include "xiaoLED.h"
#elif defined(MARAUDER_M5STICKC)
  #include "stickcLED.h"
#elif defined(HAS_NEOPIXEL_LED)
  #include "LedInterface.h"
#endif

#ifdef HAS_DIRECT_UPLOAD
  #include <WiFiClientSecure.h>
  #include <HTTPClient.h>
  #include "mbedtls/sha256.h"
#endif

#define bad_list_length 3

#define OTA_UPDATE 100
#define SHOW_INFO 101
#define ESP_UPDATE 102
#define WIFI_SCAN_OFF 0
#define WIFI_SCAN_PROBE 1
#define WIFI_SCAN_AP 2
#define WIFI_SCAN_PWN 3
#define WIFI_SCAN_EAPOL 4
#define WIFI_SCAN_DEAUTH 5
#define WIFI_SCAN_ALL 6
#define WIFI_PACKET_MONITOR 7
#define WIFI_ATTACK_BEACON_SPAM 8
#define WIFI_ATTACK_RICK_ROLL 9
#define BT_SCAN_ALL 10
#define BT_SCAN_SKIMMERS 11
#define WIFI_SCAN_ESPRESSIF 12
#define LV_JOIN_WIFI 13
#define LV_ADD_SSID 14
#define WIFI_ATTACK_BEACON_LIST 15
#define WIFI_SCAN_TARGET_AP 16
#define LV_SELECT_AP 17
#define WIFI_ATTACK_AUTH 18
#define WIFI_ATTACK_MIMIC 19
#define WIFI_ATTACK_DEAUTH 20
#define WIFI_ATTACK_AP_SPAM 21
#define WIFI_SCAN_TARGET_AP_FULL 22
#define WIFI_SCAN_ACTIVE_EAPOL 23
#define WIFI_ATTACK_DEAUTH_MANUAL 24
#define WIFI_SCAN_RAW_CAPTURE 25
#define WIFI_SCAN_STATION 26
#define WIFI_ATTACK_DEAUTH_TARGETED 27
#define WIFI_SCAN_ACTIVE_LIST_EAPOL 28
#define WIFI_SCAN_SIG_STREN 29
#define WIFI_SCAN_EVIL_PORTAL 30
#define WIFI_SCAN_GPS_DATA 31
#define WIFI_SCAN_WAR_DRIVE 32
#define WIFI_SCAN_STATION_WAR_DRIVE 33
#define BT_SCAN_WAR_DRIVE 34
#define BT_SCAN_WAR_DRIVE_CONT 35
#define BT_ATTACK_SOUR_APPLE 36
#define BT_ATTACK_SWIFTPAIR_SPAM 37
#define BT_ATTACK_SPAM_ALL 38
#define BT_ATTACK_SAMSUNG_SPAM 39
#define WIFI_SCAN_GPS_NMEA 40
#define BT_ATTACK_GOOGLE_SPAM 41
#define BT_ATTACK_FLIPPER_SPAM 42
#define BT_SCAN_AIRTAG 43
#define BT_SPOOF_AIRTAG 44
#define BT_SCAN_FLIPPER 45
#define WIFI_SCAN_CHAN_ANALYZER 46
#define BT_SCAN_ANALYZER 47
#define WIFI_SCAN_PACKET_RATE 48
#define WIFI_SCAN_AP_STA 49
#define WIFI_SCAN_PINESCAN 50
#define WIFI_SCAN_MULTISSID 51
#define WIFI_CONNECTED 52
#define WIFI_PING_SCAN 53
#define WIFI_PORT_SCAN_ALL 54
#define GPS_TRACKER 55
#define WIFI_ATTACK_BAD_MSG 56
#define WIFI_ATTACK_BAD_MSG_TARGETED 57
#define WIFI_SCAN_TELNET 58
#define WIFI_SCAN_SSH 59
#define WIFI_ARP_SCAN 60
#define WIFI_ATTACK_SLEEP 61
#define WIFI_ATTACK_SLEEP_TARGETED 62
#define GPS_POI 63
#define WIFI_SCAN_DNS 64
#define WIFI_SCAN_HTTP 65
#define WIFI_SCAN_HTTPS 66
#define WIFI_SCAN_SMTP 67
#define WIFI_SCAN_RDP 68
#define WIFI_HOSTSPOT 69 // Nice
#define BT_SCAN_AIRTAG_MON 70
#define WIFI_SCAN_CHAN_ACT 71
#define BT_SCAN_FLOCK 72
#define BT_SCAN_SIMPLE 73
#define BT_SCAN_SIMPLE_TWO 74
#define BT_SCAN_FLOCK_WARDRIVE 75
#define WIFI_SCAN_DETECT_FOLLOW 76
#define WIFI_SCAN_SAE_COMMIT 77
#define WIFI_ATTACK_SAE_COMMIT 78
#define WIFI_ATTACK_CSA 79
#define WIFI_ATTACK_QUIET 80
#define BT_SCAN_RAYBAN 81
#define BT_ATTACK_APPLE_JUICE 82
#define WIFI_SCAN_DISPLAY_AP_INFO 83
#define BT_SCAN_FOX_HUNT 84
#define BT_FINDMY_SOUND 85
#define BT_ATTACK_FINDMY_LIVE 86
#define SHARK_DRONE_RID_SCAN 87
#define SHARK_RUVIEW_CSI 88
#define WIFI_SCAN_FTP 89
#define WIFI_SCAN_SMB 90
#define WIFI_SCAN_MQTT 91
#define WIFI_SCAN_MYSQL 92
#define WIFI_SCAN_POSTGRES 93
#define WIFI_SCAN_VNC 94
#define WIFI_SCAN_REDIS 95
#define WIFI_SCAN_MQTTS 96

#define WIFI_ATTACK_FUNNY_BEACON 99

// R105 attack apps: open-system authentication flood (mdk4-style auth DoS)
// against selected access points, and the full-network clone flood that
// reuses the dormant upstream Mimic mode.
#define WIFI_ATTACK_AUTH_RUSH 101

#define BASE_MULTIPLIER 4

#define ANALYZER_NAME_REFRESH 100 // Number of events to refresh the name

// PineScan and Multi SSID
#define MULTISSID_THRESHOLD 3 // Threshold For Multi SSID
#define MAX_MULTISSID_ENTRIES 100 // Max number of confirmed MultiSSIDs to store
#define MAX_AP_ENTRIES 100 // Max number of APs to track for analysis
#define MAX_DISPLAY_ENTRIES 1 // Max Unique MACs to display
#define MAX_PINESCAN_ENTRIES 100 // PineScan Max Entries

#define MAX_CHANNEL     14

#define MAX_PORT 65535

#define WIFI_SECURITY_OPEN   0
#define WIFI_SECURITY_WEP    1
#define WIFI_SECURITY_WPA    2
#define WIFI_SECURITY_WPA2   3
#define WIFI_SECURITY_WPA3   4
#define WIFI_SECURITY_WPA_WPA2_MIXED 5
#define WIFI_SECURITY_WPA2_ENTERPRISE 6
#define WIFI_SECURITY_WPA3_ENTERPRISE 7
#define WIFI_SECURITY_WAPI 8
#define WIFI_SECURITY_UNKNOWN 255

#define WPS_CONFIG_USBA              0x0001
#define WPS_CONFIG_ETHERNET          0x0002
#define WPS_CONFIG_LABEL             0x0004
#define WPS_CONFIG_DISPLAY           0x0008
#define WPS_CONFIG_EXT_NFC_TOKEN     0x0010
#define WPS_CONFIG_INT_NFC_TOKEN     0x0020
#define WPS_CONFIG_NFC_INTERFACE     0x0040
#define WPS_CONFIG_PUSH_BUTTON       0x0080
#define WPS_CONFIG_KEYPAD            0x0100
#define WPS_CONFIG_VIRT_PUSH_BUTTON  0x1000
#define WPS_CONFIG_PHY_PUSH_BUTTON   0x2000
#define WPS_CONFIG_VIRT_DISPLAY      0x4000
#define WPS_CONFIG_PHY_DISPLAY       0x8000

#define CLEAR_APS   0
#define CLEAR_IPS   1
#define CLEAR_AT    2
#define CLEAR_FLIP  3
#define CLEAR_STA   4
#define CLEAR_PINE  5
#define CLEAR_MULTI 6
#define CLEAR_SSID  7
#define CLEAR_BLE   8
#define CLEAR_SKIM  9
#define CLEAR_PASSIVE_BLE 10
#define CLEAR_PASSIVE_WIFI 11

#define WIGLE_UPLOAD 0
#define WDG_UPLOAD   1
#define BOTH_UPLOAD  2

extern EvilPortal evil_portal_obj;

#ifdef HAS_SCREEN
  extern Display display_obj;
#endif
#ifdef HAS_SD
  extern SDInterface sd_obj;
#endif
#ifdef HAS_GPS
  extern GpsInterface gps_obj;
#endif
extern Buffer buffer_obj;
#ifdef HAS_BATTERY
  extern BatteryInterface battery_obj;
#endif
extern Settings settings_obj;
#ifdef HAS_FLIPPER_LED
  extern flipperLED flipper_led;
#elif defined(XIAO_ESP32_S3)
  extern xiaoLED xiao_led;
#elif defined(MARAUDER_M5STICKC)
  extern stickcLED stickc_led;
#elif defined(HAS_NEOPIXEL_LED)
  extern LedInterface led_obj;
#endif

esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);

#define EMPTY_ENTRY 0
#define VALID_ENTRY 1
#define TOMBSTONE_ENTRY 2

#ifdef HAS_BT

#define IS_AIRTAG 0
#define IS_FMNA   1
#define IS_DULT   2
static constexpr uint8_t AIRTAG_BEEP_COMMAND = 0xAF;

static const NimBLEUUID AIRTAG_SERVICE_UUID(
    "7dfc9000-7d1c-4951-86aa-8d9728f8d66c"
);

static const NimBLEUUID AIRTAG_CHARACTERISTIC_UUID(
    "7dfc9001-7d1c-4951-86aa-8d9728f8d66c"
);

static const NimBLEUUID FMNA_SERVICE_UUID(
    "0000fd44-0000-1000-8000-00805f9b34fb"
);

static const NimBLEUUID FMNA_SOUND_CHARACTERISTIC_UUID(
    "4f860003-943b-49ef-bed4-2f730304427a"
);

static const NimBLEUUID DULT_SERVICE_UUID(
    "15190001-12f4-c226-88ed-2ac5579f2a85"
);

static const NimBLEUUID DULT_SOUND_CHARACTERISTIC_UUID(
    "8e0c0001-1d68-fb92-bf61-48377421680e"
);

static const uint8_t FMNA_START_SOUND_COMMAND[] = {
    0x01, 0x00, 0x03
};

static const uint8_t FMNA_STOP_SOUND_COMMAND[] = {
    0x01, 0x01, 0x03
};

static const uint8_t DULT_START_SOUND_COMMAND[] = {
    0x00, 0x03
};

static const uint8_t DULT_STOP_SOUND_COMMAND[] = {
    0x01, 0x03
};

static NimBLEAddress pendingAddress(
    "00:00:00:00:00:00",
    BLE_ADDR_PUBLIC
);

// Declared here, defined once in WiFiScan.cpp. Header-level definitions get
// external linkage in every TU that includes this header, colliding at link
// time once more than one .cpp pulls it in (e.g. SharkPrank via MenuFunctions).
extern bool connectionPending;
extern bool operationInProgress;
#endif

#pragma pack(push, 1)
struct MacEntry {
  uint8_t  mac[6];
  uint32_t last_seen_ms;
  uint16_t frame_count;
  int32_t  first_lat_e6;
  int32_t  first_lon_e6;
  int32_t  last_lat_e6;
  int32_t  last_lon_e6;
  bool following;
  int32_t dloc;
  int8_t rssi;
  bool bt;
};
#pragma pack(pop)

struct AirTag {
    String mac;                  // MAC address of the AirTag
    std::vector<uint8_t> payload; // Payload data
    uint16_t payloadSize;
    bool selected;
    int8_t rssi;
    uint32_t last_seen;
    bool is_airtag   = false;
    bool is_fmna     = false;
    bool is_dult     = false;
    bool connectable = true;
    #ifdef HAS_BT
    NimBLEAddress device_address;
    #endif
};

struct Flipper {
  String mac;
  String name;
  String color;
  int rssi = -128;
};

struct CardSkimmer {
  String mac;
  String name;
  int rssi = -128;
  uint32_t hits = 0;
};

struct PassiveBleFinding {
  String mac;
  String name;
  String detail;
  int rssi = -128;
  uint32_t hits = 0;
  uint32_t last_seen = 0;
};

// Same shape as PassiveBleFinding, for the Wi-Fi passive detector dashboard
// (Probe / Beacon / Deauth / Pineapple / MultiSSID sniffs) so those tools get the
// identical card UI the BLE FindMy/Flock/Meta detectors use.
struct PassiveWifiFinding {
  String mac;
  String name;
  String detail;
  int rssi = -128;
  uint32_t hits = 0;
  uint32_t last_seen = 0;
};

struct BleDevice {
  uint8_t  mac[6];
  String   name;
  bool     selected = false;
  int      rssi     = -128;
};

#define MAX_DRONE_RID_ENTRIES 16

struct DroneRIDEntry {
  uint8_t mac[6] = {0};
  char uas_id[21] = {0};
  int8_t rssi = -128;
  uint8_t channel = 0;
  bool seen_ble = false;
  bool seen_wifi = false;
  uint32_t last_seen = 0;
  uint32_t packets = 0;
};

struct NetworkScanFinding {
  IPAddress ip;
  uint16_t port = 0;
};

#ifdef HAS_PSRAM
  extern struct mac_addr* mac_history;
#endif

enum class MacSortMode : uint8_t {
  MOST_RECENT,
  MOST_FRAMES,
  HIGH_RSSI
};

class WiFiScan
{
  private:
    // Wardriver thanks to https://github.com/JosephHewitt
    int arp_count = 0;
    #ifndef HAS_PSRAM
      struct mac_addr mac_history[mac_history_len];
    #endif

    int current_act_len = 0;

    uint32_t chanActTime = 0;

    uint8_t ap_mac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
    uint8_t sta_mac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

    // Settings
    uint mac_history_cursor = 0;
    uint8_t channel_hop_delay = 1;

    #ifdef HAS_DIRECT_UPLOAD
      WiFiClientSecure *client = new WiFiClientSecure();
    #endif
  
    int x_pos; //position along the graph x axis
    float y_pos_x; //current graph y axis position of X value
    float y_pos_x_old = 120; //old y axis position of X value
    float y_pos_y; //current graph y axis position of Y value
    float y_pos_y_old = 120; //old y axis position of Y value
    float y_pos_z; //current graph y axis position of Z value
    float y_pos_z_old = 120; //old y axis position of Z value
    int midway = 0;
    byte x_scale = 1; //scale of graph x axis, controlled by touchscreen buttons
    byte y_scale = 1;

    // Packet Monitor stacked-graph state (upstream v1.15.0 refactor, rendered in
    // the SHARK style). Three scrolling histogram lanes -- beacons, deauths,
    // probes -- each auto-scaled to its own peak over the visible window.
    #ifdef HAS_ILI9341
      static constexpr int16_t PKTG_COL_W   = 4;   // px per history column
      static constexpr int16_t PKTG_LEFT    = 34;  // left gutter for the count label
      static constexpr uint16_t PKTG_REFRESH = 200; // ms between samples
      static constexpr int16_t PKTG_LEN     = (WIDTH_1 - PKTG_LEFT) / PKTG_COL_W;
      uint16_t pktg_beacons[PKTG_LEN] = {};
      uint16_t pktg_deauths[PKTG_LEN] = {};
      uint16_t pktg_probes[PKTG_LEN]  = {};
      uint32_t pktg_last_sample = 0;
      bool pktg_graph_ready = false;
      void resetPacketMonitorGraph();
      void samplePacketMonitorGraph();
      void drawPacketMonitorLane(int16_t y0, int16_t h, uint16_t* hist,
                                 uint16_t color, const char* label);
      void drawPacketMonitorGraphs();
    #endif

    bool do_break = false;

    bool wsl_bypass_enabled = false;

    bool scan_complete = false;

    uint8_t wardrive_channel_index = 0;

    //int num_beacon = 0; // GREEN
    //int num_probe = 0; // BLUE
    //int num_deauth = 0; // RED

    uint32_t initTime = 0;
    uint32_t last_ui_update = 0;
    uint32_t last_sour_apple_update = 0;
    bool run_setup = true;
    void initWiFi(uint8_t scan_mode);
    uint8_t bluetoothScanTime = 5;
    int packets_sent = 0;
    const wifi_promiscuous_filter_t filt = {.filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA};
    #ifdef HAS_BT
      NimBLEScan* pBLEScan;
    #endif
    #ifdef HAS_NIMBLE_2
      NimBLEClient* nimbleClient;
    #endif

    const char* rick_roll[8] = {
      "01 Never gonna give you up",
      "02 Never gonna let you down",
      "03 Never gonna run around",
      "04 and desert you",
      "05 Never gonna make you cry",
      "06 Never gonna say goodbye",
      "07 Never gonna tell a lie",
      "08 and hurt you"
    };

    // H4W9 added Funny Beacon Spam
    const char* funny_beacon[12] = {
      "Abraham Linksys",
      "Benjamin FrankLAN",
      "Dora the Internet Explorer",
      "FBI Surveillance Van 4",
      "Get Off My LAN",
      "Loading...",
      "Martin Router King",
      "404 Wi-Fi Unavailable",
      "Test Wi-Fi Please Ignore",
      "This LAN is My LAN",
      "Titanic Syncing",
      "Winternet is Coming"
    };

    char* prefix = "G";

    typedef struct
    {
      int16_t fctl;
      int16_t duration;
      uint8_t da;
      uint8_t sa;
      uint8_t bssid;
      int16_t seqctl;
      unsigned char payload[];
    } __attribute__((packed)) WifiMgmtHdr;
    
    typedef struct {
      uint8_t payload[0];
      WifiMgmtHdr hdr;
    } wifi_ieee80211_packet_t;

		// Tracking structures for PineScan (similar to MultiSSID)
    struct PineScanTracker {
        uint8_t mac[6];
        bool suspicious_oui;
        bool tag_and_susp_cap;
        uint8_t channel;
        int8_t rssi;
        bool reported;
    };

    // For confirmed Pineapple devices
    struct ConfirmedPineScan {
        uint8_t mac[6];
        String detection_type;
        String essid;
        uint8_t channel;
        int8_t rssi;
        bool displayed;
    };
    LinkedList<PineScanTracker>* pinescan_trackers;
    LinkedList<ConfirmedPineScan>* confirmed_pinescan;
    bool pinescan_list_full_reported;
    
    // Security Conditions For Pineapple detection
    enum SecurityCondition {
        NONE = 0x00,
        SUSPICIOUS_WHEN_OPEN = 0x01,
        SUSPICIOUS_WHEN_PROTECTED = 0x02,
        SUSPICIOUS_ALWAYS = 0x04
    };

    // SuspiciousVendor struct
    struct SuspiciousVendor {
        const char* vendor_name;
        uint8_t security_flags;
        uint32_t ouis[20];                 // Array of OUIs (max 20 per vendor)
        uint8_t oui_count;                 // Number of OUIs for this vendor
    };

    // Declare the table for Pineapple
    static const SuspiciousVendor suspicious_vendors[];
    static const int NUM_SUSPICIOUS_VENDORS;

    // RuView-inspired, local-only CSI sensing for the M5 SHARK ESP32-C5.
    // Raw CSI never leaves the device and is not written to storage.
    static void ruviewCSICallback(void* ctx, wifi_csi_info_t* data);
    void RunRuViewCSI(uint8_t scan_mode, uint16_t color);
    void updateRuViewCSI(uint32_t currentTime);
    void renderRuViewCSI(bool full_redraw = false);
    void shutdownRuViewCSI();
    void resetRuViewCSI();
    volatile uint32_t ruview_window_frames = 0;
    volatile uint64_t ruview_window_power_sum = 0;
    volatile uint64_t ruview_window_power_sq_sum = 0;
    volatile int32_t ruview_window_rssi_sum = 0;
    uint32_t ruview_total_frames = 0;
    uint32_t ruview_started_ms = 0;
    uint32_t ruview_last_sample_ms = 0;
    uint32_t ruview_last_motion_ms = 0;
    uint32_t ruview_calibration_samples = 0;
    float ruview_calibration_sum = 0.0f;
    float ruview_calibration_sq_sum = 0.0f;
    float ruview_energy = 0.0f;
    float ruview_baseline = 0.0f;
    float ruview_threshold = 0.0f;
    float ruview_score = 0.0f;
    int8_t ruview_rssi = -127;
    uint8_t ruview_bssid[6] = {0};
    uint8_t ruview_trace[80] = {0};
    bool ruview_active = false;
    bool ruview_calibrated = false;
    bool ruview_motion = false;
    bool ruview_low_traffic = false;
    String ruview_error = "";

    // Track for AP list limit (Uninitialised, Done in RunSetup)
    bool ap_list_full_reported;

    // MULTI SSID STRUCTS

    struct MultiSSIDTracker {
        uint8_t mac[6];
        uint16_t ssid_hashes[MULTISSID_THRESHOLD];
        uint8_t unique_ssid_count;
        bool reported;
    };

    // New struct for confirmed MultiSSID devices
    struct ConfirmedMultiSSID {
        uint8_t mac[6];
        String essid;
        uint8_t channel;
        int8_t rssi;
        uint8_t ssid_count;
        bool displayed;
    };
    LinkedList<MultiSSIDTracker>* multissid_trackers;
    LinkedList<ConfirmedMultiSSID>* confirmed_multissid;
    bool multissid_list_full_reported;

    uint8_t sae_commit[32] = {
      0xb0, 0x00, 0x00, 0x00,                     // Type/Subtype, Duration
      0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB,         // Destination
      0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,         // Source
      0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB,         // BSSID (Destination)
      0x00, 0x00,                                 // Frag num
      0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x13, 0x00  // Auth alg (SAE), SAE sequence, group 19
    };

    // barebones packet
    uint8_t packet[128] = { 0x80, 0x00, 0x00, 0x00, //Frame Control, Duration
                    /*4*/   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, //Destination address 
                    /*10*/  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, //Source address - overwritten later
                    /*16*/  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, //BSSID - overwritten to the same as the source address
                    /*22*/  0xc0, 0x6c, //Seq-ctl
                    /*24*/  0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, //timestamp - the number of microseconds the AP has been active
                    /*32*/  0x64, 0x00, //Beacon interval
                    /*34*/  0x31, 0x00, //Capability info
                    /* SSID */
                    /*36*/  0x00
                    };

    uint8_t post_base[39] = {
      0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c,
      0x03, 0x01, 0x04, 0x30, 0x18, 0x01, 0x00, 0x00, 0x0f, 0xac, 
      0x02, 0x02, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x00, 0x0f, 0xac, 
      0x04, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x02, 0x00, 0x00
    };

    uint8_t prob_req_packet[128] = {0x40, 0x00, 0x00, 0x00, 
                                  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination
                                  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source
                                  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Dest
                                  0x01, 0x00, // Sequence
                                  0x00, // SSID Parameter
                                  0x00, // SSID Length
                                  /* SSID */
                                  };

    uint8_t deauth_frame_default[26] = {
                              0xc0, 0x00, 0x3a, 0x01,
                              0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0xf0, 0xff, 0x02, 0x00
                          };

    uint8_t eapol_packet_bad_msg1[153] = {
                              0x08, 0x02,                         // Frame Control (EAPOL)
                              0x00, 0x00,                         // Duration
                              0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination (Broadcast)
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source (BSSID)
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
                              0x30, 0x00,                         // Sequence Control
                              /* LLC / SNAP */
                              0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00,
                              0x88, 0x8e,                          // Ethertype = EAPOL
                              /* -------- 802.1X Header -------- */
                              0x02,                               // Version 802.1X‑2004
                              0x03,                               // Type Key
                              0x00, 0x75,                          // Length 117 bytes
                              /* -------- EAPOL‑Key frame body (117 B) -------- */
                              0x02,                               // Desc Type 2 (AES/CCMP)
                              0x00, 0xCA,                          // Key Info (Install|Ack…)
                              0x00, 0x10,                          // Key Length = 16
                              /* Replay Counter (8) */
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
                              /* Nonce (32) */
                              0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                              0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                              0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                              0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
                              /* Key IV (16) */
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              /* Key RSC (8) */
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              /* Key ID  (8) */ 
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              /* Key MIC (16) */ 
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              /* Key Data Len (2) */ 
                              0x00, 0x16,
                              /* Key Data (22 B) */
                              0xDD, 0x14,                // Vendor‑specific (PMKID IE)
                              0x00, 0x0F, 0xAC, 0x04,      // OUI + Type (PMKID)
                              /* PMKID (16 byte zero) */
                              0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 
                              0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x11
                          };

    uint8_t association_packet[200] = {
                              0x00, 0x10, // Frame Control (Association Request) PM=1
                              0x3a, 0x01, // Duration
                              0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination (Broadcast)
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source (Fake Source or BSSID)
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
                              0x00, 0x00,                         // Sequence Control
                              0x31, 0x00,                         // Capability Information (PM=1)
                              0x0a, 0x00,                         // Listen Interval
                              0x00,                               // SSID tag
                              0x00,                               // SSID length      
                          };

    enum EBLEPayloadType
    {
      Microsoft,
      Apple,
      Samsung,
      Google,
      FlipperZero,
      Airtag,
      Apple2
    };

      #ifdef HAS_BT

      struct BLEData
      {
        NimBLEAdvertisementData AdvData;
        NimBLEAdvertisementData ScanData;
      };

      struct WatchModel
      {
          uint8_t value;
          const char *name;
      };

      WatchModel* watch_models = nullptr;

      static void scanCompleteCB(BLEScanResults scanResults);
      NimBLEAdvertisementData GetUniversalAdvertisementData(EBLEPayloadType type);
    #endif

    #ifdef HAS_NIMBLE_2
      int connectAndProcessTracker(NimBLEAddress& address);
      bool backendFindMySound(NimBLEAddress& address, bool gui = false);
      bool sendAirtagSoundCommand(NimBLEClient* currentClient);
      bool sendFmnaSoundCommand(NimBLEClient* currentClient);
      bool sendDultSoundCommand(NimBLEClient* currentClient);
      bool enableTrackerResponses(NimBLERemoteCharacteristic* characteristic);
      void createNimbleClient();
      void initializeFindMyScan();
    #endif

    bool wigleUpload(String filePath);
    bool wdgwarsUpload(String filePath);
    void writeSidecar(String filePath, String service);
    bool sidecarExists(String filePath, String service); 
    #ifdef HAS_SCREEN
      void drawUploadProgress(const char* service, uint8_t percent, bool waiting = false);
    #endif

    void runFoxHunt(uint32_t currentTime);
    void throwThatShitInACircle();
    void displayTargetFilter();
    void displayTransmitRate();
    void prepareScanStage(uint16_t color_1, uint16_t color_2);
    void setLEDMode(int mode);
    void setWiFiMode(wifi_mode_t mode, wifi_promiscuous_cb_t cb);
    void writeNetworkInfo();
    void setupScanDisplayArea(uint16_t background, uint16_t color);
    void updateTrackerUI();
    void showNetworkInfo();
    void setNetworkInfo();
    void fullARP();
    bool readARP(IPAddress targ_ip);
    bool singleARP(IPAddress ip_addr);
    void pingScan(uint8_t scan_mode = WIFI_PING_SCAN);
    void portScan(uint8_t scan_mode = WIFI_PORT_SCAN_ALL, uint16_t targ_port = 22);
    void resetNetworkScanProgress(uint8_t mode, bool clear_results = true);
    void recordNetworkFinding(IPAddress ip, uint16_t port);
    bool isHostAlive(IPAddress ip);
    bool checkHostPort(IPAddress ip, uint16_t port, uint16_t timeout = 100);
    String extractManufacturer(const uint8_t* payload);
    int checkMatchAP(char addr[], bool update_ap = true);
    uint8_t getSecurityType(const uint8_t* beacon, uint16_t len);
    void addAnalyzerValue(int16_t value, int rssi_avg, int16_t target_array[], int array_size);
    bool mac_cmp(struct mac_addr addr1, struct mac_addr addr2);
    bool mac_cmp(uint8_t addr1[6], uint8_t addr2[6]);
    // POI tagging during wardrive
    File poiFile;
    bool poiFileOpen = false;
    String poiFileName = "";

    void openPoiFile();
    void closePoiFile();

    void executeWarDrive();
    void executeBLESpam(EBLEPayloadType type);
    void startWardriverWiFi();
    void saeAttackLoop(uint32_t currentTime);
    void processPwnagotchiBeacon(const uint8_t* frame, int length);

    void startWiFiAttacks(uint8_t scan_mode, uint16_t color, const char* title_string);

    void signalAnalyzerLoop(uint32_t tick);
    void channelActivityLoop(uint32_t tick);
    void packetRateLoop(uint32_t tick);
    void packetMonitorMain(uint32_t currentTime);
    void packetMonitorMain_legacy(uint32_t currentTime);
    void updateMidway();
    bool sendSAECommitFrame(uint8_t* targ_addr, uint8_t* src_addr) ;
    void sendProbeAttack(uint32_t currentTime);
    void sendBadMsgAttack(uint32_t currentTime, bool all = false);
    void sendAssocSleepAttack(uint32_t currentTime, bool all = false);
    void sendDeauthFrame(uint8_t bssid[6], int channel, uint8_t mac[6]);
    void sendAuthRushFrame(uint8_t bssid[6], int channel);
    void sendEapolBagMsg1(uint8_t bssid[6], int channel, uint8_t mac[6], uint8_t sec = WIFI_SECURITY_WPA2);
    void sendAssociationSleep(const char* ESSID, uint8_t bssid[6], int channel, uint8_t mac[6]);
    void broadcastRandomSSID(uint32_t currentTime);
    void broadcastCustomBeacon(uint32_t current_time, ssid custom_ssid, bool for_camera = false);
    void broadcastCustomBeacon(uint32_t current_time, AccessPoint custom_ssid, int scan_mode);
    void broadcastSetSSID(uint32_t current_time, const char* ESSID, uint8_t chan = 0, bool legit = false);
    void executeFindMyLive(uint32_t current_time);
    void RunAPScan(uint8_t scan_mode, uint16_t color);
    void RunGPSNmea();
    void RunPwnScan(uint8_t scan_mode, uint16_t color);
    void RunPineScan(uint8_t scan_mode, uint16_t color);
    void RunMultiSSIDScan(uint8_t scan_mode, uint16_t color);
    void RunBeaconScan(uint8_t scan_mode, uint16_t color);
    void RunRawScan(uint8_t scan_mode, uint16_t color);
    void RunDeauthScan(uint8_t scan_mode, uint16_t color);
    void RunEapolScan(uint8_t scan_mode, uint16_t color);
    void RunProbeScan(uint8_t scan_mode, uint16_t color);
    void RunSAEScan(uint8_t scan_mode, uint16_t color);
    void RunPacketMonitor(uint8_t scan_mode, uint16_t color);
    void RunBluetoothScan(uint8_t scan_mode, uint16_t color);
    void RunSourApple(uint8_t scan_mode, uint16_t color);
    void RunFindMyLive(uint8_t scan_mode, uint16_t color);
    void RunSwiftpairSpam(uint8_t scan_mode, uint16_t color);
    void RunEvilPortal(uint8_t scan_mode, uint16_t color);
    void RunPingScan(uint8_t scan_mode, uint16_t color);
    void RunPortScanAll(uint8_t scan_mode, uint16_t color);
    bool checkMem();
    void writeHeader(bool poi = false);
    void writeFooter(bool poi = false);
    void displayWardriveStats();
    void displayAPStats();


  public:
    volatile bool bt_cb_busy = false;
    volatile bool bt_pending_clear = false;

    bool send_deauth = false;

    bool channel_hop = false;
    uint8_t connected_devices = 0;


    static MacEntry mac_entries[mac_history_len_half];
    static uint8_t mac_entry_state[mac_history_len_half];

    String header_line = "WigleWifi-1.4,appRelease=" + (String)MARAUDER_VERSION + ",model=M5 SHARK v8,release=" + (String)MARAUDER_VERSION + ",device=M5 SHARK,display=SPI TFT,board=M5 SHARK v8,brand=M5Shark\nMAC,SSID,AuthMode,FirstSeen,Channel,RSSI,CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,Type\n";

    uint8_t dual_band_channels[DUAL_BAND_CHANNELS] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 32, 36, 40, 44, 48, 52, 56, 60, 64, 68, 72, 76, 80, 84, 88, 92, 96, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 144, 149, 153, 157, 161, 165, 169, 173, 177};

    uint8_t oui_list[27][3] = {
    {0x58, 0x8E, 0x81},
    {0xCC, 0xCC, 0xCC},
    {0xEC, 0x1B, 0xBD},
    {0x90, 0x35, 0xEA},
    {0x04, 0x0D, 0x84},
    {0xF0, 0x82, 0xC0},
    {0x1C, 0x34, 0xF1},
    {0x38, 0x5B, 0x44},
    {0x94, 0x34, 0x69},
    {0xB4, 0xE3, 0xF9},
    {0x70, 0xC9, 0x4E},
    {0x3C, 0x91, 0x80},
    {0xD8, 0xF3, 0xBC},
    {0x80, 0x30, 0x49},
    {0x14, 0x5A, 0xFC},
    {0x74, 0x4C, 0xA1},
    {0x08, 0x3A, 0x88},
    {0x9C, 0x2F, 0x9D},
    {0x94, 0x08, 0x53},
    {0xE4, 0xAA, 0xEA},
    {0xF4, 0x6A, 0xDD},
    {0xF8, 0xA2, 0xD6},
    {0xE0, 0x0A, 0xF6},
    {0x00, 0xF4, 0x8D},
    {0xD0, 0x39, 0x57},
    {0xE8, 0xD0, 0xFC},
    {0xB4, 0x1E, 0x52}
    };

    uint8_t dual_band_channel_index = 0;

    // Shoutout https://github.com/NullPxl
    const uint16_t META_IDENTIFIERS[6] = {
      0xFD5F,  // Meta (0xFD5F)
      0xFEB7,  // Meta (0xFEB7)
      0xFEB8,  // Meta (0xFEB8)
      0x01AB,  // Meta (0x01AB)
      0x058E,  // Meta (0x058E)
      0x0D53,   // Luxottica (0x0D53)
    };

    const uint16_t BLOCKED_IDENTIFIERS[5] = {
      0xFD5A,  // Samsung
      0xFD69,   // Samsung
      0x004C, //apple
      0x0006, // microsoft
      0xFEF3, // phone
    };

    // Stuff for RAW stats
    uint32_t mgmt_frames = 0;
    uint32_t data_frames = 0;
    uint32_t beacon_frames = 0;
    uint32_t req_frames = 0;
    uint32_t resp_frames = 0;
    uint32_t deauth_frames = 0;
    uint32_t eapol_frames = 0;
    uint32_t complete_eapol = 0;
    uint32_t flock_devices = 0;
    uint32_t drone_rid_packets = 0;
    uint8_t drone_rid_count = 0;
    DroneRIDEntry drone_rid_entries[MAX_DRONE_RID_ENTRIES];
    int8_t min_rssi = 0;
    int8_t max_rssi = -128;

    int bt_frames = 0;

    bool force_pmkid = false;
    bool force_probe = false;
    bool save_pcap = false;
    bool ep_deauth = false;
    bool ble_scanning = false;

    char* flock_ssid[5] = {
      "flock",
      "penguin",
      "pigvision",
      "fs ext battery",
      "Flock"
    };

    #ifdef HAS_DUAL_BAND
      uint8_t channel_activity[DUAL_BAND_CHANNELS] = {};
    #else
      uint8_t channel_activity[MAX_CHANNEL] = {};
    #endif

    uint8_t activity_page = 1;

    String analyzer_name_string = "";
    
    uint8_t analyzer_frames_recvd = 0;

    bool analyzer_name_update = false;

    // Live Bluetooth Analyzer state used by the M5SHARK touch dashboard.
    // RUNNING means NimBLE is actively scanning; PAUSED keeps the screen and
    // captured graph visible without receiving advertisements.
    bool bt_analyzer_running = false;
    uint16_t bt_analyzer_rate = 0;
    uint16_t bt_analyzer_peak = 0;
    volatile uint32_t bt_analyzer_total = 0;

    // Live Bluetooth Sniffer state for the M5SHARK touch dashboard. These
    // values are written only from genuine NimBLE advertisement callbacks.
    bool bt_sniffer_running = false;
    uint16_t bt_sniffer_rate = 0;
    uint16_t bt_sniffer_peak = 0;
    volatile uint32_t bt_sniffer_total = 0;
    volatile uint16_t bt_sniffer_window = 0;
    uint16_t bt_sniffer_unique = 0;
    int16_t bt_sniffer_last_rssi = -128;
    String bt_sniffer_last_name = "";
    String bt_sniffer_last_mac = "";
    uint32_t bt_sniffer_rate_tick = 0;
    uint32_t bt_sniffer_graph_tick = 0;
    uint32_t bt_sniffer_rate_base = 0;

    // M5SHARK Wi-Fi Fox Hunt state. The selected AP RSSI continues to come
    // from the existing promiscuous callback; this flag only records whether
    // that real receiver is currently enabled or paused by the touch UI.
    bool wifi_fox_running = false;

    // Live Flipper Sniff state. Counts only advertisements matching the
    // Flipper Zero BLE signatures handled by the existing callback.
    bool bt_flipper_running = false;
    uint16_t bt_flipper_rate = 0;
    uint16_t bt_flipper_peak = 0;
    volatile uint32_t bt_flipper_total = 0;
    volatile uint16_t bt_flipper_window = 0;
    uint16_t bt_flipper_unique = 0;
    int16_t bt_flipper_last_rssi = -128;
    String bt_flipper_last_name = "";
    String bt_flipper_last_mac = "";
    String bt_flipper_last_color = "";
    uint32_t bt_flipper_rate_tick = 0;
    uint32_t bt_flipper_graph_tick = 0;
    uint32_t bt_flipper_rate_base = 0;

    // Live Card Skimmer detector state. A match is counted only when the
    // advertised BLE name exactly matches the detector's HC-03/05/06 rule.
    bool bt_skimmer_running = false;
    uint16_t bt_skimmer_rate = 0;
    uint16_t bt_skimmer_peak = 0;
    volatile uint32_t bt_skimmer_total = 0;
    volatile uint16_t bt_skimmer_window = 0;
    uint16_t bt_skimmer_unique = 0;
    int16_t bt_skimmer_last_rssi = -128;
    String bt_skimmer_last_name = "";
    String bt_skimmer_last_mac = "";
    uint32_t bt_skimmer_rate_tick = 0;
    uint32_t bt_skimmer_graph_tick = 0;
    uint32_t bt_skimmer_rate_base = 0;

    // Shared real-match state for FindMy, Flock and Meta detector dashboards.
    bool bt_passive_running = false;
    uint8_t bt_passive_mode = WIFI_SCAN_OFF;
    uint16_t bt_passive_rate = 0;
    uint16_t bt_passive_peak = 0;
    volatile uint32_t bt_passive_total = 0;
    volatile uint16_t bt_passive_window = 0;
    uint16_t bt_passive_unique = 0;
    int16_t bt_passive_last_rssi = -128;
    String bt_passive_last_name = "";
    String bt_passive_last_mac = "";
    String bt_passive_last_detail = "";
    uint32_t bt_passive_rate_tick = 0;
    uint32_t bt_passive_graph_tick = 0;
    uint32_t bt_passive_rate_base = 0;

    // Sour Apple dashboard counters represent completed advertisement bursts
    // from the existing Apple payload loop. STOP gates that loop without
    // changing its payload format, timing, power, or address behavior.
    bool bt_sour_running = false;
    uint16_t bt_sour_rate = 0;
    uint16_t bt_sour_peak = 0;
    uint32_t bt_sour_total = 0;
    uint16_t bt_sour_window = 0;
    uint32_t bt_sour_rate_tick = 0;
    uint32_t bt_sour_graph_tick = 0;
    uint32_t bt_sour_rate_base = 0;

    // Same dashboard state for the Wi-Fi sniffers (Probe / Beacon / Deauth /
    // Pineapple / MultiSSID), mirroring the BLE detector UI one-for-one.
    bool wifi_passive_running = false;
    uint16_t wifi_passive_rate = 0;
    uint16_t wifi_passive_peak = 0;
    volatile uint32_t wifi_passive_total = 0;
    volatile uint16_t wifi_passive_window = 0;
    uint16_t wifi_passive_unique = 0;
    int16_t wifi_passive_last_rssi = -128;
    char wifi_passive_last_name[33] = {0};
    char wifi_passive_last_mac[18] = {0};
    uint32_t wifi_passive_rate_tick = 0;
    uint32_t wifi_passive_graph_tick = 0;
    uint32_t wifi_passive_rate_base = 0;
    bool isPassiveWifiMode();
    void snapshotPassiveWifiUi(String& name, String& mac, int16_t& rssi);
    void recordPassiveWifiFinding(String mac, String name, String detail, int rssi);
    void startPassiveWifiDetector();
    void stopPassiveWifiDetector();
    void clearPassiveWifiDetector();

    // M5SHARK Wi-Fi dashboard ownership. The menu arms this before StartScan,
    // preventing legacy scan chrome and touch handlers from painting underneath
    // the themed dashboard.
    bool wifi_tool_ui_owned = false;
    bool wifi_tool_running = false;
    void setOwnedWifiToolPaused(bool paused);
    uint16_t getPacketMonitorSample(uint8_t lane, uint8_t index) const;
    uint8_t getPacketMonitorLength() const;
    uint32_t getPacketMonitorTotal(uint8_t lane) const;

    // Real detection counts for the Cyber Defense dashboard tiles (the
    // underlying tracker lists are private).
    uint16_t pineScanConfirmedCount() const;
    uint16_t pineScanTrackedCount() const;
    uint16_t multissidConfirmedCount() const;
    uint16_t multissidTrackedCount() const;

    // Real connected-LAN scanner telemetry for the M5SHARK dashboard. One
    // ICMP or TCP check is performed per loop so touch remains responsive.
    bool network_scan_ui_owned = false;
    bool network_scan_running = false;
    uint32_t network_scan_checked = 0;
    uint32_t network_scan_total = 0;
    uint32_t network_scan_hits = 0;
    uint16_t network_scan_target_port = 0;
    uint16_t network_scan_last_ms = 0;
    NetworkScanFinding network_scan_findings[24];
    uint8_t network_scan_finding_count = 0;
    bool isNetworkScannerMode(uint8_t mode) const;
    uint16_t networkServicePort(uint8_t mode) const;
    void setNetworkScanPaused(bool paused);
    void restartNetworkScan();
    void clearNetworkScanResults();
    bool networkScanComplete() const;

    // Custom Beacon List UI telemetry. Frame counters are sourced from the
    // existing esp_wifi_80211_tx loop; the running flag only gates that loop.
    bool beacon_list_ui_owned = false;
    bool beacon_list_running = false;
    uint16_t beacon_list_rate = 0;
    uint16_t beacon_list_peak = 0;
    uint32_t beacon_list_total = 0;
    void setBeaconListRunning(bool running);
    void clearBeaconListStats();

    // Random Beacon Spam telemetry uses the existing packets_sent counter.
    // The running flag only pauses/resumes the existing random-beacon loop.
    bool beacon_spam_ui_owned = false;
    bool beacon_spam_running = false;
    uint16_t beacon_spam_rate = 0;
    uint16_t beacon_spam_peak = 0;
    uint32_t beacon_spam_total = 0;
    void setBeaconSpamRunning(bool running);
    void clearBeaconSpamStats();

    // Probe Request Flood telemetry uses the existing packets_sent counter.
    // The running flag only pauses/resumes the existing probe-attack loop, which
    // targets access points the user selected in-dashboard.
    bool probe_flood_ui_owned = false;
    bool probe_flood_running = false;
    uint16_t probe_flood_rate = 0;
    uint16_t probe_flood_peak = 0;
    uint32_t probe_flood_total = 0;
    void setProbeFloodRunning(bool running);
    void clearProbeFloodStats();

    // Deauth Flood telemetry uses the existing packets_sent counter. The
    // running flag only pauses/resumes the existing deauth-attack loop, which
    // targets access points the user selected in-dashboard.
    bool deauth_flood_ui_owned = false;
    bool deauth_flood_running = false;
    uint16_t deauth_flood_rate = 0;
    uint16_t deauth_flood_peak = 0;
    uint32_t deauth_flood_total = 0;
    void setDeauthFloodRunning(bool running);
    void clearDeauthFloodStats();

    // Generic attack dashboard (R104): one paused-first gate + telemetry set
    // shared by every remaining Wi-Fi transmit attack (AP Clone Spam, CSA,
    // Quiet, targeted deauth, Bad Msg, Assoc Sleep, SAE). Transmission loops
    // keep their own pacing timers, so the one-second telemetry sampler uses
    // its own timestamp instead of initTime.
    bool attack_dash_ui_owned = false;
    bool attack_dash_running = false;
    uint16_t attack_dash_rate = 0;
    uint16_t attack_dash_peak = 0;
    uint32_t attack_dash_total = 0;
    uint32_t attack_dash_stat_ms = 0;
    uint32_t attack_dash_sae_base = 0;
    void setAttackDashRunning(bool running);
    void clearAttackDashStats();
    bool isAttackDashMode(uint8_t scan_mode);

    // R106: in-dashboard target discovery. SCAN runs the Wi-Fi AP (or AP+STA
    // for station dashboards) scan without leaving the dashboard; the run
    // functions suppress their legacy screens while this is set.
    bool attack_dash_scan_active = false;
    uint32_t attack_dash_scan_start = 0;

    // R111 accountability & guardrails: one shared clock for the optional
    // attack duration cap (AttackCap setting, minutes, 0 = off), an
    // append-only SD audit trail for transmit events, and the mode-name
    // helper the trail prints.
    uint32_t attack_run_started = 0;
    bool attack_cap_tripped = false;
    void enforceAttackCap(uint32_t now);
    uint16_t attackCapMinutes();
    void logAttackEvent(const char* event, const char* detail);
    static const char* attackModeName(uint8_t scan_mode);

    // Funny SSID Beacon telemetry and UI access to its existing fixed list.
    bool funny_beacon_ui_owned = false;
    bool funny_beacon_running = false;
    uint16_t funny_beacon_rate = 0;
    uint16_t funny_beacon_peak = 0;
    uint32_t funny_beacon_total = 0;
    void setFunnyBeaconRunning(bool running);
    void clearFunnyBeaconStats();
    uint8_t funnyBeaconCount() const;
    const char* funnyBeaconName(uint8_t index) const;

    // Rick Roll Beacon telemetry and read-only UI access to the existing
    // eight-line SSID set. The running flag only gates the established loop.
    bool rick_roll_ui_owned = false;
    bool rick_roll_running = false;
    uint16_t rick_roll_rate = 0;
    uint16_t rick_roll_peak = 0;
    uint32_t rick_roll_total = 0;
    void setRickRollRunning(bool running);
    void clearRickRollStats();
    uint8_t rickRollCount() const;
    const char* rickRollName(uint8_t index) const;

    // Fox Hunt keeps its selected target while allowing real scan pause/resume.
    bool bt_fox_running = false;

    uint8_t set_channel = 1;

    uint8_t old_channel = 0;

    int16_t _analyzer_value = 0;

    bool orient_display = false;
    bool wifi_initialized = false;
    bool ble_initialized = false;
    bool wifi_connected = false;

    String free_ram = "";
    String old_free_ram = "";
    String connected_network = "";

    IPAddress ip_addr;
    IPAddress gateway;
    IPAddress subnet;

    IPAddress current_scan_ip;

    uint16_t current_scan_port = 1;

    String dst_mac = "ff:ff:ff:ff:ff:ff";
    byte src_mac[6] = {};

    #ifdef HAS_SCREEN
      #if !defined(MARAUDER_CARDPUTER) && !defined(MARAUDER_CARDPUTER_ADV)
        int16_t _analyzer_values[TFT_WIDTH];
        int16_t _temp_analyzer_values[TFT_WIDTH];
      #else
        int16_t _analyzer_values[SCREEN_WIDTH];
        int16_t _temp_analyzer_values[SCREEN_WIDTH];
      #endif
    #endif

    String current_mini_kb_ssid = "";

    const String alfa = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789-=[];',./`\\_+{}:\"<>?~|!@#$%^&*()";

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    #ifndef HAS_IDF_3
      wifi_init_config_t cfg2 = { \
          .event_handler = &esp_event_send_internal, \
          .osi_funcs = &g_wifi_osi_funcs, \
          .wpa_crypto_funcs = g_wifi_default_wpa_crypto_funcs, \
          .static_rx_buf_num = 6,\
          .dynamic_rx_buf_num = 6,\
          .tx_buf_type = 0,\
          .static_tx_buf_num = 1,\
          .dynamic_tx_buf_num = WIFI_DYNAMIC_TX_BUFFER_NUM,\
          .cache_tx_buf_num = 0,\
          .csi_enable = false,\
          .ampdu_rx_enable = false,\
          .ampdu_tx_enable = false,\
          .amsdu_tx_enable = false,\
          .nvs_enable = false,\
          .nano_enable = WIFI_NANO_FORMAT_ENABLED,\
          .rx_ba_win = 6,\
          .wifi_task_core_id = WIFI_TASK_CORE_ID,\
          .beacon_max_len = 752, \
          .mgmt_sbuf_num = 8, \
          .feature_caps = g_wifi_feature_caps, \
          .sta_disconnected_pm = WIFI_STA_DISCONNECTED_PM_ENABLED,  \
          .espnow_max_encrypt_num = 0, \
          .magic = WIFI_INIT_CONFIG_MAGIC\
      };
    #else
      wifi_country_t country = {
        .cc = "PH",
        .schan = 1,
        .nchan = 13,
        .policy = WIFI_COUNTRY_POLICY_AUTO,
      };

      wifi_init_config_t cfg2 = WIFI_INIT_CONFIG_DEFAULT();
    #endif

    wifi_config_t ap_config;

    bool uploadFile(String filePath, bool retry = false, uint8_t upload_type = WIGLE_UPLOAD);
    String checkEmptyProbe(String essid);
    bool checkFlockOUI(const uint8_t mac[6]);
    bool startWiFi(String ssid, String password, bool gui = true);
    bool isFlockCamera(const uint8_t* payload, size_t len, const String& name, String* serial_out);
    bool parseRemoteIDPayload(const uint8_t* payload, size_t len, String* uas_id);
    bool parseRemoteIDBLE(const uint8_t* payload, size_t len, String* uas_id);
    bool parseRemoteIDWiFi(const uint8_t* frame, size_t len, String* uas_id);
    void recordRemoteID(const uint8_t mac[6], int8_t rssi, uint8_t channel, bool via_ble, const String& uas_id);
    void resetRemoteID();
    int seenBLEDevice(BleDevice ble_device);
    uint16_t rssiToColor(int8_t rssi);
    bool isMetaIdentifier(uint16_t id);
    bool isBlockedIdentifier(uint16_t id);
    uint32_t getCompleteEapol(int check_index = -1);
    void drawChannelLine();
    #ifdef HAS_SCREEN
      int8_t checkAnalyzerButtons(uint32_t currentTime);
    #endif
    bool seen_mac(unsigned char* mac, bool simple = true);
    int update_mac_entry(const uint8_t mac[6], int8_t rssi = 0, bool bt = false);
    inline void insert_mac_entry(uint32_t idx, const uint8_t mac[6], uint32_t now_ms, int8_t rssi = 0, bool bt = false);
    void evict_and_insert(const uint8_t mac[6], uint32_t now_ms);
    uint8_t build_top10_for_ui(MacEntry* out_top10, MacSortMode mode);
    void save_mac(unsigned char* mac);
    #ifdef HAS_BT
      void copyNimbleMac(const BLEAddress &addr, unsigned char out[6]);
    #endif
    #ifdef HAS_NIMBLE_2
      bool executeFindMySound(bool gui = false);
    #endif
    bool filterActive();
    bool RunGPSInfo(bool tracker = false, bool display = true, bool poi = false);
    void logPoint(String lat, String lon, float alt, String datetime, bool poi = false);
    void setMac();
    void renderRawStats();
    void renderPacketRate();
    void displayAnalyzerString(String str);
    String security_int_to_string(int security_type);
    void RunSetup();
    int clearList(uint8_t list_type);
    bool addSSID(String essid);
    int generateSSIDs(int count = 20);
    bool shutdownWiFi();
    bool shutdownBLE();
    #ifdef HAS_BT
      void startBluetoothAnalyzer();
      void stopBluetoothAnalyzer();
      void clearBluetoothAnalyzer();
      void startBluetoothSniffer();
      void stopBluetoothSniffer();
      void clearBluetoothSniffer();
      void startFlipperSniffer();
      void stopFlipperSniffer();
      void clearFlipperSniffer();
      void startCardSkimmerDetector();
      void stopCardSkimmerDetector();
      void clearCardSkimmerDetector();
      void recordPassiveBleFinding(String mac, String name, String detail, int rssi);
      void startPassiveBleDetector();
      void stopPassiveBleDetector();
      void clearPassiveBleDetector();
      void startFoxHunt();
      void stopFoxHunt();
      void startSourApple();
      void stopSourApple();
      void clearSourApple();
    #endif
    void startWiFiFoxHunt();
    void stopWiFiFoxHunt();
    bool scanning();
    bool joinWiFi(String ssid, String password, bool gui = true);
    void getMAC(bool get_sta, uint8_t* mac);
    void changeChannel(int chan = -1);
    void RunAPInfo(uint16_t index, bool do_display = true);
    void RunInfo();
    void RunSetMac(uint8_t * mac, bool ap = true);
    void RunGenerateRandomMac(bool ap = true);
    void RunGenerateSSIDs(int count = 20);
    void RunClearSSIDs();
    void RunClearAPs();
    void RunClearStations();
    void RunSaveSSIDList(bool save_as = true);
    void RunLoadSSIDList();
    void RunSaveAPList(bool save_as = true);
    void RunLoadAPList();
    void RunSaveATList(bool save_as = true);
    void RunLoadATList();
    void RunSetupGPSTracker(uint8_t scan_mode);
    void channelHop(bool filtered = false, bool ranged = false);
    uint8_t currentScanMode = 0;

    // Cyber Defense overlays layered on existing scans (set by the menu, reset in
    // StartScan). deauth_alarm: red flood banner on the deauth sniff. evil_twin:
    // duplicate-SSID/different-BSSID warning on the AP scan.
    bool deauth_alarm = false;
    bool evil_twin = false;
    uint32_t deauth_hits = 0;        // deauth/disassoc frames in the current window
    uint32_t deauth_alarm_last = 0;  // last window tick
    bool deauth_alarm_active = false;
    uint16_t deauth_alarm_rate = 0;  // last sampled rate; passive dashboard reads it
    uint32_t evil_twin_last = 0;     // last duplicate-scan tick
    uint16_t evil_twin_count = 0;
    void drawDeauthAlarm(uint32_t currentTime);
    void scanEvilTwins(uint32_t currentTime);

    // IQ Family Watch: a family device (e.g. SharkDeck OS) identifies itself
    // when it joins this device's access point. The web route calls
    // familyBeacon() from the async_tcp task; the menu task consumes the
    // pending flag and shows the full-screen notification.
    volatile bool family_alert_pending = false;
    bool family_alert_showing = false;     // menu task owns the overlay
    bool family_watch_enabled = true;      // "IQ Family Watch" menu toggle
    char family_name[17] = {0};
    int8_t family_rssi = -128;
    uint32_t family_seen_ms = 0;
    uint32_t family_last_alert_ms = 0;     // cooldown between pop-ups
    void familyBeacon(const char* name, int8_t rssi);
    const char* familyProximityWord() const;

    void main(uint32_t currentTime);
    void StartScan(uint8_t scan_mode, uint16_t color = 0);
    void StopScan(uint8_t scan_mode);

    // --- BT crash guard -----------------------------------------------------
    // NimBLE start-up is deferred out of setup() entirely: if the stack
    // aborts during NimBLEDevice::init() on this hardware, that crash used
    // to reboot the device inside setup(), which replayed the splash forever.
    // ensureBLE() brings the stack up on first use, marks the attempt in NVS
    // so a crash locks BLE on the next boot, and auto-releases the lock after
    // five clean boots. bleBootGuard()/bleBootOk() bracket setup().
    bool ble_guard_locked = false;
    bool bleBootGuard();                  // setup() start: consume crash mark
    void bleBootOk();                     // setup() end: one clean boot
    bool ensureBLE(const char* name = "");
    void bleSoftStop();                   // stop scan/adv, keep stack resident
    void setBaseMacAddress(uint8_t macAddr[6]);

    uint16_t poiCount = 0;
    uint32_t gpx_points = 0;   // track points logged this GPS Tracker run
    void tagPOI(const char* label = nullptr);

    bool save_serial = false;
    void startPcap(const char* file_name);
    void startLog(const char* file_name);
    void startGPX(const char* file_name);

    static WiFiEventId_t eventId;
    static String lastClientMAC;
    static String lastClientIP;

    static void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info);
    static bool initMbedtls();
    static int mbedtls_entropy_source(void *data, unsigned char *output, size_t len);
    static bool getSAEACT(const uint8_t *frame, size_t frame_len, uint16_t &group_out, size_t &act_len_out);
    static bool sae_group_sizes(uint16_t group, size_t &scalar_len, size_t &element_len);
    static bool mac_cmp(const uint8_t *a, const uint8_t *b);
    static inline uint16_t le16(const uint8_t *p);
    static void getMAC(char *addr, uint8_t* data, uint16_t offset);
    static void getMAC(uint8_t* mac, const uint8_t* data, uint16_t offset);
    static void beaconSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    static void apSnifferCallbackFull(void* buf, wifi_promiscuous_pkt_type_t type);
    static void eapolSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    static void wifiSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    static void pineScanSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type); // Pineapple
    static int extractPineScanChannel(const uint8_t* payload, int len); // Pineapple
    static void multiSSIDSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type); // MultiSSID
    #ifdef HAS_NIMBLE_2
      static void trackerNotifyCallback(NimBLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify);
    #endif
    static inline uint32_t hash_mac(const uint8_t mac[6]);
};
#endif
