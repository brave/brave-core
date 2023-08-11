/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/inline_content_ad_serving.h"

#include <memory>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/ads/serving/inline_content_ad_serving_delegate.h"
#include "brave/components/brave_ads/core/internal/ads/serving/inline_content_ad_serving_feature_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsInlineContentAdServingDelegate
    : public InlineContentAdServingDelegate {
 public:
  void OnOpportunityAroseToServeInlineContentAd(
      const SegmentList& /*segments*/) override {
    opportunity_arose_to_serve_ad_ = true;
  }

  void OnDidServeInlineContentAd(const InlineContentAdInfo& ad) override {
    ad_ = ad;
    did_serve_ad_ = true;
  }

  void OnFailedToServeInlineContentAd() override { failed_to_serve_ad_ = true; }

  const InlineContentAdInfo& ad() const { return ad_; }

  bool opportunity_arose_to_serve_ad() const {
    return opportunity_arose_to_serve_ad_;
  }

  bool did_serve_ad() const { return did_serve_ad_; }

  bool failed_to_serve_ad() const { return failed_to_serve_ad_; }

 private:
  InlineContentAdInfo ad_;
  bool opportunity_arose_to_serve_ad_ = false;
  bool did_serve_ad_ = false;
  bool failed_to_serve_ad_ = false;
};

class BraveAdsInlineContentAdServingTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    ForceInlineContentAdServingVersionForTesting(1);

    subdivision_targeting_ = std::make_unique<SubdivisionTargeting>();
    anti_targeting_resource_ = std::make_unique<AntiTargetingResource>();
    serving_ = std::make_unique<InlineContentAdServing>(
        *subdivision_targeting_, *anti_targeting_resource_);
    serving_->SetDelegate(&serving_delegate_);
  }

  std::unique_ptr<SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<AntiTargetingResource> anti_targeting_resource_;
  std::unique_ptr<InlineContentAdServing> serving_;

  BraveAdsInlineContentAdServingDelegate serving_delegate_;
};

TEST_F(BraveAdsInlineContentAdServingTest, DoNotServeAdForUnsupportedVersion) {
  // Arrange
  ForceInlineContentAdServingVersionForTesting(0);

  // Act
  serving_->MaybeServeAd(
      "200x100",
      base::BindOnce(
          [](BraveAdsInlineContentAdServingDelegate* serving_delegate,
             const std::string& /*dimensions*/,
             const absl::optional<InlineContentAdInfo>& ad) {
            // Assert
            EXPECT_FALSE(ad);
            EXPECT_FALSE(serving_delegate->opportunity_arose_to_serve_ad());
            EXPECT_FALSE(serving_delegate->did_serve_ad());
            EXPECT_TRUE(serving_delegate->failed_to_serve_ad());
          },
          base::Unretained(&serving_delegate_)));
}

TEST_F(BraveAdsInlineContentAdServingTest, ServeAd) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  database::SaveCreativeInlineContentAds({creative_ad});

  // Act
  serving_->MaybeServeAd(
      "200x100",
      base::BindOnce(
          [](BraveAdsInlineContentAdServingDelegate* serving_delegate,
             const std::string& /*dimensions*/,
             const absl::optional<InlineContentAdInfo>& ad) {
            // Assert
            EXPECT_TRUE(ad);
            EXPECT_TRUE(serving_delegate->opportunity_arose_to_serve_ad());
            EXPECT_TRUE(serving_delegate->did_serve_ad());
            EXPECT_FALSE(serving_delegate->failed_to_serve_ad());
            EXPECT_EQ(ad, serving_delegate->ad());
          },
          base::Unretained(&serving_delegate_)));
}

TEST_F(BraveAdsInlineContentAdServingTest,
       DoNotServeAdForNonExistentDimensions) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  database::SaveCreativeInlineContentAds({creative_ad});

  // Act
  serving_->MaybeServeAd(
      "?x?",
      base::BindOnce(
          [](BraveAdsInlineContentAdServingDelegate* serving_delegate,
             const std::string& /*dimensions*/,
             const absl::optional<InlineContentAdInfo>& ad) {
            // Assert
            EXPECT_FALSE(ad);
            EXPECT_FALSE(serving_delegate->opportunity_arose_to_serve_ad());
            EXPECT_FALSE(serving_delegate->did_serve_ad());
            EXPECT_TRUE(serving_delegate->failed_to_serve_ad());
          },
          base::Unretained(&serving_delegate_)));

  // Assert
}

TEST_F(BraveAdsInlineContentAdServingTest,
       DoNotServeAdIfNotAllowedDueToPermissionRules) {
  // Arrange
  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  database::SaveCreativeInlineContentAds({creative_ad});

  // Act
  serving_->MaybeServeAd(
      "200x100",
      base::BindOnce(
          [](BraveAdsInlineContentAdServingDelegate* serving_delegate,
             const std::string& /*dimensions*/,
             const absl::optional<InlineContentAdInfo>& ad) {
            // Assert
            EXPECT_FALSE(ad);
            EXPECT_FALSE(serving_delegate->opportunity_arose_to_serve_ad());
            EXPECT_FALSE(serving_delegate->did_serve_ad());
            EXPECT_TRUE(serving_delegate->failed_to_serve_ad());
          },
          base::Unretained(&serving_delegate_)));

  // Assert
}

}  // namespace brave_ads
