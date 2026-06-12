/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "brave/components/local_ai/buildflags/buildflags.h"
#include "content/browser/speech/on_device_speech_recognition_engine_impl.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"

#if BUILDFLAG(ENABLE_LOCAL_AI)
#include "brave/components/local_ai/core/features.h"
#include "brave/content/browser/speech/brave_on_device_speech_recognition_engine.h"
#endif

namespace {

// The engine the on-device branch builds. Brave serves on-device speech from
// its own model, so it replaces the engine that branch would construct, but
// only where that subclass is compiled.
#if BUILDFLAG(ENABLE_LOCAL_AI)
using OnDeviceSpeechRecognitionEngineAlias =
    content::BraveOnDeviceSpeechRecognitionEngine;
#else
using OnDeviceSpeechRecognitionEngineAlias =
    content::OnDeviceSpeechRecognitionEngine;
#endif

// Whether Brave's own model serves this quality, and so whether the session
// takes the on-device branch. Or-ed in front of upstream's condition rather
// than replacing it, so upstream keeps deciding its own backends.
bool UsesBraveOnDeviceSpeech(media::mojom::SpeechRecognitionQuality quality) {
#if BUILDFLAG(ENABLE_LOCAL_AI)
  return base::FeatureList::IsEnabled(
             local_ai::kBraveOnDeviceSpeechRecognition) &&
         (quality == media::mojom::SpeechRecognitionQuality::kCommand ||
          quality == media::mojom::SpeechRecognitionQuality::kDictation);
#else
  return false;
#endif
}

}  // namespace

#include <content/browser/speech/speech_recognition_manager_impl.cc>
