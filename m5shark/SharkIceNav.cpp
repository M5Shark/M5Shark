#include "SharkIceNav.h"

#if defined(HAS_SCREEN) && defined(MARAUDER_V8) && defined(HAS_GPS)

#include <Preferences.h>
#include <SD.h>
#include <WiFi.h>
#include <math.h>
#ifdef HAS_BT
  #include <NimBLEDevice.h>
#endif
#include "Display.h"
#include "GpsInterface.h"
#include "MenuFunctions.h"
#include "SDInterface.h"
#include "SharkTheme.h"
#include "WiFiScan.h"

extern Display display_obj;
extern GpsInterface gps_obj;
extern MenuFunctions menu_function_obj;
extern SDInterface sd_obj;
extern WiFiScan wifi_scan_obj;

SharkIceNav shark_icenav_obj;

namespace {
  constexpr int16_t CONTENT_TOP = STATUS_BAR_WIDTH + 18;
  constexpr int16_t CONTROL_TOP = 284;
  constexpr float EARTH_RADIUS_M = 6371000.0f;

  enum NavPage : uint8_t {
    PAGE_NAV = 0,
    PAGE_COMPASS,
    PAGE_SATS,
    PAGE_ADD_WPT,
    PAGE_RADAR,
    PAGE_DATA,
    PAGE_INFO,
    PAGE_COUNT
  };

  struct NavWaypoint {
    bool valid = false;
    int32_t lat_e6 = 0;
    int32_t lon_e6 = 0;
    String name = "NO TARGET";
    String source = "NONE";
  };

  struct RadioNode {
    uint16_t key = 0;
    int16_t rssi = -127;
    char kind = 'W'; // W = Wi-Fi, B = BLE, G = GPS/GNSS broadcaster.
  };

  struct SensorSnapshot {
    static const uint8_t MAX_NODES = 28;
    RadioNode nodes[MAX_NODES];
    uint8_t nodeCount = 0;
    uint16_t wifiCount = 0;
    uint16_t bleCount = 0;
    uint16_t gpsBroadcasters = 0;
    uint32_t scannedAt = 0;
    bool valid = false;
  };

  float radiansFromDegrees(float degrees) {
    return degrees * (PI / 180.0f);
  }

  // Standard Haversine range and initial great-circle course, extended from
  // the pre-existing M5SHARK Waypoint tool. Coordinates are decimal degrees.
  float distanceMetres(float lat1, float lon1, float lat2, float lon2) {
    const float dLat = radiansFromDegrees(lat2 - lat1);
    const float dLon = radiansFromDegrees(lon2 - lon1);
    const float rLat1 = radiansFromDegrees(lat1);
    const float rLat2 = radiansFromDegrees(lat2);
    const float a = sinf(dLat * 0.5f) * sinf(dLat * 0.5f) +
                    cosf(rLat1) * cosf(rLat2) *
                    sinf(dLon * 0.5f) * sinf(dLon * 0.5f);
    const float c = 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));
    return EARTH_RADIUS_M * c;
  }

  float courseDegrees(float lat1, float lon1, float lat2, float lon2) {
    const float rLat1 = radiansFromDegrees(lat1);
    const float rLat2 = radiansFromDegrees(lat2);
    const float dLon = radiansFromDegrees(lon2 - lon1);
    const float y = sinf(dLon) * cosf(rLat2);
    const float x = cosf(rLat1) * sinf(rLat2) -
                    sinf(rLat1) * cosf(rLat2) * cosf(dLon);
    float result = atan2f(y, x) * (180.0f / PI);
    if (result < 0.0f) result += 360.0f;
    return result;
  }

  float normalizedTurn(float degrees) {
    while (degrees > 180.0f) degrees -= 360.0f;
    while (degrees < -180.0f) degrees += 360.0f;
    return degrees;
  }

  const char* cardinal8(float degrees) {
    static const char* points[8] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    while (degrees < 0.0f) degrees += 360.0f;
    while (degrees >= 360.0f) degrees -= 360.0f;
    return points[(int)((degrees + 22.5f) / 45.0f) & 7];
  }

  String distanceText(float metres) {
    if (metres < 1000.0f) return String(metres, 0) + " m";
    if (metres < 10000.0f) return String(metres / 1000.0f, 2) + " km";
    return String(metres / 1000.0f, 1) + " km";
  }

  bool validCoordinates(float lat, float lon) {
    return isfinite(lat) && isfinite(lon) && lat >= -90.0f && lat <= 90.0f &&
           lon >= -180.0f && lon <= 180.0f;
  }

  uint16_t hashBytes(const uint8_t* bytes, size_t length) {
    uint32_t hash = 2166136261UL;
    for (size_t i = 0; i < length; i++) {
      hash ^= bytes[i];
      hash *= 16777619UL;
    }
    hash ^= hash >> 16;
    return (uint16_t)hash;
  }

  uint16_t hashText(const String& text) {
    return hashBytes((const uint8_t*)text.c_str(), text.length());
  }

  bool looksLikeGpsDevice(String name) {
    name.toUpperCase();
    return name.indexOf("GPS") >= 0 || name.indexOf("GNSS") >= 0 ||
           name.indexOf("NMEA") >= 0 || name.indexOf("NAV") >= 0;
  }

  void addRadioNode(SensorSnapshot& snapshot, uint16_t key, int rssi, char kind) {
    if (snapshot.nodeCount >= SensorSnapshot::MAX_NODES) return;
    RadioNode& node = snapshot.nodes[snapshot.nodeCount++];
    node.key = key;
    node.rssi = constrain(rssi, -127, -20);
    node.kind = kind;
  }

  void scanSensors(SensorSnapshot& snapshot) {
    snapshot = SensorSnapshot();

    const wifi_mode_t previousMode = WiFi.getMode();
    const bool startedWifi = previousMode == WIFI_MODE_NULL;
    const bool addedStation = previousMode == WIFI_MODE_AP;
    if (startedWifi) WiFi.mode(WIFI_STA);
    else if (addedStation) WiFi.mode(WIFI_AP_STA);
    menu_function_obj.setSharkHudActivity(SHARK_HUD_WIFI_ACTIVITY, true);
    const int networks = WiFi.scanNetworks(false, true);
    if (networks > 0) {
      snapshot.wifiCount = networks;
      for (int i = 0; i < networks; i++) {
        const String ssid = WiFi.SSID(i);
        const bool gpsDevice = looksLikeGpsDevice(ssid);
        if (gpsDevice) snapshot.gpsBroadcasters++;
        const uint8_t* bssid = WiFi.BSSID(i);
        const uint16_t key = bssid ? hashBytes(bssid, 6) : hashText(ssid);
        if (i < 14 || gpsDevice)
          addRadioNode(snapshot, key, WiFi.RSSI(i), gpsDevice ? 'G' : 'W');
      }
    }
    WiFi.scanDelete();
    if (startedWifi) WiFi.mode(WIFI_OFF);
    else if (addedStation) WiFi.mode(WIFI_AP);
    menu_function_obj.setSharkHudActivity(SHARK_HUD_WIFI_ACTIVITY, false);

    // Keep receiving the GPS UART between the two blocking radio scans.
    gps_obj.main();

    #ifdef HAS_BT
      const bool bleWasInitialized = NimBLEDevice::isInitialized();
      // The crash guard may refuse BLE here (init previously crashed); the
      // radar then just reports zero BLE nodes instead of rebooting.
      if (!bleWasInitialized) wifi_scan_obj.ensureBLE("");
      if (NimBLEDevice::isInitialized()) {
        menu_function_obj.setSharkHudActivity(SHARK_HUD_BLE_ACTIVITY, true);
        NimBLEScan* scan = NimBLEDevice::getScan();
        if (scan) {
          scan->setActiveScan(true);
          scan->setInterval(97);
          scan->setWindow(37);
          NimBLEScanResults results = scan->getResults(1000, false);
          snapshot.bleCount = results.getCount();
          for (int i = 0; i < results.getCount(); i++) {
            const NimBLEAdvertisedDevice* device = results.getDevice(i);
            if (!device) continue;
            String name = device->getName().length()
                            ? String(device->getName().c_str())
                            : String(device->getAddress().toString().c_str());
            const bool gpsDevice = looksLikeGpsDevice(name);
            if (gpsDevice) snapshot.gpsBroadcasters++;
            if (i < 14 || gpsDevice)
              addRadioNode(snapshot,
                           hashText(String(device->getAddress().toString().c_str())),
                           device->getRSSI(),
                           gpsDevice ? 'G' : 'B');
          }
          scan->clearResults();
        }
        // Stack stays resident even when the radar brought it up: a full
        // deinit asserts inside the C5's ROM controller (see
        // WiFiScan::bleSoftStop). Just leave the scan cleared and warm.
        wifi_scan_obj.ble_initialized = bleWasInitialized;
        wifi_scan_obj.ble_initialized = bleWasInitialized;
      }
      menu_function_obj.setSharkHudActivity(SHARK_HUD_BLE_ACTIVITY, false);
    #endif

    snapshot.scannedAt = millis();
    snapshot.valid = true;
  }

  String boundedName(String value) {
    value.trim();
    if (!value.length()) value = "GPX TARGET";
    if (value.length() > 18) value.remove(18);
    return value;
  }

  bool loadSavedWaypoint(NavWaypoint& waypoint) {
    Preferences prefs;
    if (!prefs.begin("shark_nav", true)) return false;
    const bool saved = prefs.getBool("valid", false);
    if (saved) {
      waypoint.valid = true;
      waypoint.lat_e6 = prefs.getInt("lat_e6", 0);
      waypoint.lon_e6 = prefs.getInt("lon_e6", 0);
      waypoint.name = boundedName(prefs.getString("name", "MARKED POINT"));
      waypoint.source = "SAVED";
    }
    prefs.end();
    return saved;
  }

  void saveWaypoint(const NavWaypoint& waypoint) {
    Preferences prefs;
    if (!prefs.begin("shark_nav", false)) return;
    prefs.putBool("valid", waypoint.valid);
    prefs.putInt("lat_e6", waypoint.lat_e6);
    prefs.putInt("lon_e6", waypoint.lon_e6);
    prefs.putString("name", waypoint.name);
    prefs.end();
  }

  bool appendWaypointLog(const NavWaypoint& waypoint) {
    if (!sd_obj.supported) return false;
    if (!SD.exists("/WPT") && !SD.mkdir("/WPT")) return false;
    const char* path = "/WPT/m5shark-waypoints.csv";
    const bool newFile = !SD.exists(path);
    File file = SD.open(path, FILE_APPEND);
    if (!file) return false;
    if (newFile) file.println("name,latitude,longitude,utc");
    String utc = gps_obj.getDatetime();
    utc.replace(',', ' ');
    file.print(waypoint.name);
    file.print(',');
    file.print(waypoint.lat_e6 / 1000000.0f, 6);
    file.print(',');
    file.print(waypoint.lon_e6 / 1000000.0f, 6);
    file.print(',');
    file.println(utc);
    file.close();
    return true;
  }

  int attributeStart(const String& text, const char* attribute, int from) {
    String doubleQuoted = String(attribute) + "=\"";
    int at = text.indexOf(doubleQuoted, from);
    if (at >= 0) return at + doubleQuoted.length();
    String singleQuoted = String(attribute) + "='";
    at = text.indexOf(singleQuoted, from);
    return at < 0 ? -1 : at + singleQuoted.length();
  }

  bool loadGpxWaypoint(NavWaypoint& waypoint) {
    if (!sd_obj.supported || !SD.exists("/WPT/icenav.gpx")) return false;
    File file = SD.open("/WPT/icenav.gpx", FILE_READ);
    if (!file) return false;

    // Only the first waypoint is needed; cap memory use on the ESP32-C5.
    String document;
    document.reserve(4096);
    while (file.available() && document.length() < 4096)
      document += (char)file.read();
    file.close();

    const int wpt = document.indexOf("<wpt");
    if (wpt < 0) return false;
    const int latStart = attributeStart(document, "lat", wpt);
    const int lonStart = attributeStart(document, "lon", wpt);
    if (latStart < 0 || lonStart < 0) return false;
    const char latQuote = document.charAt(latStart - 1);
    const char lonQuote = document.charAt(lonStart - 1);
    const int latEnd = document.indexOf(latQuote, latStart);
    const int lonEnd = document.indexOf(lonQuote, lonStart);
    if (latEnd < 0 || lonEnd < 0) return false;

    const float lat = document.substring(latStart, latEnd).toFloat();
    const float lon = document.substring(lonStart, lonEnd).toFloat();
    if (!validCoordinates(lat, lon)) return false;

    String name = "GPX TARGET";
    const int wptEnd = document.indexOf("</wpt>", wpt);
    const int nameStartTag = document.indexOf("<name>", wpt);
    if (nameStartTag >= 0 && (wptEnd < 0 || nameStartTag < wptEnd)) {
      const int nameStart = nameStartTag + 6;
      const int nameEnd = document.indexOf("</name>", nameStart);
      if (nameEnd > nameStart) name = document.substring(nameStart, nameEnd);
    }

    waypoint.valid = true;
    waypoint.lat_e6 = (int32_t)lroundf(lat * 1000000.0f);
    waypoint.lon_e6 = (int32_t)lroundf(lon * 1000000.0f);
    waypoint.name = boundedName(name);
    waypoint.source = "SD GPX";
    return true;
  }

  void drawHeader(const char* page) {
    TFT_eSPI& tft = display_obj.tft;
    menu_function_obj.drawStatusBar();
    const int16_t y = STATUS_BAR_WIDTH + 1;
    tft.fillRect(0, y, tft.width(), 17, TFT_BLACK);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString(String("// ICENAV / ") + page, 6, y + 8, 2);
    tft.drawFastHLine(0, y + 16, tft.width(), WD_EDGE);
    tft.setTextDatum(TL_DATUM);
  }

  void drawControls(uint8_t page) {
    TFT_eSPI& tft = display_obj.tft;
    static const char* const nextLabels[PAGE_COUNT] = {
      "COMP >", "SATS >", "WPT >", "RADAR >",
      "DATA >", "INFO >", "NAV >"
    };
    const char* nextPage = page < PAGE_COUNT ? nextLabels[page] : "NAV >";
    const char* labels[3] = {"MARK", nextPage, "EXIT"};
    for (uint8_t i = 0; i < 3; i++) {
      const int16_t x = i * 80;
      const uint16_t fill = i == 2 ? WD_PANEL : WD_SURFACE;
      const uint16_t accent = i == 2 ? WD_AMBER : WD_CYAN;
      wdPanel(tft, x + 1, CONTROL_TOP, 78, 35, fill, WD_EDGE, accent);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(i == 2 ? WD_AMBER : WD_BONE, fill);
      tft.drawString(labels[i], x + 40, CONTROL_TOP + 18, 2);
    }
    tft.setTextDatum(TL_DATUM);
  }

  void drawArrow(TFT_eSPI& tft, int16_t cx, int16_t cy, int16_t radius, float degrees) {
    const float angle = radiansFromDegrees(degrees);
    const int16_t tipX = cx + (int16_t)(sinf(angle) * radius);
    const int16_t tipY = cy - (int16_t)(cosf(angle) * radius);
    const int16_t tailX = cx - (int16_t)(sinf(angle) * 20.0f);
    const int16_t tailY = cy + (int16_t)(cosf(angle) * 20.0f);
    const float leftAngle = angle - 0.42f;
    const float rightAngle = angle + 0.42f;
    const int16_t leftX = tipX - (int16_t)(sinf(leftAngle) * 19.0f);
    const int16_t leftY = tipY + (int16_t)(cosf(leftAngle) * 19.0f);
    const int16_t rightX = tipX - (int16_t)(sinf(rightAngle) * 19.0f);
    const int16_t rightY = tipY + (int16_t)(cosf(rightAngle) * 19.0f);
    tft.drawLine(tailX, tailY, tipX, tipY, WD_CYAN);
    tft.fillTriangle(tipX, tipY, leftX, leftY, rightX, rightY, WD_CYAN);
    tft.fillCircle(cx, cy, 4, WD_WHITE);
  }

  void drawNavPage(const NavWaypoint& waypoint) {
    TFT_eSPI& tft = display_obj.tft;
    tft.fillRect(0, CONTENT_TOP, tft.width(), CONTROL_TOP - CONTENT_TOP, TFT_BLACK);

    const bool fix = gps_obj.getFixStatus();
    if (!fix) {
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(WD_AMBER, TFT_BLACK);
      tft.drawString("ACQUIRING GPS FIX", 120, 116, 2);
      tft.setTextColor(WD_DIM, TFT_BLACK);
      tft.drawString(String(gps_obj.getNumSats()) + " SATELLITES", 120, 142, 2);
      tft.drawString("MOVE OUTDOORS / CLEAR SKY", 120, 174, 1);
      tft.setTextDatum(TL_DATUM);
      return;
    }
    if (!waypoint.valid) {
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(WD_CYAN, TFT_BLACK);
      tft.drawString("NO DESTINATION", 120, 112, 2);
      tft.setTextColor(WD_BONE, TFT_BLACK);
      tft.drawString("TOUCH MARK TO SAVE", 120, 142, 2);
      tft.drawString("YOUR CURRENT POSITION", 120, 162, 2);
      tft.setTextColor(WD_DIM, TFT_BLACK);
      tft.drawString("OR ADD /WPT/icenav.gpx", 120, 202, 1);
      tft.setTextDatum(TL_DATUM);
      return;
    }

    const float lat = gps_obj.getLatInt() / 1000000.0f;
    const float lon = gps_obj.getLonInt() / 1000000.0f;
    const float targetLat = waypoint.lat_e6 / 1000000.0f;
    const float targetLon = waypoint.lon_e6 / 1000000.0f;
    const float distance = distanceMetres(lat, lon, targetLat, targetLon);
    const float bearing = courseDegrees(lat, lon, targetLat, targetLon);
    const float course = gps_obj.getCourseDeg();
    const bool moving = gps_obj.getSpeedKmph() >= 1.0f;
    const float arrowAngle = moving ? normalizedTurn(bearing - course) : bearing;

    tft.drawCircle(76, 126, 58, WD_EDGE);
    tft.drawCircle(76, 126, 47, WD_PANEL);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WD_DIM, TFT_BLACK);
    tft.drawString("N", 76, 58, 1);
    tft.drawString("S", 76, 194, 1);
    tft.drawString("W", 10, 126, 1);
    tft.drawString("E", 142, 126, 1);
    drawArrow(tft, 76, 126, 43, arrowAngle);

    wdPanel(tft, 148, 52, 88, 145, WD_PANEL, WD_EDGE, WD_CYAN);
    tft.setTextColor(WD_DIM, WD_PANEL);
    tft.drawString("DISTANCE", 192, 66, 1);
    tft.setTextColor(WD_WHITE, WD_PANEL);
    tft.drawString(distanceText(distance), 192, 87, 2);
    tft.setTextColor(WD_DIM, WD_PANEL);
    tft.drawString("BEARING", 192, 112, 1);
    tft.setTextColor(WD_CYAN, WD_PANEL);
    tft.drawString(String(bearing, 0) + "  " + cardinal8(bearing), 192, 133, 2);
    tft.setTextColor(WD_DIM, WD_PANEL);
    tft.drawString(moving ? "TURN" : "ABSOLUTE", 192, 157, 1);
    tft.setTextColor(WD_BONE, WD_PANEL);
    if (moving) {
      String turn = fabsf(arrowAngle) < 6.0f ? "STRAIGHT" :
                    String(arrowAngle < 0.0f ? "L " : "R ") + String(fabsf(arrowAngle), 0);
      tft.drawString(turn, 192, 177, 2);
    } else {
      tft.drawString("NORTH UP", 192, 177, 1);
    }

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString(waypoint.name, 8, 211, 2);
    tft.setTextColor(WD_DIM, TFT_BLACK);
    tft.drawString(waypoint.source + "  //  " + String(gps_obj.getNumSats()) + " SAT", 8, 231, 1);
    tft.setTextColor(WD_BONE, TFT_BLACK);
    tft.drawString(String(gps_obj.getSpeedKmph(), 1) + " km/h", 8, 252, 2);
    if (gps_obj.getSpeedKmph() >= 1.0f) {
      const uint32_t etaSeconds = (uint32_t)(distance / (gps_obj.getSpeedKmph() / 3.6f));
      const String eta = String("ETA ") + (etaSeconds / 60) + "m " + (etaSeconds % 60) + "s";
      tft.setTextDatum(TR_DATUM);
      tft.drawString(eta, 232, 252, 2);
    }
    tft.setTextDatum(TL_DATUM);
  }

  void drawRadioSummary(TFT_eSPI& tft, const SensorSnapshot& sensors, int16_t y) {
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString(String("W ") + sensors.wifiCount, 8, y, 1);
    tft.setTextColor(WD_BONE, TFT_BLACK);
    tft.drawString(String("B ") + sensors.bleCount, 67, y, 1);
    tft.setTextColor(WD_AMBER, TFT_BLACK);
    tft.drawString(String("G ") + sensors.gpsBroadcasters, 126, y, 1);
    tft.setTextColor(gps_obj.getFixStatus() ? WD_CYAN : WD_DIM, TFT_BLACK);
    tft.drawString(String("SAT ") + gps_obj.getNumSats(), 183, y, 1);
    tft.setTextDatum(TL_DATUM);
  }

  void drawCompassPage() {
    TFT_eSPI& tft = display_obj.tft;
    tft.fillRect(0, CONTENT_TOP, tft.width(), CONTROL_TOP - CONTENT_TOP, TFT_BLACK);
    const float course = gps_obj.getCourseDeg();
    const bool moving = gps_obj.getFixStatus() && gps_obj.getSpeedKmph() >= 1.0f;
    const int16_t cx = 120, cy = 145, radius = 78;
    tft.drawCircle(cx, cy, radius, WD_EDGE);
    tft.drawCircle(cx, cy, radius - 12, WD_PANEL);
    for (uint8_t i = 0; i < 12; i++) {
      const float angle = radiansFromDegrees(i * 30.0f);
      const int16_t x1 = cx + (int16_t)(sinf(angle) * (radius - 7));
      const int16_t y1 = cy - (int16_t)(cosf(angle) * (radius - 7));
      const int16_t x2 = cx + (int16_t)(sinf(angle) * radius);
      const int16_t y2 = cy - (int16_t)(cosf(angle) * radius);
      tft.drawLine(x1, y1, x2, y2, i % 3 == 0 ? WD_BONE : WD_DIM);
    }
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WD_CYAN, TFT_BLACK); tft.drawString("N", cx, cy - 91, 2);
    tft.setTextColor(WD_BONE, TFT_BLACK); tft.drawString("E", cx + 91, cy, 2);
    tft.drawString("S", cx, cy + 91, 2); tft.drawString("W", cx - 91, cy, 2);
    // No magnetometer on this hardware: a still device has no heading to show.
    // Only draw the live needle while actually moving, so a stale course value
    // never masquerades as a working compass.
    if (moving) {
      drawArrow(tft, cx, cy, 56, course);
    } else {
      tft.drawCircle(cx, cy, 6, WD_DIM);
      tft.fillCircle(cx, cy, 2, WD_DIM);
    }
    tft.setTextColor(moving ? WD_CYAN : WD_AMBER, TFT_BLACK);
    const bool fix = gps_obj.getFixStatus();
    const String readout = moving
        ? (String(course, 0) + "  " + cardinal8(course))
        : (fix ? String("STAND STILL") : String("NO FIX"));
    tft.drawString(readout, cx, 249, 2);
    tft.setTextColor(WD_DIM, TFT_BLACK);
    tft.drawString(moving ? "GPS COURSE OVER GROUND"
                          : (fix ? "MOVE >1 km/h TO UPDATE COURSE"
                                 : "WAITING FOR SATELLITE FIX"),
                   cx, 269, 1);
    tft.setTextDatum(TL_DATUM);
  }

  void drawAddWaypointPage(const NavWaypoint& waypoint) {
    TFT_eSPI& tft = display_obj.tft;
    tft.fillRect(0, CONTENT_TOP, tft.width(), CONTROL_TOP - CONTENT_TOP, TFT_BLACK);
    const bool fix = gps_obj.getFixStatus();
    wdPanel(tft, 8, 48, 224, 54, WD_PANEL, WD_EDGE, fix ? WD_CYAN : WD_AMBER);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(fix ? WD_CYAN : WD_AMBER, WD_PANEL);
    tft.drawString(fix ? "POSITION READY" : "WAITING FOR GPS FIX", 120, 66, 2);
    tft.setTextColor(WD_DIM, WD_PANEL);
    tft.drawString(String(gps_obj.getNumSats()) + " SATELLITES", 120, 87, 1);

    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(WD_DIM, TFT_BLACK); tft.drawString("LAT", 10, 124, 1);
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(WD_BONE, TFT_BLACK); tft.drawString(gps_obj.getLat(), 230, 124, 2);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(WD_DIM, TFT_BLACK); tft.drawString("LON", 10, 154, 1);
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(WD_BONE, TFT_BLACK); tft.drawString(gps_obj.getLon(), 230, 154, 2);
    tft.drawFastHLine(8, 173, 224, WD_EDGE);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString("PRESS MARK BELOW TO SAVE", 120, 197, 2);
    tft.setTextColor(WD_DIM, TFT_BLACK);
    tft.drawString("NVS + /WPT/m5shark-waypoints.csv", 120, 220, 1);
    tft.setTextColor(waypoint.valid ? WD_BONE : WD_DIM, TFT_BLACK);
    tft.drawString(waypoint.valid ? String("ACTIVE: ") + waypoint.name : "NO ACTIVE WAYPOINT", 120, 249, 2);
    tft.setTextColor(WD_DIM, TFT_BLACK);
    tft.drawString("GPX TARGET STILL LOADS ON NEXT START", 120, 271, 1);
    tft.setTextDatum(TL_DATUM);
  }

  void drawNodeIcon(TFT_eSPI& tft, int16_t x, int16_t y, char kind, uint16_t color) {
    tft.fillCircle(x, y, 6, TFT_BLACK);
    tft.drawCircle(x, y, 6, color);
    if (kind == 'W') {
      tft.drawLine(x - 4, y - 2, x, y - 5, color);
      tft.drawLine(x, y - 5, x + 4, y - 2, color);
      tft.drawLine(x - 2, y + 1, x, y - 1, color);
      tft.drawLine(x, y - 1, x + 2, y + 1, color);
      tft.fillCircle(x, y + 3, 1, color);
    } else if (kind == 'B') {
      tft.drawFastVLine(x, y - 4, 9, color);
      tft.drawLine(x, y - 4, x + 3, y - 1, color);
      tft.drawLine(x + 3, y - 1, x - 3, y + 3, color);
      tft.drawLine(x - 3, y - 3, x + 3, y + 2, color);
      tft.drawLine(x + 3, y + 2, x, y + 4, color);
    } else {
      tft.drawCircle(x, y - 1, 3, color);
      tft.fillCircle(x, y - 1, 1, color);
      tft.fillTriangle(x - 2, y + 2, x + 2, y + 2, x, y + 5, color);
    }
  }

  void drawSensorRadarPage(const SensorSnapshot& sensors) {
    TFT_eSPI& tft = display_obj.tft;
    tft.fillRect(0, CONTENT_TOP, tft.width(), CONTROL_TOP - CONTENT_TOP, TFT_BLACK);
    const int16_t cx = 120, cy = 145, radius = 82;
    for (int16_t r = radius; r > 0; r -= 27) tft.drawCircle(cx, cy, r, WD_EDGE);
    tft.drawFastHLine(cx - radius, cy, radius * 2, WD_PANEL);
    tft.drawFastVLine(cx, cy - radius, radius * 2, WD_PANEL);
    for (uint8_t i = 0; i < sensors.nodeCount; i++) {
      const RadioNode& node = sensors.nodes[i];
      const float angle = (node.key / 65535.0f) * 2.0f * PI;
      const float proximity = constrain((float)(-30 - node.rssi) / 75.0f, 0.12f, 1.0f);
      const int16_t r = (int16_t)(proximity * radius);
      const int16_t x = cx + (int16_t)(cosf(angle) * r);
      const int16_t y = cy + (int16_t)(sinf(angle) * r);
      const uint16_t color = node.kind == 'W' ? WD_CYAN : node.kind == 'B' ? WD_BONE : WD_AMBER;
      drawNodeIcon(tft, x, y, node.kind, color);
    }
    tft.fillCircle(cx, cy, 5, gps_obj.getFixStatus() ? WD_CYAN : WD_AMBER);
    tft.drawCircle(cx, cy, 9, WD_BONE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WD_DIM, TFT_BLACK);
    tft.drawString(sensors.valid ? "PASSIVE SENSOR SWEEP" : "SCANNING RADIOS...", 120, 50, 1);
    drawRadioSummary(tft, sensors, 244);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WD_DIM, TFT_BLACK);
    tft.drawString("ICONS = RSSI PROXIMITY / NOT MAP POSITIONS", 120, 265, 1);
    tft.setTextColor(WD_AMBER, TFT_BLACK);
    tft.drawString("G = GPS/GNSS NAME BROADCAST", 120, 276, 1);
    tft.setTextDatum(TL_DATUM);
  }

  void drawDataPage() {
    TFT_eSPI& tft = display_obj.tft;
    tft.fillRect(0, CONTENT_TOP, tft.width(), CONTROL_TOP - CONTENT_TOP, TFT_BLACK);
    const bool fix = gps_obj.getFixStatus();
    const uint16_t fixColor = fix ? WD_CYAN : WD_AMBER;
    wdPanel(tft, 6, 46, 228, 35, WD_PANEL, WD_EDGE, fixColor);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(fixColor, WD_PANEL);
    tft.drawString(fix ? "GPS FIX LOCKED" : "WAITING FOR GPS FIX", 15, 63, 2);
    tft.setTextDatum(MR_DATUM);
    tft.drawString(String(gps_obj.getNumSats()) + " SAT", 226, 63, 2);

    const char* labels[7] = {"LATITUDE", "LONGITUDE", "ALTITUDE", "SPEED", "COURSE", "ACCURACY", "UTC"};
    String values[7] = {
      gps_obj.getLat(),
      gps_obj.getLon(),
      String(gps_obj.getAlt(), 1) + " m",
      String(gps_obj.getSpeedKmph(), 1) + " km/h",
      String(gps_obj.getCourseDeg(), 0) + "  " + cardinal8(gps_obj.getCourseDeg()),
      String(gps_obj.getAccuracy(), 1) + " m",
      gps_obj.getDatetime()
    };
    tft.setTextDatum(ML_DATUM);
    for (uint8_t i = 0; i < 7; i++) {
      const int16_t y = 94 + i * 26;
      tft.setTextColor(WD_DIM, TFT_BLACK);
      tft.drawString(labels[i], 8, y, 1);
      tft.setTextDatum(MR_DATUM);
      tft.setTextColor(i < 2 ? WD_CYAN : WD_BONE, TFT_BLACK);
      String value = values[i];
      while (value.length() > 4 && tft.textWidth(value, 2) > 148)
        value.remove(value.length() - 1);
      tft.drawString(value, 232, y, 2);
      tft.drawFastHLine(7, y + 13, 226, WD_PANEL);
      tft.setTextDatum(ML_DATUM);
    }
    tft.setTextDatum(TL_DATUM);
  }

  void drawSatellitePage() {
    TFT_eSPI& tft = display_obj.tft;
    tft.fillRect(0, CONTENT_TOP, tft.width(), CONTROL_TOP - CONTENT_TOP, TFT_BLACK);
    const int satellites = constrain(gps_obj.getNumSats(), 0, 24);
    const bool fix = gps_obj.getFixStatus();

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(fix ? WD_CYAN : WD_AMBER, TFT_BLACK);
    tft.drawString(String(satellites), 120, 86, 4);
    tft.setTextColor(WD_BONE, TFT_BLACK);
    tft.drawString(fix ? "SATELLITES / FIX LOCKED" : "SATELLITES / ACQUIRING", 120, 116, 2);

    wdPanel(tft, 9, 132, 222, 80, WD_PANEL, WD_EDGE, fix ? WD_CYAN : WD_AMBER);
    for (uint8_t i = 0; i < 24; i++) {
      const int16_t col = i % 12;
      const int16_t row = i / 12;
      const int16_t x = 21 + col * 18;
      const int16_t y = 148 + row * 36;
      const bool active = i < satellites;
      const uint16_t color = active ? (fix ? WD_CYAN : WD_AMBER) : WD_EDGE;
      tft.drawCircle(x, y, 5, color);
      if (active) tft.fillCircle(x, y, 2, color);
      tft.setTextColor(active ? WD_BONE : WD_DIM, WD_PANEL);
      tft.drawString(String(i + 1), x, y + 11, 1);
    }
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(WD_DIM, TFT_BLACK);
    tft.drawString("ACCURACY", 9, 225, 1);
    tft.setTextColor(WD_BONE, TFT_BLACK);
    tft.drawString(String(gps_obj.getAccuracy(), 1) + " m", 72, 225, 2);
    tft.setTextColor(WD_DIM, TFT_BLACK);
    tft.drawString("ALT", 150, 225, 1);
    tft.setTextColor(WD_BONE, TFT_BLACK);
    tft.drawString(String(gps_obj.getAlt(), 1) + " m", 178, 225, 2);
    tft.setTextColor(WD_DIM, TFT_BLACK);
    tft.drawString("UTC", 9, 249, 1);
    tft.setTextColor(WD_BONE, TFT_BLACK);
    String utc = gps_obj.getDatetime();
    while (utc.length() > 4 && tft.textWidth(utc, 1) > 190) utc.remove(utc.length() - 1);
    tft.drawString(utc.length() ? utc : "WAITING FOR TIME", 39, 249, 1);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WD_DIM, TFT_BLACK);
    tft.drawString("COUNT VIEW / NO INVENTED SKY POSITIONS", 120, 273, 1);
    tft.setTextDatum(TL_DATUM);
  }

  void drawInfoPage() {
    TFT_eSPI& tft = display_obj.tft;
    tft.fillRect(0, CONTENT_TOP, tft.width(), CONTROL_TOP - CONTENT_TOP, TFT_BLACK);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WD_CYAN, TFT_BLACK);
    tft.drawString("ICENAV SENSOR SUITE", 120, 51, 2);
    tft.setTextColor(WD_DIM, TFT_BLACK);
    tft.drawString("WHAT THIS TOOL DOES", 120, 68, 1);

    struct InfoRow {
      const char* title;
      const char* detail;
      uint16_t accent;
    };
    const InfoRow rows[4] = {
      {"NAVIGATION", "real range, bearing, turn and ETA", WD_CYAN},
      {"COMPASS + SATS", "GPS course, fix and satellite count", WD_BONE},
      {"ADD WPT", "NVS + SD CSV position saving", WD_CYAN},
      {"SENSOR RADAR", "passive Wi-Fi / BLE / GPS-name scan", WD_BONE}
    };

    for (uint8_t i = 0; i < 4; i++) {
      const int16_t y = 82 + i * 46;
      wdPanel(tft, 8, y, 224, 40, WD_PANEL, WD_EDGE, rows[i].accent);
      tft.setTextDatum(ML_DATUM);
      tft.setTextColor(rows[i].accent, WD_PANEL);
      tft.drawString(rows[i].title, 17, y + 12, 2);
      tft.setTextColor(WD_DIM, WD_PANEL);
      tft.drawString(rows[i].detail, 17, y + 28, 1);
    }

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WD_AMBER, TFT_BLACK);
    tft.drawString("NO INTERNET / RADIO ICONS ARE RSSI-ONLY", 120, 271, 1);
    tft.setTextDatum(TL_DATUM);
  }

  void message(const char* text, uint16_t color) {
    TFT_eSPI& tft = display_obj.tft;
    tft.fillRect(8, 246, 224, 30, TFT_BLACK);
    tft.drawRect(8, 246, 224, 30, color);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(color, TFT_BLACK);
    tft.drawString(text, 120, 261, 2);
    tft.setTextDatum(TL_DATUM);
  }

  void waitTouchRelease() {
    uint16_t x, y;
    while (display_obj.updateTouch(&x, &y)) delay(10);
  }
}

void SharkIceNav::runPage(uint8_t initialPage) {
  TFT_eSPI& tft = display_obj.tft;
  NavWaypoint waypoint;
  loadSavedWaypoint(waypoint);
  loadGpxWaypoint(waypoint); // An SD GPX destination intentionally overrides NVS.

  SensorSnapshot sensors;

  static const char* const pageNames[PAGE_COUNT] = {
    "NAV", "COMPASS", "SAT INFO", "ADD WPT",
    "RADAR", "DATA", "INFO"
  };

  uint8_t page = initialPage < PAGE_COUNT ? initialPage : PAGE_NAV;
  bool redrawChrome = true;
  uint32_t lastDraw = 0;

  tft.fillScreen(TFT_BLACK);
  while (true) {
    gps_obj.main();

    uint16_t touchX, touchY;
    if (display_obj.updateTouch(&touchX, &touchY) && touchY >= CONTROL_TOP) {
      waitTouchRelease();
      if (touchX < 80) {
        if (gps_obj.getFixStatus()) {
          waypoint.valid = true;
          waypoint.lat_e6 = gps_obj.getLatInt();
          waypoint.lon_e6 = gps_obj.getLonInt();
          waypoint.name = "MARKED POINT";
          waypoint.source = "SAVED";
          saveWaypoint(waypoint);
          const bool savedToSd = appendWaypointLog(waypoint);
          message(savedToSd ? "WPT SAVED: NVS + SD" : "WPT SAVED TO NVS", WD_CYAN);
        } else {
          message("WAIT FOR GPS FIX", WD_AMBER);
        }
        delay(650);
        lastDraw = 0;
      } else if (touchX < 160) {
        page = (page + 1) % PAGE_COUNT;
        redrawChrome = true;
      } else {
        break;
      }
    }

    if (redrawChrome) {
      tft.fillScreen(TFT_BLACK);
      drawHeader(pageNames[page]);
      drawControls(page);
      redrawChrome = false;
      lastDraw = 0;
    }

    const bool sensorPage = page == PAGE_RADAR;
    if (sensorPage && (!sensors.valid || millis() - sensors.scannedAt >= 15000)) {
      message("PASSIVE RADIO SCAN", WD_AMBER);
      scanSensors(sensors);
      lastDraw = 0;
    }

    if (millis() - lastDraw >= 400 || lastDraw == 0) {
      switch (page) {
        case PAGE_NAV:      drawNavPage(waypoint); break;
        case PAGE_COMPASS:  drawCompassPage(); break;
        case PAGE_SATS:     drawSatellitePage(); break;
        case PAGE_ADD_WPT:  drawAddWaypointPage(waypoint); break;
        case PAGE_RADAR:    drawSensorRadarPage(sensors); break;
        case PAGE_DATA:     drawDataPage(); break;
        default:            drawInfoPage(); break;
      }
      // Keep the live GPS lamp above the page redraw and change amber->cyan as
      // soon as a genuine fix arrives while IceNav is open.
      menu_function_obj.updateStatusBar();
      lastDraw = millis();
    }
    delay(20);
  }
  tft.setTextDatum(TL_DATUM);
}

void SharkIceNav::run()               { runPage(PAGE_NAV); }
void SharkIceNav::runCompass()        { runPage(PAGE_COMPASS); }
void SharkIceNav::runSatelliteInfo()  { runPage(PAGE_SATS); }
void SharkIceNav::runAddWaypoint()    { runPage(PAGE_ADD_WPT); }
void SharkIceNav::runSensorRadar()    { runPage(PAGE_RADAR); }

#endif
