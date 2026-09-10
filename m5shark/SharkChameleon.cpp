#include "SharkChameleon.h"

#if defined(HAS_SCREEN) && defined(MARAUDER_V8) && defined(HAS_BT)

#include <NimBLEDevice.h>
#include "SharkTheme.h"
#include "Display.h"
#include "MenuFunctions.h"
#include "WiFiScan.h"

extern Display display_obj;
extern MenuFunctions menu_function_obj;
extern WiFiScan wifi_scan_obj;

SharkChameleon shark_chameleon_obj;

namespace {
  // ---- Chameleon Ultra protocol ------------------------------------------------
  // Current CU firmware (verified from RfidResearchGroup/ChameleonUltra
  // ble_main.c) carries the command channel on the Nordic UART Service:
  //   service 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
  //   RX (we write)     ...0002...
  //   TX (it notifies)  ...0003...
  // The old 16-bit 0xC760/61/62 set was pre-DFU firmware and is kept only
  // as a fallback probe.
  constexpr uint8_t  CU_SOF = 0x11;
  const NimBLEUUID CU_SVC_NUS("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
  const NimBLEUUID CU_CHR_WRITE_NUS("6e400002-b5a3-f393-e0a9-e50e24dcca9e");
  const NimBLEUUID CU_CHR_NOTIFY_NUS("6e400003-b5a3-f393-e0a9-e50e24dcca9e");
  constexpr uint16_t CU_SVC_LEGACY = 0xC760;
  constexpr uint16_t CU_CHR_WRITE_LEGACY = 0xC762;
  constexpr uint16_t CU_CHR_NOTIFY_LEGACY = 0xC761;

  constexpr uint16_t CU_GET_APP_VERSION = 0x03E8;
  constexpr uint16_t CU_CHANGE_MODE     = 0x03E9;  // 0 emulator / 1 reader
  constexpr uint16_t CU_GET_MODE        = 0x03EA;
  constexpr uint16_t CU_SET_ACTIVE_SLOT = 0x03EB;  // slot 0..7
  constexpr uint16_t CU_SET_SLOT_TAG_TYPE = 0x03EC; // slot + tag_type BE16
  constexpr uint16_t CU_SET_SLOT_ENABLE  = 0x03EE;  // slot + sense + enable
  constexpr uint16_t CU_GET_ACTIVE_SLOT = 0x03FA;
  constexpr uint16_t CU_GET_BATTERY     = 0x0401;
  constexpr uint16_t CU_HF14A_SCAN      = 0x07D0;
  constexpr uint16_t CU_EM410X_SCAN     = 0x0BB8;

  // From the CU firmware's app_status.h / tag_base_type.h.
  constexpr uint16_t CU_STATUS_SUCCESS = 0x68;      // NOT 0x00 - this is why
                                                   // battery/mode showed "-"
  constexpr uint8_t  CU_SENSE_LF = 1, CU_SENSE_HF = 2;
  constexpr uint16_t CU_TAG_EM410X = 100;           // TAG_TYPE_EM410X
  constexpr uint16_t CU_TAG_MF1_1K = 1001;          // TAG_TYPE_MIFARE_1024

  inline bool cuOk(uint16_t st) { return st == CU_STATUS_SUCCESS || st == 0; }

  // ---- session state -------------------------------------------------------------
  NimBLEClient* cu_client = nullptr;
  NimBLERemoteCharacteristic* cu_write = nullptr;
  volatile bool cu_connected = false;
  volatile int  cu_fail_reason = 0;      // 0 none, 1 not found, 2 connect, 3 no service, 4 subscribe

  uint8_t  cu_rx[600];
  uint16_t cu_rx_len = 0;
  volatile bool cu_frame_ready = false;
  uint16_t cu_last_cmd = 0;
  uint16_t cu_last_status = 0xFFFF;
  uint16_t cu_last_len = 0;
  uint8_t  cu_last_data[520];

  // Nearby-BLE diagnostic names (deduped, most recent first). Unnamed
  // devices are shown by address so the operator always sees the air.
  String nearby[6];
  uint8_t nearby_n = 0;
  void noteNearby(const String& name, const NimBLEAdvertisedDevice* d) {
    String shown = name.length() ? name
                                 : String(d->getAddress().toString().c_str());
    for (uint8_t i = 0; i < nearby_n; i++)
      if (nearby[i] == shown) return;
    if (nearby_n < 6) nearby[nearby_n++] = shown;
    else { for (uint8_t i = 1; i < 6; i++) nearby[i-1] = nearby[i]; nearby[5] = shown; }
  }

  // A Chameleon matches by NAME or by an advertised service UUID from either
  // generation: NUS 128-bit (current firmware) or 16-bit 0xC760 (legacy).
  bool looksLikeChameleon(const NimBLEAdvertisedDevice* d) {
    String name = String(d->getName().c_str());
    String lname = name; lname.toLowerCase();
    if (lname.indexOf("chameleon") >= 0) return true;
    for (uint8_t u = 0; u < d->getServiceUUIDCount(); u++) {
      NimBLEUUID uuid = d->getServiceUUID(u);
      if (uuid == CU_SVC_NUS) return true;
      if (uuid.bitSize() == 16 && uuid == NimBLEUUID(CU_SVC_LEGACY)) return true;
    }
    return false;
  }

  uint8_t lrc8(const uint8_t* d, int n) {
    uint16_t sum = 0;
    for (int i = 0; i < n; i++) sum += d[i];
    return (uint8_t)(0x100 - (sum & 0xFF));
  }

  void cuNotify(NimBLERemoteCharacteristic*, uint8_t* data, size_t len, bool) {
    if (len == 0 || cu_rx_len + len > sizeof(cu_rx)) {
      if (cu_rx_len + len > sizeof(cu_rx)) cu_rx_len = 0;   // resync
      if (len > sizeof(cu_rx)) return;
    }
    memcpy(cu_rx + cu_rx_len, data, len);
    cu_rx_len += len;
    while (cu_rx_len >= 10) {
      if (cu_rx[0] != CU_SOF) { memmove(cu_rx, cu_rx + 1, --cu_rx_len); continue; }
      const uint16_t dlen = ((uint16_t)cu_rx[6] << 8) | cu_rx[7];
      const uint16_t total = 10 + dlen;
      if (total > sizeof(cu_rx)) { cu_rx_len = 0; continue; }
      if (cu_rx_len < total) return;
      uint8_t hdr[6] = {cu_rx[2], cu_rx[3], cu_rx[4], cu_rx[5], cu_rx[6], cu_rx[7]};
      if (lrc8(hdr, 6) != cu_rx[8]) { memmove(cu_rx, cu_rx + 1, --cu_rx_len); continue; }
      cu_last_cmd    = ((uint16_t)cu_rx[2] << 8) | cu_rx[3];
      cu_last_status = ((uint16_t)cu_rx[4] << 8) | cu_rx[5];
      cu_last_len    = dlen;
      if (dlen) memcpy(cu_last_data, cu_rx + 9, dlen);
      cu_frame_ready = true;
      cu_rx_len -= total;
      if (cu_rx_len) memmove(cu_rx, cu_rx + total, cu_rx_len);
      return;
    }
  }

  class CuClientCB : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient*) override {}
    void onDisconnect(NimBLEClient* c, int) override {
      cu_connected = false;
      cu_write = nullptr;
      NimBLEDevice::deleteClient(c);
      cu_client = nullptr;
    }
  };
  CuClientCB cu_cb;
}

bool SharkChameleon::connectOnce() {
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setActiveScan(true);
  scan->setInterval(100);
  scan->setWindow(80);
  scan->setDuplicateFilter(false);
  scan->setMaxResults(0);
  scan->start(0);   // continuous, non-blocking

  const uint32_t t0 = millis();
  const NimBLEAdvertisedDevice* found_dev = nullptr;
  uint16_t seen_total = 0;
  while (millis() - t0 < 15000) {
    NimBLEScanResults r = scan->getResults();
    if (r.getCount() > seen_total) {
      seen_total = r.getCount();
      Serial.printf("[CU] scan: %u devices, %u named nearby\n", seen_total, nearby_n);
    }
    for (int i = 0; i < r.getCount() && !found_dev; i++) {
      const NimBLEAdvertisedDevice* d = r.getDevice(i);
      noteNearby(String(d->getName().c_str()), d);
      if (looksLikeChameleon(d)) {
        Serial.printf("[CU] match: %s\n", d->getName().c_str());
        found_dev = d;
      }
    }
    if (found_dev) break;
    uint16_t tx, ty;
    if (display_obj.updateTouch(&tx, &ty, 350)) { scan->stop(); scan->clearResults(); cu_fail_reason = 99; return false; }
    delay(60);
  }
  scan->stop();
  scan->clearResults();
  if (!found_dev) {
    Serial.printf("[CU] no match after 15 s (%u devices seen)\n", seen_total);
    cu_fail_reason = 1;
    return false;
  }

  cu_fail_reason = 0;
  if (!cu_client) {
    cu_client = NimBLEDevice::createClient();
    cu_client->setClientCallbacks(&cu_cb, false);
    cu_client->setConnectTimeout(8000);
  }
  if (!cu_client->connect(found_dev)) {
    NimBLEDevice::deleteClient(cu_client);
    cu_client = nullptr;
    cu_fail_reason = 2;
    return false;
  }
  // Current firmware: Nordic UART Service. Legacy firmware: 0xC760 set.
  NimBLERemoteService* svc = cu_client->getService(CU_SVC_NUS);
  NimBLEUUID write_uuid = CU_CHR_WRITE_NUS;
  NimBLEUUID notify_uuid = CU_CHR_NOTIFY_NUS;
  if (!svc) {
    svc = cu_client->getService(NimBLEUUID(CU_SVC_LEGACY));
    write_uuid = NimBLEUUID(CU_CHR_WRITE_LEGACY);
    notify_uuid = NimBLEUUID(CU_CHR_NOTIFY_LEGACY);
  }
  if (!svc) { cu_client->disconnect(); cu_fail_reason = 3; return false; }
  cu_write = svc->getCharacteristic(write_uuid);
  NimBLERemoteCharacteristic* notify = svc->getCharacteristic(notify_uuid);
  if (!cu_write || !notify) { cu_client->disconnect(); cu_fail_reason = 3; return false; }
  cu_rx_len = 0;
  cu_frame_ready = false;
  if (!notify->subscribe(true, cuNotify)) { cu_client->disconnect(); cu_fail_reason = 4; return false; }
  cu_connected = true;
  return true;
}

bool SharkChameleon::sendCmd(uint16_t cmd, const uint8_t* data, uint16_t len) {
  if (!cu_connected || !cu_write) return false;
  uint8_t frame[530];
  frame[0] = CU_SOF;
  frame[1] = 0xEF;
  frame[2] = cmd >> 8;   frame[3] = cmd & 0xFF;
  frame[4] = 0x00;       frame[5] = 0x00;
  frame[6] = len >> 8;   frame[7] = len & 0xFF;
  uint8_t hdr[6] = {frame[2], frame[3], frame[4], frame[5], frame[6], frame[7]};
  frame[8] = lrc8(hdr, 6);
  if (len) memcpy(frame + 9, data, len);
  frame[9 + len] = len ? lrc8(frame + 9, len) : lrc8(frame + 9, 0);
  cu_frame_ready = false;
  return cu_write->writeValue(frame, 10 + len, true);
}

bool SharkChameleon::waitReply(uint16_t ms) {
  const uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    if (cu_frame_ready) { delay(20); return true; }
    if (!cu_connected) return false;
    delay(10);
  }
  return false;
}

String SharkChameleon::hexStr(const uint8_t* d, int n) {
  String s;
  for (int i = 0; i < n; i++) {
    if (d[i] < 0x10) s += "0";
    s += String(d[i], HEX);
    if (i < n - 1) s += " ";
  }
  s.toUpperCase();
  return s;
}

void SharkChameleon::fetchInfo() {
  if (sendCmd(CU_GET_APP_VERSION) && waitReply(1200) && cuOk(cu_last_status) && cu_last_len >= 2)
    info.version = String(cu_last_data[0]) + "." + String(cu_last_data[1]);
  if (sendCmd(CU_GET_MODE) && waitReply(1200) && cuOk(cu_last_status) && cu_last_len >= 1)
    info.mode = cu_last_data[0];
  if (sendCmd(CU_GET_BATTERY) && waitReply(1200) && cuOk(cu_last_status) && cu_last_len >= 3) {
    info.batt_mv = ((uint16_t)cu_last_data[0] << 8) | cu_last_data[1];
    info.batt_pct = cu_last_data[2];
  }
  if (sendCmd(CU_GET_ACTIVE_SLOT) && waitReply(1200) && cuOk(cu_last_status) && cu_last_len >= 1)
    info.slot = cu_last_data[0];
}

// ---- zone-based rendering: chrome once, then only the zone that changes -------
// (the R144 full-screen redraw on every state tick was the visible blink)

void SharkChameleon::drawChrome() {
  TFT_eSPI& tft = display_obj.tft;
  const int16_t w = tft.width();
  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(NULL);
  tft.setTextWrap(false);
  tft.setTextSize(1);
  menu_function_obj.drawStatusBar();
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(WD_CYAN, TFT_BLACK);
  tft.drawString("// CHAMELEON ULTRA", 6, STATUS_BAR_WIDTH + 9, 2);
  tft.setTextDatum(TL_DATUM);
  tft.drawFastHLine(0, STATUS_BAR_WIDTH + 17, w, WD_EDGE);
  tft.drawFastHLine(0, STATUS_BAR_WIDTH + 17, 66, WD_CYAN);

  // Info row labels (static).
  const char* labels[4] = {"VERSION", "MODE", "BATTERY", "SLOT"};
  for (uint8_t i = 0; i < 4; i++) {
    tft.setTextColor(WD_DIM, TFT_BLACK);
    tft.drawString(labels[i], 8, 47 + i * 26, 1);
  }

  // Keys.
  wdPanel(tft, 6, 150, 74, 34, WD_CYAN, WD_CYAN, WD_CYAN);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, WD_CYAN);
  tft.drawString("MODE", 43, 167, 2);
  wdPanel(tft, 84, 150, 74, 34, WD_SURFACE, WD_GREY, WD_GREY);
  tft.setTextColor(WD_GREY, WD_SURFACE);
  tft.drawString("SLOT-", 121, 167, 2);
  wdPanel(tft, 162, 150, 74, 34, WD_SURFACE, WD_GREY, WD_GREY);
  tft.drawString("SLOT+", 199, 167, 2);
  wdPanel(tft, 6, 190, 112, 38, WD_PANEL, WD_EDGE, WD_CYAN);
  tft.setTextColor(WD_BONE, WD_PANEL);
  tft.drawString("READ HF", 62, 209, 2);
  wdPanel(tft, 122, 190, 112, 38, WD_PANEL, WD_EDGE, WD_CYAN);
  tft.drawString("READ LF", 178, 209, 2);
  // Emulation row: configure the active slot and switch to emulator mode.
  wdPanel(tft, 6, 232, 112, 38, WD_PANEL, WD_EDGE, WD_AMBER);
  tft.setTextColor(WD_BONE, WD_PANEL);
  tft.drawString("EMU HF", 62, 245, 2);
  tft.setTextColor(WD_DIM, WD_PANEL);
  tft.drawString("MF 1K on slot", 62, 260, 1);
  wdPanel(tft, 122, 232, 112, 38, WD_PANEL, WD_EDGE, WD_AMBER);
  tft.setTextColor(WD_BONE, WD_PANEL);
  tft.drawString("EMU LF", 178, 245, 2);
  tft.setTextColor(WD_DIM, WD_PANEL);
  tft.drawString("EM4100 on slot", 178, 260, 1);
  // Result panel frame (content via drawResult). Height 22 so it can
  // never paint over the BACK key below it again.
  wdPanel(tft, 4, 272, w - 8, 22, WD_SURFACE, WD_EDGE, WD_EDGE);
  // BACK - full width, tall enough to hit easily.
  wdPanel(tft, 6, 298, w - 12, 20, WD_SURFACE, WD_GREY, WD_GREY);
  tft.setTextColor(WD_GREY, WD_SURFACE);
  tft.drawString("BACK", w / 2, 308, 1);
  tft.setTextDatum(TL_DATUM);
}

void SharkChameleon::drawPill() {
  TFT_eSPI& tft = display_obj.tft;
  const char* txt;
  uint16_t col;
  if (cu_connected)                { txt = "LINK OK";  col = WD_CYAN; }
  else if (cu_fail_reason == 0 || cu_fail_reason == 99) { txt = "SCANNING"; col = WD_AMBER; }
  else                             { txt = "NO LINK";  col = WD_AMBER; }
  tft.fillRect(158, STATUS_BAR_WIDTH + 2, 78, 21, TFT_BLACK);
  tft.fillRoundRect(162, STATUS_BAR_WIDTH + 3, 72, 18, 3, col);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, col);
  tft.drawString(txt, 198, STATUS_BAR_WIDTH + 12, 1);
  tft.setTextDatum(TL_DATUM);
}

void SharkChameleon::drawInfo() {
  TFT_eSPI& tft = display_obj.tft;
  const String values[4] = {
    info.version.length() ? info.version : String("-"),
    info.mode == 1 ? "READER" : (info.mode == 0 ? "EMULATOR" : "-"),
    info.batt_pct ? (String(info.batt_pct) + "% " + String(info.batt_mv) + "mV") : String("-"),
    String(info.slot + 1) + " / 8"};
  // Values zone only (labels are static chrome).
  tft.fillRect(60, 42, 180, 104, TFT_BLACK);
  tft.setTextDatum(MR_DATUM);
  for (uint8_t i = 0; i < 4; i++) {
    tft.setTextColor(WD_BONE, TFT_BLACK);
    tft.drawString(values[i], 232, 42 + i * 26, 2);
  }
  tft.setTextDatum(TL_DATUM);
}

void SharkChameleon::drawResult() {
  TFT_eSPI& tft = display_obj.tft;
  const int16_t w = tft.width();
  // Single-line results: the panel is 22px tall and must stay above the
  // BACK key (drawResult once painted over it, making BACK vanish).
  tft.fillRect(6, 274, w - 12, 18, WD_SURFACE);
  tft.setTextDatum(MC_DATUM);
  const String line = result_line1.length() ? result_line1 : result_line2;
  if (line.length()) {
    tft.setTextColor(result_ok ? WD_CYAN : WD_AMBER, WD_SURFACE);
    String l = line;
    while (l.length() > 3 && tft.textWidth(l, 2) > 218) l.remove(l.length() - 1);
    if (tft.textWidth(l, 2) <= 218)
      tft.drawString(l, w / 2, 283, 2);
    else {
      while (l.length() > 3 && tft.textWidth(l, 1) > 218) l.remove(l.length() - 1);
      tft.setTextColor(WD_GREY, WD_SURFACE);
      tft.drawString(l, w / 2, 283, 1);
    }
  }
  tft.setTextDatum(TL_DATUM);
}

void SharkChameleon::run() {
  if (!wifi_scan_obj.ensureBLE("M5-SHARK")) return;   // guarded stack bring-up

  state = ST_SCAN;
  ever_connected = false;
  info = {String(""), 0xFF, 0, 0, 0};
  result_ok = false;
  result_line1 = "";
  result_line2 = "";
  nearby_n = 0;
  cu_fail_reason = 0;
  drawChrome();
  drawPill();
  drawInfo();
  drawResult();

  uint32_t scan_start = millis();
  uint32_t anim_ms = 0;
  uint8_t  anim = 0;

  while (true) {
    // ---- auto-connect / auto-reconnect (zone updates only - no blink) ----
    // Every tick: refresh the status text, then make ONE real connection
    // attempt (connectOnce blocks for its scan window). The R145 version
    // guarded connectOnce() behind a freshly-reset timer, so it could never
    // fire - the app just animated SCANNING forever.
    if (!cu_connected && cu_fail_reason != 99) {
      if (millis() - scan_start > 1200) {
        scan_start = millis();
        anim = (anim + 1) % 4;
        if (cu_fail_reason == 0) {
          result_ok = false;
          result_line1 = String("SCANNING") + String("...").substring(0, anim ? anim : 1);
          result_line2 = nearby_n ? ("nearby: " + nearby[anim % nearby_n])
                                  : "no BLE names seen yet";
        }
        else if (cu_fail_reason == 1) {
          result_ok = false;
          result_line1 = "NOT FOUND";
          result_line2 = nearby_n ? ("nearby: " + nearby[anim % nearby_n])
                                  : "no BLE devices nearby";
        }
        else if (cu_fail_reason == 2) {
          result_ok = false;
          result_line1 = "CONNECT FAILED";
          result_line2 = "pairing on? disable it in CU app";
        }
        else if (cu_fail_reason == 3) {
          result_ok = false;
          result_line1 = "NO CU SERVICE";
          result_line2 = "old CU firmware? update it";
        }
        else if (cu_fail_reason == 4) {
          result_ok = false;
          result_line1 = "SUBSCRIBE FAILED";
          result_line2 = "retrying...";
        }
        drawPill();
        drawResult();

        // The actual attempt. Blocks up to the scan window length.
        if (connectOnce()) {
          ever_connected = true;
          drawPill();
          result_ok = true;
          result_line1 = "CONNECTED";
          result_line2 = "reading device info...";
          drawResult();
          fetchInfo();
          result_line1 = "";
          result_line2 = "";
          drawInfo();
          drawResult();
        }
        else {
          drawPill();
          drawResult();
        }
      }
    }

    // ---- touch ----
    uint16_t tx, ty;
    if (display_obj.updateTouch(&tx, &ty, 350)) {
      while (display_obj.updateTouch(&tx, &ty, 350)) delay(10);
      if (ty >= 294) break;   // BACK (generous strip, key at 298)

      if (ty >= 150 && ty < 184 && cu_connected) {
        if (tx < 80) {          // MODE toggle
          const uint8_t next = info.mode == 1 ? 0 : 1;
          result_ok = false;
          result_line1 = "SWITCHING...";
          result_line2 = next == 1 ? "to reader mode" : "to emulator mode";
          drawResult();
          sendCmd(CU_CHANGE_MODE, &next, 1);
          waitReply(1200);
          sendCmd(CU_GET_MODE);
          if (waitReply(1200) && cuOk(cu_last_status) && cu_last_len >= 1)
            info.mode = cu_last_data[0];
          result_line1 = "";
          result_line2 = "";
          drawInfo();
          drawResult();
        }
        else {                  // SLOT - / +
          uint8_t s = (tx < 158) ? (info.slot > 0 ? info.slot - 1 : 7)
                                 : (info.slot < 7 ? info.slot + 1 : 0);
          sendCmd(CU_SET_ACTIVE_SLOT, &s, 1);
          waitReply(800);
          sendCmd(CU_GET_ACTIVE_SLOT);
          if (waitReply(800) && cuOk(cu_last_status) && cu_last_len >= 1)
            info.slot = cu_last_data[0];
          drawInfo();
        }
      }
      else if (ty >= 190 && ty < 228 && cu_connected) {
        const bool hf = tx < 118;
        if (info.mode == 0) {   // reads need reader mode; switch automatically
          const uint8_t reader = 1;
          sendCmd(CU_CHANGE_MODE, &reader, 1);
          waitReply(1200);
          sendCmd(CU_GET_MODE);
          if (waitReply(800) && cuOk(cu_last_status) && cu_last_len >= 1) {
            info.mode = cu_last_data[0];
            drawInfo();
          }
        }
        result_ok = false;
        result_line1 = "READING...";
        result_line2 = hf ? "hold HF card on antenna" : "hold LF tag on antenna";
        drawResult();

        const bool sent = sendCmd(hf ? CU_HF14A_SCAN : CU_EM410X_SCAN);
        const bool got = sent && waitReply(2500);
        if (!cu_connected) { drawPill(); continue; }
        if (!got) {
          result_ok = false;
          result_line1 = "TIMEOUT";
          result_line2 = "no response from Chameleon";
        }
        else if (!cuOk(cu_last_status)) {
          result_ok = false;
          result_line1 = hf ? "NO HF TAG" : "NO LF TAG";
          result_line2 = "status 0x" + String(cu_last_status, HEX);
        }
        else if (hf) {
          if (cu_last_len >= 1 && cu_last_data[0] > 0 && cu_last_len >= 1 + cu_last_data[0] + 3) {
            const uint8_t uidlen = cu_last_data[0];
            String uid = hexStr(cu_last_data + 1, uidlen);
            const uint8_t* p = cu_last_data + 1 + uidlen;
            result_ok = true;
            result_line1 = uid;
            result_line2 = "ATQA " + hexStr(p, 2) + "  SAK " + hexStr(p + 2, 1);
          }
          else { result_ok = false; result_line1 = "NO HF TAG"; result_line2 = ""; }
        }
        else {
          if (cu_last_len >= 5) {
            result_ok = true;
            result_line1 = hexStr(cu_last_data, 5);
            result_line2 = "EM4100 tag ID";
          }
          else { result_ok = false; result_line1 = "NO LF TAG"; result_line2 = ""; }
        }
        drawResult();
      }
      else if (ty >= 232 && ty < 270 && cu_connected) {
        // EMULATE: configure the active slot with the requested tag type,
        // enable it for that sense, and switch the Chameleon to emulator
        // mode. Payload layouts verified against app_cmd.c / tag_base_type.h.
        const bool hf = tx < 118;
        result_ok = false;
        result_line1 = "SETTING UP...";
        result_line2 = String("slot ") + String(info.slot + 1) +
                       (hf ? " as MF 1K" : " as EM4100");
        drawResult();

        uint8_t typePayload[3] = {(uint8_t)info.slot,
                                  (uint8_t)((hf ? CU_TAG_MF1_1K : CU_TAG_EM410X) >> 8),
                                  (uint8_t)((hf ? CU_TAG_MF1_1K : CU_TAG_EM410X) & 0xFF)};
        sendCmd(CU_SET_SLOT_TAG_TYPE, typePayload, 3);
        waitReply(1000);
        uint8_t enPayload[3] = {(uint8_t)info.slot,
                                hf ? CU_SENSE_HF : CU_SENSE_LF, 1};
        sendCmd(CU_SET_SLOT_ENABLE, enPayload, 3);
        waitReply(1000);
        const uint8_t emulator = 0;
        sendCmd(CU_CHANGE_MODE, &emulator, 1);
        waitReply(1500);
        sendCmd(CU_GET_MODE);
        if (waitReply(800) && cuOk(cu_last_status) && cu_last_len >= 1)
          info.mode = cu_last_data[0];
        result_ok = info.mode == 0;
        result_line1 = result_ok ? "EMULATING" : "MODE SWITCH FAILED";
        result_line2 = String("slot ") + String(info.slot + 1) +
                       (hf ? " - MF Classic 1K" : " - EM4100");
        drawInfo();
        drawResult();
      }
    }
    delay(20);
  }

  if (cu_client) {
    cu_client->disconnect();
    delay(120);
  }
  wifi_scan_obj.bleSoftStop();
}

#endif
