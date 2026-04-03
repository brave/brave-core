/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/features.h"

// Pre-include so the declaration of kOnDeviceWebSpeechGeminiNano
// in media_switches.h is unaffected by the #define below.
#include "media/base/media_switches.h"

// Alias our feature flag into media:: namespace so the
// redirected media::kOnDeviceWebSpeechGeminiNano resolves.
namespace media {
inline const auto& kBraveOnDeviceSpeechRecognition =
    ::local_ai::features::kBraveOnDeviceSpeechRecognition;
}  // namespace media

#define kOnDeviceWebSpeechGeminiNano kBraveOnDeviceSpeechRecognition

#include <content/browser/speech/speech_recognition_dispatcher_host.cc>

#undef kOnDeviceWebSpeechGeminiNano
