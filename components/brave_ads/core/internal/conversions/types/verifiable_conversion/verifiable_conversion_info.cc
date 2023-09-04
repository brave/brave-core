/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_info.h"

#include <tuple>

namespace brave_ads {

bool VerifiableConversionInfo::IsValid() const {
  return !id.empty() && !advertiser_public_key_base64.empty();
}

bool operator==(const VerifiableConversionInfo& lhs,
                const VerifiableConversionInfo& rhs) {
  const auto tie = [](const VerifiableConversionInfo& verifiable_conversion) {
    return std::tie(verifiable_conversion.id,
                    verifiable_conversion.advertiser_public_key_base64);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const VerifiableConversionInfo& lhs,
                const VerifiableConversionInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
