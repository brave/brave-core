/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/speech/on_device_speech_recognition_impl.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/feature_list.h"
#include "brave/components/local_ai/core/features.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"
#include "ui/base/l10n/l10n_util.h"

#define Available(...) Available_ChromiumImpl(__VA_ARGS__)
#include <chrome/browser/speech/on_device_speech_recognition_impl.cc>
#undef Available

namespace speech {

// Brave's on-device speech recognition serves all qualities for the
// languages it supports, replacing SODA. The model is delivered via the
// component updater rather than SODA's language packs, so the upstream
// download-state availability checks (which gate on SODA and the
// optimization-guide feature flags) do not apply. Report availability
// directly: `kAvailable` when every requested language is English and the
// feature is enabled, otherwise fall back to the Chromium implementation.
void OnDeviceSpeechRecognitionImpl::Available(
    const std::vector<std::string>& languages,
    media::mojom::SpeechRecognitionQuality quality,
    OnDeviceSpeechRecognitionImpl::AvailableCallback callback) {
  if (base::FeatureList::IsEnabled(local_ai::kBraveOnDeviceSpeechRecognition)) {
    media::mojom::AvailabilityStatus status =
        languages.empty() ? media::mojom::AvailabilityStatus::kUnavailable
                          : media::mojom::AvailabilityStatus::kAvailable;
    for (std::string_view language : languages) {
      if (l10n_util::GetLanguage(language) != "en") {
        status = media::mojom::AvailabilityStatus::kUnavailable;
        break;
      }
    }
    std::move(callback).Run(status);
    return;
  }

  Available_ChromiumImpl(languages, quality, std::move(callback));
}

}  // namespace speech
