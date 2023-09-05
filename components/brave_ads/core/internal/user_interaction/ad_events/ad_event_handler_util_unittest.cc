/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_interaction/ad_events/ad_event_handler_util.h"

#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/user_interaction/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_interaction/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/public/ad_info.h"
#include "brave/components/brave_ads/core/public/confirmation_type.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsAdEventHandlerUtilTest, HasFiredAdEvent) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  AdEventList ad_events;
  const AdEventInfo ad_event = BuildAdEvent(ad, ConfirmationType::kServed,
                                            /*created_at*/ Now());
  ad_events.push_back(ad_event);

  // Act

  // Assert
  EXPECT_TRUE(HasFiredAdEvent(ad, ad_events, ConfirmationType::kServed));
}

TEST(BraveAdsAdEventHandlerUtilTest, HasNotFiredAdEvent) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  AdEventList ad_events;
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at*/ Now());
  ad_events.push_back(ad_event);

  // Act

  // Assert
  EXPECT_FALSE(HasFiredAdEvent(ad, ad_events, ConfirmationType::kViewed));
}

}  // namespace brave_ads
