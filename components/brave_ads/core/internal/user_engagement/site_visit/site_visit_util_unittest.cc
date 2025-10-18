/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit_util.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSiteVisitUtilTest : public test::TestBase {};

TEST_F(BraveAdsSiteVisitUtilTest,
       AllowNewTabPageAdPageLandIfRewardsUserAndOptedInToNewTabPageAds) {
  // Act & Assert
  EXPECT_TRUE(IsAllowedToLandOnPage(mojom::AdType::kNewTabPageAd));
}

TEST_F(BraveAdsSiteVisitUtilTest,
       DoNotAllowNewTabPageAdPageLandIfRewardsUserAndOptedOutOfNewTabPageAds) {
  // Arrange
  test::OptOutOfNewTabPageAds();

  // Act & Assert
  EXPECT_FALSE(IsAllowedToLandOnPage(mojom::AdType::kNewTabPageAd));
}

TEST_F(
    BraveAdsSiteVisitUtilTest,
    DoNotAllowNewTabPageAdPageLandIfNonRewardsUserAndOptedInToNewTabPageAds) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_FALSE(IsAllowedToLandOnPage(mojom::AdType::kNewTabPageAd));
}

TEST_F(
    BraveAdsSiteVisitUtilTest,
    DoNotAllowNewTabPageAdPageLandIfNonRewardsUserAndOptedOutOfNewTabPageAds) {
  // Arrange
  test::DisableBraveRewards();
  test::OptOutOfNewTabPageAds();

  // Act & Assert
  EXPECT_FALSE(IsAllowedToLandOnPage(mojom::AdType::kNewTabPageAd));
}

TEST_F(BraveAdsSiteVisitUtilTest,
       AllowNotificationAdPageLandIfRewardsUserAndOptedInToNotificationAds) {
  // Act & Assert
  EXPECT_TRUE(IsAllowedToLandOnPage(mojom::AdType::kNotificationAd));
}

TEST_F(
    BraveAdsSiteVisitUtilTest,
    DoNotAllowNotificationAdPageLandIfRewardsUserAndOptedOutOfNotificationAds) {
  // Arrange
  test::OptOutOfNotificationAds();

  // Act & Assert
  EXPECT_FALSE(IsAllowedToLandOnPage(mojom::AdType::kNotificationAd));
}

TEST_F(BraveAdsSiteVisitUtilTest,
       DoNotAllowNotificationAdPageLandIfNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_FALSE(IsAllowedToLandOnPage(mojom::AdType::kNotificationAd));
}

TEST_F(BraveAdsSiteVisitUtilTest,
       AllowSearchResultAdPageLandIfRewardsUserAndOptedInToSearchResultAds) {
  // Act & Assert
  EXPECT_TRUE(IsAllowedToLandOnPage(mojom::AdType::kSearchResultAd));
}

TEST_F(
    BraveAdsSiteVisitUtilTest,
    DoNotAllowSearchResultAdPageLandIfRewardsUserAndOptedOutOfSearchResultAds) {
  // Arrange
  test::OptOutOfSearchResultAds();

  // Act & Assert
  EXPECT_FALSE(IsAllowedToLandOnPage(mojom::AdType::kSearchResultAd));
}

TEST_F(
    BraveAdsSiteVisitUtilTest,
    DoNotAllowSearchResultAdPageLandIfNonRewardsUserAndOptedInToSearchResultAds) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_FALSE(IsAllowedToLandOnPage(mojom::AdType::kSearchResultAd));
}

TEST_F(BraveAdsSiteVisitUtilTest, ShouldResumePageLand) {
  // Arrange
  NotifyBrowserDidBecomeActive();
  NotifyBrowserDidEnterForeground();

  SimulateOpeningNewTab(/*tab_id=*/1,
                        /*redirect_chain=*/{GURL("https://brave.com")},
                        net::HTTP_OK);

  // Act & Assert
  EXPECT_TRUE(ShouldResumePageLand(/*tab_id=*/1));
}

TEST_F(BraveAdsSiteVisitUtilTest, ShouldNotResumePageLandIfTabIsOccluded) {
  // Arrange
  NotifyBrowserDidBecomeActive();
  NotifyBrowserDidEnterForeground();

  SimulateOpeningNewTab(/*tab_id=*/1,
                        /*redirect_chain=*/{GURL("https://brave.com")},
                        net::HTTP_OK);
  SimulateOpeningNewTab(
      /*tab_id=*/2,
      /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
      net::HTTP_OK);

  // Act & Assert
  EXPECT_FALSE(ShouldResumePageLand(/*tab_id=*/1));
}

TEST_F(BraveAdsSiteVisitUtilTest, ShouldNotResumePageLandIfBrowserIsInactive) {
  // Arrange
  NotifyBrowserDidResignActive();
  NotifyBrowserDidEnterForeground();

  SimulateOpeningNewTab(/*tab_id=*/1,
                        /*redirect_chain=*/{GURL("https://brave.com")},
                        net::HTTP_OK);

  // Act & Assert
  EXPECT_FALSE(ShouldResumePageLand(/*tab_id=*/1));
}

TEST_F(BraveAdsSiteVisitUtilTest,
       ShouldNotResumePageLandIfBrowserDidEnterBackground) {
  // Arrange
  NotifyBrowserDidBecomeActive();
  NotifyBrowserDidEnterBackground();

  SimulateOpeningNewTab(/*tab_id=*/1,
                        /*redirect_chain=*/{GURL("https://brave.com")},
                        net::HTTP_OK);

  // Act & Assert
  EXPECT_FALSE(ShouldResumePageLand(/*tab_id=*/1));
}

TEST_F(BraveAdsSiteVisitUtilTest, DidLandOnPage) {
  // Arrange
  SimulateOpeningNewTab(/*tab_id=*/1,
                        /*redirect_chain=*/{GURL("https://brave.com")},
                        net::HTTP_OK);

  // Act & Assert
  EXPECT_TRUE(DidLandOnPage(/*tab_id=*/1, GURL("https://brave.com")));
}

TEST_F(BraveAdsSiteVisitUtilTest, DoNotLandOnPageForClosedTab) {
  // Arrange
  SimulateOpeningNewTab(/*tab_id=*/1,
                        /*redirect_chain=*/{GURL("https://brave.com")},
                        net::HTTP_OK);

  NotifyDidCloseTab(/*tab_id=*/1);

  // Act & Assert
  EXPECT_FALSE(DidLandOnPage(/*tab_id=*/1, GURL("https://brave.com")));
}

TEST_F(BraveAdsSiteVisitUtilTest, DoNotLandOnPageForDomainOrHostMismatch) {
  // Arrange
  SimulateOpeningNewTab(/*tab_id=*/1,
                        /*redirect_chain=*/{GURL("https://foo.com")},
                        net::HTTP_OK);

  // Act & Assert
  EXPECT_FALSE(DidLandOnPage(/*tab_id=*/1, GURL("https://brave.com")));
}

}  // namespace brave_ads
