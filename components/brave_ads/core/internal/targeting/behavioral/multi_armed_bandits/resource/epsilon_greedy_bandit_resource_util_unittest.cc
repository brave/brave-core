/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/resource/epsilon_greedy_bandit_resource_util.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsEpsilonGreedyBanditResourceUtilTest : public UnitTestBase {};

TEST_F(BraveAdsEpsilonGreedyBanditResourceUtilTest, SetEligibleSegments) {
  // Arrange
  const SegmentList segments = {"foo", "bar"};

  // Act
  SetEpsilonGreedyBanditEligibleSegments(segments);

  // Assert
  EXPECT_EQ(segments, GetEpsilonGreedyBanditEligibleSegments());
}

TEST_F(BraveAdsEpsilonGreedyBanditResourceUtilTest, SetNoEligibleSegments) {
  // Act
  SetEpsilonGreedyBanditEligibleSegments({});

  // Assert
  const SegmentList segments = GetEpsilonGreedyBanditEligibleSegments();
  EXPECT_TRUE(segments.empty());
}

}  // namespace brave_ads
