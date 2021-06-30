/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SECURITY_CONVERSIONS_CONVERSIONS_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SECURITY_CONVERSIONS_CONVERSIONS_UTIL_H_

#include <string>

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

struct VerifiableConversionInfo;

namespace security {

struct VerifiableConversionEnvelopeInfo;

absl::optional<VerifiableConversionEnvelopeInfo> EnvelopeSeal(
    const VerifiableConversionInfo& verifiable_conversion);

}  // namespace security
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SECURITY_CONVERSIONS_CONVERSIONS_UTIL_H_
