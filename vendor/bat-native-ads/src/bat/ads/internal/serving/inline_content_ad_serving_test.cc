/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/inline_content_ad_serving.h"

#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/base/net/http/http_status_code.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_mock_util.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ad_unittest_util.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ads_database_table.h"
#include "bat/ads/internal/creatives/inline_content_ads/inline_content_ad_builder.h"
#include "bat/ads/internal/serving/permission_rules/user_activity_permission_rule_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsInlineContentAdServingIntegrationTest : public UnitTestBase {
 protected:
  BatAdsInlineContentAdServingIntegrationTest() = default;

  ~BatAdsInlineContentAdServingIntegrationTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUpForTesting(/* is_integration_test */ true);
  }

  void SetUpMocks() override {
    CopyFileFromTestPathToTempPath("confirmations_with_unblinded_tokens.json",
                                   kConfirmationsFilename);

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
  }

  void Save(const CreativeInlineContentAdList& creative_ads) {
    database::table::CreativeInlineContentAds database_table;
    database_table.Save(creative_ads,
                        [](const bool success) { ASSERT_TRUE(success); });
  }
};

TEST_F(BatAdsInlineContentAdServingIntegrationTest, ServeAd) {
  // Arrange
  ForceUserActivityPermissionRule();

  CreativeInlineContentAdList creative_ads;
  CreativeInlineContentAdInfo creative_ad = BuildCreativeInlineContentAd();
  creative_ads.push_back(creative_ad);
  Save(creative_ads);

  // Act
  GetAds()->GetInlineContentAd(
      "200x100",
      [&creative_ad](const bool success, const std::string& dimensions,
                     const InlineContentAdInfo& ad) {
        ASSERT_TRUE(success);

        InlineContentAdInfo expected_ad = BuildInlineContentAd(creative_ad);
        expected_ad.placement_id = ad.placement_id;

        EXPECT_EQ(expected_ad, ad);
      });

  // Assert
}

TEST_F(BatAdsInlineContentAdServingIntegrationTest,
       DoNotServeAdForUnavailableDimensions) {
  // Arrange
  ForceUserActivityPermissionRule();

  CreativeInlineContentAdList creative_ads;
  CreativeInlineContentAdInfo creative_ad = BuildCreativeInlineContentAd();
  creative_ads.push_back(creative_ad);
  Save(creative_ads);

  // Act
  GetAds()->GetInlineContentAd(
      "?x?", [](const bool success, const std::string& dimensions,
                const InlineContentAdInfo& ad) { EXPECT_FALSE(success); });

  // Assert
}

TEST_F(BatAdsInlineContentAdServingIntegrationTest,
       DoNotServeAdIfNotAllowedDueToPermissionRules) {
  // Arrange
  CreativeInlineContentAdList creative_ads;
  CreativeInlineContentAdInfo creative_ad = BuildCreativeInlineContentAd();
  creative_ads.push_back(creative_ad);
  Save(creative_ads);

  // Act
  GetAds()->GetInlineContentAd(
      "200x100", [](const bool success, const std::string& dimensions,
                    const InlineContentAdInfo& ad) { EXPECT_FALSE(success); });

  // Assert
}

}  // namespace ads
