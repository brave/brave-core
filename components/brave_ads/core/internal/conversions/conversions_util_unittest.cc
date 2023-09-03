/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/conversions_util.h"

#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user_interaction/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_interaction/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/public/ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionsUtilTest : public UnitTestBase {};

TEST_F(BraveAdsConversionsUtilTest, CanConvertInlineContentAdViewedEvent) {
  // Arrange
  const AdInfo ad =
      BuildAdForTesting(AdType::kInlineContentAd, /*should_use_random_uuids*/
                        true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now());

  // Assert
  EXPECT_TRUE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest, CanConvertInlineContentAdClickedEvent) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kInlineContentAd,
                                      /*should_use_random_uuids*/ true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kClicked, /*created_at*/ Now());

  // Assert
  EXPECT_TRUE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest, CannotConvertInlineContentAdEvent) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kInlineContentAd,
                                      /*should_use_random_uuids*/ true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at*/ Now());

  // Assert
  EXPECT_FALSE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest,
       CannotConvertInlineContentAdEventIfBraveNewsAdsAreDisabled) {
  // Arrange
  DisableBraveNewsAdsForTesting();

  const AdInfo ad = BuildAdForTesting(AdType::kInlineContentAd,
                                      /*should_use_random_uuids*/ true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now());

  // Assert
  EXPECT_FALSE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest, CanConvertPromotedContentAdViewedEvent) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kPromotedContentAd,
                                      /*should_use_random_uuids*/ true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now());

  // Assert
  EXPECT_TRUE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest, CanConvertPromotedContentAdClickedEvent) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kPromotedContentAd,
                                      /*should_use_random_uuids*/ true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kClicked, /*created_at*/ Now());

  // Assert
  EXPECT_TRUE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest, CannotConvertPromotedContentAdEvent) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kPromotedContentAd,
                                      /*should_use_random_uuids*/ true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at*/ Now());

  // Assert
  EXPECT_FALSE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest,
       CannotConvertPromotedContentAdEventIfBraveNewsAdsAreDisabled) {
  // Arrange
  DisableBraveNewsAdsForTesting();

  const AdInfo ad = BuildAdForTesting(AdType::kPromotedContentAd,
                                      /*should_use_random_uuids*/ true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now());

  // Assert
  EXPECT_FALSE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest, CanConvertNotificationAdViewedEvent) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now());

  // Assert
  EXPECT_TRUE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest, CanConvertNotificationAdClickedEvent) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kClicked, /*created_at*/ Now());

  // Assert
  EXPECT_TRUE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest, CannotConvertNotificationAdEvent) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at*/ Now());

  // Assert
  EXPECT_FALSE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest,
       CannotConvertNotificationAdEventIfOptedOutOfNotificationAds) {
  // Arrange
  DisableNotificationAdsForTesting();

  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now());

  // Assert
  EXPECT_FALSE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest, CanConvertNewTabPageAdViewedEvent) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNewTabPageAd,
                                      /*should_use_random_uuids*/ true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now());

  // Assert
  EXPECT_TRUE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest, CanConvertNewTabPageAdClickedEvent) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNewTabPageAd,
                                      /*should_use_random_uuids*/ true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kClicked, /*created_at*/ Now());

  // Assert
  EXPECT_TRUE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest, CannotConvertNewTabPageAdEvent) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNewTabPageAd,
                                      /*should_use_random_uuids*/ true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at*/ Now());

  // Assert
  EXPECT_FALSE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest,
       CannotConvertNewTabPageAdEventIfNewTabPageAdsAreDisabled) {
  // Arrange
  DisableNewTabPageAdsForTesting();

  const AdInfo ad = BuildAdForTesting(AdType::kNewTabPageAd,
                                      /*should_use_random_uuids*/ true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now());

  // Assert
  EXPECT_FALSE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest, CanConvertSearchResultAdViewedEvent) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kSearchResultAd,
                                      /*should_use_random_uuids*/ true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now());

  // Assert
  EXPECT_TRUE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest, CanConvertSearchResultAdClickedEvent) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kSearchResultAd,
                                      /*should_use_random_uuids*/ true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kClicked, /*created_at*/ Now());

  // Assert
  EXPECT_TRUE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest, CannotConvertSearchResultAdEvent) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kSearchResultAd,
                                      /*should_use_random_uuids*/ true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at*/ Now());

  // Assert
  EXPECT_FALSE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest,
       CanConvertSearchResultAdEventIfAdsAreDisabled) {
  // Arrange
  DisableBraveNewsAdsForTesting();
  DisableNotificationAdsForTesting();
  DisableNewTabPageAdsForTesting();

  const AdInfo ad = BuildAdForTesting(AdType::kNewTabPageAd,
                                      /*should_use_random_uuids*/ true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now());

  // Assert
  EXPECT_FALSE(CanConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest, HasObservationWindowForAdEventExpired) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now());

  AdvanceClockBy(base::Days(1) + base::Milliseconds(1));

  // Assert
  EXPECT_TRUE(HasObservationWindowForAdEventExpired(
      /*observation_window*/ base::Days(1), ad_event));
}

TEST_F(BraveAdsConversionsUtilTest, HasObservationWindowForAdEventNotExpired) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  // Act
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now());

  AdvanceClockBy(base::Days(1));

  // Assert
  EXPECT_FALSE(HasObservationWindowForAdEventExpired(
      /*observation_window*/ base::Days(1), ad_event));
}

}  // namespace brave_ads
