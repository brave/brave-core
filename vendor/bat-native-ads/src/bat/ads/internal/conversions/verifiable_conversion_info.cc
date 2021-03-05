/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/verifiable_conversion_info.h"

namespace ads {

VerifiableConversionInfo::VerifiableConversionInfo() = default;

VerifiableConversionInfo::VerifiableConversionInfo(
    const VerifiableConversionInfo& info) = default;

VerifiableConversionInfo::~VerifiableConversionInfo() = default;

bool VerifiableConversionInfo::operator==(
    const VerifiableConversionInfo& rhs) const {
  return id == rhs.id && public_key == rhs.public_key;
}

bool VerifiableConversionInfo::operator!=(
    const VerifiableConversionInfo& rhs) const {
  return !(*this == rhs);
}

bool VerifiableConversionInfo::IsValid() const {
  if (id.empty() || public_key.empty()) {
    return false;
  }

  return true;
}

}  // namespace ads
