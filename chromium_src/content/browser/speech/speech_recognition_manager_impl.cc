/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/features.h"
#include "brave/content/browser/speech/brave_on_device_speech_recognition_engine.h"

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
#define OnDeviceSpeechRecognitionEngine BraveOnDeviceSpeechRecognitionEngine

// Route the `kCommand` (default) and `kDictation` qualities to Brave's
// on-device engine, but NOT `kConversation` (that falls through to the
// upstream SODA path). Every gating site has the form
// `IsEnabled(kOnDeviceWebSpeechGeminiNano) && config.quality ==
// ...::kConversation`, and `kOnDeviceWebSpeechGeminiNano` is already
// aliased to Brave's feature above. Rewriting the `kConversation` token
// turns each of those into
//   IsEnabled(Brave) && config.quality == ...::kCommand ||
//   IsEnabled(Brave) && config.quality == ...::kDictation
// which (since `&&` binds tighter than `||`) is
//   IsEnabled(Brave) && (quality == kCommand || quality == kDictation).
// Both disjuncts stay gated on Brave's feature, so disabling the feature
// restores upstream behavior. `kConversation` appears only in these
// comparisons (verified), and the enum's own declaration is pre-included
// above, so rewriting the token here is safe.
#define kConversation                                     \
  kCommand ||                                             \
      base::FeatureList::IsEnabled(                       \
          ::local_ai::kBraveOnDeviceSpeechRecognition) && \
          config.quality == media::mojom::SpeechRecognitionQuality::kDictation

#include <content/browser/speech/speech_recognition_manager_impl.cc>

#undef kConversation
#undef OnDeviceSpeechRecognitionEngine
#undef kOnDeviceWebSpeechGeminiNano
