/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/analytics/p2a/opportunities/p2a_opportunity_util.h"

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsP2AOpportunityUtilTest, BuildP2AAdOpportunityEvents) {
  // Act
  const std::vector<std::string> p2a_ad_opportunity_events =
      BuildP2AAdOpportunityEvents(
          mojom::AdType::kNotificationAd,
          /*segments=*/{"technology & computing", "personal finance-crypto",
                        "travel"});

  // Assert
  const std::vector<std::string> expected_p2a_ad_opportunity_events = {
      "Brave.P2A.ad_notification.opportunities_per_segment."
      "technologycomputing",
      "Brave.P2A.ad_notification.opportunities_per_segment."
      "personalfinance",
      "Brave.P2A.ad_notification.opportunities_per_segment.travel",
      "Brave.P2A.ad_notification.opportunities"};
  EXPECT_EQ(expected_p2a_ad_opportunity_events, p2a_ad_opportunity_events);
}

TEST(BraveAdsP2AOpportunityUtilTest,
     BuildP2AAdOpportunityEventsForEmptySegments) {
  // Act
  const std::vector<std::string> p2a_ad_opportunity_events =
      BuildP2AAdOpportunityEvents(mojom::AdType::kNotificationAd,
                                  /*segments=*/{});

  // Assert
  const std::vector<std::string> expected_p2a_ad_opportunity_events = {
      "Brave.P2A.ad_notification.opportunities"};
  EXPECT_EQ(expected_p2a_ad_opportunity_events, p2a_ad_opportunity_events);
}

}  // namespace brave_ads
