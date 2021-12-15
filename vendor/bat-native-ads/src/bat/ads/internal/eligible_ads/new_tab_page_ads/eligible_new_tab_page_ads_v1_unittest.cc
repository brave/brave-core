/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/new_tab_page_ads/eligible_new_tab_page_ads_v1.h"

#include <memory>

#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_builder_unittest_util.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_info.h"
#include "bat/ads/internal/bundle/creative_new_tab_page_ad_unittest_util.h"
#include "bat/ads/internal/database/tables/creative_new_tab_page_ads_database_table.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsEligibleNewTabPageAdsV1Test : public UnitTestBase {
 protected:
  BatAdsEligibleNewTabPageAdsV1Test()
      : database_table_(
            std::make_unique<database::table::CreativeNewTabPageAds>()) {}

  ~BatAdsEligibleNewTabPageAdsV1Test() override = default;

  void Save(const CreativeNewTabPageAdList& creative_ads) {
    database_table_->Save(creative_ads,
                          [](const bool success) { ASSERT_TRUE(success); });
  }

  std::unique_ptr<database::table::CreativeNewTabPageAds> database_table_;
};

TEST_F(BatAdsEligibleNewTabPageAdsV1Test, GetAdsForParentChildSegment) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  CreativeNewTabPageAdInfo creative_ad_1 = BuildCreativeNewTabPageAd();
  creative_ad_1.segment = "technology & computing";
  creative_ads.push_back(creative_ad_1);

  CreativeNewTabPageAdInfo creative_ad_2 = BuildCreativeNewTabPageAd();
  creative_ad_2.segment = "technology & computing-software";
  creative_ads.push_back(creative_ad_2);

  Save(creative_ads);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  new_tab_page_ads::EligibleAdsV1 eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeNewTabPageAdList& expected_creative_ads = {creative_ad_2};

  eligible_ads.GetForUserModel(
      ad_targeting::BuildUserModel({"technology & computing-software"}, {}, {}),
      [&expected_creative_ads](const bool success,
                               const CreativeNewTabPageAdList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleNewTabPageAdsV1Test, GetAdsForParentSegment) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  CreativeNewTabPageAdInfo creative_ad = BuildCreativeNewTabPageAd();
  creative_ad.segment = "technology & computing";
  creative_ads.push_back(creative_ad);

  Save(creative_ads);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  new_tab_page_ads::EligibleAdsV1 eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeNewTabPageAdList& expected_creative_ads = {creative_ad};

  eligible_ads.GetForUserModel(
      ad_targeting::BuildUserModel({"technology & computing-software"}, {}, {}),
      [&expected_creative_ads](const bool success,
                               const CreativeNewTabPageAdList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleNewTabPageAdsV1Test, GetAdsForUntargetedSegment) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  CreativeNewTabPageAdInfo creative_ad = BuildCreativeNewTabPageAd();
  creative_ad.segment = "untargeted";
  creative_ads.push_back(creative_ad);

  Save(creative_ads);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  new_tab_page_ads::EligibleAdsV1 eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeNewTabPageAdList& expected_creative_ads = {creative_ad};

  eligible_ads.GetForUserModel(
      ad_targeting::BuildUserModel({"finance-banking"}, {}, {}),
      [&expected_creative_ads](const bool success,
                               const CreativeNewTabPageAdList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleNewTabPageAdsV1Test, GetAdsForMultipleSegments) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  CreativeNewTabPageAdInfo creative_ad_1 = BuildCreativeNewTabPageAd();
  creative_ad_1.segment = "technology & computing";
  creative_ads.push_back(creative_ad_1);

  CreativeNewTabPageAdInfo creative_ad_2 = BuildCreativeNewTabPageAd();
  creative_ad_2.segment = "finance-banking";
  creative_ads.push_back(creative_ad_2);

  CreativeNewTabPageAdInfo creative_ad_3 = BuildCreativeNewTabPageAd();
  creative_ad_3.segment = "food & drink";
  creative_ads.push_back(creative_ad_3);

  Save(creative_ads);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  new_tab_page_ads::EligibleAdsV1 eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeNewTabPageAdList& expected_creative_ads = {creative_ad_1,
                                                           creative_ad_2};

  eligible_ads.GetForUserModel(
      ad_targeting::BuildUserModel({"technology & computing", "food & drink"},
                                   {}, {}),
      [&expected_creative_ads](const bool success,
                               const CreativeNewTabPageAdList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleNewTabPageAdsV1Test, GetAdsForNoSegments) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  CreativeNewTabPageAdInfo creative_ad = BuildCreativeNewTabPageAd();
  creative_ad.segment = "untargeted";
  creative_ads.push_back(creative_ad);

  Save(creative_ads);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  new_tab_page_ads::EligibleAdsV1 eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeNewTabPageAdList& expected_creative_ads = {creative_ad};

  eligible_ads.GetForUserModel(
      {},
      [&expected_creative_ads](const bool success,
                               const CreativeNewTabPageAdList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

TEST_F(BatAdsEligibleNewTabPageAdsV1Test, GetAdsForUnmatchedSegments) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  CreativeNewTabPageAdInfo creative_ad = BuildCreativeNewTabPageAd();
  creative_ad.segment = "technology & computing";
  creative_ads.push_back(creative_ad);

  Save(creative_ads);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  new_tab_page_ads::EligibleAdsV1 eligible_ads(&subdivision_targeting,
                                               &anti_targeting_resource);

  const CreativeNewTabPageAdList expected_creative_ads;

  eligible_ads.GetForUserModel(
      ad_targeting::BuildUserModel({"UNMATCHED"}, {}, {}),
      [&expected_creative_ads](const bool success,
                               const CreativeNewTabPageAdList& creative_ads) {
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });

  // Assert
}

}  // namespace ads
