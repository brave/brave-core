/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/creative_new_tab_page_ads_database_table.h"

#include <vector>

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsCreativeNewTabPageAdsDatabaseTableIntegrationTest
    : public UnitTestBase {
 protected:
  BatAdsCreativeNewTabPageAdsDatabaseTableIntegrationTest() = default;

  ~BatAdsCreativeNewTabPageAdsDatabaseTableIntegrationTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUpForTesting(/* is_integration_test */ true);
  }
};

TEST_F(BatAdsCreativeNewTabPageAdsDatabaseTableIntegrationTest,
       GetCreativeNewTabPageAdsFromCatalogEndpoint) {
  // Arrange
  const URLEndpoints endpoints = {
      {"/v9/catalog", {{net::HTTP_OK, "/catalog.json"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  InitializeAds();

  // Act

  // Assert
  const SegmentList& segments = {"technology & computing"};

  database::table::CreativeNewTabPageAds database_table;
  database_table.GetForSegments(
      segments, [](const bool success, const SegmentList& segments,
                   const CreativeNewTabPageAdList& creative_ads) {
        EXPECT_TRUE(success);
        EXPECT_EQ(1UL, creative_ads.size());
      });
}

}  // namespace ads
