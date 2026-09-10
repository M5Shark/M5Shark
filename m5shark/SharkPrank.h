#pragma once

#include <Arduino.h>
#include "configs.h"

#if defined(HAS_SCREEN) && defined(MARAUDER_V8)

// Prank suite. Each function is a self-contained blocking screen with on-screen
// START / STOP / BACK controls. Starting acquires the radio (SoftAP / portal),
// STOP releases it, and BACK always fully tears the resource down before
// returning to the menu, so a prank never leaks into other firmware features.

class SharkPrank {
  public:
    // --- WiFi pranks -------------------------------------------------------
    void funnyHotspot();   // a real AP with a funny SSID
    void ssidRotator();    // AP whose SSID cycles through a funny list
    void guestCounter();   // AP + a big live connected-client count
    void prankPortal();    // captive portal with a funny "gotcha" page
    void memePortal();     // captive portal hosting a meme page
    void customPortal();   // captive portal serving SD /prank/portal.html
    void monkeyWifi();     // FREE WIFI open AP + full-screen prank-image portal

    // --- Bluetooth (BLE) pranks -------------------------------------------
    #if defined(HAS_BT)
      void bleNameBroadcast();  // advertise a custom BLE device name
      void bleNameRotator();    // cycle a custom BLE name list
      void bleAdvertiser();     // broadcast benign custom manufacturer data
      void bleBeacon();         // iBeacon-format beacon broadcast
      void bleRadar();          // scan nearby BLE devices (name + RSSI)
      void bleHunt();           // lock a device; RSSI -> COLD/WARM/HOT/FOUND
      void sharkHuntBeacon();   // Shark Hunt: advertise "SHARK-HUNT" as the target
      void sharkHuntSeeker();   // Shark Hunt: hunt "SHARK-HUNT" by RSSI + trend
    #endif

    // Serve the active Monkey image (SD /monkey/active.<ext> or embedded default)
    // and its content type. Public so the admin web controller can reuse it.
    static void serveMonkeyImage(class AsyncWebServerRequest* req);
    static const char* monkeyImageSource();   // "SD" or "DEFAULT"
    static void monkeyRestoreDefault();        // delete the SD override

  private:
    // Shared chrome: top bar + accent title + a three-key control bar.
    void frame(const char* title, bool running);
    void drawControls(bool running);
    // Reads the control bar. Returns 0=BACK, 1=START, 2=STOP, -1=none.
    int  control();
    // Status panel (up to three lines) under the title.
    void status(const char* l1, const char* l2, const char* l3, uint16_t col);

    // Shared captive-portal prank runner (kind: 0=prank, 1=meme, 2=custom SD).
    void runPortal(const char* title, const char* ssid_label, const char* page, int kind);

    // WiFi helpers shared by the AP pranks.
    void apUp(const char* ssid);
    void apDown();
};

extern SharkPrank shark_prank_obj;

#endif
