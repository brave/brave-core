/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/inline_content_ads/inline_content_ad_serving.h"

#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/ads/inline_content_ads/inline_content_ad_builder.h"
#include "bat/ads/internal/bundle/creative_inline_content_ad_unittest_util.h"
#include "bat/ads/internal/database/tables/creative_inline_content_ads_database_table.h"
#include "bat/ads/internal/frequency_capping/permission_rules/user_activity_frequency_cap_unittest_util.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsInlineContentAdServingTest : public UnitTestBase {
 protected:
  BatAdsInlineContentAdServingTest()
      : subdivision_targeting_(
            std::make_unique<ad_targeting::geographic::SubdivisionTargeting>()),
        anti_targeting_resource_(std::make_unique<resource::AntiTargeting>()),
        ad_serving_(std::make_unique<inline_content_ads::AdServing>(
            subdivision_targeting_.get(),
            anti_targeting_resource_.get())),
        database_table_(
            std::make_unique<database::table::CreativeInlineContentAds>()) {}

  ~BatAdsInlineContentAdServingTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(CopyFileFromTestPathToTempDir(
        "confirmations_with_unblinded_tokens.json", "confirmations.json"));

    UnitTestBase::SetUpForTesting(/* integration_test */ true);

    const URLEndpoints endpoints = {
        {"/v9/catalog", {{net::HTTP_OK, "/empty_catalog.json"}}},
        {// Get issuers request
         R"(/v1/issuers/)",
         {{net::HTTP_OK, R"(
        {
          "ping": 7200000,
          "issuers": [
            {
              "name": "confirmations",
              "publicKeys": [
                {
                  "publicKey": "JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=",
                  "associatedValue": ""
                },
                {
                  "publicKey": "crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=",
                  "associatedValue": ""
                }
              ]
            },
            {
              "name": "payments",
              "publicKeys": [
                {
                  "publicKey": "JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=",
                  "associatedValue": "0.0"
                },
                {
                  "publicKey": "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=",
                  "associatedValue": "0.1"
                }
              ]
            }
          ]
        }
        )"}}}};
    MockUrlRequest(ads_client_mock_, endpoints);

    InitializeAds();
  }

  void Save(const CreativeInlineContentAdList& creative_ads) {
    database_table_->Save(creative_ads,
                          [](const bool success) { ASSERT_TRUE(success); });
  }

  std::unique_ptr<ad_targeting::geographic::SubdivisionTargeting>
      subdivision_targeting_;
  std::unique_ptr<resource::AntiTargeting> anti_targeting_resource_;
  std::unique_ptr<inline_content_ads::AdServing> ad_serving_;

  std::unique_ptr<database::table::CreativeInlineContentAds> database_table_;
};

TEST_F(BatAdsInlineContentAdServingTest, ServeAd) {
  // Arrange
  ForceUserActivityFrequencyCapPermission();

  CreativeInlineContentAdList creative_ads;
  CreativeInlineContentAdInfo creative_ad = BuildCreativeInlineContentAd();
  creative_ads.push_back(creative_ad);
  Save(creative_ads);

  // Act
  ad_serving_->MaybeServeAd(
      "200x100",
      [&creative_ad](const bool success, const std::string& dimensions,
                     const InlineContentAdInfo& ad) {
        ASSERT_TRUE(success);

        InlineContentAdInfo expected_ad = BuildInlineContentAd(creative_ad);
        expected_ad.uuid = ad.uuid;

        EXPECT_EQ(expected_ad, ad);
      });

  // Assert
}

TEST_F(BatAdsInlineContentAdServingTest, DoNotServeAdForUnavailableDimensions) {
  // Arrange
  ForceUserActivityFrequencyCapPermission();

  CreativeInlineContentAdList creative_ads;
  CreativeInlineContentAdInfo creative_ad = BuildCreativeInlineContentAd();
  creative_ads.push_back(creative_ad);
  Save(creative_ads);

  // Act
  ad_serving_->MaybeServeAd(
      "?x?", [](const bool success, const std::string& dimensions,
                const InlineContentAdInfo& ad) { EXPECT_FALSE(success); });

  // Assert
}

TEST_F(BatAdsInlineContentAdServingTest,
       DoNotServeAdIfNotAllowedDueToPermissionRules) {
  // Arrange
  CreativeInlineContentAdList creative_ads;
  CreativeInlineContentAdInfo creative_ad = BuildCreativeInlineContentAd();
  creative_ads.push_back(creative_ad);
  Save(creative_ads);

  // Act
  ad_serving_->MaybeServeAd(
      "200x100", [](const bool success, const std::string& dimensions,
                    const InlineContentAdInfo& ad) { EXPECT_FALSE(success); });

  // Assert
}

}  // namespace ads
