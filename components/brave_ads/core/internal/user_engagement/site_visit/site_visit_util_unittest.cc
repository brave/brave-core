/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit_util.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSiteVisitUtilTest : public test::TestBase {};

TEST_F(BraveAdsSiteVisitUtilTest, DidNotLandOnClosedTab) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  NotifyDidCloseTab(/*tab_id=*/1);

  // Act & Assert
  EXPECT_FALSE(DidLandOnPage(/*tab_id=*/1, GURL("https://brave.com")));
}

TEST_F(BraveAdsSiteVisitUtilTest, DidNotLandOnTabIfMismatchingDomainOrHost) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://foo.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_FALSE(DidLandOnPage(/*tab_id=*/1, GURL("https://brave.com")));
}

TEST_F(BraveAdsSiteVisitUtilTest, DidLandOnPage) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_TRUE(DidLandOnPage(/*tab_id=*/1, GURL("https://brave.com")));
}

}  // namespace brave_ads
