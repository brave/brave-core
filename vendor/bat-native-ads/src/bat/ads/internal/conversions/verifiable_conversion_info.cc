/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/verifiable_conversion_info.h"

namespace ads {

bool VerifiableConversionInfo::operator==(
    const VerifiableConversionInfo& other) const {
  return id == other.id && public_key == other.public_key;
}

bool VerifiableConversionInfo::operator!=(
    const VerifiableConversionInfo& other) const {
  return !(*this == other);
}

bool VerifiableConversionInfo::IsValid() const {
  return !(id.empty() || public_key.empty());
}

}  // namespace ads
