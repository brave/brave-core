/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/classification/purchase_intent_classifier/funnel_sites.h"

#include <string>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace classification {

namespace {

struct TestInfo {
  std::string url;
  FunnelSiteInfo funnel_site_info;
};

const std::vector<TestInfo> kTests = {
  {
    "https://www.carmax.com",
    _automotive_funnel_sites.at(1)
  },
  {
    "https://www.carmax.com/foobar",
    _automotive_funnel_sites.at(1)
  },
  {
    "https://carmax.com",
    _automotive_funnel_sites.at(1)
  },
  {
    "https://brave.com/foobar",
    FunnelSiteInfo()
  }
};

}  // namespace

TEST(BatAdsPurchaseIntentFunnelSitesTest,
    MatchFunnelSites) {
  for (const auto& test : kTests) {
    // Arrange

    // Act
    const FunnelSiteInfo funnel_site = FunnelSites::GetFunnelSite(test.url);

    // Assert
    const FunnelSiteInfo expected_funnel_site = test.funnel_site_info;

    EXPECT_EQ(expected_funnel_site, funnel_site);
  }
}

}  // namespace classification
}  // namespace ads
