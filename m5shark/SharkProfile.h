#pragma once

#include <Arduino.h>
#include "configs.h"

#if defined(HAS_SCREEN) && defined(MARAUDER_V8)
  void sharkProfileBegin();
  void sharkProfileTick(uint32_t now);
  void sharkProfileSave();
  void sharkProfileVisit();
  uint32_t sharkProfileXp();
  uint32_t sharkProfileMinutes();
  uint32_t sharkProfileSessions();
  uint8_t sharkProfileLevel();
  uint8_t sharkProfileLevelProgress();
  const char* sharkProfileRank();
#endif
