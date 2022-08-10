/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/promoted_content_ads/creative_promoted_content_ads_database_table.h"

#include <vector>

#include "bat/ads/internal/base/net/http/http_status_code.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_mock_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsCreativePromotedContentAdsDatabaseTableIntegrationTest
    : public UnitTestBase {
 protected:
  BatAdsCreativePromotedContentAdsDatabaseTableIntegrationTest() = default;

  ~BatAdsCreativePromotedContentAdsDatabaseTableIntegrationTest() override =
      default;

  void SetUp() override {
    UnitTestBase::SetUpForTesting(/* is_integration_test */ true);
  }

  void SetUpMocks() override {
    const URLResponseMap responses = {
        {"/v9/catalog", {{net::HTTP_OK, "/catalog.json"}}}};
    MockUrlResponses(ads_client_mock_, responses);
  }
};

TEST_F(BatAdsCreativePromotedContentAdsDatabaseTableIntegrationTest,
       GetCreativePromotedContentAdsFromCatalogResponse) {
  // Arrange

  // Act

  // Assert
  const std::vector<std::string> segments = {"technology & computing"};

  database::table::CreativePromotedContentAds creative_promoted_content_ads;
  creative_promoted_content_ads.GetForSegments(
      segments,
      [](const bool success, const SegmentList& segments,
         const CreativePromotedContentAdList& creative_promoted_content_ads) {
        EXPECT_TRUE(success);
        EXPECT_EQ(1UL, creative_promoted_content_ads.size());
      });
}

}  // namespace ads
