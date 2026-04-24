/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/features.h"
#include "brave/content/browser/speech/brave_speech_recognition_engine.h"

// Pre-include the engine header BEFORE the #define below so the base
// class's type declaration is unaffected — only the `make_unique<>`
// construction site inside the manager's .cc gets renamed.
#include "content/browser/speech/on_device_speech_recognition_engine_impl.h"

// Pre-include so the declaration of `kOnDeviceWebSpeechGeminiNano`
// in media_switches.h is unaffected by the #define below.
#include "media/base/media_switches.h"

// Alias Brave's feature flag into the media namespace so the redirect
// `media::kOnDeviceWebSpeechGeminiNano` ->
// `media::kBraveOnDeviceSpeechRecognition` resolves to a valid reference.
namespace media {
inline const auto& kBraveOnDeviceSpeechRecognition =
    ::local_ai::kBraveOnDeviceSpeechRecognition;
}  // namespace media

// Swap the feature flag that gates the on-device engine branch.
#define kOnDeviceWebSpeechGeminiNano kBraveOnDeviceSpeechRecognition

// Swap the engine class that gets instantiated in the branch.
#define OnDeviceSpeechRecognitionEngine BraveSpeechRecognitionEngine

#include <content/browser/speech/speech_recognition_manager_impl.cc>

#undef OnDeviceSpeechRecognitionEngine
#undef kOnDeviceWebSpeechGeminiNano
