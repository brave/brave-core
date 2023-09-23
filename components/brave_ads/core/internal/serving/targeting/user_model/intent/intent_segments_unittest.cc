/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/intent/intent_segments.h"

#include <memory>
#include <vector>

#include "base/metrics/field_trial_params.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/resources/language_components_unittest_constants.h"
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

    targeting_ = std::make_unique<TargetingHelperForTesting>();

    NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion,
                                     kCountryComponentId);

    NotifyDidUpdateResourceComponent(kLanguageComponentManifestVersion,
                                     kLanguageComponentId);

    NotifyDidInitializeAds();

    task_environment_.RunUntilIdle();
  }

  base::test::ScopedFeatureList scoped_feature_list_;

  std::unique_ptr<TargetingHelperForTesting> targeting_;
};

TEST_F(BraveAdsIntentSegmentsTest, BuildIntentSegments) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  enabled_features.emplace_back(kPurchaseIntentFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  scoped_feature_list_.InitWithFeaturesAndParameters(enabled_features,
                                                     disabled_features);

  targeting_->MockIntent();

  // Act

  // Assert
  const SegmentList expected_intent_segments =
      TargetingHelperForTesting::IntentExpectation().segments;
  EXPECT_EQ(expected_intent_segments, BuildIntentSegments());
}

TEST_F(BraveAdsIntentSegmentsTest, BuildIntentSegmentsIfNoTargeting) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  enabled_features.emplace_back(kPurchaseIntentFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  scoped_feature_list_.InitWithFeaturesAndParameters(enabled_features,
                                                     disabled_features);

  // Act
  const SegmentList segments = BuildIntentSegments();

  // Assert
  EXPECT_TRUE(segments.empty());
}

TEST_F(BraveAdsIntentSegmentsTest,
       DoNotBuildIntentSegmentsIfFeatureIsDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kPurchaseIntentFeature);

  scoped_feature_list_.InitWithFeaturesAndParameters(enabled_features,
                                                     disabled_features);

  targeting_->MockIntent();

  // Act
  const SegmentList segments = BuildIntentSegments();

  // Assert
  EXPECT_TRUE(segments.empty());
}

}  // namespace brave_ads
