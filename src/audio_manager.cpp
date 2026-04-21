#include "audio_manager.h"

#include <Arduino.h>
#include <esp_check.h>

#include "ESP_I2S.h"
#include "I2C_Driver.h"
#include "app_config.h"
#include "es8311.h"

namespace {
constexpr uint32_t SAMPLE_RATE_HZ = 24000;
constexpr uint32_t MCLK_MULTIPLE  = 256;
constexpr uint32_t MCLK_FREQ_HZ   = SAMPLE_RATE_HZ * MCLK_MULTIPLE;
constexpr int      VOICE_VOLUME   = 50;
constexpr int      DEFAULT_AMP    = 12000;

I2SClass         g_i2s;
es8311_handle_t  g_esHandle   = nullptr;
bool             g_i2sReady   = false;
bool             g_codecReady = false;
unsigned long    g_lastVoiceMs = 0;

static esp_err_t initCodec() {
  I2C_Init();
  Serial.println("[AUDIO] I2C init ok");

  g_esHandle = es8311_create(I2C_NUM_0, ES8311_ADDRRES_0);
  ESP_RETURN_ON_FALSE(g_esHandle, ESP_FAIL, "AUDIO", "es8311 create failed");

  const es8311_clock_config_t clk = {
      .mclk_inverted = false,
      .sclk_inverted = false,
      .mclk_from_mclk_pin = true,
      .mclk_frequency = (int)MCLK_FREQ_HZ,
      .sample_frequency = (int)SAMPLE_RATE_HZ,
  };

  ESP_RETURN_ON_ERROR(es8311_init(g_esHandle, &clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16),
                      "AUDIO", "es8311_init failed");
  ESP_RETURN_ON_ERROR(es8311_voice_volume_set(g_esHandle, VOICE_VOLUME, nullptr),
                      "AUDIO", "volume set failed");
  ESP_RETURN_ON_ERROR(es8311_microphone_config(g_esHandle, false),
                      "AUDIO", "microphone config failed");
  ESP_RETURN_ON_ERROR(es8311_voice_mute(g_esHandle, false),
                      "AUDIO", "unmute failed");

  Serial.println("[AUDIO] codec init ok");
  return ESP_OK;
}

static bool initI2S() {
  g_i2s.end();
  g_i2s.setPins(AUDIO_I2S_BCLK,
                AUDIO_I2S_WS,
                AUDIO_I2S_DOUT,
                GPIO_NUM_39,
                AUDIO_I2S_MCLK);

  g_i2sReady = g_i2s.begin(I2S_MODE_STD,
                           SAMPLE_RATE_HZ,
                           I2S_DATA_BIT_WIDTH_16BIT_COMPAT,
                           I2S_SLOT_MODE_MONO_COMPAT,
                           I2S_STD_SLOT_LEFT_COMPAT);
  if (g_i2sReady) {
    Serial.printf("[AUDIO] I2S ready SR=%lu BCK=%d WS=%d DOUT=%d MCLK=%d\n",
                  (unsigned long)SAMPLE_RATE_HZ,
                  AUDIO_I2S_BCLK,
                  AUDIO_I2S_WS,
                  AUDIO_I2S_DOUT,
                  AUDIO_I2S_MCLK);
  } else {
    Serial.println("[AUDIO] I2S init failed");
  }
  return g_i2sReady;
}

static void playBuffer(const int16_t* data, size_t samples) {
  if (!g_i2sReady) return;
  g_i2s.write(reinterpret_cast<const uint8_t*>(data), samples * sizeof(int16_t));
}

static void playSilence(int durationMs) {
  static int16_t zeros[256] = {0};
  uint32_t remaining = (SAMPLE_RATE_HZ * (uint32_t)durationMs) / 1000;
  while (remaining > 0) {
    const size_t chunk = remaining > 256 ? 256 : remaining;
    playBuffer(zeros, chunk);
    remaining -= chunk;
  }
}

static void fillSquareWave(int16_t* buffer, size_t samples, int freqHz, int amp = DEFAULT_AMP) {
  static uint32_t phase = 0;
  const uint32_t period = SAMPLE_RATE_HZ / (uint32_t)freqHz;
  const uint32_t half   = period > 1 ? period / 2 : 1;

  for (size_t i = 0; i < samples; ++i) {
    const uint32_t p = phase % period;
    buffer[i] = (p < half) ? amp : -amp;
    ++phase;
  }
}

static void playSquareTone(int freqHz, int durationMs, int amp = DEFAULT_AMP) {
  static int16_t buffer[256];
  uint32_t remaining = (SAMPLE_RATE_HZ * (uint32_t)durationMs) / 1000;
  while (remaining > 0) {
    const size_t chunk = remaining > 256 ? 256 : remaining;
    fillSquareWave(buffer, chunk, freqHz, amp);
    playBuffer(buffer, chunk);
    remaining -= chunk;
  }
}

// Per-tone duration kept >= 250ms — shorter tones are not audibly resolved
// through the onboard NS4150B amp + small speaker.

static void voiceGreeting() {
  playSquareTone(523, 280);  playSilence(60);
  playSquareTone(784, 280);  playSilence(60);
  playSquareTone(1046, 420);
  playSilence(120);
}

static void voiceUpdate() {
  playSquareTone(880, 260);  playSilence(60);
  playSquareTone(1320, 320);
  playSilence(100);
}

static void voiceFail() {
  playSquareTone(880, 260);  playSilence(60);
  playSquareTone(587, 260);  playSilence(60);
  playSquareTone(392, 420);
  playSilence(120);
}

}  // namespace

namespace AudioManager {

void begin() {
  pinMode(AUDIO_PA_CTRL, OUTPUT);
  digitalWrite(AUDIO_PA_CTRL, LOW);

  g_codecReady = (initCodec() == ESP_OK);
  g_i2sReady   = initI2S();

  if (g_codecReady && g_i2sReady) {
    digitalWrite(AUDIO_PA_CTRL, HIGH);
    delay(300);

    if (g_esHandle) {
      es8311_register_dump(g_esHandle);
    }

    Serial.println("[AUDIO] diag tone start");
    playSquareTone(1000, 500);
    playSilence(200);
    Serial.println("[AUDIO] diag tone done");
  }

  Serial.printf("[AUDIO] begin done  i2s=%d codec=%d\n",
                g_i2sReady ? 1 : 0,
                g_codecReady ? 1 : 0);
}

void speak(VoiceEvent event) {
  if (!g_i2sReady || !g_codecReady) {
    Serial.println("[VOICE] skipped - audio not ready");
    return;
  }

  const unsigned long now = millis();
  if (event == VoiceEvent::Query &&
      now - g_lastVoiceMs < VOICE_MIN_INTERVAL_MS) {
    Serial.printf("[VOICE] skipped - interval guard (%lu ms)\n", now - g_lastVoiceMs);
    return;
  }
  g_lastVoiceMs = now;

  Serial.printf("[VOICE] event=%d\n", static_cast<int>(event));
  switch (event) {
    case VoiceEvent::Query:  voiceGreeting(); break;
    case VoiceEvent::Update: voiceUpdate();   break;
    case VoiceEvent::Fail:   voiceFail();     break;
    default: break;
  }
}

}  // namespace AudioManager
