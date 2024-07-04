/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/account.h"

#include "base/memory/raw_ptr.h"
#include "base/test/mock_callback.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/account_observer_mock.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/statement/statement_feature.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/redeem_reward_confirmation_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/create_reward_confirmation_url_request_builder_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/create_reward_confirmation_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/fetch_payment_token_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/ads_observer_mock.h"
#include "brave/components/brave_ads/core/internal/ads_observer_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_converter_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds

namespace brave_ads {

class BraveAdsAccountTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    ads_observer_mock_ = AddAdsObserverMock();

    account_ = std::make_unique<Account>(&token_generator_mock_);
    account_->AddObserver(&account_observer_mock_);
  }

  void TearDown() override {
    account_->RemoveObserver(&account_observer_mock_);

    UnitTestBase::TearDown();
  }

  TokenGeneratorMock token_generator_mock_;

  raw_ptr<AdsObserverMock> ads_observer_mock_ = nullptr;

  std::unique_ptr<Account> account_;
  AccountObserverMock account_observer_mock_;
};

TEST_F(BraveAdsAccountTest, SupportUserRewardsForRewardsUser) {
  // Arrange
  account_->SetWallet(kWalletPaymentId, kWalletRecoverySeed);

  NotifyDidInitializeAds();

  // Act & Assert
  EXPECT_TRUE(account_->IsUserRewardsSupported());
}

TEST_F(BraveAdsAccountTest, DoNotSupportUserRewardsForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  NotifyDidInitializeAds();

  // Act & Assert
  EXPECT_FALSE(account_->IsUserRewardsSupported());
}

TEST_F(BraveAdsAccountTest, SetWallet) {
  // Act & Assert
  EXPECT_CALL(account_observer_mock_, OnDidInitializeWallet);
  EXPECT_CALL(account_observer_mock_, OnFailedToInitializeWallet).Times(0);
  account_->SetWallet(kWalletPaymentId, kWalletRecoverySeed);
}

TEST_F(BraveAdsAccountTest, DoNotSetWalletWithEmptyPaymentId) {
  // Act & Assert
  EXPECT_CALL(account_observer_mock_, OnDidInitializeWallet).Times(0);
  EXPECT_CALL(account_observer_mock_, OnFailedToInitializeWallet);
  account_->SetWallet(/*payment_id=*/{}, kWalletRecoverySeed);
}

TEST_F(BraveAdsAccountTest, DoNotSetWalletWithInvalidRecoverySeed) {
  // Act & Assert
  EXPECT_CALL(account_observer_mock_, OnDidInitializeWallet).Times(0);
  EXPECT_CALL(account_observer_mock_, OnFailedToInitializeWallet);
  account_->SetWallet(kWalletPaymentId, kInvalidWalletRecoverySeed);
}

TEST_F(BraveAdsAccountTest, DoNotSetWalletWithEmptyRecoverySeed) {
  // Act & Assert
  EXPECT_CALL(account_observer_mock_, OnDidInitializeWallet).Times(0);
  EXPECT_CALL(account_observer_mock_, OnFailedToInitializeWallet);
  account_->SetWallet(kWalletPaymentId, /*recovery_seed=*/"");
}

TEST_F(BraveAdsAccountTest, GetStatementForRewardsUser) {
  // Arrange
  TransactionList transactions;

  AdvanceClockTo(TimeFromString("31 October 2020"));

  const TransactionInfo transaction_1 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = test::BuildTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression, /*reconciled_at=*/Now(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_2);

  AdvanceClockTo(TimeFromString("18 November 2020"));

  const TransactionInfo transaction_3 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_3);

  const TransactionInfo transaction_4 = test::BuildTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression, /*reconciled_at=*/Now(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_4);

  AdvanceClockTo(TimeFromString("25 December 2020"));

  const TransactionInfo transaction_5 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_5);

  const TransactionInfo transaction_6 = test::BuildTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression, /*reconciled_at=*/Now(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_6);

  const TransactionInfo transaction_7 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_7);

  test::SaveTransactions(transactions);

  // Act & Assert
  const mojom::StatementInfoPtr expected_statement =
      mojom::StatementInfo::New();
  expected_statement->min_earnings_last_month =
      0.01 * kMinEstimatedEarningsMultiplier.Get();
  expected_statement->max_earnings_last_month = 0.01;
  expected_statement->min_earnings_this_month =
      0.05 * kMinEstimatedEarningsMultiplier.Get();
  expected_statement->max_earnings_this_month = 0.05;
  expected_statement->next_payment_date =
      TimeFromUTCString("7 January 2021 23:59:59.999");
  expected_statement->ads_received_this_month = 3;
  expected_statement->ads_summary_this_month = {{"ad_notification", 3}};

  base::MockCallback<GetStatementOfAccountsCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Eq(std::ref(expected_statement))));
  Account::GetStatement(callback.Get());
}

TEST_F(BraveAdsAccountTest, DoNotGetStatementForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  base::MockCallback<GetStatementOfAccountsCallback> callback;
  EXPECT_CALL(callback, Run(/*statement=*/::testing::IsFalse()));
  Account::GetStatement(callback.Get());
}

TEST_F(BraveAdsAccountTest, DepositForCash) {
  // Arrange
  test::BuildAndSetIssuers();

  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/1);

  const URLResponseMap url_responses = {
      {BuildCreateRewardConfirmationUrlPath(
           kTransactionId, kCreateRewardConfirmationCredential),
       {{net::HTTP_CREATED,
         test::BuildCreateRewardConfirmationUrlResponseBody()}}},
      {BuildFetchPaymentTokenUrlPath(kTransactionId),
       {{net::HTTP_OK, test::BuildFetchPaymentTokenUrlResponseBody()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act & Assert
  EXPECT_CALL(account_observer_mock_, OnDidProcessDeposit);
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);
  EXPECT_CALL(*ads_observer_mock_, OnAdRewardsDidChange);
  account_->Deposit(kCreativeInstanceId, kSegment, AdType::kNotificationAd,
                    ConfirmationType::kViewedImpression);
}

TEST_F(BraveAdsAccountTest, DepositForCashWithUserData) {
  // Arrange
  test::BuildAndSetIssuers();

  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/1);

  const URLResponseMap url_responses = {
      {BuildCreateRewardConfirmationUrlPath(
           kTransactionId, kCreateRewardConfirmationCredential),
       {{net::HTTP_CREATED,
         test::BuildCreateRewardConfirmationUrlResponseBody()}}},
      {BuildFetchPaymentTokenUrlPath(kTransactionId),
       {{net::HTTP_OK, test::BuildFetchPaymentTokenUrlResponseBody()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act & Assert
  EXPECT_CALL(account_observer_mock_, OnDidProcessDeposit);
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);
  EXPECT_CALL(*ads_observer_mock_, OnAdRewardsDidChange);
  account_->DepositWithUserData(kCreativeInstanceId, kSegment,
                                AdType::kNotificationAd,
                                ConfirmationType::kViewedImpression,
                                /*user_data=*/{});
}

TEST_F(BraveAdsAccountTest, DepositForNonCash) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/1);

  // Act & Assert
  EXPECT_CALL(account_observer_mock_, OnDidProcessDeposit);
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);
  EXPECT_CALL(*ads_observer_mock_, OnAdRewardsDidChange);
  account_->Deposit(kCreativeInstanceId, kSegment, AdType::kNotificationAd,
                    ConfirmationType::kClicked);
}

TEST_F(BraveAdsAccountTest, DepositForNonCashWithUserData) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/1);

  // Act & Assert
  EXPECT_CALL(account_observer_mock_, OnDidProcessDeposit);
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);
  EXPECT_CALL(*ads_observer_mock_, OnAdRewardsDidChange);
  account_->DepositWithUserData(kCreativeInstanceId, kSegment,
                                AdType::kNotificationAd,
                                ConfirmationType::kClicked, /*user_data=*/{});
}

TEST_F(BraveAdsAccountTest, DoNotDepositCashIfCreativeInstanceIdDoesNotExist) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act & Assert
  EXPECT_CALL(account_observer_mock_, OnDidProcessDeposit).Times(0);
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit);
  EXPECT_CALL(*ads_observer_mock_, OnAdRewardsDidChange).Times(0);
  account_->Deposit(kMissingCreativeInstanceId, kSegment,
                    AdType::kNotificationAd,
                    ConfirmationType::kViewedImpression);
}

TEST_F(BraveAdsAccountTest, AddTransactionWhenDepositingCashForRewardsUser) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act
  EXPECT_CALL(account_observer_mock_, OnDidProcessDeposit);
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);
  EXPECT_CALL(*ads_observer_mock_, OnAdRewardsDidChange);

  account_->Deposit(kCreativeInstanceId, kSegment, AdType::kNotificationAd,
                    ConfirmationType::kViewedImpression);

  // Assert
  base::MockCallback<database::table::GetTransactionsCallback> callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true, /*transactions=*/::testing::SizeIs(1)));
  const database::table::Transactions database_table;
  database_table.GetForDateRange(/*from_time=*/DistantPast(),
                                 /*to_time=*/DistantFuture(), callback.Get());
}

TEST_F(BraveAdsAccountTest, AddTransactionWhenDepositingNonCashForRewardsUser) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act
  EXPECT_CALL(account_observer_mock_, OnDidProcessDeposit);
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);
  EXPECT_CALL(*ads_observer_mock_, OnAdRewardsDidChange);

  account_->Deposit(kCreativeInstanceId, kSegment, AdType::kNotificationAd,
                    ConfirmationType::kClicked);

  // Assert
  base::MockCallback<database::table::GetTransactionsCallback> callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true, /*transactions=*/::testing::SizeIs(1)));
  const database::table::Transactions database_table;
  database_table.GetForDateRange(/*from_time=*/DistantPast(),
                                 /*to_time=*/DistantFuture(), callback.Get());
}

TEST_F(BraveAdsAccountTest,
       DoNotAddTransactionWhenDepositingCashForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act
  EXPECT_CALL(account_observer_mock_, OnDidProcessDeposit);
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);
  EXPECT_CALL(*ads_observer_mock_, OnAdRewardsDidChange);

  account_->Deposit(kCreativeInstanceId, kSegment, AdType::kNotificationAd,
                    ConfirmationType::kViewedImpression);

  // Assert
  base::MockCallback<database::table::GetTransactionsCallback> callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true, /*transactions=*/::testing::IsEmpty()));
  const database::table::Transactions database_table;
  database_table.GetForDateRange(/*from_time=*/DistantPast(),
                                 /*to_time=*/DistantFuture(), callback.Get());
}

TEST_F(BraveAdsAccountTest,
       DoNotAddTransactionWhenDepositingNonCashForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act
  EXPECT_CALL(account_observer_mock_, OnDidProcessDeposit);
  EXPECT_CALL(account_observer_mock_, OnFailedToProcessDeposit).Times(0);
  EXPECT_CALL(*ads_observer_mock_, OnAdRewardsDidChange);

  account_->Deposit(kCreativeInstanceId, kSegment, AdType::kNotificationAd,
                    ConfirmationType::kClicked);

  // Assert
  base::MockCallback<database::table::GetTransactionsCallback> callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true, /*transactions=*/::testing::IsEmpty()));
  const database::table::Transactions database_table;
  database_table.GetForDateRange(/*from_time=*/DistantPast(),
                                 /*to_time=*/DistantFuture(), callback.Get());
}

}  // namespace brave_ads
