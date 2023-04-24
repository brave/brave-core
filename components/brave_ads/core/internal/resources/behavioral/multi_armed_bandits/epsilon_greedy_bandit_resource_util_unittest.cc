/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/behavioral/multi_armed_bandits/epsilon_greedy_bandit_resource_util.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsEpsilonGreedyBanditResourceUtilTest : public UnitTestBase {};

TEST_F(BraveAdsEpsilonGreedyBanditResourceUtilTest, SetEligibleSegments) {
  const SegmentList expected_segments = {"foo", "bar"};

  // Arrange

  // Act
  SetEpsilonGreedyBanditEligibleSegments(expected_segments);

  // Assert
  EXPECT_EQ(expected_segments, GetEpsilonGreedyBanditEligibleSegments());
}

TEST_F(BraveAdsEpsilonGreedyBanditResourceUtilTest, SetNoEligibleSegments) {
  // Arrange

  // Act
  SetEpsilonGreedyBanditEligibleSegments(/*segments*/ {});

  // Assert
  const SegmentList segments = GetEpsilonGreedyBanditEligibleSegments();
  EXPECT_TRUE(segments.empty());
}

}  // namespace brave_ads
