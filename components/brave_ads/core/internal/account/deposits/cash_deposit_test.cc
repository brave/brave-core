/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/deposits/cash_deposit.h"

#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCashDepositIntegrationTest : public test::TestBase {
 protected:
  void SetUp() override { test::TestBase::SetUp(/*is_integration_test=*/true); }

  void SetUpMocks() override {
    const test::URLResponseMap url_responses = {
        {BuildCatalogUrlPath(),
         {{net::HTTP_OK, /*response_body=*/"/catalog.json"}}}};
    test::MockUrlResponses(ads_client_mock_, url_responses);
  }
};

TEST_F(BraveAdsCashDepositIntegrationTest, GetValue) {
  // Arrange
  CashDeposit deposit;

  // Act & Assert
  base::test::TestFuture<bool, double> test_future;
  deposit.GetValue(test::kCreativeInstanceId, test_future.GetCallback());
  const auto [success, value] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_DOUBLE_EQ(test::kValue, value);
}

TEST_F(BraveAdsCashDepositIntegrationTest,
       DoNotGetValueForMissingCreativeInstanceId) {
  // Arrange
  CashDeposit deposit;

  // Act & Assert
  base::test::TestFuture<bool, double> test_future;
  deposit.GetValue(test::kMissingCreativeInstanceId, test_future.GetCallback());
  const auto [success, value] = test_future.Take();
  EXPECT_FALSE(success);
  EXPECT_DOUBLE_EQ(0.0, value);
}

}  // namespace brave_ads
