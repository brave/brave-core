/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_database_table.h"

#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "net/http/http_status_code.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionsDatabaseTableIntegrationTest : public test::TestBase {
 public:
  BraveAdsConversionsDatabaseTableIntegrationTest()
      : test::TestBase(/*is_integration_test=*/true) {}

 protected:
  void SetUpMocks() override {
    const test::URLResponseMap url_responses = {
        {BuildCatalogUrlPath(),
         {{net::HTTP_OK, /*response_body=*/"/catalog.json"}}}};
    test::MockUrlResponses(ads_client_mock_, url_responses);
  }
};

TEST_F(BraveAdsConversionsDatabaseTableIntegrationTest,
       GetConversionsFromCatalogResponse) {
  // Arrange
  const database::table::CampaignConversions database_table;

  // Act & Assert
  base::test::TestFuture<bool, CampaignConversionList> test_future;
  database_table.GetUnexpired(
      test_future.GetCallback<bool, const CampaignConversionList&>());
  const auto [success, campaign_conversions] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(campaign_conversions, ::testing::SizeIs(2));
}

}  // namespace brave_ads
