/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit_util.h"

#include "brave/components/brave_ads/core/internal/ad_units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSiteVisitUtilTest : public UnitTestBase {};

TEST_F(BraveAdsSiteVisitUtilTest, DidNotLandOnOccludedTab) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/true);

  NotifyTabDidChange(
      /*tab_id=*/2, /*redirect_chain=*/{GURL("https://bar.com")},
      /*is_error_page=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_FALSE(DidLandOnPage(
      /*tab_id=*/1, /*ad=*/test::BuildAd(AdType::kNotificationAd,
                                         /*should_use_random_uuids=*/true)));
}

TEST_F(BraveAdsSiteVisitUtilTest, DidNotLandOnClosedTab) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/true);

  NotifyDidCloseTab(/*tab_id=*/1);

  // Act & Assert
  EXPECT_FALSE(DidLandOnPage(
      /*tab_id=*/1, /*ad=*/test::BuildAd(AdType::kNotificationAd,
                                         /*should_use_random_uuids=*/true)));
}

TEST_F(BraveAdsSiteVisitUtilTest, DidNotLandOnTabWithNoTabRedirectChain) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{},
      /*is_error_page=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_FALSE(DidLandOnPage(
      /*tab_id=*/1, /*ad=*/test::BuildAd(AdType::kNotificationAd,
                                         /*should_use_random_uuids=*/true)));
}

TEST_F(BraveAdsSiteVisitUtilTest, DidNotLandOnTabForMismatchingDomainOrHost) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://foo.com")},
      /*is_error_page=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_FALSE(DidLandOnPage(
      /*tab_id=*/1, /*ad=*/test::BuildAd(AdType::kNotificationAd,
                                         /*should_use_random_uuids=*/true)));
}

TEST_F(BraveAdsSiteVisitUtilTest, DidLandOnPageWithoutUrlPath) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_TRUE(DidLandOnPage(
      /*tab_id=*/1, /*ad=*/test::BuildAd(AdType::kNotificationAd,
                                         /*should_use_random_uuids=*/true)));
}

TEST_F(BraveAdsSiteVisitUtilTest, DidLandOnPageWithUrlPath) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com/bar")},
      /*is_error_page=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_TRUE(DidLandOnPage(
      /*tab_id=*/1, /*ad=*/test::BuildAd(AdType::kNotificationAd,
                                         /*should_use_random_uuids=*/true)));
}

}  // namespace brave_ads
