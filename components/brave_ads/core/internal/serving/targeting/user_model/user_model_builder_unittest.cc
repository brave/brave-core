/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_builder.h"

#include <memory>
#include <vector>

#include "base/metrics/field_trial_params.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/resources/language_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/targeting_unittest_helper.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsUserModelBuilderTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    SetUpFeatures();

    targeting_ = std::make_unique<TargetingHelperForTesting>();

    NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion,
                                     kCountryComponentId);

    NotifyDidUpdateResourceComponent(kLanguageComponentManifestVersion,
                                     kLanguageComponentId);

    NotifyDidInitializeAds();

    task_environment_.RunUntilIdle();
  }

  void SetUpFeatures() {
    std::vector<base::test::FeatureRefAndParams> enabled_features;
    base::FieldTrialParams params;
    enabled_features.emplace_back(kEpsilonGreedyBanditFeature, params);
    enabled_features.emplace_back(kPurchaseIntentFeature, params);
    enabled_features.emplace_back(kTextClassificationFeature, params);
    enabled_features.emplace_back(kTextEmbeddingFeature, params);

    const std::vector<base::test::FeatureRef> disabled_features;

    scoped_feature_list_.InitWithFeaturesAndParameters(enabled_features,
                                                       disabled_features);
  }

  base::test::ScopedFeatureList scoped_feature_list_;

  std::unique_ptr<TargetingHelperForTesting> targeting_;
};

TEST_F(BraveAdsUserModelBuilderTest, BuildUserModel) {
  // Arrange
  targeting_->Mock();

  base::MockCallback<BuildUserModelCallback> callback;
  EXPECT_CALL(callback, Run).WillOnce([](const UserModelInfo& user_model) {
    EXPECT_EQ(TargetingHelperForTesting::Expectation(), user_model);
  });

  // Act
  BuildUserModel(callback.Get());

  // Assert
}

TEST_F(BraveAdsUserModelBuilderTest, BuildUserModelIfNoTargeting) {
  // Arrange
  const UserModelInfo expected_user_model;

  base::MockCallback<BuildUserModelCallback> callback;
  EXPECT_CALL(callback, Run).WillOnce([=](const UserModelInfo& user_model) {
    EXPECT_EQ(expected_user_model, user_model);
  });

  // Act
  BuildUserModel(callback.Get());

  // Assert
}

}  // namespace brave_ads
