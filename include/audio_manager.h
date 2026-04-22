#pragma once
#include <stdint.h>
#include <stddef.h>
#include "data_model.h"

namespace AudioManager {
void begin();
void speak(VoiceEvent event);

// Stream 16-bit signed mono PCM at the current I2S sample rate (24 kHz).
// Blocks until playback finishes. Safe to call from setup() or loop().
void playPcm(const int16_t* samples, size_t count);

// Convenience: play the embedded Korean greeting ("안녕하세요. 슬라워입니다.").
void playGreetingVoice();

// Composed switch-state announcement: "<N>번, 스위치가 켜/꺼졌습니다."
// Supported N: 1, 2, 3 (extend by adding num_<N> clip in gen_greeting_tts.py).
void playSwitchStateVoice(int switchNumber, bool isOn);

// "온도가 높아요." — spoken during the sensor splash.
void playTempHighVoice();
}
