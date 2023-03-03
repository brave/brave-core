/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/promoted_content_ads/creative_promoted_content_ads_database_table.h"

#include "base/functional/bind.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_mock_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsCreativePromotedContentAdsDatabaseTableIntegrationTest
    : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUpForTesting(/*is_integration_test*/ true);
  }

  void SetUpMocks() override {
    const URLResponseMap url_responses = {
        {"/v9/catalog", {{net::HTTP_OK, "/catalog.json"}}}};
    MockUrlResponses(ads_client_mock_, url_responses);
  }
};

TEST_F(BatAdsCreativePromotedContentAdsDatabaseTableIntegrationTest,
       GetCreativePromotedContentAdsFromCatalogResponse) {
  // Arrange

  // Act

  // Assert
  const std::vector<std::string> segments = {"technology & computing"};

  const database::table::CreativePromotedContentAds
      creative_promoted_content_ads;
  creative_promoted_content_ads.GetForSegments(
      segments,
      base::BindOnce([](const bool success, const SegmentList& /*segments*/,
                        const CreativePromotedContentAdList&
                            creative_promoted_content_ads) {
        EXPECT_TRUE(success);
        EXPECT_EQ(1UL, creative_promoted_content_ads.size());
      }));
}

}  // namespace ads
