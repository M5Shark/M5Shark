#pragma once

#include <Arduino.h>
#include "configs.h"

#if defined(HAS_SCREEN) && defined(MARAUDER_V8) && defined(HAS_GPS)

// Native M5SHARK adaptation of IceNav-v3 navigation concepts. It deliberately
// uses the existing TFT_eSPI/GPS/SD stack so every original GPS tool and the
// ESP32-C5 hardware behavior remain intact.
class SharkIceNav {
  public:
    void run();
    void runCompass();
    void runSatelliteInfo();
    void runAddWaypoint();
    void runSensorRadar();

  private:
    void runPage(uint8_t initialPage);
};

extern SharkIceNav shark_icenav_obj;

#endif
