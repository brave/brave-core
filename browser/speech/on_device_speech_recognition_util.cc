/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Implements the hook that the chromium_src override of
// `chrome/browser/speech/on_device_speech_recognition_util.cc` forward
// declares.

#include <string_view>

#include "brave/components/local_ai/core/on_device_speech_locales.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"

namespace speech {

// Brave's on-device speech recognition availability, in place of upstream's.
media::mojom::AvailabilityStatus GetBraveOnDeviceSpeechAvailability(
    content::BrowserContext* context,
    std::string_view language,
    media::mojom::SpeechRecognitionQuality quality) {
  // Private, Tor and Guest profiles get none.
  if (!Profile::FromBrowserContext(context)->IsRegularProfile()) {
    return media::mojom::AvailabilityStatus::kUnavailable;
  }

  // The qualities Brave's models support.
  if (quality != media::mojom::SpeechRecognitionQuality::kCommand &&
      quality != media::mojom::SpeechRecognitionQuality::kDictation) {
    return media::mojom::AvailabilityStatus::kUnavailable;
  }

  // Brave serves a language only while it can offer the language's model. With
  // the feature or the master switch off Brave serves nothing, and there is no
  // upstream on-device backend to fall back to.
  const local_ai::OnDeviceSpeechLocale* locale =
      local_ai::ResolveOnDeviceSpeechLocale(language);
  if (!locale || !local_ai::CanOfferOnDeviceSpeechModel(
                     g_browser_process->local_state(), locale->model)) {
    return media::mojom::AvailabilityStatus::kUnavailable;
  }

  return local_ai::OnDeviceSpeechModelsState::GetInstance()->IsModelInstalled(
             locale->model)
             ? media::mojom::AvailabilityStatus::kAvailable
             : media::mojom::AvailabilityStatus::kDownloadable;
}

}  // namespace speech
