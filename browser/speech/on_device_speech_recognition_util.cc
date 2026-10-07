/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Implements the hook that the chromium_src override of
// `chrome/browser/speech/on_device_speech_recognition_util.cc` forward
// declares.

#include <optional>
#include <string_view>

#include "base/i18n/language_tag.h"
#include "base/i18n/tag_converters.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/utils.h"
#include "chrome/browser/browser_process.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"

namespace speech {

// Brave's on-device speech recognition availability, in place of upstream's.
media::mojom::AvailabilityStatus GetBraveOnDeviceSpeechAvailability(
    std::string_view language,
    media::mojom::SpeechRecognitionQuality quality) {
  // With the feature or the master switch off Brave serves nothing, and there
  // is no upstream on-device backend to fall back to.
  if (!local_ai::IsOnDeviceSpeechRecognitionAllowed(
          g_browser_process->local_state())) {
    return media::mojom::AvailabilityStatus::kUnavailable;
  }

  // The qualities Brave's model supports.
  if (quality != media::mojom::SpeechRecognitionQuality::kCommand &&
      quality != media::mojom::SpeechRecognitionQuality::kDictation) {
    return media::mojom::AvailabilityStatus::kUnavailable;
  }

  // English only for now. Parsing the tag normalizes case, so a plain compare
  // covers "EN-GB" too.
  std::optional<base::i18n::LanguageTag> tag =
      base::i18n::GetLanguageTagFromString(language);
  if (!tag || tag->language_subtag() != "en") {
    return media::mojom::AvailabilityStatus::kUnavailable;
  }

  return local_ai::OnDeviceSpeechModelsState::GetInstance()->IsModelInstalled()
             ? media::mojom::AvailabilityStatus::kAvailable
             : media::mojom::AvailabilityStatus::kDownloadable;
}

}  // namespace speech
