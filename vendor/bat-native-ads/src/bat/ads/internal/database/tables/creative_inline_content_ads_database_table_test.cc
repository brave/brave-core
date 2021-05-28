/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/creative_inline_content_ads_database_table.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsCreativeInlineContentAdsDatabaseTableIntegrationTest
    : public UnitTestBase {
 protected:
  BatAdsCreativeInlineContentAdsDatabaseTableIntegrationTest() = default;

  ~BatAdsCreativeInlineContentAdsDatabaseTableIntegrationTest() override =
      default;

  void SetUp() override {
    UnitTestBase::SetUpForTesting(/* integration_test */ true);
  }
};

TEST_F(BatAdsCreativeInlineContentAdsDatabaseTableIntegrationTest,
       GetCreativeInlineContentAdsFromCatalogEndpoint) {
  // Arrange
  const URLEndpoints endpoints = {
      {"/v8/catalog", {{net::HTTP_OK, "/catalog.json"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  InitializeAds();

  // Act

  // Assert
  const std::vector<std::string> segments = {"Technology & Computing"};

  database::table::CreativeInlineContentAds creative_inline_content_ads;
  creative_inline_content_ads.GetForSegments(
      segments,
      [](const Result result, const SegmentList& segments,
         const CreativeInlineContentAdList& creative_inline_content_ads) {
        EXPECT_EQ(Result::SUCCESS, result);
        EXPECT_EQ(1UL, creative_inline_content_ads.size());
      });
}

}  // namespace ads
