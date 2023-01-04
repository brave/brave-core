/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/marked_to_no_longer_receive_exclusion_rule.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
constexpr char kSegment[] = "segment";
}  // namespace

class BatAdsMarkedToNoLongerReceiveExclusionRuleTest : public UnitTestBase {};

TEST_F(BatAdsMarkedToNoLongerReceiveExclusionRuleTest, AllowAd) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.segment = kSegment;

  // Act
  MarkedToNoLongerReceiveExclusionRule exclusion_rule;
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsMarkedToNoLongerReceiveExclusionRuleTest, DoNotAllowAd) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.segment = kSegment;

  ClientStateManager::GetInstance()->ToggleAdOptOut(
      creative_ad.segment, CategoryContentOptActionType::kNone);

  // Act
  MarkedToNoLongerReceiveExclusionRule exclusion_rule;
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
