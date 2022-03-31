/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/new_tab_page_ads/eligible_new_tab_page_ads_v2.h"

#include <memory>

#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_builder_unittest_util.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_info.h"
#include "bat/ads/internal/bundle/creative_new_tab_page_ad_unittest_util.h"
#include "bat/ads/internal/database/tables/creative_new_tab_page_ads_database_table.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsEligibleNewTabPageAdsV2Test : public UnitTestBase {
 protected:
  BatAdsEligibleNewTabPageAdsV2Test()
      : database_table_(
            std::make_unique<database::table::CreativeNewTabPageAds>()) {}

  ~BatAdsEligibleNewTabPageAdsV2Test() override = default;

  void Save(const CreativeNewTabPageAdList& creative_ads) {
    database_table_->Save(creative_ads,
                          [](const bool success) { ASSERT_TRUE(success); });
  }

  std::unique_ptr<database::table::CreativeNewTabPageAds> database_table_;
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

  Save(creative_ads);

  const SegmentList& interest_segments = {"foo-bar3"};
  const SegmentList latent_interest_segments;
  const SegmentList& purchase_intent_segments = {"foo-bar1", "foo-bar2"};

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  new_tab_page_ads::EligibleAdsV2 eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  eligible_ads.GetForUserModel(
      ad_targeting::BuildUserModel(interest_segments, latent_interest_segments,
                                   purchase_intent_segments),
      [](const bool had_opportunity,
         const CreativeNewTabPageAdList& creative_ads) {
        EXPECT_TRUE(!creative_ads.empty());
      });

  // Assert
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

  Save(creative_ads);

  const SegmentList interest_segments;
  const SegmentList latent_interest_segments;
  const SegmentList purchase_intent_segments;

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  new_tab_page_ads::EligibleAdsV2 eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  eligible_ads.GetForUserModel(
      ad_targeting::BuildUserModel(interest_segments, latent_interest_segments,
                                   purchase_intent_segments),
      [](const bool had_opportunity,
         const CreativeNewTabPageAdList& creative_ads) {
        EXPECT_TRUE(!creative_ads.empty());
      });

  // Assert
}

TEST_F(BatAdsEligibleNewTabPageAdsV2Test, GetIfNoEligibleAds) {
  // Arrange
  const SegmentList& interest_segments = {"interest-foo", "interest-bar"};
  const SegmentList latent_interest_segments;
  const SegmentList& purchase_intent_segments = {"intent-foo", "intent-bar"};

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  new_tab_page_ads::EligibleAdsV2 eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeNewTabPageAdList expected_creative_ads;

  eligible_ads.GetForUserModel(
      ad_targeting::BuildUserModel(interest_segments, latent_interest_segments,
                                   purchase_intent_segments),
      [&expected_creative_ads](const bool had_opportunity,
                               const CreativeNewTabPageAdList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

}  // namespace ads
