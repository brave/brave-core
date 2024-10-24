/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit_util.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSiteVisitUtilTest : public test::TestBase {};

TEST_F(BraveAdsSiteVisitUtilTest, AllowInlineContentAdPageLand) {
  // Act & Assert
  EXPECT_TRUE(IsAllowedToLandOnPage(mojom::AdType::kInlineContentAd));
}

TEST_F(BraveAdsSiteVisitUtilTest,
       AllowInlineContentAdPageLandForNonRewardsUserIfOptedInToBraveNews) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_TRUE(IsAllowedToLandOnPage(mojom::AdType::kInlineContentAd));
}

TEST_F(
    BraveAdsSiteVisitUtilTest,
    DoNotAllowInlineContentAdPageLandForNonRewardsUserIfOptedOutOfBraveNews) {
  // Arrange
  test::DisableBraveRewards();
  test::OptOutOfBraveNewsAds();

  // Act & Assert
  EXPECT_FALSE(IsAllowedToLandOnPage(mojom::AdType::kInlineContentAd));
}

TEST_F(BraveAdsSiteVisitUtilTest, AllowPromotedContentAdPageLand) {
  // Act & Assert
  EXPECT_TRUE(IsAllowedToLandOnPage(mojom::AdType::kPromotedContentAd));
}

TEST_F(BraveAdsSiteVisitUtilTest,
       AllowPromotedContentAdPageLandForNonRewardsUserIfOptedInToBraveNews) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_TRUE(IsAllowedToLandOnPage(mojom::AdType::kPromotedContentAd));
}

TEST_F(
    BraveAdsSiteVisitUtilTest,
    DoNotAllowPromotedContentAdPageLandForNonRewardsUserIfOptedOutOfBraveNews) {
  // Arrange
  test::DisableBraveRewards();
  test::OptOutOfBraveNewsAds();

  // Act & Assert
  EXPECT_FALSE(IsAllowedToLandOnPage(mojom::AdType::kPromotedContentAd));
}

TEST_F(BraveAdsSiteVisitUtilTest, AllowNewTabPageAdPageLand) {
  // Act & Assert
  EXPECT_TRUE(IsAllowedToLandOnPage(mojom::AdType::kNewTabPageAd));
}

TEST_F(BraveAdsSiteVisitUtilTest,
       DoNotAllowNewTabPageAdPageLandIfOptedOutOfNewTabPageAds) {
  // Arrange
  test::OptOutOfNewTabPageAds();

  // Act & Assert
  EXPECT_FALSE(IsAllowedToLandOnPage(mojom::AdType::kNewTabPageAd));
}

TEST_F(BraveAdsSiteVisitUtilTest,
       DoNotAllowNewTabPageAdPageLandForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_FALSE(IsAllowedToLandOnPage(mojom::AdType::kNewTabPageAd));
}

TEST_F(
    BraveAdsSiteVisitUtilTest,
    DoNotAllowNewTabPageAdPageLandForNonRewardsUserIfShouldAlwaysTriggerNewTabPageAdEvents) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveNewTabPageAdEventsFeature);

  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_FALSE(IsAllowedToLandOnPage(mojom::AdType::kNewTabPageAd));
}

TEST_F(BraveAdsSiteVisitUtilTest, AllowNotificationAdPageLand) {
  // Act & Assert
  EXPECT_TRUE(IsAllowedToLandOnPage(mojom::AdType::kNotificationAd));
}

TEST_F(BraveAdsSiteVisitUtilTest,
       DoNotAllowNotificationAdPageLandIfOptedOutOfNotificationAds) {
  // Arrange
  test::OptOutOfNotificationAds();

  // Act & Assert
  EXPECT_FALSE(IsAllowedToLandOnPage(mojom::AdType::kNotificationAd));
}

TEST_F(BraveAdsSiteVisitUtilTest,
       DoNotAllowNotificationAdPageLandForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_FALSE(IsAllowedToLandOnPage(mojom::AdType::kNotificationAd));
}

TEST_F(BraveAdsSiteVisitUtilTest, AllowSearchResultAdPageLand) {
  // Act & Assert
  EXPECT_TRUE(IsAllowedToLandOnPage(mojom::AdType::kSearchResultAd));
}

TEST_F(BraveAdsSiteVisitUtilTest,
       DoNotAllowSearchResultAdPageLandIfOptedOutOfSearchResultAds) {
  // Arrange
  test::OptOutOfSearchResultAds();

  // Act & Assert
  EXPECT_FALSE(IsAllowedToLandOnPage(mojom::AdType::kSearchResultAd));
}

TEST_F(BraveAdsSiteVisitUtilTest,
       DoNotAllowSearchResultAdPageLandForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_FALSE(IsAllowedToLandOnPage(mojom::AdType::kSearchResultAd));
}

TEST_F(
    BraveAdsSiteVisitUtilTest,
    DoNotAllowSearchResultAdPageLandForNonRewardsUserIfShouldAlwaysTriggerSearchResultAdEvents) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveNewTabPageAdEventsFeature);

  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_FALSE(IsAllowedToLandOnPage(mojom::AdType::kSearchResultAd));
}

TEST_F(BraveAdsSiteVisitUtilTest, DidLandOnPage) {
  // Arrange
  SimulateOpeningNewTab(/*tab_id=*/1,
                        /*redirect_chain=*/{GURL("https://brave.com")});

  // Act & Assert
  EXPECT_TRUE(DidLandOnPage(/*tab_id=*/1, GURL("https://brave.com")));
}

TEST_F(BraveAdsSiteVisitUtilTest, DoNotLandOnPageForClosedTab) {
  // Arrange
  SimulateOpeningNewTab(/*tab_id=*/1,
                        /*redirect_chain=*/{GURL("https://brave.com")});

  NotifyDidCloseTab(/*tab_id=*/1);

  // Act & Assert
  EXPECT_FALSE(DidLandOnPage(/*tab_id=*/1, GURL("https://brave.com")));
}

TEST_F(BraveAdsSiteVisitUtilTest, DoNotLandOnPageForDomainOrHostMismatch) {
  // Arrange
  SimulateOpeningNewTab(/*tab_id=*/1,
                        /*redirect_chain=*/{GURL("https://foo.com")});

  // Act & Assert
  EXPECT_FALSE(DidLandOnPage(/*tab_id=*/1, GURL("https://brave.com")));
}

}  // namespace brave_ads
