/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/inline_content_ad_serving.h"

#include <utility>

#include "base/metrics/field_trial_params.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/internal/serving/inline_content_ad_serving_delegate.h"
#include "brave/components/brave_ads/core/internal/serving/inline_content_ad_serving_feature.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/public/units/inline_content_ad/inline_content_ad_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class InlineContentAdServingDelegateForTesting
    : public InlineContentAdServingDelegate {
 public:
  const InlineContentAdInfo& ad() const { return ad_; }

  bool opportunity_arose_to_serve_ad() const {
    return opportunity_arose_to_serve_ad_;
  }

  bool did_serve_ad() const { return did_serve_ad_; }

  bool failed_to_serve_ad() const { return failed_to_serve_ad_; }

 private:
  // InlineContentAdServingDelegate:
  void OnOpportunityAroseToServeInlineContentAd(
      const SegmentList& /*segments*/) override {
    opportunity_arose_to_serve_ad_ = true;
  }

  void OnDidServeInlineContentAd(const InlineContentAdInfo& ad) override {
    ad_ = ad;
    did_serve_ad_ = true;
  }

  void OnFailedToServeInlineContentAd() override { failed_to_serve_ad_ = true; }

  InlineContentAdInfo ad_;
  bool opportunity_arose_to_serve_ad_ = false;
  bool did_serve_ad_ = false;
  bool failed_to_serve_ad_ = false;
};

class BraveAdsInlineContentAdServingTest : public UnitTestBase {
 protected:
  void MaybeServeAd(const std::string& dimensions,
                    MaybeServeInlineContentAdCallback callback) {
    SubdivisionTargeting subdivision_targeting;
    AntiTargetingResource anti_targeting_resource;
    InlineContentAdServing ad_serving(subdivision_targeting,
                                      anti_targeting_resource);
    ad_serving.SetDelegate(&ad_serving_delegate_);

    ad_serving.MaybeServeAd(dimensions, std::move(callback));
  }

  InlineContentAdServingDelegateForTesting ad_serving_delegate_;
};

TEST_F(BraveAdsInlineContentAdServingTest, DoNotServeAdForUnsupportedVersion) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  database::SaveCreativeInlineContentAds({creative_ad});

  base::FieldTrialParams params;
  params["version"] = "0";
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kInlineContentAdServingFeature, params);

  // Act
  base::MockCallback<MaybeServeInlineContentAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([=](const std::string& /*dimensions*/,
                    const absl::optional<InlineContentAdInfo>& ad) {
        // Assert
        EXPECT_FALSE(ad);
        EXPECT_FALSE(ad_serving_delegate_.opportunity_arose_to_serve_ad());
        EXPECT_FALSE(ad_serving_delegate_.did_serve_ad());
        EXPECT_TRUE(ad_serving_delegate_.failed_to_serve_ad());
      });

  MaybeServeAd("200x100", callback.Get());
}

TEST_F(BraveAdsInlineContentAdServingTest, ServeAd) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  database::SaveCreativeInlineContentAds({creative_ad});

  // Act
  base::MockCallback<MaybeServeInlineContentAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([=](const std::string& /*dimensions*/,
                    const absl::optional<InlineContentAdInfo>& ad) {
        // Assert
        EXPECT_TRUE(ad);
        EXPECT_TRUE(ad_serving_delegate_.opportunity_arose_to_serve_ad());
        EXPECT_TRUE(ad_serving_delegate_.did_serve_ad());
        EXPECT_FALSE(ad_serving_delegate_.failed_to_serve_ad());
        EXPECT_EQ(ad, ad_serving_delegate_.ad());
      });

  MaybeServeAd("200x100", callback.Get());
}

TEST_F(BraveAdsInlineContentAdServingTest,
       DoNotServeAdForNonExistentDimensions) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  database::SaveCreativeInlineContentAds({creative_ad});

  // Act
  base::MockCallback<MaybeServeInlineContentAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([=](const std::string& /*dimensions*/,
                    const absl::optional<InlineContentAdInfo>& ad) {
        // Assert
        EXPECT_FALSE(ad);
        EXPECT_FALSE(ad_serving_delegate_.opportunity_arose_to_serve_ad());
        EXPECT_FALSE(ad_serving_delegate_.did_serve_ad());
        EXPECT_TRUE(ad_serving_delegate_.failed_to_serve_ad());
      });

  MaybeServeAd("?x?", callback.Get());
}

TEST_F(BraveAdsInlineContentAdServingTest,
       DoNotServeAdIfNotAllowedDueToPermissionRules) {
  // Arrange
  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  database::SaveCreativeInlineContentAds({creative_ad});

  // Act
  base::MockCallback<MaybeServeInlineContentAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([=](const std::string& /*dimensions*/,
                    const absl::optional<InlineContentAdInfo>& ad) {
        // Assert
        EXPECT_FALSE(ad);
        EXPECT_FALSE(ad_serving_delegate_.opportunity_arose_to_serve_ad());
        EXPECT_FALSE(ad_serving_delegate_.did_serve_ad());
        EXPECT_TRUE(ad_serving_delegate_.failed_to_serve_ad());
      });

  MaybeServeAd("200x100", callback.Get());
}

}  // namespace brave_ads
