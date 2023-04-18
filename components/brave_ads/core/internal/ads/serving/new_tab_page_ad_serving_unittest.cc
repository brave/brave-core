/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/new_tab_page_ad_serving.h"

#include <memory>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads/serving/new_tab_page_ad_serving_delegate.h"
#include "brave/components/brave_ads/core/internal/ads/serving/new_tab_page_ad_serving_features_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/new_tab_page_ad_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::new_tab_page_ads {

class BraveAdsNewTabPageAdServingDelegate : public ServingDelegate {
 public:
  void OnOpportunityAroseToServeNewTabPageAd(
      const SegmentList& /*segments*/) override {
    opportunity_arose_to_serve_ad_ = true;
  }

  void OnDidServeNewTabPageAd(const NewTabPageAdInfo& ad) override {
    ad_ = ad;
    did_serve_ad_ = true;
  }

  void OnFailedToServeNewTabPageAd() override { failed_to_serve_ad_ = true; }

  const NewTabPageAdInfo& ad() const { return ad_; }

  bool opportunity_arose_to_serve_ad() const {
    return opportunity_arose_to_serve_ad_;
  }

  bool did_serve_ad() const { return did_serve_ad_; }

  bool failed_to_serve_ad() const { return failed_to_serve_ad_; }

 private:
  NewTabPageAdInfo ad_;
  bool opportunity_arose_to_serve_ad_ = false;
  bool did_serve_ad_ = false;
  bool failed_to_serve_ad_ = false;
};

class BraveAdsNewTabPageAdServingTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    ForceServingVersion(1);

    subdivision_targeting_ = std::make_unique<SubdivisionTargeting>();
    anti_targeting_resource_ = std::make_unique<resource::AntiTargeting>();
    serving_ = std::make_unique<Serving>(*subdivision_targeting_,
                                         *anti_targeting_resource_);
    serving_->SetDelegate(&serving_delegate_);
  }

  std::unique_ptr<SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<resource::AntiTargeting> anti_targeting_resource_;
  std::unique_ptr<Serving> serving_;

  BraveAdsNewTabPageAdServingDelegate serving_delegate_;
};

TEST_F(BraveAdsNewTabPageAdServingTest, DoNotServeAdForUnsupportedVersion) {
  // Arrange
  ForceServingVersion(0);

  // Act
  serving_->MaybeServeAd(base::BindOnce(
      [](BraveAdsNewTabPageAdServingDelegate* serving_delegate,
         const absl::optional<NewTabPageAdInfo>& ad) {
        // Assert
        EXPECT_FALSE(ad);
        EXPECT_FALSE(serving_delegate->opportunity_arose_to_serve_ad());
        EXPECT_FALSE(serving_delegate->did_serve_ad());
        EXPECT_TRUE(serving_delegate->failed_to_serve_ad());
      },
      base::Unretained(&serving_delegate_)));
}

TEST_F(BraveAdsNewTabPageAdServingTest, ServeAd) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativeNewTabPageAdInfo creative_ad =
      BuildCreativeNewTabPageAd(/*should_use_random_guids*/ true);
  SaveCreativeAds({creative_ad});

  // Act
  serving_->MaybeServeAd(base::BindOnce(
      [](BraveAdsNewTabPageAdServingDelegate* serving_delegate,
         const absl::optional<NewTabPageAdInfo>& ad) {
        // Assert
        EXPECT_TRUE(ad);
        EXPECT_TRUE(serving_delegate->opportunity_arose_to_serve_ad());
        EXPECT_TRUE(serving_delegate->did_serve_ad());
        EXPECT_FALSE(serving_delegate->failed_to_serve_ad());
        EXPECT_EQ(ad, serving_delegate->ad());
      },
      base::Unretained(&serving_delegate_)));
}

TEST_F(BraveAdsNewTabPageAdServingTest, DoNotServeAdIfMissingWallpapers) {
  // Arrange
  ForcePermissionRulesForTesting();

  CreativeNewTabPageAdInfo creative_ad =
      BuildCreativeNewTabPageAd(/*should_use_random_guids*/ true);
  creative_ad.wallpapers.clear();
  SaveCreativeAds({creative_ad});

  // Act
  serving_->MaybeServeAd(base::BindOnce(
      [](BraveAdsNewTabPageAdServingDelegate* serving_delegate,
         const absl::optional<NewTabPageAdInfo>& ad) {
        // Assert
        EXPECT_FALSE(ad);
        EXPECT_FALSE(serving_delegate->opportunity_arose_to_serve_ad());
        EXPECT_FALSE(serving_delegate->did_serve_ad());
        EXPECT_TRUE(serving_delegate->failed_to_serve_ad());
      },
      base::Unretained(&serving_delegate_)));
}

TEST_F(BraveAdsNewTabPageAdServingTest, DoNotServeAdIfNoEligibleAdsFound) {
  // Arrange
  ForcePermissionRulesForTesting();

  // Act
  serving_->MaybeServeAd(base::BindOnce(
      [](BraveAdsNewTabPageAdServingDelegate* serving_delegate,
         const absl::optional<NewTabPageAdInfo>& ad) {
        // Assert
        EXPECT_FALSE(ad);
        EXPECT_FALSE(serving_delegate->opportunity_arose_to_serve_ad());
        EXPECT_FALSE(serving_delegate->did_serve_ad());
        EXPECT_TRUE(serving_delegate->failed_to_serve_ad());
      },
      base::Unretained(&serving_delegate_)));
}

TEST_F(BraveAdsNewTabPageAdServingTest,
       DoNotServeAdIfNotAllowedDueToPermissionRules) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad =
      BuildCreativeNewTabPageAd(/*should_use_random_guids*/ true);
  SaveCreativeAds({creative_ad});

  // Act
  serving_->MaybeServeAd(base::BindOnce(
      [](BraveAdsNewTabPageAdServingDelegate* serving_delegate,
         const absl::optional<NewTabPageAdInfo>& ad) {
        // Assert
        EXPECT_FALSE(ad);
        EXPECT_FALSE(serving_delegate->opportunity_arose_to_serve_ad());
        EXPECT_FALSE(serving_delegate->did_serve_ad());
        EXPECT_TRUE(serving_delegate->failed_to_serve_ad());
      },
      base::Unretained(&serving_delegate_)));
}

}  // namespace brave_ads::new_tab_page_ads
