/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/features.h"
#include "brave/content/browser/speech/brave_speech_recognition_engine.h"

// Pre-include the engine header BEFORE the #define
// so it doesn't get renamed.
#include "content/browser/speech/on_device_speech_recognition_engine_impl.h"

// Pre-include so the declaration of kOnDeviceWebSpeechGeminiNano
// in media_switches.h is unaffected by the #define below.
#include "media/base/media_switches.h"

// Alias our feature flag into media:: namespace.
namespace media {
inline const auto& kBraveOnDeviceSpeechRecognition =
    ::local_ai::features::kBraveOnDeviceSpeechRecognition;
}  // namespace media

#define kOnDeviceWebSpeechGeminiNano kBraveOnDeviceSpeechRecognition

// Redirect engine construction to our implementation.
// The original header is already included above, so
// this only affects the make_unique<> call.
#define OnDeviceSpeechRecognitionEngine BraveSpeechRecognitionEngine

#include <content/browser/speech/speech_recognition_manager_impl.cc>

#undef OnDeviceSpeechRecognitionEngine
#undef kOnDeviceWebSpeechGeminiNano
