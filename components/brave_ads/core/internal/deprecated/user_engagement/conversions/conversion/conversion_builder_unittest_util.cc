/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/conversion/conversion_builder_unittest_util.h"

#include "brave/components/brave_ads/core/internal/ad_units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"

namespace brave_ads::test {

ConversionInfo BuildVerifiableConversion(
    const AdType ad_type,
    const ConfirmationType confirmation_type,
    const VerifiableConversionInfo& verifiable_conversion,
    const bool should_use_random_uuids) {
  const AdInfo ad = test::BuildAd(ad_type, should_use_random_uuids);

  const AdEventInfo ad_event =
      BuildAdEvent(ad, confirmation_type, /*created_at=*/Now());

  return BuildConversion(ad_event, verifiable_conversion);
}

ConversionInfo BuildConversion(const AdType ad_type,
                               const ConfirmationType confirmation_type,
                               const bool should_use_random_uuids) {
  const AdInfo ad = test::BuildAd(ad_type, should_use_random_uuids);

  const AdEventInfo ad_event =
      BuildAdEvent(ad, confirmation_type, /*created_at=*/Now());

  return BuildConversion(ad_event, /*verifiable_conversion=*/std::nullopt);
}

}  // namespace brave_ads::test
