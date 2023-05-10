/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_table.h"

#include <vector>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeInlineContentAdsDatabaseTableIntegrationTest
    : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUpForTesting(/*is_integration_test*/ true);
  }

  void SetUpMocks() override {
    const URLResponseMap url_responses = {
        {BuildCatalogUrlPath(),
         {{net::HTTP_OK, /*response_body*/ "/catalog.json"}}}};
    MockUrlResponses(ads_client_mock_, url_responses);
  }
};

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableIntegrationTest,
       GetCreativeInlineContentAdsForSegmentsAndDimensionsFromCatalogResponse) {
  // Arrange

  // Act

  // Assert
  const database::table::CreativeInlineContentAds database_table;
  database_table.GetForSegmentsAndDimensions(
      /*segments*/ {"technology & computing"}, /*dimensions*/ "200x100",
      base::BindOnce([](const bool success, const SegmentList& /*segments*/,
                        const CreativeInlineContentAdList& creative_ads) {
        EXPECT_TRUE(success);
        EXPECT_EQ(1U, creative_ads.size());
      }));
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableIntegrationTest,
       GetCreativeInlineContentAdsForDimensionsFromCatalogResponse) {
  // Arrange

  // Act

  // Assert
  const database::table::CreativeInlineContentAds database_table;
  database_table.GetForDimensions(
      "200x100",
      base::BindOnce([](const bool success,
                        const CreativeInlineContentAdList& creative_ads) {
        EXPECT_TRUE(success);
        EXPECT_EQ(1U, creative_ads.size());
      }));
}

}  // namespace brave_ads
