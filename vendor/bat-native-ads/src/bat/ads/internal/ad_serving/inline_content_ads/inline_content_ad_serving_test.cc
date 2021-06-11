/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/inline_content_ads/inline_content_ad_serving.h"

#include "base/guid.h"
#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/ad_targeting/ad_targeting.h"
#include "bat/ads/internal/ads/inline_content_ads/inline_content_ad_builder.h"
#include "bat/ads/internal/database/tables/creative_inline_content_ads_database_table.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsInlineContentAdServingTest : public UnitTestBase {
 protected:
  BatAdsInlineContentAdServingTest()
      : ad_targeting_(std::make_unique<AdTargeting>()),
        subdivision_targeting_(
            std::make_unique<ad_targeting::geographic::SubdivisionTargeting>()),
        anti_targeting_resource_(std::make_unique<resource::AntiTargeting>()),
        ad_serving_(std::make_unique<inline_content_ads::AdServing>(
            ad_targeting_.get(),
            subdivision_targeting_.get(),
            anti_targeting_resource_.get())),
        database_table_(
            std::make_unique<database::table::CreativeInlineContentAds>()) {}

  ~BatAdsInlineContentAdServingTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUpForTesting(/* integration_test */ true);

    MockLoad(ads_client_mock_, "confirmations.json",
             "confirmations_with_unblinded_tokens.json");

    const URLEndpoints endpoints = {
        {"/v8/catalog", {{net::HTTP_OK, "/empty_catalog.json"}}}};
    MockUrlRequest(ads_client_mock_, endpoints);

    InitializeAds();

    RecordUserActivityEvents();
  }

  void RecordUserActivityEvents() {
    UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
    UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);
  }

  CreativeInlineContentAdInfo GetCreativeInlineContentAd() {
    CreativeInlineContentAdInfo creative_inline_content_ad;

    creative_inline_content_ad.creative_instance_id = base::GenerateGUID();
    creative_inline_content_ad.creative_set_id = base::GenerateGUID();
    creative_inline_content_ad.campaign_id = base::GenerateGUID();
    creative_inline_content_ad.start_at_timestamp = DistantPastAsTimestamp();
    creative_inline_content_ad.end_at_timestamp = DistantFutureAsTimestamp();
    creative_inline_content_ad.daily_cap = 1;
    creative_inline_content_ad.advertiser_id = base::GenerateGUID();
    creative_inline_content_ad.priority = 1;
    creative_inline_content_ad.ptr = 1.0;
    creative_inline_content_ad.per_day = 1;
    creative_inline_content_ad.per_week = 1;
    creative_inline_content_ad.per_month = 1;
    creative_inline_content_ad.total_max = 1;
    creative_inline_content_ad.segment = "untargeted";
    creative_inline_content_ad.geo_targets = {"US"};
    creative_inline_content_ad.target_url = "https://brave.com";
    CreativeDaypartInfo daypart;
    creative_inline_content_ad.dayparts = {daypart};
    creative_inline_content_ad.title = "Test Ad Title";
    creative_inline_content_ad.description = "Test Ad Description";
    creative_inline_content_ad.image_url = "https://brave.com/image.jpg";
    creative_inline_content_ad.dimensions = "200x100";
    creative_inline_content_ad.cta_text = "Call to action text";

    return creative_inline_content_ad;
  }

  void Save(const CreativeInlineContentAdList& creative_inline_content_ads) {
    database_table_->Save(creative_inline_content_ads, [](const Result result) {
      ASSERT_EQ(Result::SUCCESS, result);
    });
  }

  std::unique_ptr<AdTargeting> ad_targeting_;
  std::unique_ptr<ad_targeting::geographic::SubdivisionTargeting>
      subdivision_targeting_;
  std::unique_ptr<resource::AntiTargeting> anti_targeting_resource_;
  std::unique_ptr<inline_content_ads::AdServing> ad_serving_;

  std::unique_ptr<database::table::CreativeInlineContentAds> database_table_;
};

TEST_F(BatAdsInlineContentAdServingTest, ServeAd) {
  // Arrange
  CreativeInlineContentAdList creative_inline_content_ads;

  CreativeInlineContentAdInfo creative_inline_content_ad =
      GetCreativeInlineContentAd();
  creative_inline_content_ads.push_back(creative_inline_content_ad);

  Save(creative_inline_content_ads);

  // Act
  const InlineContentAdInfo expected_inline_content_ad =
      BuildInlineContentAd(creative_inline_content_ad);

  ad_serving_->MaybeServeAd(
      "200x100", [&expected_inline_content_ad](
                     const bool success, const std::string& dimensions,
                     const InlineContentAdInfo& inline_content_ad) {
        EXPECT_EQ(expected_inline_content_ad, inline_content_ad);
      });

  // Assert
}

TEST_F(BatAdsInlineContentAdServingTest, DoNotServeAdForUnavailableDimensions) {
  // Arrange
  CreativeInlineContentAdList creative_inline_content_ads;

  CreativeInlineContentAdInfo creative_inline_content_ad =
      GetCreativeInlineContentAd();
  creative_inline_content_ads.push_back(creative_inline_content_ad);

  Save(creative_inline_content_ads);

  // Act
  ad_serving_->MaybeServeAd(
      "?x?", [](const bool success, const std::string& dimensions,
                const InlineContentAdInfo& inline_content_ad) {
        EXPECT_FALSE(success);
      });

  // Assert
}

}  // namespace ads
