/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"

#include "base/functional/bind.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_mock_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsCreativeNewTabPageAdsDatabaseTableIntegrationTest
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

TEST_F(BatAdsCreativeNewTabPageAdsDatabaseTableIntegrationTest,
       GetCreativeNewTabPageAdsFromCatalogResponse) {
  // Arrange

  // Act

  // Assert
  const SegmentList segments = {"technology & computing"};

  const database::table::CreativeNewTabPageAds database_table;
  database_table.GetForSegments(
      segments,
      base::BindOnce([](const bool success, const SegmentList& /*segments*/,
                        const CreativeNewTabPageAdList& creative_ads) {
        EXPECT_TRUE(success);
        EXPECT_EQ(1UL, creative_ads.size());
      }));
}

}  // namespace ads
