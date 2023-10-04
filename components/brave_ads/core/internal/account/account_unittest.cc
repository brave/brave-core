/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/account.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/account/account_observer_mock.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
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
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/refill_confirmation_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/get_signed_tokens/get_signed_tokens_url_request_builder_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/get_signed_tokens/get_signed_tokens_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/request_signed_tokens/request_signed_tokens_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/units/ad_type.h"
#include "net/http/http_status_code.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds

namespace brave_ads {

class BraveAdsAccountTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    account_ = std::make_unique<Account>(&token_generator_mock_);
    account_->AddObserver(&observer_mock_);
  }

  void TearDown() override {
    account_->RemoveObserver(&observer_mock_);

    UnitTestBase::TearDown();
  }

  TokenGeneratorMock token_generator_mock_;

  std::unique_ptr<Account> account_;
  AccountObserverMock observer_mock_;
};

TEST_F(BraveAdsAccountTest, SetWallet) {
  // Act & Assert
  EXPECT_CALL(observer_mock_, OnDidInitializeWallet);
  EXPECT_CALL(observer_mock_, OnFailedToInitializeWallet).Times(0);

  account_->SetWallet(kWalletPaymentId, kWalletRecoverySeed);
}

TEST_F(BraveAdsAccountTest, SetWalletWithEmptyPaymentId) {
  // Act & Assert
  EXPECT_CALL(observer_mock_, OnDidInitializeWallet).Times(0);
  EXPECT_CALL(observer_mock_, OnFailedToInitializeWallet);

  account_->SetWallet(/*payment_id*/ {}, kWalletRecoverySeed);
}

TEST_F(BraveAdsAccountTest, SetWalletWithInvalidRecoverySeed) {
  // Act & Assert
  EXPECT_CALL(observer_mock_, OnDidInitializeWallet).Times(0);
  EXPECT_CALL(observer_mock_, OnFailedToInitializeWallet);

  account_->SetWallet(kWalletPaymentId, kInvalidWalletRecoverySeed);
}

TEST_F(BraveAdsAccountTest, SetWalletWithEmptyRecoverySeed) {
  // Act & Assert
  EXPECT_CALL(observer_mock_, OnDidInitializeWallet).Times(0);
  EXPECT_CALL(observer_mock_, OnFailedToInitializeWallet);

  account_->SetWallet(kWalletPaymentId, /*recovery_seed*/ "");
}

TEST_F(BraveAdsAccountTest, GetIssuersForRewardsUser) {
  // Arrange
  MockTokenGenerator(token_generator_mock_, /*count*/ 50);

  account_->SetWallet(kWalletPaymentId, kWalletRecoverySeed);

  const URLResponseMap url_responses = {
      {BuildIssuersUrlPath(),
       {{net::HTTP_OK, BuildIssuersUrlResponseBodyForTesting()}}},
      {BuildRequestSignedTokensUrlPath(kWalletPaymentId),
       {{net::HTTP_CREATED,
         BuildRequestSignedTokensUrlResponseBodyForTesting()}}},
      {BuildGetSignedTokensUrlPath(kWalletPaymentId, kGetSignedTokensNonce),
       {{net::HTTP_OK, BuildGetSignedTokensUrlResponseBodyForTesting()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  NotifyDidInitializeAds();

  // Act & Assert
  EXPECT_EQ(BuildIssuersForTesting(), GetIssuers());
}

TEST_F(BraveAdsAccountTest, DoNotGetIssuersForNonRewardsUser) {
  // Arrange
  DisableBraveRewardsForTesting();

  account_->SetWallet(kWalletPaymentId, kWalletRecoverySeed);

  EXPECT_CALL(ads_client_mock_, UrlRequest).Times(0);

  NotifyDidInitializeAds();

  // Act & Assert
  EXPECT_FALSE(GetIssuers());
}

TEST_F(BraveAdsAccountTest, DoNotGetInvalidIssuers) {
  // Arrange
  account_->SetWallet(kWalletPaymentId, kWalletRecoverySeed);

  const URLResponseMap url_responses = {
      {BuildIssuersUrlPath(), {{net::HTTP_OK, /*response_body*/ R"(
          {
            "ping": 7200000,
            "issuers": [
              {
                "name": "confirmations",
                "publicKeys": [
                  {
                    "publicKey": "bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=",
                    "associatedValue": ""
                  },
                  {
                    "publicKey": "QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=",
                    "associatedValue": ""
                  },
                  {
                    "publicKey": "6Orbju/jPQQGldu/MVyBi2wXKz8ynHIcdsbCWc9gGHQ=",
                    "associatedValue": ""
                  },
                  {
                    "publicKey": "ECEKAGeRCNmAWimTs7fo0tTMcg8Kcmoy8w+ccOSYXT8=",
                    "associatedValue": ""
                  },
                  {
                    "publicKey": "xp9WArE+RkSt579RCm6EhdmcW4RfS71kZHMgXpwgZyI=",
                    "associatedValue": ""
                  },
                  {
                    "publicKey": "AE7e4Rh38yFmnyLyPYcyWKT//zLOsEEX+WdLZqvJxH0=",
                    "associatedValue": ""
                  },
                  {
                    "publicKey": "HjID7G6LRrcRu5ezW0nLZtEARIBnjpaQFKTHChBuJm8=",
                    "associatedValue": ""
                  }
                ]
              },
              {
                "name": "payments",
                "publicKeys": [
                  {
                    "publicKey": "JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=",
                    "associatedValue": "0.0"
                  },
                  {
                    "publicKey": "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=",
                    "associatedValue": "0.1"
                  },
                  {
                    "publicKey": "XovQyvVWM8ez0mAzTtfqgPIbSpH5/idv8w0KJxhirwA=",
                    "associatedValue": "0.1"
                  },
                  {
                    "publicKey": "wAcnJtb34Asykf+2jrTWrjFiaTqilklZ6bxLyR3LyFo=",
                    "associatedValue": "0.1"
                  },
                  {
                    "publicKey": "ZvzeYOT1geUQXfOsYXBxZj/H26IfiBUVodHl51j68xI=",
                    "associatedValue": "0.1"
                  },
                  {
                    "publicKey": "JlOezORiqLkFkvapoNRGWcMH3/g09/7M2UPEwMjRpFE=",
                    "associatedValue": "0.1"
                  },
                  {
                    "publicKey": "hJP1nDjTdHcVDw347oH0XO+XBPPh5wZA2xWZE8QUSSA=",
                    "associatedValue": "0.1"
                  },
                  {
                    "publicKey": "+iyhYDv7W6cuFAD1tzsJIEQKEStTX9B/Tt62tqt+tG0=",
                    "associatedValue": "0.1"
                  }
                ]
              }
            ]
          })"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  NotifyDidInitializeAds();

  // Act & Assert
  EXPECT_FALSE(GetIssuers());
}

TEST_F(BraveAdsAccountTest, DoNotGetMissingIssuers) {
  // Arrange
  account_->SetWallet(kWalletPaymentId, kWalletRecoverySeed);

  const URLResponseMap url_responses = {
      {BuildIssuersUrlPath(), {{net::HTTP_OK, /*response_body*/ R"(
          {
            "ping": 7200000,
            "issuers": []
          })"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  NotifyDidInitializeAds();

  // Act & Assert
  EXPECT_FALSE(GetIssuers());
}

TEST_F(BraveAdsAccountTest, DoNotGetIssuersFromInvalidResponse) {
  // Arrange
  account_->SetWallet(kWalletPaymentId, kWalletRecoverySeed);

  const URLResponseMap url_responses = {
      {BuildIssuersUrlPath(), {{net::HTTP_OK, /*response_body*/ "{INVALID}"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  NotifyDidInitializeAds();

  // Act & Assert
  EXPECT_FALSE(GetIssuers());
}

TEST_F(BraveAdsAccountTest, DepositForCash) {
  // Arrange
  BuildAndSetIssuersForTesting();

  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  const URLResponseMap url_responses = {
      {BuildCreateRewardConfirmationUrlPath(
           kTransactionId, kCreateRewardConfirmationCredential),
       {{net::HTTP_CREATED,
         BuildCreateRewardConfirmationUrlResponseBodyForTesting()}}},
      {BuildFetchPaymentTokenUrlPath(kTransactionId),
       {{net::HTTP_OK, BuildFetchPaymentTokenUrlResponseBodyForTesting()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  SetConfirmationTokensForTesting(/*count*/ 1);

  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnDidProcessDeposit);
  EXPECT_CALL(observer_mock_, OnFailedToProcessDeposit).Times(0);
  EXPECT_CALL(observer_mock_, OnStatementOfAccountsDidChange);

  account_->Deposit(creative_ad.creative_instance_id, creative_ad.segment,
                    AdType::kNotificationAd, ConfirmationType::kViewed);
}

TEST_F(BraveAdsAccountTest, DepositForNonCash) {
  // Arrange
  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  SetConfirmationTokensForTesting(/*count*/ 1);

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnDidProcessDeposit);
  EXPECT_CALL(observer_mock_, OnFailedToProcessDeposit).Times(0);
  EXPECT_CALL(observer_mock_, OnStatementOfAccountsDidChange);

  account_->Deposit(kCreativeInstanceId, kSegment, AdType::kNotificationAd,
                    ConfirmationType::kClicked);
}

TEST_F(BraveAdsAccountTest, DoNotDepositCashIfCreativeInstanceIdDoesNotExist) {
  // Arrange
  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnDidProcessDeposit).Times(0);
  EXPECT_CALL(observer_mock_, OnFailedToProcessDeposit);
  EXPECT_CALL(observer_mock_, OnStatementOfAccountsDidChange).Times(0);

  account_->Deposit(kMissingCreativeInstanceId, kSegment,
                    AdType::kNotificationAd, ConfirmationType::kViewed);
}

TEST_F(BraveAdsAccountTest, GetStatement) {
  // Arrange
  TransactionList transactions;

  AdvanceClockTo(TimeFromString("31 October 2020", /*is_local*/ true));

  const TransactionInfo transaction_1 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = BuildTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_2);

  AdvanceClockTo(TimeFromString("18 November 2020", /*is_local*/ true));

  const TransactionInfo transaction_3 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_3);

  const TransactionInfo transaction_4 = BuildTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_4);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local*/ true));

  const TransactionInfo transaction_5 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_5);

  const TransactionInfo transaction_6 = BuildTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_6);

  const TransactionInfo transaction_7 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_7);

  SaveTransactionsForTesting(transactions);

  // Act & Assert
  mojom::StatementInfoPtr expected_statement = mojom::StatementInfo::New();
  expected_statement->min_earnings_last_month =
      0.01 * kMinEstimatedEarningsMultiplier.Get();
  expected_statement->max_earnings_last_month = 0.01;
  expected_statement->min_earnings_this_month =
      0.05 * kMinEstimatedEarningsMultiplier.Get();
  expected_statement->max_earnings_this_month = 0.05;
  expected_statement->next_payment_date =
      TimeFromString("7 January 2021 23:59:59.999", /*is_local*/ false);
  expected_statement->ads_received_this_month = 3;
  expected_statement->ad_types_received_this_month = {{"ad_notification", 3}};

  base::MockCallback<GetStatementOfAccountsCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Eq(std::ref(expected_statement))));
  Account::GetStatement(callback.Get());
}

TEST_F(BraveAdsAccountTest, DoNotGetStatementForNonRewardsUser) {
  // Arrange
  DisableBraveRewardsForTesting();

  // Act & Assert
  base::MockCallback<GetStatementOfAccountsCallback> callback;
  EXPECT_CALL(callback, Run(/*statement*/ ::testing::IsFalse()));
  Account::GetStatement(callback.Get());
}

}  // namespace brave_ads
