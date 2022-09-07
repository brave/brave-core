/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/new_tab_page_ad_serving.h"

#include <memory>

#include "absl/types/optional.h"
#include "bat/ads/internal/ads/serving/new_tab_page_ad_serving_observer.h"
#include "bat/ads/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "bat/ads/internal/ads/serving/serving_features_unittest_util.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_unittest_util.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/segments/segment_alias.h"
#include "bat/ads/new_tab_page_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::new_tab_page_ads {

class BatAdsNewTabPageAdServingTest : public ServingObserver,
                                      public UnitTestBase {
 protected:
  BatAdsNewTabPageAdServingTest() = default;

  ~BatAdsNewTabPageAdServingTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUp();

    features::ForceServingVersion(1);

    subdivision_targeting_ =
        std::make_unique<geographic::SubdivisionTargeting>();
    anti_targeting_resource_ = std::make_unique<resource::AntiTargeting>();
    serving_ = std::make_unique<Serving>(subdivision_targeting_.get(),
                                         anti_targeting_resource_.get());
    serving_->AddObserver(this);
  }

  void TearDown() override {
    serving_->RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  void OnOpportunityAroseToServeNewTabPageAd(
      const SegmentList& /*segments*/) override {
    had_opportunuity_ = true;
  }

  void OnDidServeNewTabPageAd(const NewTabPageAdInfo& ad) override {
    ad_ = ad;
    did_serve_ad_ = true;
  }

  void OnFailedToServeNewTabPageAd() override { failed_to_serve_ad_ = true; }

  std::unique_ptr<geographic::SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<resource::AntiTargeting> anti_targeting_resource_;
  std::unique_ptr<Serving> serving_;

  NewTabPageAdInfo ad_;
  bool had_opportunuity_ = false;
  bool did_serve_ad_ = false;
  bool failed_to_serve_ad_ = false;
};

TEST_F(BatAdsNewTabPageAdServingTest, DoNotServeAdForUnsupportedVersion) {
  // Arrange
  features::ForceServingVersion(0);

  // Act
  serving_->MaybeServeAd([=](const absl::optional<NewTabPageAdInfo>& ad) {
    // Assert
    EXPECT_FALSE(ad);
    EXPECT_FALSE(had_opportunuity_);
    EXPECT_FALSE(did_serve_ad_);
    EXPECT_TRUE(failed_to_serve_ad_);
  });
}

TEST_F(BatAdsNewTabPageAdServingTest, ServeAd) {
  // Arrange
  ForcePermissionRules();

  CreativeNewTabPageAdList creative_ads;
  const CreativeNewTabPageAdInfo creative_ad = BuildCreativeNewTabPageAd();
  creative_ads.push_back(creative_ad);
  SaveCreativeAds(creative_ads);

  // Act
  serving_->MaybeServeAd([=](const absl::optional<NewTabPageAdInfo>& ad) {
    // Assert
    EXPECT_TRUE(ad);
    EXPECT_TRUE(had_opportunuity_);
    EXPECT_TRUE(did_serve_ad_);
    EXPECT_FALSE(failed_to_serve_ad_);
    EXPECT_EQ(ad, ad_);
  });
}

TEST_F(BatAdsNewTabPageAdServingTest, DoNotServeAdIfMissingWallpapers) {
  // Arrange
  ForcePermissionRules();

  CreativeNewTabPageAdList creative_ads;
  CreativeNewTabPageAdInfo creative_ad = BuildCreativeNewTabPageAd();
  creative_ad.wallpapers = {};
  creative_ads.push_back(creative_ad);
  SaveCreativeAds(creative_ads);

  // Act
  serving_->MaybeServeAd([=](const absl::optional<NewTabPageAdInfo>& ad) {
    // Assert
    EXPECT_FALSE(ad);
    EXPECT_FALSE(had_opportunuity_);
    EXPECT_FALSE(did_serve_ad_);
    EXPECT_TRUE(failed_to_serve_ad_);
  });
}

TEST_F(BatAdsNewTabPageAdServingTest, DoNotServeAdIfNoEligibleAdsFound) {
  // Arrange
  ForcePermissionRules();

  // Act
  serving_->MaybeServeAd([=](const absl::optional<NewTabPageAdInfo>& ad) {
    // Assert
    EXPECT_FALSE(ad);
    EXPECT_FALSE(had_opportunuity_);
    EXPECT_FALSE(did_serve_ad_);
    EXPECT_TRUE(failed_to_serve_ad_);
  });
}

TEST_F(BatAdsNewTabPageAdServingTest,
       DoNotServeAdIfNotAllowedDueToPermissionRules) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;
  const CreativeNewTabPageAdInfo creative_ad = BuildCreativeNewTabPageAd();
  creative_ads.push_back(creative_ad);
  SaveCreativeAds(creative_ads);

  // Act
  serving_->MaybeServeAd([=](const absl::optional<NewTabPageAdInfo>& ad) {
    // Assert
    EXPECT_FALSE(ad);
    EXPECT_FALSE(had_opportunuity_);
    EXPECT_FALSE(did_serve_ad_);
    EXPECT_TRUE(failed_to_serve_ad_);
  });
}

}  // namespace ads::new_tab_page_ads
