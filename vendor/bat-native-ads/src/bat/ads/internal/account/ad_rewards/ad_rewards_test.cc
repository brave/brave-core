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
       FailedToUpdateAdRewardsDueToLossOfPrecisionIssue15801) {
  // Arrange
  const URLEndpoints endpoints = {
      {"/v1/confirmation/payment/c387c2d8-a26d-4451-83e4-5c0c6fd942be",
       {{net::HTTP_OK,
         R"([
              {
                "month":"2021-05",
                "transactionCount":"180",
                "balance":"1.025"
              },
              {
                "month":"2021-04",
                "transactionCount":"275",
                "balance":"2.6895"
              },
              {
                "month":"2021-03",
                "transactionCount":"498",
                "balance":"6.015"
              },
              {
                "month":"2021-02",
                "transactionCount":"430",
                "balance":"6.235"
              },
              {
                "month":"2021-01",
                "transactionCount":"273",
                "balance":"3.1"
              },
              {
                "month":"2020-12",
                "transactionCount":"390",
                "balance":"10.66"
              },
              {
                "month":"2020-11",
                "transactionCount":"198",
                "balance":"5.795"
              },
              {
                "month":"2020-10",
                "transactionCount":"228",
                "balance":"6.8"
              },
              {
                "month":"2020-09",
                "transactionCount":"93",
                "balance":"2.05"
              }
            ])"}}},
      {"/v1/promotions/ads/grants/"
       "summary?paymentId=c387c2d8-a26d-4451-83e4-5c0c6fd942be",
       {{net::HTTP_OK,
         R"({
              "type" : "ads",
              "amount" : "42.98",
              "lastClaim" : "2021-05-06T20:55:56Z"
            })"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  MockLoad(ads_client_mock_, "confirmations.json",
           "confirmations_issue_15801.json");

  InitializeAds();

  AdvanceClock(TimeFromDateString("19 May 2021"));

  // Act
  GetAds()->GetAccountStatement(
      [](const bool success, const StatementInfo& statement) {
        ASSERT_TRUE(success);

        // Jimmy: Estimated pending rewards 4.147 BAT
        //        Next payment date Jun 5

        StatementInfo expected_statement;
        expected_statement.next_payment_date =
            TimestampFromDateString("5 June 2021");

        // Calculated by subtracting the ad grant balance from the accumulated
        // payment balances
        expected_statement.estimated_pending_rewards = 1.3895;

        // Calculated from the above payment balance for May
        expected_statement.earnings_this_month = 1.025;

        // Calculated from the above payment balance for April
        expected_statement.earnings_last_month = 2.6895;

        EXPECT_EQ(expected_statement, statement);
      });

  // Assert
}

TEST_F(
    BatAdsAdRewardsIntegrationTest,
    FailedToUpdateAdRewardsDueToNotZeroingUnreconciledEstimatedPendingRewardsIssue16678) {  // NOLINT
  // Arrange
  const URLEndpoints endpoints = {
      {"/v1/confirmation/payment/c387c2d8-a26d-4451-83e4-5c0c6fd942be",
       {{net::HTTP_OK,
         R"([
              {
                "month": "2021-06",
                "transactionCount": "224",
                "balance": "1.8375"
              },
              {
                "month": "2021-05",
                "transactionCount": "170",
                "balance": "1.0385"
              },
              {
                "month": "2021-04",
                "transactionCount": "290",
                "balance": "2.8025"
              },
              {
                "month": "2021-03",
                "transactionCount": "342",
                "balance": "4.25"
              },
              {
                "month": "2021-02",
                "transactionCount": "420",
                "balance": "6.73"
              },
              {
                "month": "2021-01",
                "transactionCount": "432",
                "balance": "5.775"
              },
              {
                "month": "2020-12",
                "transactionCount": "180",
                "balance": "5.475"
              },
              {
                "month": "2020-11",
                "transactionCount": "151",
                "balance": "5.405"
              },
              {
                "month": "2020-10",
                "transactionCount": "33",
                "balance": "0.825"
              },
              {
                "month": "2020-09",
                "transactionCount": "80",
                "balance": "1.625"
              },
              {
                "month": "2020-08",
                "transactionCount": "156",
                "balance": "6.45"
              },
              {
                "month": "2020-07",
                "transactionCount": "241",
                "balance": "12.2"
              },
              {
                "month": "2020-06",
                "transactionCount": "242",
                "balance": "11.79"
              },
              {
                "month": "2020-05",
                "transactionCount": "238",
                "balance": "12.45"
              },
              {
                "month": "2020-04",
                "transactionCount": "293",
                "balance": "15.55"
              },
              {
                "month": "2020-03",
                "transactionCount": "284",
                "balance": "15.05"
              },
              {
                "month": "2020-02",
                "transactionCount": "149",
                "balance": "7.65"
              },
              {
                "month": "2020-01",
                "transactionCount": "65",
                "balance": "3.45"
              },
              {
                "month": "2019-12",
                "transactionCount": "110",
                "balance": "5.5"
              },
              {
                "month": "2019-11",
                "transactionCount": "42",
                "balance": "2.1"
              },
              {
                "month": "2019-10",
                "transactionCount": "124",
                "balance": "6.2"
              },
              {
                "month": "2019-09",
                "transactionCount": "115",
                "balance": "7.25"
              },
              {
                "month": "2019-08",
                "transactionCount": "225",
                "balance": "13.65"
              },
              {
                "month": "2019-07",
                "transactionCount": "114",
                "balance": "6.1"
              },
              {
                "month": "2019-06",
                "transactionCount": "253",
                "balance": "12.65"
              }
            ])"}}},
      {"/v1/promotions/ads/grants/"
       "summary?paymentId=c387c2d8-a26d-4451-83e4-5c0c6fd942be",
       {{net::HTTP_OK,
         R"({
              "type" : "ads",
              "amount" : "169.68",
              "lastClaim" : "2021-05-06T20:55:56Z"
            })"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  MockLoad(ads_client_mock_, "confirmations.json",
           "confirmations_issue_16678.json");

  InitializeAds();

  AdvanceClock(TimeFromDateString("19 June 2021"));

  // Act
  GetAds()->GetAccountStatement(
      [](const bool success, const StatementInfo& statement) {
        ASSERT_TRUE(success);

        StatementInfo expected_statement;
        expected_statement.next_payment_date =
            TimestampFromDateString("5 July 2021");

        // Calculated by subtracting the ad grant balance from the accumulated
        // payment balances
        expected_statement.estimated_pending_rewards = 4.1235;

        // Calculated from the above payment balance for June
        expected_statement.earnings_this_month = 1.8375;

        // Calculated from the above payment balance for May
        expected_statement.earnings_last_month = 1.0385;

        EXPECT_EQ(expected_statement, statement);
      });

  // Assert
}

TEST_F(
    BatAdsAdRewardsIntegrationTest,
    FailedToUpdateAdRewardsDueToNotZeroingUnreconciledEstimatedPendingRewardsIssue16744) {  // NOLINT
  // Arrange
  const URLEndpoints endpoints = {
      {"/v1/confirmation/payment/c387c2d8-a26d-4451-83e4-5c0c6fd942be",
       {{net::HTTP_OK,
         R"([
              {
                "month": "2021-07",
                "transactionCount": "5",
                "balance": "0.05"
              },
              {
                "month": "2021-06",
                "transactionCount": "234",
                "balance": "1.9475"
              },
              {
                "month": "2021-05",
                "transactionCount": "170",
                "balance": "1.0385"
              },
              {
                "month": "2021-04",
                "transactionCount": "290",
                "balance": "2.8025"
              },
              {
                "month": "2021-03",
                "transactionCount": "342",
                "balance": "4.25"
              },
              {
                "month": "2021-02",
                "transactionCount": "420",
                "balance": "6.73"
              },
              {
                "month": "2021-01",
                "transactionCount": "432",
                "balance": "5.775"
              },
              {
                "month": "2020-12",
                "transactionCount": "180",
                "balance": "5.475"
              },
              {
                "month": "2020-11",
                "transactionCount": "151",
                "balance": "5.405"
              },
              {
                "month": "2020-10",
                "transactionCount": "33",
                "balance": "0.825"
              },
              {
                "month": "2020-09",
                "transactionCount": "80",
                "balance": "1.625"
              },
              {
                "month": "2020-08",
                "transactionCount": "156",
                "balance": "6.45"
              },
              {
                "month": "2020-07",
                "transactionCount": "241",
                "balance": "12.2"
              },
              {
                "month": "2020-06",
                "transactionCount": "242",
                "balance": "11.79"
              },
              {
                "month": "2020-05",
                "transactionCount": "238",
                "balance": "12.45"
              },
              {
                "month": "2020-04",
                "transactionCount": "293",
                "balance": "15.55"
              },
              {
                "month": "2020-03",
                "transactionCount": "284",
                "balance": "15.05"
              },
              {
                "month": "2020-02",
                "transactionCount": "149",
                "balance": "7.65"
              },
              {
                "month": "2020-01",
                "transactionCount": "65",
                "balance": "3.45"
              },
              {
                "month": "2019-12",
                "transactionCount": "110",
                "balance": "5.5"
              },
              {
                "month": "2019-11",
                "transactionCount": "42",
                "balance": "2.1"
              },
              {
                "month": "2019-10",
                "transactionCount": "124",
                "balance": "6.2"
              },
              {
                "month": "2019-09",
                "transactionCount": "115",
                "balance": "7.25"
              },
              {
                "month": "2019-08",
                "transactionCount": "225",
                "balance": "13.65"
              },
              {
                "month": "2019-07",
                "transactionCount": "114",
                "balance": "6.1"
              },
              {
                "month": "2019-06",
                "transactionCount": "253",
                "balance": "12.65"
              }
            ])"}}},
      {"/v1/promotions/ads/grants/"
       "summary?paymentId=c387c2d8-a26d-4451-83e4-5c0c6fd942be",
       {{net::HTTP_OK,
         R"({
              "type" : "ads",
              "amount" : "169.68",
              "lastClaim" : "2021-05-06T20:55:56Z"
            })"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  MockLoad(ads_client_mock_, "confirmations.json",
           "confirmations_issue_16744.json");

  InitializeAds();

  AdvanceClock(TimeFromDateString("4 July 2021"));

  // Act
  GetAds()->GetAccountStatement(
      [](const bool success, const StatementInfo& statement) {
        ASSERT_TRUE(success);

        StatementInfo expected_statement;
        expected_statement.next_payment_date =
            TimestampFromDateString("5 July 2021");

        // Calculated by subtracting the ad grant balance from the accumulated
        // payment balances
        expected_statement.estimated_pending_rewards = 4.3135;

        // Calculated from the above payment balance for June
        expected_statement.earnings_this_month = 0.08;

        // Calculated from the above payment balance for May
        expected_statement.earnings_last_month = 1.9475;

        // Calculated from ads received during July
        expected_statement.ads_received_this_month = 3;

        // Transactions
        TransactionInfo transaction_1;  // June 30, 2021 11:40:31 PM
        transaction_1.timestamp = 1625096431;
        transaction_1.estimated_redemption_value = 0.01;
        transaction_1.confirmation_type = "view";
        expected_statement.transactions.push_back(transaction_1);

        TransactionInfo transaction_2;  //  July 1, 2021 12:33:32 PM
        transaction_2.timestamp = 1625142812;
        transaction_2.estimated_redemption_value = 0.01;
        transaction_2.confirmation_type = "view";
        expected_statement.transactions.push_back(transaction_2);

        TransactionInfo transaction_3;  //  July 1, 2021 12:33:33 PM
        transaction_3.timestamp = 1625142813;
        transaction_3.estimated_redemption_value = 0.0;
        transaction_3.confirmation_type = "dismiss";
        expected_statement.transactions.push_back(transaction_3);

        TransactionInfo transaction_4;  //  July 1, 2021 12:34:21 PM
        transaction_4.timestamp = 1625142861;
        transaction_4.estimated_redemption_value = 0.01;
        transaction_4.confirmation_type = "view";
        expected_statement.transactions.push_back(transaction_4);

        TransactionInfo transaction_5;  // July 1, 2021 12:47:29 PM
        transaction_5.timestamp = 1625143649;
        transaction_5.estimated_redemption_value = 0.01;
        transaction_5.confirmation_type = "view";
        expected_statement.transactions.push_back(transaction_5);

        // Uncleared transactions
        TransactionInfo uncleared_transaction_1;  // July 1, 2021 12:33:32 PM
        uncleared_transaction_1.timestamp = 1625142812;
        uncleared_transaction_1.estimated_redemption_value = 0.01;
        uncleared_transaction_1.confirmation_type = "view";
        expected_statement.uncleared_transactions.push_back(
            uncleared_transaction_1);

        TransactionInfo uncleared_transaction_2;  // July 1, 2021 12:33:33 PM
        uncleared_transaction_2.timestamp = 1625142813;
        uncleared_transaction_2.estimated_redemption_value = 0.0;
        uncleared_transaction_2.confirmation_type = "dismiss";
        expected_statement.uncleared_transactions.push_back(
            uncleared_transaction_2);

        TransactionInfo uncleared_transaction_3;  // July 1, 2021 12:34:21 PM
        uncleared_transaction_3.timestamp = 1625142861;
        uncleared_transaction_3.estimated_redemption_value = 0.01;
        uncleared_transaction_3.confirmation_type = "view";
        expected_statement.uncleared_transactions.push_back(
            uncleared_transaction_3);

        TransactionInfo uncleared_transaction_4;  // July 1, 2021 12:47:29 PM
        uncleared_transaction_4.timestamp = 1625143649;
        uncleared_transaction_4.estimated_redemption_value = 0.01;
        uncleared_transaction_4.confirmation_type = "view";
        expected_statement.uncleared_transactions.push_back(
            uncleared_transaction_4);

        EXPECT_EQ(expected_statement, statement);
      });

  // Assert
}

TEST_F(BatAdsAdRewardsIntegrationTest, GetAdRewardsFromEndPoints) {
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
                "balance" : "7.32",
                "month" : "2021-04",
                "transactionCount" : "9"
              }
            ])"}}},
      {"/v1/promotions/ads/grants/"
       "summary?paymentId=c387c2d8-a26d-4451-83e4-5c0c6fd942be",
       {{net::HTTP_OK,
         R"({
              "type" : "ads",
              "amount" : "19.42",
              "lastClaim" : "1945-06-10T12:34:56.789Z"
            })"}}}};

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

        // Calculated by subtracting the ad grant balance from the accumulated
        // payment balances
        expected_statement.estimated_pending_rewards = 7.54;

        // Calculated from the above payment balance for May
        expected_statement.earnings_this_month = 19.64;

        // Calculated from the above payment balance for April
        expected_statement.earnings_last_month = 7.32;

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

        // Calculated by subtracting the cached ad grant balance from the cached
        // payment balance configured in |data/test/confirmations.json|
        expected_statement.estimated_pending_rewards = 43.79;

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
       GetCachedAdRewardsIfGetAdGrantsEndPointReturnsNonHttpOk) {
  // Arrange
  const URLEndpoints endpoints = {
      {"/v1/confirmation/payment/c387c2d8-a26d-4451-83e4-5c0c6fd942be",
       {{net::HTTP_OK,
         R"([
              {
                "balance" : "19.70",
                "month" : "2021-11",
                "transactionCount" : "51"
              },
              {
                "balance" : "6.66",
                "month" : "2021-10",
                "transactionCount" : "31"
              }
            ])"}}},
      {"/v1/promotions/ads/grants/"
       "summary?paymentId=c387c2d8-a26d-4451-83e4-5c0c6fd942be",
       {{net::HTTP_INTERNAL_SERVER_ERROR, ""}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  InitializeAds();

  AdvanceClock(TimeFromDateString("18 November 2021"));

  // Act
  GetAds()->GetAccountStatement(
      [](const bool success, const StatementInfo& statement) {
        ASSERT_TRUE(success);

        StatementInfo expected_statement;
        expected_statement.next_payment_date =
            TimestampFromDateString("5 December 2021");

        // Calculated by subtracting the cached ad grant balance configured in
        // |data/test/confirmations.json| from the above accumulated payment
        // balances
        expected_statement.estimated_pending_rewards = 22.15;

        // Calculated from the above payment balance for November
        expected_statement.earnings_this_month = 19.7;

        // Calculated from the above payment balance for October
        expected_statement.earnings_last_month = 6.66;

        EXPECT_EQ(expected_statement, statement);
      });

  // Assert
}

}  // namespace ads
