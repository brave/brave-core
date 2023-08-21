/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/analytics/p2a/opportunities/p2a_opportunity_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::p2a {

TEST(BraveAdsP2AOpportunityUtilTest, BuildAdOpportunityEvents) {
  // Arrange

  // Act

  // Assert
  const std::vector<std::string> expected_events = {
      "Brave.P2A.AdOpportunitiesPerSegment.technologycomputing",
      "Brave.P2A.AdOpportunitiesPerSegment.personalfinance",
      "Brave.P2A.AdOpportunitiesPerSegment.travel",
      "Brave.P2A.TotalAdOpportunities"};

  EXPECT_EQ(expected_events, BuildAdOpportunityEvents(/*segments*/ {
                                 "technology & computing",
                                 "personal finance-crypto", "travel"}));
}

TEST(BraveAdsP2AOpportunityUtilTest, BuildAdOpportunityEventsForEmptySegments) {
  // Arrange

  // Act

  // Assert
  const std::vector<std::string> expected_events = {
      "Brave.P2A.TotalAdOpportunities"};

  EXPECT_EQ(expected_events, BuildAdOpportunityEvents(/*segments*/ {}));
}

}  // namespace brave_ads::p2a
