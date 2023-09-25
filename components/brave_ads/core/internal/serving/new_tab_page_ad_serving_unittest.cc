/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/new_tab_page_ad_serving.h"

#include <utility>

#include "base/metrics/field_trial_params.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/serving/new_tab_page_ad_serving_delegate.h"
#include "brave/components/brave_ads/core/internal/serving/new_tab_page_ad_serving_feature.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class NewTabPageAdServingDelegateForTesting
    : public NewTabPageAdServingDelegate {
 public:
  const NewTabPageAdInfo& ad() const { return ad_; }

  bool opportunity_arose_to_serve_ad() const {
    return opportunity_arose_to_serve_ad_;
  }

  bool did_serve_ad() const { return did_serve_ad_; }

  bool failed_to_serve_ad() const { return failed_to_serve_ad_; }

 private:
  // NewTabPageAdServingDelegate:
  void OnOpportunityAroseToServeNewTabPageAd() override {
    opportunity_arose_to_serve_ad_ = true;
  }

  void OnDidServeNewTabPageAd(const NewTabPageAdInfo& ad) override {
    ad_ = ad;
    did_serve_ad_ = true;
  }

  void OnFailedToServeNewTabPageAd() override { failed_to_serve_ad_ = true; }

  NewTabPageAdInfo ad_;
  bool opportunity_arose_to_serve_ad_ = false;
  bool did_serve_ad_ = false;
  bool failed_to_serve_ad_ = false;
};

class BraveAdsNewTabPageAdServingTest : public UnitTestBase {
 protected:
  void MaybeServeAd(MaybeServeNewTabPageAdCallback callback) {
    SubdivisionTargeting subdivision_targeting;
    AntiTargetingResource anti_targeting_resource;
    NewTabPageAdServing ad_serving(subdivision_targeting,
                                   anti_targeting_resource);
    ad_serving.SetDelegate(&ad_serving_delegate_);

    ad_serving.MaybeServeAd(std::move(callback));
  }

  NewTabPageAdServingDelegateForTesting ad_serving_delegate_;
};

TEST_F(BraveAdsNewTabPageAdServingTest, DoNotServeAdForUnsupportedVersion) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativeNewTabPageAdInfo creative_ad =
      BuildCreativeNewTabPageAdForTesting(/*should_use_random_uuids*/ true);
  database::SaveCreativeNewTabPageAds({creative_ad});

  base::FieldTrialParams params;
  params["version"] = "0";
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNewTabPageAdServingFeature, params);

  // Act
  base::MockCallback<MaybeServeNewTabPageAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([=](const absl::optional<NewTabPageAdInfo>& ad) {
        // Assert
        EXPECT_FALSE(ad);
        EXPECT_FALSE(ad_serving_delegate_.opportunity_arose_to_serve_ad());
        EXPECT_FALSE(ad_serving_delegate_.did_serve_ad());
        EXPECT_TRUE(ad_serving_delegate_.failed_to_serve_ad());
      });

  MaybeServeAd(callback.Get());
}

TEST_F(BraveAdsNewTabPageAdServingTest, ServeAd) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativeNewTabPageAdInfo creative_ad =
      BuildCreativeNewTabPageAdForTesting(/*should_use_random_uuids*/ true);
  database::SaveCreativeNewTabPageAds({creative_ad});

  // Act
  base::MockCallback<MaybeServeNewTabPageAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([=](const absl::optional<NewTabPageAdInfo>& ad) {
        // Assert
        EXPECT_TRUE(ad);
        EXPECT_TRUE(ad_serving_delegate_.opportunity_arose_to_serve_ad());
        EXPECT_TRUE(ad_serving_delegate_.did_serve_ad());
        EXPECT_FALSE(ad_serving_delegate_.failed_to_serve_ad());
        EXPECT_EQ(ad, ad_serving_delegate_.ad());
      });

  MaybeServeAd(callback.Get());
}

TEST_F(BraveAdsNewTabPageAdServingTest, DoNotServeAdIfMissingWallpapers) {
  // Arrange
  ForcePermissionRulesForTesting();

  CreativeNewTabPageAdInfo creative_ad =
      BuildCreativeNewTabPageAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad.wallpapers.clear();
  database::SaveCreativeNewTabPageAds({creative_ad});

  // Act
  base::MockCallback<MaybeServeNewTabPageAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([=](const absl::optional<NewTabPageAdInfo>& ad) {
        // Assert
        EXPECT_FALSE(ad);
        EXPECT_TRUE(ad_serving_delegate_.opportunity_arose_to_serve_ad());
        EXPECT_FALSE(ad_serving_delegate_.did_serve_ad());
        EXPECT_TRUE(ad_serving_delegate_.failed_to_serve_ad());
      });

  MaybeServeAd(callback.Get());
}

TEST_F(BraveAdsNewTabPageAdServingTest, DoNotServeAdIfNoEligibleAdsFound) {
  // Arrange
  ForcePermissionRulesForTesting();

  // Act
  base::MockCallback<MaybeServeNewTabPageAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([=](const absl::optional<NewTabPageAdInfo>& ad) {
        // Assert
        EXPECT_FALSE(ad);
        EXPECT_TRUE(ad_serving_delegate_.opportunity_arose_to_serve_ad());
        EXPECT_FALSE(ad_serving_delegate_.did_serve_ad());
        EXPECT_TRUE(ad_serving_delegate_.failed_to_serve_ad());
      });

  MaybeServeAd(callback.Get());
}

TEST_F(BraveAdsNewTabPageAdServingTest,
       DoNotServeAdIfNotAllowedDueToPermissionRules) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad =
      BuildCreativeNewTabPageAdForTesting(/*should_use_random_uuids*/ true);
  database::SaveCreativeNewTabPageAds({creative_ad});

  // Act
  base::MockCallback<MaybeServeNewTabPageAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([=](const absl::optional<NewTabPageAdInfo>& ad) {
        // Assert
        EXPECT_FALSE(ad);
        EXPECT_FALSE(ad_serving_delegate_.opportunity_arose_to_serve_ad());
        EXPECT_FALSE(ad_serving_delegate_.did_serve_ad());
        EXPECT_TRUE(ad_serving_delegate_.failed_to_serve_ad());
      });

  MaybeServeAd(callback.Get());
}

}  // namespace brave_ads
