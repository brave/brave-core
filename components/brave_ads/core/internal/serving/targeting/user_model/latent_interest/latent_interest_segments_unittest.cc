/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/latent_interest/latent_interest_segments.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/latent_interest/latent_interest_user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/targeting_test_helper.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsLatentInterestSegmentsTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    targeting_helper_ =
        std::make_unique<test::TargetingHelper>(task_environment_);
  }

  std::unique_ptr<test::TargetingHelper> targeting_helper_;
};

TEST_F(BraveAdsLatentInterestSegmentsTest, BuildLatentInterestSegments) {
  // Arrange
  targeting_helper_->MockLatentInterest();

  // Act
  const SegmentList latent_interest_segments = BuildLatentInterestSegments();

  // Assert
  EXPECT_EQ(test::TargetingHelper::LatentInterestExpectation().segments,
            latent_interest_segments);
}

TEST_F(BraveAdsLatentInterestSegmentsTest,
       BuildLatentInterestSegmentsIfNoTargeting) {
  // Act
  const SegmentList latent_interest_segments = BuildLatentInterestSegments();

  // Assert
  EXPECT_THAT(latent_interest_segments, ::testing::IsEmpty());
}

TEST_F(BraveAdsLatentInterestSegmentsTest,
       DoNotBuildLatentInterestSegmentsIfFeatureIsDisabled) {
  // Arrange
  targeting_helper_->MockLatentInterest();

  // Act
  const SegmentList latent_interest_segments = BuildLatentInterestSegments();

  // Assert
  EXPECT_THAT(latent_interest_segments, ::testing::IsEmpty());
}

}  // namespace brave_ads
