/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/speech/on_device_speech_recognition_util.h"

#include <utility>

#include "base/functional/callback.h"
#include "brave/components/local_ai/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_LOCAL_AI)

#include <string_view>

#include "brave/browser/speech/on_device_speech_recognition_util.h"
#include "chrome/browser/browser_process.h"

namespace speech {

// Brave ships its own on-device speech model, delivered by the component
// updater, instead of SODA and the optimization-guide models. Both halves of
// the on-device contract resolve here: availability, reached from the
// `available()` query through `GetMaskedAvailabilityStatus` and from the
// session-start gate through `ChromeContentBrowserClient`, and install, reached
// from `Install()`'s Brave arm.
media::mojom::AvailabilityStatus GetOnDeviceSpeechRecognitionAvailabilityStatus(
    content::BrowserContext* context,
    std::string_view language,
    media::mojom::SpeechRecognitionQuality quality) {
  return GetBraveOnDeviceSpeechAvailability(context, language, quality);
}

bool IsOnDeviceSpeechQualitySupported(
    media::mojom::SpeechRecognitionQuality quality) {
  // Brave's model is the only on-device backend, so what it serves is the whole
  // answer. Returning false with the feature off is what keeps a request from
  // reaching upstream's SODA arm.
  return UsesBraveOnDeviceSpeech(quality);
}

void InstallBraveOnDeviceSpeechModel(base::OnceCallback<void(bool)> callback) {
  InstallBraveOnDeviceSpeechModel(g_browser_process->local_state(),
                                  g_browser_process->component_updater(),
                                  std::move(callback));
}

}  // namespace speech

#else  // BUILDFLAG(ENABLE_LOCAL_AI)

#include "base/feature_list.h"
#include "base/notreached.h"
#include "media/base/media_switches.h"

// Local AI is compiled out (e.g. Android, Brave Origin), so upstream answers
// availability.
#include <chrome/browser/speech/on_device_speech_recognition_util.cc>

namespace speech {

// Upstream's own backend selection, which lives inline in `Available()` and
// `Install()` there. Named here so that the declarations those functions now
// call resolve without local AI built in, with upstream's behaviour unchanged.
bool IsOnDeviceSpeechQualitySupported(
    media::mojom::SpeechRecognitionQuality quality) {
  switch (quality) {
    case media::mojom::SpeechRecognitionQuality::kCommand:
      return true;  // SODA.
    case media::mojom::SpeechRecognitionQuality::kConversation:
      return base::FeatureList::IsEnabled(media::kOnDeviceWebSpeechGeminiNano);
    case media::mojom::SpeechRecognitionQuality::kDictation:
      return base::FeatureList::IsEnabled(
          media::kOnDeviceWebSpeechSmallExpertModel);
  }
}

bool UsesBraveOnDeviceSpeech(media::mojom::SpeechRecognitionQuality quality) {
  return false;
}

void InstallBraveOnDeviceSpeechModel(base::OnceCallback<void(bool)> callback) {
  NOTREACHED();
}

}  // namespace speech

#endif  // BUILDFLAG(ENABLE_LOCAL_AI)
