/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Implements the hook that the chromium_src override of
// `chrome/browser/speech/on_device_speech_recognition_util.cc` forward
// declares.

#include <string_view>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "brave/components/local_ai/core/utils.h"
#include "chrome/browser/browser_process.h"
#include "components/prefs/pref_service.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"
#include "ui/base/l10n/l10n_util.h"

namespace speech {

// Brave's on-device speech recognition availability check. Replaces the
// upstream SODA / optimization-guide computation, since Brave ships neither.
// Both `SpeechRecognition.available()` and `SpeechRecognition.start()` of the
// Web Speech API resolve here through the chromium_src replacement of
// `speech::GetOnDeviceSpeechRecognitionAvailabilityStatus`.
//
// Brave serves the `kCommand` and `kDictation` qualities in English only, from
// a model delivered by the component updater. Reports `kUnavailable` when the
// feature is off (there is no on-device backend at all), `kDownloadable` until
// the model is installed, and `kAvailable` once it is.
media::mojom::AvailabilityStatus GetBraveOnDeviceSpeechAvailability(
    std::string_view language,
    media::mojom::SpeechRecognitionQuality quality) {
  PrefService* local_state = g_browser_process->local_state();
  CHECK(local_state);

  // The Local AI umbrella switches off every local AI surface at once.
  if (!local_state->GetBoolean(local_ai::prefs::kBraveLocalAIEnabled)) {
    return media::mojom::AvailabilityStatus::kUnavailable;
  }

  // Brave ships no upstream on-device backend, so a quality its own model does
  // not serve, or the feature being off, leaves nothing to fall back to.
  if (!local_ai::IsQualityServedByBraveOnDeviceSpeech(quality)) {
    return media::mojom::AvailabilityStatus::kUnavailable;
  }

  // English only for now.
  if (!base::EqualsCaseInsensitiveASCII(l10n_util::GetLanguage(language),
                                        "en")) {
    return media::mojom::AvailabilityStatus::kUnavailable;
  }

  // The model is delivered by the component updater. Report `kDownloadable`
  // until it lands so the page can drive the install flow.
  if (!local_ai::OnDeviceSpeechModelsState::GetInstance()->IsModelInstalled()) {
    return media::mojom::AvailabilityStatus::kDownloadable;
  }

  return media::mojom::AvailabilityStatus::kAvailable;
}

}  // namespace speech
