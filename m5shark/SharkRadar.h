#pragma once

#include <Arduino.h>
#include "configs.h"

#if defined(HAS_SCREEN) && defined(MARAUDER_V8)

// Radar-style visualizers. Self-contained blocking screens (like Web Control):
// enter, animate a sweeping radar with live blips, exit on touch. They own the
// radio / GPS only for their own lifetime and restore on exit.

class SharkRadar {
  public:
    // Wi-Fi radar: nearby APs plotted by RSSI (stronger = nearer the centre)
    // and a stable angle per BSSID, refreshed by a periodic scan.
    void runWifi();

    // Short themed animated intro pages shown when entering the Wi-Fi,
    // Bluetooth and GPS sections. Auto-advance; a touch skips them.
    void playWifiIntro();
    void playBluetoothIntro();
    void playGpsIntro();

    #ifdef HAS_GPS
    // GPS sky view: a compass rose with satellite blips, fix state, and the
    // live position/altitude readout.
    void runGps();

    // Pure-GPS tool screens (self-contained, touch to exit). They pump the GPS
    // parser themselves so the readouts stay live.
    void runSpeed();      // speedometer: speed, heading, altitude
    void runCompass();    // heading compass rose with a live needle
    void runClock();      // UTC date/time + fix quality from the satellites
    void runWaypoint();   // mark a point, then range + bearing back to it
    #endif

  private:
    void frame(int16_t cx, int16_t cy, int16_t r, float sweep, const char* title);
    void header(const char* title);
    bool exitTouched();
};

extern SharkRadar shark_radar_obj;

#endif
