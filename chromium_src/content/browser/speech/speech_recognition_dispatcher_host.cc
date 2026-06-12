/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/features.h"

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

// Swap the feature flag so the dispatcher host routes to the
// on-device path when Brave's flag is on.
#define kOnDeviceWebSpeechGeminiNano kBraveOnDeviceSpeechRecognition

#include <content/browser/speech/speech_recognition_dispatcher_host.cc>

#undef kOnDeviceWebSpeechGeminiNano
