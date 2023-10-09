/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/latent_interest/latent_interest_segments.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/latent_interest/latent_interest_user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/targeting_unittest_helper.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsLatentInterestSegmentsTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    targeting_ = std::make_unique<test::TargetingHelper>();

    NotifyDidInitializeAds();
  }

  std::unique_ptr<test::TargetingHelper> targeting_;
};

TEST_F(BraveAdsLatentInterestSegmentsTest, BuildLatentInterestSegments) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kEpsilonGreedyBanditFeature, {{"epsilon_value", "0.0"}});

  targeting_->MockLatentInterest();

  // Act & Assert
  const SegmentList expected_latent_interest_segments =
      test::TargetingHelper::LatentInterestExpectation().segments;
  EXPECT_EQ(expected_latent_interest_segments, BuildLatentInterestSegments());
}

TEST_F(BraveAdsLatentInterestSegmentsTest,
       BuildLatentInterestSegmentsIfNoTargeting) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kEpsilonGreedyBanditFeature);

  // Act
  const SegmentList segments = BuildLatentInterestSegments();

  // Assert
  EXPECT_TRUE(segments.empty());
}

TEST_F(BraveAdsLatentInterestSegmentsTest,
       DoNotBuildLatentInterestSegmentsIfFeatureIsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kEpsilonGreedyBanditFeature);

  targeting_->MockLatentInterest();

  // Act
  const SegmentList segments = BuildLatentInterestSegments();

  // Assert
  EXPECT_TRUE(segments.empty());
}

}  // namespace brave_ads
