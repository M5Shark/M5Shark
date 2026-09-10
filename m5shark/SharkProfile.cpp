#include "SharkProfile.h"

#if defined(HAS_SCREEN) && defined(MARAUDER_V8)

#include <Preferences.h>

namespace {
  uint32_t profile_xp = 0;
  uint32_t profile_minutes = 0;
  uint32_t profile_sessions = 0;
  uint32_t profile_last_tick = 0;
  uint8_t profile_dirty_minutes = 0;
  bool profile_ready = false;

  void saveProfile() {
    Preferences prefs;
    if (!prefs.begin("shark_profile", false)) return;
    prefs.putULong("xp", profile_xp);
    prefs.putULong("minutes", profile_minutes);
    prefs.putULong("sessions", profile_sessions);
    prefs.end();
    profile_dirty_minutes = 0;
  }
}

void sharkProfileBegin() {
  if (profile_ready) return;
  Preferences prefs;
  if (prefs.begin("shark_profile", true)) {
    profile_xp = prefs.getULong("xp", 0);
    profile_minutes = prefs.getULong("minutes", 0);
    profile_sessions = prefs.getULong("sessions", 0);
    prefs.end();
  }
  profile_sessions++;
  profile_xp += 5;  // A harmless session bonus; radio activity never grants XP.
  profile_last_tick = millis();
  profile_ready = true;
  saveProfile();
}

void sharkProfileTick(uint32_t now) {
  if (!profile_ready) sharkProfileBegin();
  if (now == 0) return;
  const uint32_t elapsed = now - profile_last_tick;
  if (elapsed < 60000UL) return;
  const uint32_t minutes = elapsed / 60000UL;
  profile_last_tick += minutes * 60000UL;
  profile_minutes += minutes;
  profile_xp += minutes;
  profile_dirty_minutes = min((uint32_t)255,
                              (uint32_t)profile_dirty_minutes + minutes);
  // Limit NVS wear while still persisting normal device-use progress.
  if (profile_dirty_minutes >= 10) saveProfile();
}

void sharkProfileSave() {
  if (!profile_ready) sharkProfileBegin();
  saveProfile();
}

void sharkProfileVisit() {
  if (!profile_ready) sharkProfileBegin();
  profile_xp += 2;
  saveProfile();
}

uint32_t sharkProfileXp() { return profile_xp; }
uint32_t sharkProfileMinutes() { return profile_minutes; }
uint32_t sharkProfileSessions() { return profile_sessions; }

uint8_t sharkProfileLevel() {
  const uint32_t level = 1 + profile_xp / 100UL;
  return level > 99 ? 99 : (uint8_t)level;
}

uint8_t sharkProfileLevelProgress() {
  return profile_xp >= 9800UL ? 100 : (uint8_t)(profile_xp % 100UL);
}

const char* sharkProfileRank() {
  const uint8_t level = sharkProfileLevel();
  if (level >= 35) return "APEX";
  if (level >= 20) return "VANGUARD";
  if (level >= 10) return "HUNTER";
  if (level >= 5) return "SCOUT";
  return "DECKHAND";
}

#endif
