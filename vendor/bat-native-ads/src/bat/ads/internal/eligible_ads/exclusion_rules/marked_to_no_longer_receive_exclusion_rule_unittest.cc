/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/exclusion_rules/marked_to_no_longer_receive_exclusion_rule.h"

#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/deprecated/client/client.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
constexpr char kSegment[] = "segment";
}  // namespace

class BatAdsMarkedToNoLongerReceiveExclusionRuleTest : public UnitTestBase {
 protected:
  BatAdsMarkedToNoLongerReceiveExclusionRuleTest() = default;

  ~BatAdsMarkedToNoLongerReceiveExclusionRuleTest() override = default;
};

TEST_F(BatAdsMarkedToNoLongerReceiveExclusionRuleTest, AllowAd) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.segment = kSegment;

  // Act
  MarkedToNoLongerReceiveExclusionRule frequency_cap;
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsMarkedToNoLongerReceiveExclusionRuleTest, DoNotAllowAd) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.segment = kSegment;

  Client::Get()->ToggleAdOptOut(creative_ad.segment,
                                CategoryContentOptActionType::kNone);

  // Act
  MarkedToNoLongerReceiveExclusionRule frequency_cap;
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
