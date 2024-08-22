/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/intent/intent_segments.h"

#include <memory>

#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/intent/intent_user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/targeting_test_helper.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsIntentSegmentsTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    targeting_helper_ =
        std::make_unique<test::TargetingHelper>(task_environment_);

    NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                     test::kCountryComponentId);
  }

  std::unique_ptr<test::TargetingHelper> targeting_helper_;
};

TEST_F(BraveAdsIntentSegmentsTest, BuildIntentSegments) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kPurchaseIntentFeature);

  targeting_helper_->MockIntent();

  // Act & Assert
  base::MockCallback<BuildSegmentsCallback> callback;
  EXPECT_CALL(callback,
              Run(test::TargetingHelper::IntentExpectation().segments));
  BuildIntentSegments(callback.Get());
}

TEST_F(BraveAdsIntentSegmentsTest, BuildIntentSegmentsIfNoTargeting) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kPurchaseIntentFeature);

  // Act & Assert
  base::MockCallback<BuildSegmentsCallback> callback;
  EXPECT_CALL(callback, Run(/*segments=*/::testing::IsEmpty()));
  BuildIntentSegments(callback.Get());
}

TEST_F(BraveAdsIntentSegmentsTest,
       DoNotBuildIntentSegmentsIfFeatureIsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kPurchaseIntentFeature);

  targeting_helper_->MockIntent();

  // Act & Assert
  base::MockCallback<BuildSegmentsCallback> callback;
  EXPECT_CALL(callback, Run(/*segments=*/::testing::IsEmpty()));
  BuildIntentSegments(callback.Get());
}

}  // namespace brave_ads
