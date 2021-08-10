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

class BatAdsAdRewardsIntegrationTest : public UnitTestBase {
 protected:
  BatAdsAdRewardsIntegrationTest() = default;

  ~BatAdsAdRewardsIntegrationTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUpForTesting(/* integration_test */ true);
  }
};

TEST_F(BatAdsAdRewardsIntegrationTest,
       GetAdRewardsIfGetPaymentsEndPointReturnsEqualBalance) {
  // Arrange
  const URLEndpoints endpoints = {
      {"/v1/confirmation/payment/c387c2d8-a26d-4451-83e4-5c0c6fd942be",
       {{net::HTTP_OK,
         R"([
              {
                "balance": "48.0",
                "month": "2021-04",
                "transactionCount": "16"
              }
            ])"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  InitializeAds();

  AdvanceClock(TimeFromDateString("1 April 2021"));

  // Act
  GetAds()->GetAccountStatement(
      [](const bool success, const StatementInfo& statement) {
        ASSERT_TRUE(success);

        StatementInfo expected_statement;
        expected_statement.next_payment_date =
            TimestampFromDateString("5 May 2021");

        // Calculated from earnings in April configured in
        // |data/test/confirmations.json|
        expected_statement.earnings_this_month = 48.0;

        // Calculated from earnings in March configured in
        // |data/test/confirmations.json|
        expected_statement.earnings_last_month = 0.0;

        EXPECT_EQ(expected_statement, statement);
      });

  // Assert
}

TEST_F(BatAdsAdRewardsIntegrationTest,
       GetAdRewardsIfGetPaymentsEndPointReturnsGreaterBalance) {
  // Arrange
  const URLEndpoints endpoints = {
      {"/v1/confirmation/payment/c387c2d8-a26d-4451-83e4-5c0c6fd942be",
       {{net::HTTP_OK,
         R"([
              {
                "balance": "48.001",
                "month": "2021-04",
                "transactionCount": "16"
              }
            ])"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  InitializeAds();

  AdvanceClock(TimeFromDateString("1 April 2021"));

  // Act
  GetAds()->GetAccountStatement(
      [](const bool success, const StatementInfo& statement) {
        ASSERT_TRUE(success);

        StatementInfo expected_statement;
        expected_statement.next_payment_date =
            TimestampFromDateString("5 May 2021");

        // Calculated from earnings in April configured in
        // |data/test/confirmations.json|
        expected_statement.earnings_this_month = 48.001;

        // Calculated from earnings in March configured in
        // |data/test/confirmations.json|
        expected_statement.earnings_last_month = 0.0;

        EXPECT_EQ(expected_statement, statement);
      });

  // Assert
}

TEST_F(BatAdsAdRewardsIntegrationTest,
       GetCachedAdRewardsIfGetPaymentsEndPointReturnsLessBalance) {
  // Arrange
  const URLEndpoints endpoints = {
      {"/v1/confirmation/payment/c387c2d8-a26d-4451-83e4-5c0c6fd942be",
       {{net::HTTP_OK,
         R"([
              {
                "balance": "47.999",
                "month": "2021-04",
                "transactionCount": "16"
              }
            ])"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  InitializeAds();

  AdvanceClock(TimeFromDateString("1 April 2021"));

  // Act
  GetAds()->GetAccountStatement(
      [](const bool success, const StatementInfo& statement) {
        ASSERT_TRUE(success);

        StatementInfo expected_statement;
        expected_statement.next_payment_date =
            TimestampFromDateString("5 May 2021");

        // Calculated from earnings in April configured in
        // |data/test/confirmations.json|
        expected_statement.earnings_this_month = 48.0;

        // Calculated from earnings in March configured in
        // |data/test/confirmations.json|
        expected_statement.earnings_last_month = 0.0;

        EXPECT_EQ(expected_statement, statement);
      });

  // Assert
}

TEST_F(BatAdsAdRewardsIntegrationTest, GetAdRewardsForMultipleBalances) {
  // Arrange
  const URLEndpoints endpoints = {
      {"/v1/confirmation/payment/c387c2d8-a26d-4451-83e4-5c0c6fd942be",
       {{net::HTTP_OK,
         R"([
              {
                "balance" : "19.64",
                "month" : "2021-05",
                "transactionCount" : "28"
              },
              {
                "balance" : "28.37",
                "month" : "2021-04",
                "transactionCount" : "9"
              }
            ])"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  InitializeAds();

  AdvanceClock(TimeFromDateString("19 May 2021"));

  // Act
  GetAds()->GetAccountStatement(
      [](const bool success, const StatementInfo& statement) {
        ASSERT_TRUE(success);

        StatementInfo expected_statement;
        expected_statement.next_payment_date =
            TimestampFromDateString("5 June 2021");

        // Calculated from the above payment balance for May
        expected_statement.earnings_this_month = 19.64;

        // Calculated from the above payment balance for April
        expected_statement.earnings_last_month = 28.37;

        EXPECT_EQ(expected_statement, statement);
      });

  // Assert
}

TEST_F(BatAdsAdRewardsIntegrationTest,
       GetCachedAdRewardsIfGetPaymentsEndPointReturnsNonHttpOk) {
  // Arrange
  const URLEndpoints endpoints = {
      {"/v1/confirmation/payment/c387c2d8-a26d-4451-83e4-5c0c6fd942be",
       {{net::HTTP_INTERNAL_SERVER_ERROR, ""}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  InitializeAds();

  AdvanceClock(TimeFromDateString("1 April 2021"));

  // Act
  GetAds()->GetAccountStatement(
      [](const bool success, const StatementInfo& statement) {
        ASSERT_TRUE(success);

        StatementInfo expected_statement;
        expected_statement.next_payment_date =
            TimestampFromDateString("5 May 2021");

        // Calculated from earnings in April configured in
        // |data/test/confirmations.json|
        expected_statement.earnings_this_month = 48.0;

        // Calculated from earnings in March configured in
        // |data/test/confirmations.json|
        expected_statement.earnings_last_month = 0.0;

        EXPECT_EQ(expected_statement, statement);
      });

  // Assert
}

}  // namespace ads
