/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/speech/on_device_speech_recognition_util.h"

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

// Brave's answer to `GetOnDeviceSpeechRecognitionAvailabilityStatus`, which the
// plaster for this source returns from the top of that function.
//
// It answers for every quality, on every build, rather than deferring any case
// to upstream. Upstream would consult SODA and the optimization guide,
// and Brave ships neither, so deferring reports a model as downloadable that we
// cannot deliver.
media::mojom::AvailabilityStatus
GetBraveOnDeviceSpeechRecognitionAvailabilityStatus(
    std::string_view language,
    media::mojom::SpeechRecognitionQuality quality) {
#if BUILDFLAG(ENABLE_LOCAL_AI)
  // Brave ships its own on-device speech model, delivered by the component
  // updater, instead of SODA and the optimization-guide models.
  return GetBraveOnDeviceSpeechAvailability(language, quality);
#else
  // No on-device backend available.
  return media::mojom::AvailabilityStatus::kUnavailable;
#endif
}

}  // namespace

}  // namespace speech

#include <chrome/browser/speech/on_device_speech_recognition_util.cc>
