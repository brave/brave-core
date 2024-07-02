/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_util.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionsInlineContentAdUtilTest : public UnitTestBase {};

TEST_F(BraveAdsConversionsInlineContentAdUtilTest, CanConvertViewedAdEvent) {
  // Arrange
  const AdInfo ad =
      test::BuildAd(AdType::kInlineContentAd, /*should_generate_random_uuids=*/
                    true);
  const AdEventInfo ad_event = BuildAdEvent(
      ad, ConfirmationType::kViewedImpression, /*created_at=*/Now());

  // Act & Assert
  EXPECT_TRUE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsInlineContentAdUtilTest,
       CannotConvertViewedAdEventIfOptedOutOfBraveNewsAds) {
  // Arrange
  test::OptOutOfBraveNewsAds();

  const AdInfo ad = test::BuildAd(AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event = BuildAdEvent(
      ad, ConfirmationType::kViewedImpression, /*created_at=*/Now());

  // Act & Assert
  EXPECT_FALSE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsInlineContentAdUtilTest,
       CannotConvertViewedAdEventForNonRewardsUsers) {
  // Arrange
  test::DisableBraveRewards();

  const AdInfo ad = test::BuildAd(AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event = BuildAdEvent(
      ad, ConfirmationType::kViewedImpression, /*created_at=*/Now());

  // Act & Assert
  EXPECT_FALSE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsInlineContentAdUtilTest, CanConvertAdClickedEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kClicked, /*created_at=*/Now());

  // Act & Assert
  EXPECT_TRUE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsInlineContentAdUtilTest,
       CannotConvertAdClickedEventIfOptedOutOfBraveNewsAds) {
  // Arrange
  test::OptOutOfBraveNewsAds();

  const AdInfo ad = test::BuildAd(AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kClicked, /*created_at=*/Now());

  // Act & Assert
  EXPECT_FALSE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsInlineContentAdUtilTest,
       CanConvertAdClickedEventForNonRewardsUsers) {
  // Arrange
  test::DisableBraveRewards();

  const AdInfo ad = test::BuildAd(AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kClicked, /*created_at=*/Now());

  // Act & Assert
  EXPECT_TRUE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsInlineContentAdUtilTest,
       CannotConvertAdNonViewedOrClickedEvents) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/false);

  // Act & Assert
  for (int i = 0; i < static_cast<int>(ConfirmationType::kMaxValue); ++i) {
    const auto confirmation_type = static_cast<ConfirmationType>(i);
    if (confirmation_type == ConfirmationType::kViewedImpression ||
        confirmation_type == ConfirmationType::kClicked) {
      continue;
    }

    const AdEventInfo ad_event =
        BuildAdEvent(ad, confirmation_type, /*created_at=*/Now());
    EXPECT_FALSE(CanConvertAdEvent(ad_event));
  }
}

}  // namespace brave_ads
