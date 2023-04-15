/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

class BatAdsCreativeNewTabPageAdsDatabaseTableIntegrationTest
    : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUpForTesting(/*is_integration_test*/ true);
  }

  void SetUpMocks() override {
    const URLResponseMap url_responses = {
        {"/v9/catalog", {{net::HTTP_OK, /*response_body*/ "/catalog.json"}}}};
    MockUrlResponses(ads_client_mock_, url_responses);
  }
};

TEST_F(BatAdsCreativeNewTabPageAdsDatabaseTableIntegrationTest,
       GetCreativeNewTabPageAdsFromCatalogResponse) {
  // Arrange

  // Act

  // Assert
  const database::table::CreativeNewTabPageAds database_table;
  database_table.GetForSegments(
      /*segments*/ {"technology & computing"},
      base::BindOnce([](const bool success, const SegmentList& /*segments*/,
                        const CreativeNewTabPageAdList& creative_ads) {
        EXPECT_TRUE(success);
        EXPECT_EQ(1U, creative_ads.size());
      }));
}

}  // namespace brave_ads
