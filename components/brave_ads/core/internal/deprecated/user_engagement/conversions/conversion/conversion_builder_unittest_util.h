/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_USER_ENGAGEMENT_CONVERSIONS_CONVERSION_CONVERSION_BUILDER_UNITTEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_USER_ENGAGEMENT_CONVERSIONS_CONVERSION_CONVERSION_BUILDER_UNITTEST_UTIL_H_

#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

namespace brave_ads {

struct ConversionInfo;
struct VerifiableConversionInfo;

namespace test {

ConversionInfo BuildVerifiableConversion(
    AdType ad_type,
    ConfirmationType confirmation_type,
    const VerifiableConversionInfo& verifiable_conversion,
    bool should_use_random_uuids);

ConversionInfo BuildConversion(AdType ad_type,
                               ConfirmationType confirmation_type,
                               bool should_use_random_uuids);

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_USER_ENGAGEMENT_CONVERSIONS_CONVERSION_CONVERSION_BUILDER_UNITTEST_UTIL_H_
