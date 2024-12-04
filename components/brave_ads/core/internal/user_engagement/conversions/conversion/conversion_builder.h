/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSION_CONVERSION_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSION_CONVERSION_BUILDER_H_

#include <optional>

namespace brave_ads {

struct AdEventInfo;
struct ConversionInfo;
struct VerifiableConversionInfo;

ConversionInfo BuildConversion(
    const AdEventInfo& ad_event,
    std::optional<VerifiableConversionInfo> verifiable_conversion);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSION_CONVERSION_BUILDER_H_
