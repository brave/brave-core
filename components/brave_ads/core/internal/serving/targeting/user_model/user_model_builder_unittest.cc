/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_builder.h"

#include <memory>
#include <vector>

#include "base/metrics/field_trial_params.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/resources/language_components_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/targeting_test_helper.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsUserModelBuilderTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    scoped_feature_list_.InitWithFeatures(
        /*enabled_features=*/{kPurchaseIntentFeature,
                              kTextClassificationFeature},
        /*disabled_features=*/{});

    targeting_helper_ =
        std::make_unique<test::TargetingHelper>(task_environment_);

    NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                     test::kCountryComponentId);

    NotifyResourceComponentDidChange(test::kLanguageComponentManifestVersion,
                                     test::kLanguageComponentId);
  }

  base::test::ScopedFeatureList scoped_feature_list_;

  std::unique_ptr<test::TargetingHelper> targeting_helper_;
};

TEST_F(BraveAdsUserModelBuilderTest, BuildUserModel) {
  // Arrange
  targeting_helper_->Mock();

  // Act & Assert
  EXPECT_EQ(test::TargetingHelper::Expectation(), BuildUserModel());
}

TEST_F(BraveAdsUserModelBuilderTest, BuildUserModelIfNoTargeting) {
  // Act & Assert
  EXPECT_EQ(UserModelInfo{}, BuildUserModel());
}

}  // namespace brave_ads
