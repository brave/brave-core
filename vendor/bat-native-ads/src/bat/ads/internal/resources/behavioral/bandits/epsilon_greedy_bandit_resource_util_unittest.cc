/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/bandits/epsilon_greedy_bandit_resource_util.h"

#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/segments/segment_alias.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::resource {

namespace {
const SegmentList kSegments = {"foo", "bar"};
}  // namespace

class BatAdsEpsilonGreedyBanditResourceUtilTest : public UnitTestBase {
 protected:
  BatAdsEpsilonGreedyBanditResourceUtilTest() = default;

  ~BatAdsEpsilonGreedyBanditResourceUtilTest() override = default;
};

TEST_F(BatAdsEpsilonGreedyBanditResourceUtilTest, SetEligibleSegments) {
  // Arrange

  // Act
  SetEpsilonGreedyBanditEligibleSegments(kSegments);

  // Assert
  EXPECT_EQ(kSegments, GetEpsilonGreedyBanditEligibleSegments());
}

TEST_F(BatAdsEpsilonGreedyBanditResourceUtilTest, SetNoEligibleSegments) {
  // Arrange

  // Act
  SetEpsilonGreedyBanditEligibleSegments({});

  // Assert
  const SegmentList segments = GetEpsilonGreedyBanditEligibleSegments();
  EXPECT_TRUE(segments.empty());
}

}  // namespace ads::resource
