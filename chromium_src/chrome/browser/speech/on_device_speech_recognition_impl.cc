/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/speech/on_device_speech_recognition_impl.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/feature_list.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"
#include "ui/base/l10n/l10n_util.h"

#define Available(...) Available_ChromiumImpl(__VA_ARGS__)
#include <chrome/browser/speech/on_device_speech_recognition_impl.cc>
#undef Available

namespace speech {

// Brave's on-device speech recognition replaces SODA and the upstream
// optimization-guide models. It only serves the `kCommand` and `kDictation`
// qualities for the languages it supports; `kConversation` has no backend,
// so it is always unavailable. The model is delivered via the component
// updater rather than SODA's language packs. We do not ship the upstream
// on-device backends, so when the feature is disabled there is no on-device
// backend at all and we report `kUnavailable` instead of falling back to
// `Available_ChromiumImpl`. When enabled, report the component-updater
// state: `kAvailable` once the model is installed, `kDownloadable` while it
// is not, and `kUnavailable` for any non-English request or the
// `kConversation` quality.
void OnDeviceSpeechRecognitionImpl::Available(
    const std::vector<std::string>& languages,
    media::mojom::SpeechRecognitionQuality quality,
    OnDeviceSpeechRecognitionImpl::AvailableCallback callback) {
  media::mojom::AvailabilityStatus status =
      media::mojom::AvailabilityStatus::kUnavailable;
  if (base::FeatureList::IsEnabled(local_ai::kBraveOnDeviceSpeechRecognition) &&
      quality != media::mojom::SpeechRecognitionQuality::kConversation &&
      !languages.empty()) {
    status = media::mojom::AvailabilityStatus::kAvailable;
    for (std::string_view language : languages) {
      if (l10n_util::GetLanguage(language) != "en") {
        status = media::mojom::AvailabilityStatus::kUnavailable;
        break;
      }
    }
    if (status == media::mojom::AvailabilityStatus::kAvailable &&
        local_ai::OnDeviceSpeechModelsState::GetInstance()
            ->GetInstallDir()
            .empty()) {
      status = media::mojom::AvailabilityStatus::kDownloadable;
    }
  }
  std::move(callback).Run(status);
}

}  // namespace speech
