/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_VERIFIABLE_CONVERSION_ENVELOPE_UNITTEST_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_VERIFIABLE_CONVERSION_ENVELOPE_UNITTEST_UTIL_H_

#include <string>

#include "absl/types/optional.h"
#include "base/values.h"

namespace ads::security {

struct VerifiableConversionEnvelopeInfo;

absl::optional<security::VerifiableConversionEnvelopeInfo>
GetVerifiableConversionEnvelopeForUserData(const base::Value::Dict& user_data);

absl::optional<std::string> OpenEnvelope(
    const VerifiableConversionEnvelopeInfo& verifiable_conversion_envelope,
    const std::string& advertiser_secret_key_base64);

absl::optional<std::string> OpenEnvelopeForUserDataAndAdvertiserSecretKey(
    const base::Value::Dict& user_data,
    const std::string& advertiser_secret_key);

}  // namespace ads::security

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_VERIFIABLE_CONVERSION_ENVELOPE_UNITTEST_UTIL_H_
