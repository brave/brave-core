/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Implements the hook that the chromium_src override of
// `chrome/browser/speech/on_device_speech_recognition_util.cc` forward
// declares.

#include <string_view>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/strings/string_util.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "components/prefs/pref_service.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"
#include "ui/base/l10n/l10n_util.h"

namespace speech {

// Brave's on-device speech recognition availability, in place of upstream's.
media::mojom::AvailabilityStatus GetBraveOnDeviceSpeechAvailability(
    std::string_view language,
    media::mojom::SpeechRecognitionQuality quality) {
  PrefService* local_state = g_browser_process->local_state();
  CHECK(local_state);

  // With the feature off Brave serves nothing, and there is no upstream
  // on-device backend to fall back to.
  if (!base::FeatureList::IsEnabled(
          local_ai::kBraveOnDeviceSpeechRecognition) ||
      !local_state->GetBoolean(local_ai::prefs::kBraveLocalAIEnabled)) {
    return media::mojom::AvailabilityStatus::kUnavailable;
  }

  // The qualities Brave's model supports.
  if (quality != media::mojom::SpeechRecognitionQuality::kCommand &&
      quality != media::mojom::SpeechRecognitionQuality::kDictation) {
    return media::mojom::AvailabilityStatus::kUnavailable;
  }

  // English only for now.
  if (!base::EqualsCaseInsensitiveASCII(l10n_util::GetLanguage(language),
                                        "en")) {
    return media::mojom::AvailabilityStatus::kUnavailable;
  }

  return local_ai::OnDeviceSpeechModelsState::GetInstance()->IsModelInstalled()
             ? media::mojom::AvailabilityStatus::kAvailable
             : media::mojom::AvailabilityStatus::kDownloadable;
}

}  // namespace speech
