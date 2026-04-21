#include "audio_manager.h"
#include <Arduino.h>
#include "app_config.h"

namespace {
unsigned long g_lastVoiceMs = 0;

void beepPattern(int count, int onMs, int offMs) {
  if (!ENABLE_BEEP_FALLBACK || BUZZER_PIN < 0) return;
  for (int i = 0; i < count; ++i) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(onMs);
    digitalWrite(BUZZER_PIN, LOW);
    delay(offMs);
  }
}
}

namespace AudioManager {

void begin() {
  if (BUZZER_PIN >= 0) {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
  }
  Serial.println("[AUDIO] English voice playback placeholder + event beeps ready");
}

void speak(VoiceEvent event) {
  const unsigned long now = millis();
  if (now - g_lastVoiceMs < VOICE_MIN_INTERVAL_MS && event != VoiceEvent::Fail) return;
  g_lastVoiceMs = now;

  switch (event) {
    case VoiceEvent::Query:
      Serial.println("[VOICE] query");
      beepPattern(1, 60, 40);
      break;
    case VoiceEvent::Update:
      Serial.println("[VOICE] update");
      beepPattern(2, 50, 40);
      break;
    case VoiceEvent::Fail:
      Serial.println("[VOICE] fail");
      beepPattern(3, 80, 50);
      break;
    default:
      break;
  }

  // To switch from text/beep events to real spoken English,
  // replace this function with board-specific WAV/I2S playback for "query", "update", "fail".
}

}
