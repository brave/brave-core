/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/conversions_database_table.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsConversionsDatabaseTableIntegrationTest : public UnitTestBase {
 protected:
  BatAdsConversionsDatabaseTableIntegrationTest() = default;

  ~BatAdsConversionsDatabaseTableIntegrationTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUpForTesting(/* integration_test */ true);
  }
};

TEST_F(BatAdsConversionsDatabaseTableIntegrationTest,
       GetConversionsFromCatalogEndpoint) {
  // Arrange
  const URLEndpoints endpoints = {
      {"/v8/catalog", {{net::HTTP_OK, "/catalog.json"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  InitializeAds();

  // Act

  // Assert
  database::table::Conversions conversions;
  conversions.GetAll(
      [](const Result result, const ConversionList& conversions) {
        EXPECT_EQ(Result::SUCCESS, result);
        EXPECT_EQ(2UL, conversions.size());
      });
}

}  // namespace ads
