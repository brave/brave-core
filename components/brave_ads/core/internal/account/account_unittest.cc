/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/account.h"

#include "base/memory/raw_ptr.h"
#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/account/account_observer_mock.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_test_util.h"
#include "brave/components/brave_ads/core/internal/account/statement/statement_feature.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_test_constants.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_test_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/redeem_reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/create_reward_confirmation_url_request_builder_test_constants.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/create_reward_confirmation_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/fetch_payment_token_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_test_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/ads_observer_mock.h"
#include "brave/components/brave_ads/core/internal/ads_observer_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds

namespace brave_ads {

class BraveAdsAccountTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    ads_observer_mock_ = test::MockAdsObserver();

    GetAccount().AddObserver(&account_observer_mock_);
  }

  void TearDown() override {
    GetAccount().RemoveObserver(&account_observer_mock_);

    test::TestBase::TearDown();
  }

  raw_ptr<AdsObserverMock> ads_observer_mock_ = nullptr;

  AccountObserverMock account_observer_mock_;
};

TEST_F(BraveAdsAccountTest, SupportUserRewardsForRewardsUser) {
  // Arrange
  GetAccount().SetWallet(test::kWalletPaymentId,
                         test::kWalletRecoverySeedBase64);

  NotifyDidInitializeAds();

  // Act & Assert
  EXPECT_TRUE(GetAccount().IsUserRewardsSupported());
}

TEST_F(BraveAdsAccountTest, DoNotSupportUserRewardsForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  NotifyDidInitializeAds();

  // Act & Assert
  EXPECT_FALSE(GetAccount().IsUserRewardsSupported());
}

TEST_F(BraveAdsAccountTest, SetWallet) {
  // Act & Assert
  EXPECT_CALL(account_observer_mock_, OnDidInitializeWallet);
  EXPECT_CALL(account_observer_mock_, OnFailedToInitializeWallet).Times(0);
  GetAccount().SetWallet(test::kWalletPaymentId,
                         test::kWalletRecoverySeedBase64);
}

TEST_F(BraveAdsAccountTest, DoNotSetWalletWithEmptyPaymentId) {
  // Act & Assert
  EXPECT_CALL(account_observer_mock_, OnDidInitializeWallet).Times(0);
  EXPECT_CALL(account_observer_mock_, OnFailedToInitializeWallet);
  GetAccount().SetWallet(/*payment_id=*/"", test::kWalletRecoverySeedBase64);
}

TEST_F(BraveAdsAccountTest, DoNotSetWalletWithInvalidRecoverySeed) {
  // Act & Assert
  EXPECT_CALL(account_observer_mock_, OnDidInitializeWallet).Times(0);
  EXPECT_CALL(account_observer_mock_, OnFailedToInitializeWallet);
  GetAccount().SetWallet(test::kWalletPaymentId,
                         test::kInvalidWalletRecoverySeed);
}

TEST_F(BraveAdsAccountTest, DoNotSetWalletWithEmptyRecoverySeed) {
  // Act & Assert
  EXPECT_CALL(account_observer_mock_, OnDidInitializeWallet).Times(0);
  EXPECT_CALL(account_observer_mock_, OnFailedToInitializeWallet);
  GetAccount().SetWallet(test::kWalletPaymentId, /*recovery_seed_base64=*/"");
}

TEST_F(BraveAdsAccountTest, GetStatementForRewardsUser) {
  // Arrange
  TransactionList transactions;

  AdvanceClockTo(test::TimeFromString("31 October 2020"));

  const TransactionInfo transaction_1 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_2);

  AdvanceClockTo(test::TimeFromString("18 November 2020"));

  const TransactionInfo transaction_3 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_3);

  const TransactionInfo transaction_4 = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_4);

  AdvanceClockTo(test::TimeFromString("25 December 2020"));

  const TransactionInfo transaction_5 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_5);

  const TransactionInfo transaction_6 = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_6);

  const TransactionInfo transaction_7 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_7);

  database::SaveTransactions(transactions);

  // Act & Assert
  const mojom::StatementInfoPtr expected_mojom_statement =
      mojom::StatementInfo::New();
  expected_mojom_statement->min_earnings_previous_month =
      0.01 * kMinEstimatedEarningsMultiplier.Get();
  expected_mojom_statement->max_earnings_previous_month = 0.01;
  expected_mojom_statement->min_earnings_this_month =
      0.05 * kMinEstimatedEarningsMultiplier.Get();
  expected_mojom_statement->max_earnings_this_month = 0.05;
  expected_mojom_statement->next_payment_date =
      test::TimeFromUTCString("7 January 2021 23:59:59.999");
  expected_mojom_statement->ads_received_this_month = 3;
  expected_mojom_statement->ads_summary_this_month = {
      {mojom::AdType::kNotificationAd, 3}};

  base::MockCallback<GetStatementOfAccountsCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Eq(std::ref(expected_mojom_statement))));
  GetAccount().GetStatement(callback.Get());
}

TEST_F(BraveAdsAccountTest, DoNotGetStatementForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  base::MockCallback<GetStatementOfAccountsCallback> callback;
  EXPECT_CALL(callback, Run(/*statement=*/::testing::IsFalse()));
  GetAccount().GetStatement(callback.Get());
}

TEST_F(BraveAdsAccountTest, DepositForCash) {
  // Arrange
  test::BuildAndSetIssuers();

  test::MockTokenGenerator(/*count=*/1);

  const test::URLResponseMap url_responses = {
      {BuildCreateRewardConfirmationUrlPath(test::kTransactionId,
                                            test::kCredentialBase64Url),
       {{net::HTTP_CREATED,
         test::BuildCreateRewardConfirmationUrlResponseBody()}}},
      {BuildFetchPaymentTokenUrlPath(test::kTransactionId),
       {{net::HTTP_OK, test::BuildFetchPaymentTokenUrlResponseBody()}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act & Assert
  EXPECT_CALL(account_observer_mock_,
              OnDidProcessDeposit(/*transaction=*/::testing::FieldsAre(
                  /*id*/ ::testing::_, /*created_at*/ test::Now(),
                  test::kCreativeInstanceId, test::kSegment, test::kValue,
                  mojom::AdType::kNotificationAd,
                  mojom::ConfirmationType::kViewedImpression,
                  /*reconciled_at*/ std::nullopt)));
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);
  EXPECT_CALL(*ads_observer_mock_, OnAdRewardsDidChange);
  GetAccount().Deposit(test::kCreativeInstanceId, test::kSegment,
                       mojom::AdType::kNotificationAd,
                       mojom::ConfirmationType::kViewedImpression);
}

TEST_F(BraveAdsAccountTest, DepositForCashWithUserData) {
  // Arrange
  test::BuildAndSetIssuers();

  test::MockTokenGenerator(/*count=*/1);

  const test::URLResponseMap url_responses = {
      {BuildCreateRewardConfirmationUrlPath(test::kTransactionId,
                                            test::kCredentialBase64Url),
       {{net::HTTP_CREATED,
         test::BuildCreateRewardConfirmationUrlResponseBody()}}},
      {BuildFetchPaymentTokenUrlPath(test::kTransactionId),
       {{net::HTTP_OK, test::BuildFetchPaymentTokenUrlResponseBody()}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act & Assert
  EXPECT_CALL(account_observer_mock_,
              OnDidProcessDeposit(/*transaction=*/::testing::FieldsAre(
                  /*id*/ ::testing::_, /*created_at*/ test::Now(),
                  test::kCreativeInstanceId, test::kSegment, test::kValue,
                  mojom::AdType::kNotificationAd,
                  mojom::ConfirmationType::kViewedImpression,
                  /*reconciled_at*/ std::nullopt)));
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);
  EXPECT_CALL(*ads_observer_mock_, OnAdRewardsDidChange);
  GetAccount().DepositWithUserData(test::kCreativeInstanceId, test::kSegment,
                                   mojom::AdType::kNotificationAd,
                                   mojom::ConfirmationType::kViewedImpression,
                                   /*user_data=*/{});
}

TEST_F(BraveAdsAccountTest, DepositForNonCash) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);

  // Act & Assert
  EXPECT_CALL(
      account_observer_mock_,
      OnDidProcessDeposit(/*transaction=*/::testing::FieldsAre(
          /*id*/ ::testing::_, /*created_at*/ test::Now(),
          test::kCreativeInstanceId, test::kSegment, /*value*/ 0.0,
          mojom::AdType::kNotificationAd, mojom::ConfirmationType::kClicked,
          /*reconciled_at*/ std::nullopt)));
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);
  EXPECT_CALL(*ads_observer_mock_, OnAdRewardsDidChange);
  GetAccount().Deposit(test::kCreativeInstanceId, test::kSegment,
                       mojom::AdType::kNotificationAd,
                       mojom::ConfirmationType::kClicked);
}

TEST_F(BraveAdsAccountTest, DepositForNonCashWithUserData) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);

  // Act & Assert
  EXPECT_CALL(
      account_observer_mock_,
      OnDidProcessDeposit(/*transaction=*/::testing::FieldsAre(
          /*id*/ ::testing::_, /*created_at*/ test::Now(),
          test::kCreativeInstanceId, test::kSegment, /*value*/ 0.0,
          mojom::AdType::kNotificationAd, mojom::ConfirmationType::kClicked,
          /*reconciled_at*/ std::nullopt)));
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);
  EXPECT_CALL(*ads_observer_mock_, OnAdRewardsDidChange);
  GetAccount().DepositWithUserData(
      test::kCreativeInstanceId, test::kSegment, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kClicked, /*user_data=*/{});
}

TEST_F(BraveAdsAccountTest, DoNotDepositCashIfCreativeInstanceIdDoesNotExist) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act & Assert
  EXPECT_CALL(account_observer_mock_, OnDidProcessDeposit).Times(0);
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit);
  EXPECT_CALL(*ads_observer_mock_, OnAdRewardsDidChange).Times(0);
  GetAccount().Deposit(test::kMissingCreativeInstanceId, test::kSegment,
                       mojom::AdType::kNotificationAd,
                       mojom::ConfirmationType::kViewedImpression);
}

TEST_F(BraveAdsAccountTest, AddTransactionWhenDepositingCashForRewardsUser) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act
  EXPECT_CALL(account_observer_mock_,
              OnDidProcessDeposit(/*transaction=*/::testing::FieldsAre(
                  /*id*/ ::testing::_, /*created_at*/ test::Now(),
                  test::kCreativeInstanceId, test::kSegment, test::kValue,
                  mojom::AdType::kNotificationAd,
                  mojom::ConfirmationType::kViewedImpression,
                  /*reconciled_at*/ std::nullopt)));
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);
  EXPECT_CALL(*ads_observer_mock_, OnAdRewardsDidChange);

  GetAccount().Deposit(test::kCreativeInstanceId, test::kSegment,
                       mojom::AdType::kNotificationAd,
                       mojom::ConfirmationType::kViewedImpression);

  // Assert
  base::MockCallback<database::table::GetTransactionsCallback> callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true, /*transactions=*/::testing::SizeIs(1)));
  const database::table::Transactions database_table;
  database_table.GetForDateRange(/*from_time=*/test::DistantPast(),
                                 /*to_time=*/test::DistantFuture(),
                                 callback.Get());
}

TEST_F(BraveAdsAccountTest, AddTransactionWhenDepositingNonCashForRewardsUser) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act
  EXPECT_CALL(
      account_observer_mock_,
      OnDidProcessDeposit(/*transaction=*/::testing::FieldsAre(
          /*id*/ ::testing::_, /*created_at*/ test::Now(),
          test::kCreativeInstanceId, test::kSegment, /*value*/ 0.0,
          mojom::AdType::kNotificationAd, mojom::ConfirmationType::kClicked,
          /*reconciled_at*/ std::nullopt)));
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);
  EXPECT_CALL(*ads_observer_mock_, OnAdRewardsDidChange);

  GetAccount().Deposit(test::kCreativeInstanceId, test::kSegment,
                       mojom::AdType::kNotificationAd,
                       mojom::ConfirmationType::kClicked);

  // Assert
  base::MockCallback<database::table::GetTransactionsCallback> callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true, /*transactions=*/::testing::SizeIs(1)));
  const database::table::Transactions database_table;
  database_table.GetForDateRange(/*from_time=*/test::DistantPast(),
                                 /*to_time=*/test::DistantFuture(),
                                 callback.Get());
}

TEST_F(BraveAdsAccountTest,
       DoNotAddTransactionWhenDepositingCashForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act
  EXPECT_CALL(account_observer_mock_, OnDidProcessDeposit).Times(0);
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);
  EXPECT_CALL(*ads_observer_mock_, OnAdRewardsDidChange).Times(0);
  GetAccount().Deposit(test::kCreativeInstanceId, test::kSegment,
                       mojom::AdType::kSearchResultAd,
                       mojom::ConfirmationType::kViewedImpression);

  // Assert
  base::MockCallback<database::table::GetTransactionsCallback> callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true, /*transactions=*/::testing::IsEmpty()));
  const database::table::Transactions database_table;
  database_table.GetForDateRange(/*from_time=*/test::DistantPast(),
                                 /*to_time=*/test::DistantFuture(),
                                 callback.Get());
}

TEST_F(BraveAdsAccountTest,
       DoNotAddTransactionWhenDepositingNonCashForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act
  EXPECT_CALL(account_observer_mock_, OnDidProcessDeposit).Times(0);
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);
  EXPECT_CALL(*ads_observer_mock_, OnAdRewardsDidChange).Times(0);
  GetAccount().Deposit(test::kCreativeInstanceId, test::kSegment,
                       mojom::AdType::kNewTabPageAd,
                       mojom::ConfirmationType::kClicked);

  // Assert
  base::MockCallback<database::table::GetTransactionsCallback> callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true, /*transactions=*/::testing::IsEmpty()));
  const database::table::Transactions database_table;
  database_table.GetForDateRange(/*from_time=*/test::DistantPast(),
                                 /*to_time=*/test::DistantFuture(),
                                 callback.Get());
}

}  // namespace brave_ads
