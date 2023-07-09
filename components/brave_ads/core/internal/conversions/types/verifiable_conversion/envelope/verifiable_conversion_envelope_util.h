/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_TYPES_VERIFIABLE_CONVERSION_ENVELOPE_VERIFIABLE_CONVERSION_ENVELOPE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_TYPES_VERIFIABLE_CONVERSION_ENVELOPE_VERIFIABLE_CONVERSION_ENVELOPE_UTIL_H_

#include <string>

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

struct VerifiableConversionEnvelopeInfo;
struct VerifiableConversionInfo;

std::string GetVerifiableConversionEnvelopeAlgorithm();

absl::optional<VerifiableConversionEnvelopeInfo>
SealVerifiableConversionEnvelope(
    const VerifiableConversionInfo& verifiable_conversion);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_TYPES_VERIFIABLE_CONVERSION_ENVELOPE_VERIFIABLE_CONVERSION_ENVELOPE_UTIL_H_
