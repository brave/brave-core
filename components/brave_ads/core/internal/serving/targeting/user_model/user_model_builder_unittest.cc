/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_builder.h"

#include <memory>
#include <vector>

#include "base/metrics/field_trial_params.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/resources/language_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/targeting_unittest_helper.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsUserModelBuilderTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    SetUpFeatures();

    targeting_ = std::make_unique<test::TargetingHelper>();

    LoadResources();

    NotifyDidInitializeAds();
  }

  void LoadResources() {
    NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion,
                                     kCountryComponentId);
    task_environment_.RunUntilIdle();
    NotifyDidUpdateResourceComponent(kLanguageComponentManifestVersion,
                                     kLanguageComponentId);
    task_environment_.RunUntilIdle();
  }

  void SetUpFeatures() {
    std::vector<base::test::FeatureRefAndParams> enabled_features;

    enabled_features.emplace_back(kPurchaseIntentFeature,
                                  base::FieldTrialParams({}));

    enabled_features.emplace_back(kTextClassificationFeature,
                                  base::FieldTrialParams({}));

    const std::vector<base::test::FeatureRef> disabled_features;

    scoped_feature_list_.InitWithFeaturesAndParameters(enabled_features,
                                                       disabled_features);
  }

  base::test::ScopedFeatureList scoped_feature_list_;

  std::unique_ptr<test::TargetingHelper> targeting_;
};

TEST_F(BraveAdsUserModelBuilderTest, BuildUserModel) {
  // Arrange
  targeting_->Mock();
  task_environment_.RunUntilIdle();

  // Act & Assert
  EXPECT_EQ(test::TargetingHelper::Expectation(), BuildUserModel());
}

TEST_F(BraveAdsUserModelBuilderTest, BuildUserModelIfNoTargeting) {
  // Arrange
  const UserModelInfo expected_user_model;

  // Act & Assert
  EXPECT_EQ(expected_user_model, BuildUserModel());
}

}  // namespace brave_ads
