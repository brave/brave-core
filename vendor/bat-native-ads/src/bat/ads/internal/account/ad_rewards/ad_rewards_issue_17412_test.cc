/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/ad_rewards/ad_rewards.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsAdRewardsIssue17412IntegrationTest : public UnitTestBase {
 protected:
  BatAdsAdRewardsIssue17412IntegrationTest() = default;

  ~BatAdsAdRewardsIssue17412IntegrationTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(CopyFileFromTestPathToTempDir("confirmations_issue_17412.json",
                                              "confirmations.json"));

    UnitTestBase::SetUpForTesting(/* integration_test */ true);
  }
};

TEST_F(BatAdsAdRewardsIssue17412IntegrationTest, GetAdRewards) {
  // Arrange
  const URLEndpoints endpoints = {
      {"/v1/confirmation/payment/c387c2d8-a26d-4451-83e4-5c0c6fd942be",
       {{net::HTTP_OK,
         R"([
              {
                "balance": "0.1325",
                "month": "2021-08",
                "transactionCount": "23"
              },
              {
                "balance": "0.6525",
                "month": "2021-07",
                "transactionCount": "90"
              },
              {
                "balance": "1.1525",
                "month": "2021-06",
                "transactionCount": "192"
              },
              {
                "balance": "0.63",
                "month": "2021-05",
                "transactionCount": "141"
              }
            ])"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  InitializeAds();

  AdvanceClock(TimeFromDateString("8 August 2021"));

  // Act
  GetAds()->GetAccountStatement(
      [](const bool success, const StatementInfo& statement) {
        ASSERT_TRUE(success);

        StatementInfo expected_statement;
        expected_statement.next_payment_date =
            TimestampFromDateString("5 September 2021");

        // Calculated from earnings in April configured in
        // |data/test/confirmations.json|
        expected_statement.earnings_this_month = 0.1325;

        // Calculated from earnings in March configured in
        // |data/test/confirmations.json|
        expected_statement.earnings_last_month = 0.6525;

        EXPECT_EQ(expected_statement, statement);
      });

  // Assert
}

}  // namespace ads
