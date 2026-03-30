/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/soda/soda_util.h"

#include "base/feature_list.h"
#include "brave/components/local_ai/core/features.h"
#include "ui/base/l10n/l10n_util.h"

#define IsOnDeviceSpeechRecognitionSupported \
  IsOnDeviceSpeechRecognitionSupported_ChromiumImpl
#define GetSodaAvailabilityStatus \
  GetSodaAvailabilityStatus_ChromiumImpl

#include <components/soda/soda_util.cc>

#undef GetSodaAvailabilityStatus
#undef IsOnDeviceSpeechRecognitionSupported

namespace speech {

bool IsOnDeviceSpeechRecognitionSupported() {
  if (base::FeatureList::IsEnabled(
          local_ai::features::
              kBraveOnDeviceSpeechRecognition)) {
    return true;
  }
  return IsOnDeviceSpeechRecognitionSupported_ChromiumImpl();
}

media::mojom::AvailabilityStatus GetSodaAvailabilityStatus(
    std::string_view language) {
  if (base::FeatureList::IsEnabled(
          local_ai::features::
              kBraveOnDeviceSpeechRecognition)) {
    if (l10n_util::GetLanguage(language) == "en") {
      return media::mojom::AvailabilityStatus::kAvailable;
    }
    return media::mojom::AvailabilityStatus::kUnavailable;
  }
  return GetSodaAvailabilityStatus_ChromiumImpl(language);
}

}  // namespace speech
