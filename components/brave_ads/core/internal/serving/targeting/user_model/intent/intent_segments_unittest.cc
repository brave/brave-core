/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/intent/intent_segments.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/intent/intent_user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/targeting_unittest_helper.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsIntentSegmentsTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    targeting_helper_ =
        std::make_unique<test::TargetingHelper>(task_environment_);

    NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion,
                                     kCountryComponentId);
  }

  std::unique_ptr<test::TargetingHelper> targeting_helper_;
};

TEST_F(BraveAdsIntentSegmentsTest, BuildIntentSegments) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kPurchaseIntentFeature);

  targeting_helper_->MockIntent();

  // Act & Assert
  const SegmentList expected_intent_segments =
      test::TargetingHelper::IntentExpectation().segments;
  EXPECT_EQ(expected_intent_segments, BuildIntentSegments());
}

TEST_F(BraveAdsIntentSegmentsTest, BuildIntentSegmentsIfNoTargeting) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kPurchaseIntentFeature);

  // Act & Assert
  EXPECT_THAT(BuildIntentSegments(), ::testing::IsEmpty());
}

TEST_F(BraveAdsIntentSegmentsTest,
       DoNotBuildIntentSegmentsIfFeatureIsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kPurchaseIntentFeature);

  targeting_helper_->MockIntent();

  // Act & Assert
  EXPECT_THAT(BuildIntentSegments(), ::testing::IsEmpty());
}

}  // namespace brave_ads
