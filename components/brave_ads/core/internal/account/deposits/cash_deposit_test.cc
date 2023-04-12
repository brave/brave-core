/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/deposits/cash_deposit.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

class BatAdsCashDepositIntegrationTest : public UnitTestBase {
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

TEST_F(BatAdsCashDepositIntegrationTest, GetValue) {
  // Arrange
  CashDeposit deposit;

  // Act

  // Assert
  deposit.GetValue(kCreativeInstanceId,
                   base::BindOnce([](const bool success, const double value) {
                     EXPECT_TRUE(success);
                     EXPECT_EQ(1.0, value);
                   }));
}

TEST_F(BatAdsCashDepositIntegrationTest,
       DoNotGetValueForMissingCreativeInstanceId) {
  // Arrange
  CashDeposit deposit;

  // Act

  // Assert
  deposit.GetValue(kMissingCreativeInstanceId,
                   base::BindOnce([](const bool success, const double value) {
                     EXPECT_FALSE(success);
                     EXPECT_EQ(0.0, value);
                   }));
}

}  // namespace brave_ads
