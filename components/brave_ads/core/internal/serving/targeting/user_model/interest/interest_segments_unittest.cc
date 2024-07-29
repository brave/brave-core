/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/interest/interest_segments.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/resources/language_components_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/interest/interest_user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/targeting_test_helper.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsInterestSegmentsTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    targeting_helper_ =
        std::make_unique<test::TargetingHelper>(task_environment_);

    NotifyResourceComponentDidChange(test::kLanguageComponentManifestVersion,
                                     test::kLanguageComponentId);
  }

  std::unique_ptr<test::TargetingHelper> targeting_helper_;
};

TEST_F(BraveAdsInterestSegmentsTest, BuildInterestSegments) {
  // Arrange
  targeting_helper_->MockInterest();

  // Act
  const SegmentList interest_segments = BuildInterestSegments();

  // Assert
  EXPECT_EQ(test::TargetingHelper::InterestExpectation().segments,
            interest_segments);
}

TEST_F(BraveAdsInterestSegmentsTest, BuildInterestSegmentsIfNoTargeting) {
  // Act
  const SegmentList interest_segments = BuildInterestSegments();

  // Assert
  EXPECT_THAT(interest_segments, ::testing::IsEmpty());
}

TEST_F(BraveAdsInterestSegmentsTest,
       DoNotBuildInterestSegmentsIfFeatureIsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kTextClassificationFeature);

  targeting_helper_->MockInterest();

  // Act
  const SegmentList interest_segments = BuildInterestSegments();

  // Assert
  EXPECT_THAT(interest_segments, ::testing::IsEmpty());
}

}  // namespace brave_ads
