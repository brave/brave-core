/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_v2.h"

#include <memory>

#include "base/functional/bind.h"
#include "bat/ads/internal/ads/serving/targeting/user_model_builder_unittest_util.h"
#include "bat/ads/internal/ads/serving/targeting/user_model_info.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_unittest_util.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::new_tab_page_ads {

class BatAdsEligibleNewTabPageAdsV2Test : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    subdivision_targeting_ =
        std::make_unique<geographic::SubdivisionTargeting>();
    anti_targeting_resource_ = std::make_unique<resource::AntiTargeting>();
    eligible_ads_ = std::make_unique<EligibleAdsV2>(
        subdivision_targeting_.get(), anti_targeting_resource_.get());
  }

  std::unique_ptr<geographic::SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<resource::AntiTargeting> anti_targeting_resource_;
  std::unique_ptr<EligibleAdsV2> eligible_ads_;
};

TEST_F(BatAdsEligibleNewTabPageAdsV2Test, GetAds) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  CreativeNewTabPageAdInfo creative_ad_1 = BuildCreativeNewTabPageAd();
  creative_ad_1.segment = "foo-bar1";
  creative_ads.push_back(creative_ad_1);

  CreativeNewTabPageAdInfo creative_ad_2 = BuildCreativeNewTabPageAd();
  creative_ad_2.segment = "foo-bar3";
  creative_ads.push_back(creative_ad_2);

  SaveCreativeAds(creative_ads);

  // Act
  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel({"foo-bar3"}, {}, {"foo-bar1", "foo-bar2"}),
      base::BindOnce([](const bool had_opportunity,
                        const CreativeNewTabPageAdList& creative_ads) {
        // Assert
        EXPECT_TRUE(had_opportunity);
        EXPECT_TRUE(!creative_ads.empty());
      }));
}

TEST_F(BatAdsEligibleNewTabPageAdsV2Test, GetAdsForNoSegments) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  CreativeNewTabPageAdInfo creative_ad_1 = BuildCreativeNewTabPageAd();
  creative_ad_1.segment = "foo";
  creative_ads.push_back(creative_ad_1);

  CreativeNewTabPageAdInfo creative_ad_2 = BuildCreativeNewTabPageAd();
  creative_ad_2.segment = "foo-bar";
  creative_ads.push_back(creative_ad_2);

  SaveCreativeAds(creative_ads);

  // Act
  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel({}, {}, {}),
      base::BindOnce([](const bool had_opportunity,
                        const CreativeNewTabPageAdList& creative_ads) {
        // Assert
        EXPECT_TRUE(had_opportunity);
        EXPECT_TRUE(!creative_ads.empty());
      }));
}

TEST_F(BatAdsEligibleNewTabPageAdsV2Test, DoNotGetAdsIfNoEligibleAds) {
  // Arrange

  // Act
  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel({"interest-foo", "interest-bar"}, {},
                                {"intent-foo", "intent-bar"}),
      base::BindOnce([](const bool had_opportunity,
                        const CreativeNewTabPageAdList& creative_ads) {
        // Assert
        EXPECT_FALSE(had_opportunity);
        EXPECT_TRUE(creative_ads.empty());
      }));
}

}  // namespace ads::new_tab_page_ads
