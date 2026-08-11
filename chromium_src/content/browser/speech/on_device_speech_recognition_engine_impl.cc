/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/buildflags/buildflags.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"

#if BUILDFLAG(ENABLE_LOCAL_AI)
#include "brave/components/local_ai/core/utils.h"
#endif

namespace {

// Whether Brave's own model serves this quality. False where local AI is not
// built, since there is then no model to serve anything.
bool UsesBraveOnDeviceSpeech(media::mojom::SpeechRecognitionQuality quality) {
#if BUILDFLAG(ENABLE_LOCAL_AI)
  return local_ai::IsQualityServedByBraveOnDeviceSpeech(quality);
#else
  return false;
#endif
}

}  // namespace

#include <content/browser/speech/on_device_speech_recognition_engine_impl.cc>
