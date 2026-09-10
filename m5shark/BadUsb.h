#pragma once

#include <Arduino.h>
#include "configs.h"

// BadUSB over BLE. The M5 SHARK v8 is an ESP32-C5, whose USB peripheral is
// Serial/JTAG only (no USB-OTG / HID), so a cable BadUSB is not possible on
// this hardware. Instead this advertises as a Bluetooth keyboard; once a host
// pairs with it, a Ducky-style script is typed into that host.
//
// R120 multi-tool suite: the entry screen is a tool list rather than a single
// fixed payload. Tools: SD payload picker (any /SCRIPTS/*.duck|*.txt), free
// text typing, self-test, Windows/Linux recon, Windows Wi-Fi key export,
// parameterized bash reverse shell (authorized targets only), and a lock
// test. Everything funnels through the same guarded BLE bring-up.
//
// Authorized use only: run this against systems you own or are explicitly
// permitted to test.

#if defined(HAS_SCREEN) && defined(MARAUDER_V8) && defined(HAS_BT)

class BadUsb {
  public:
    // Shows the tool list; the chosen tool loads its payload and runs the
    // pair-and-type flow. Blocks until BACK, then tears the BLE stack down.
    void run();

  private:
    // Screens / flows.
    bool pickSdPayload();          // file picker; fills payload/payload_source
    bool buildTypeText();          // free text via touch keyboard
    bool buildRevshell();          // LHOST:LPORT via touch keyboard
    void executePayload();         // advertise, wait for host, type, exit
    void drawScreen(const char* state, uint16_t color, const char* detail);
    void runPayload();
    void typeLine(const String& line);
    void sendChar(char c);
    void sendChord(uint8_t modifiers, uint8_t key);
    void releaseKeys();
    uint8_t modFromToken(const String& t);
    uint16_t keyFromToken(const String& t);

    String payload;
    String payload_source = "built-in demo";
};

extern BadUsb bad_usb_obj;

#endif
