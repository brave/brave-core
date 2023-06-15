/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/verifiable_conversion_info.h"

#include <tuple>

namespace brave_ads {

bool VerifiableConversionInfo::operator==(
    const VerifiableConversionInfo& other) const {
  const auto tie = [](const VerifiableConversionInfo& verifiable_conversion) {
    return std::tie(verifiable_conversion.id, verifiable_conversion.public_key);
  };

  return tie(*this) == tie(other);
}

bool VerifiableConversionInfo::operator!=(
    const VerifiableConversionInfo& other) const {
  return !(*this == other);
}

bool VerifiableConversionInfo::IsValid() const {
  return !id.empty() && !public_key.empty();
}

}  // namespace brave_ads
