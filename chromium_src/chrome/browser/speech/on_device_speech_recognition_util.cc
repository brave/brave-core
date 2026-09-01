/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/speech/on_device_speech_recognition_util.h"

#include <utility>

#include "brave/components/local_ai/buildflags/buildflags.h"

namespace speech {

#if BUILDFLAG(ENABLE_LOCAL_AI)
// Forward declared to avoid adding a compile-time dependency.
// Implementation is provided by //brave/browser/speech:chromium_impl.
media::mojom::AvailabilityStatus GetBraveOnDeviceSpeechAvailability(
    std::string_view language,
    media::mojom::SpeechRecognitionQuality quality);
#endif

namespace {

// Brave's answer to `GetOnDeviceSpeechRecognitionAvailabilityStatus`. It
// answers every quality on every build, because upstream's answer consults SODA
// and the optimization guide and would report a model Brave cannot deliver as
// downloadable.
media::mojom::AvailabilityStatus
GetBraveOnDeviceSpeechRecognitionAvailabilityStatus(
    std::string_view language,
    media::mojom::SpeechRecognitionQuality quality) {
#if BUILDFLAG(ENABLE_LOCAL_AI)
  return GetBraveOnDeviceSpeechAvailability(language, quality);
#else
  return media::mojom::AvailabilityStatus::kUnavailable;
#endif
}

}  // namespace

}  // namespace speech

#include <chrome/browser/speech/on_device_speech_recognition_util.cc>
