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

// Pre-include so SpeechRecognitionQuality's own `kConversation`
// enumerator declaration is processed before the `kConversation` macro
// below mangles the token. base/feature_list.h backs the IsEnabled call
// the macro expands to.
#include "base/feature_list.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"

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

// Cover every quality, not just the GeminiNano (kConversation) slot, so
// Brave's on-device engine fully replaces SODA: the default Web Speech
// quality is `kCommand`, which would otherwise route to SODA -> network.
// Both gating sites (IsOptimizationGuideSpeechModel and the inline
// service-bypass check) test `config.quality == ...::kConversation`;
// widen that test to also pass for any quality when Brave's feature is
// enabled. `kConversation` appears only in those two comparisons
// (verified), and the enum's own declaration is pre-included above, so
// rewriting the token here is safe. With kOnDeviceWebSpeechGeminiNano
// already aliased to Brave's feature, the surrounding `&&` makes the
// whole check reduce to "Brave feature enabled" for every quality.
#define kConversation                            \
  kConversation || base::FeatureList::IsEnabled( \
                       ::local_ai::kBraveOnDeviceSpeechRecognition)

#include <content/browser/speech/speech_recognition_manager_impl.cc>

#undef kConversation
#undef OnDeviceSpeechRecognitionEngine
#undef kOnDeviceWebSpeechGeminiNano
