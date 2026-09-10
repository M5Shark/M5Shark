#pragma once

#include <Arduino.h>
#include "configs.h"

// Chameleon Ultra remote over BLE. The M5SHARK connects as a BLE central to
// the Chameleon Ultra (service 0xC760, write 0xC762, notify 0xC761) and
// speaks the official frame protocol (SOF 0x11 + LRC checks). The screen
// auto-scans, auto-connects and auto-reconnects; the operator gets device
// info (version/mode/battery/slot), mode toggle, slot switching and HF/LF
// tag reads on the themed dashboard.
//
// Authorized testing only, on equipment you own or are permitted to test.

#if defined(HAS_SCREEN) && defined(MARAUDER_V8) && defined(HAS_BT)

class SharkChameleon {
  public:
    void run();

  private:
    // Session state.
    enum { ST_SCAN, ST_LIVE } state = ST_SCAN;
    bool ever_connected = false;
    struct {
      String version;
      uint8_t mode;        // 0 emulator / 1 reader / 0xFF unknown
      uint16_t batt_mv;
      uint8_t batt_pct;
      uint8_t slot;        // 0..7
    } info = {String(""), 0xFF, 0, 0, 0};
    bool result_ok = false;
    String result_line1 = "";
    String result_line2 = "";

    // Zone-based rendering (chrome once; only changed zones repaint - the
    // R144 full-screen redraw was the visible blink).
    void drawChrome();           // static layout, once per session
    void drawPill();             // connection state pill
    void drawInfo();             // the four info values
    void drawResult();           // result/status panel
    bool connectOnce();          // scan + connect + subscribe; sets cu_fail_reason
    bool sendCmd(uint16_t cmd, const uint8_t* data = nullptr, uint16_t len = 0);
    bool waitReply(uint16_t ms); // true when a frame for the last cmd arrived
    void fetchInfo();            // version, mode, battery, slot
    String hexStr(const uint8_t* d, int n);
};

extern SharkChameleon shark_chameleon_obj;

#endif
