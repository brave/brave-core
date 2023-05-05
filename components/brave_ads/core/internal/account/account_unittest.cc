/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/account.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"
#include "net/http/http_status_code.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds

namespace brave_ads {

using ::testing::NiceMock;

class BraveAdsAccountTest : public AccountObserver, public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    account_ = std::make_unique<Account>(&token_generator_mock_);
    account_->AddObserver(this);
  }

  void TearDown() override {
    account_->RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  void OnWalletWasCreated(const WalletInfo& /*wallet*/) override {
    wallet_was_created_ = true;
  }

  void OnWalletDidUpdate(const WalletInfo& /*wallet*/) override {
    wallet_did_update_ = true;
  }

  void OnWalletDidChange(const WalletInfo& /*wallet*/) override {
    wallet_did_change_ = true;
  }

  void OnInvalidWallet() override { invalid_wallet_ = true; }

  void OnDidProcessDeposit(const TransactionInfo& transaction) override {
    did_process_deposit_ = true;
    transaction_ = transaction;
  }

  void OnFailedToProcessDeposit(
      const std::string& /*creative_instance_id*/,
      const AdType& /*ad_type*/,
      const ConfirmationType& /*confirmation_type*/) override {
    failed_to_process_deposit_ = true;
  }

  void OnStatementOfAccountsDidChange() override {
    statement_of_accounts_did_change_ = true;
  }

  NiceMock<privacy::TokenGeneratorMock> token_generator_mock_;

  std::unique_ptr<Account> account_;

  bool wallet_was_created_ = false;
  bool wallet_did_update_ = false;
  bool wallet_did_change_ = false;
  bool invalid_wallet_ = false;

  TransactionInfo transaction_;
  bool did_process_deposit_ = false;
  bool failed_to_process_deposit_ = false;

  bool statement_of_accounts_did_change_ = false;
};

TEST_F(BraveAdsAccountTest, SetWallet) {
  // Arrange

  // Act
  account_->SetWallet(kWalletPaymentId, kWalletRecoverySeed);

  // Assert
  EXPECT_TRUE(wallet_was_created_);
  EXPECT_TRUE(wallet_did_update_);
  EXPECT_FALSE(wallet_did_change_);
  EXPECT_FALSE(invalid_wallet_);
}

TEST_F(BraveAdsAccountTest, SetWalletWithEmptyPaymentId) {
  // Arrange
  MockUrlResponses(ads_client_mock_, GetValidIssuersUrlResponses());

  // Act
  account_->SetWallet(/*payment_id*/ {}, kWalletRecoverySeed);

  // Assert
  EXPECT_FALSE(wallet_was_created_);
  EXPECT_FALSE(wallet_did_update_);
  EXPECT_FALSE(wallet_did_change_);
  EXPECT_TRUE(invalid_wallet_);
}

TEST_F(BraveAdsAccountTest, SetWalletWithInvalidRecoverySeed) {
  // Arrange
  MockUrlResponses(ads_client_mock_, GetValidIssuersUrlResponses());

  // Act
  account_->SetWallet(kWalletPaymentId, kInvalidWalletRecoverySeed);

  // Assert
  EXPECT_FALSE(wallet_was_created_);
  EXPECT_FALSE(wallet_did_update_);
  EXPECT_FALSE(wallet_did_change_);
  EXPECT_TRUE(invalid_wallet_);
}

TEST_F(BraveAdsAccountTest, SetWalletWithEmptyRecoverySeed) {
  // Arrange
  MockUrlResponses(ads_client_mock_, GetValidIssuersUrlResponses());

  // Act
  account_->SetWallet(kWalletPaymentId, /*recovery_seed*/ "");

  // Assert
  EXPECT_FALSE(wallet_was_created_);
  EXPECT_FALSE(wallet_did_update_);
  EXPECT_FALSE(wallet_did_change_);
  EXPECT_TRUE(invalid_wallet_);
}

TEST_F(BraveAdsAccountTest, ChangeWallet) {
  // Arrange
  account_->SetWallet(kWalletPaymentId, kWalletRecoverySeed);

  // Act
  account_->SetWallet(/*payment_id*/ "c1bf0a09-cac8-48eb-8c21-7ca6d995b0a3",
                      kWalletRecoverySeed);

  // Assert
  EXPECT_TRUE(wallet_was_created_);
  EXPECT_TRUE(wallet_did_update_);
  EXPECT_TRUE(wallet_did_change_);
  EXPECT_FALSE(invalid_wallet_);
}

TEST_F(BraveAdsAccountTest, GetWallet) {
  // Arrange
  account_->SetWallet(kWalletPaymentId, kWalletRecoverySeed);

  // Act
  const WalletInfo& wallet = account_->GetWallet();

  // Assert
  WalletInfo expected_wallet;
  expected_wallet.payment_id = kWalletPaymentId;
  expected_wallet.public_key = kWalletPublicKey;
  expected_wallet.secret_key = kWalletSecretKey;

  EXPECT_EQ(expected_wallet, wallet);
}

TEST_F(BraveAdsAccountTest, GetIssuersWhenWalletIsCreated) {
  // Arrange
  MockUrlResponses(ads_client_mock_, GetValidIssuersUrlResponses());

  privacy::SetUnblindedTokens(/*count*/ 50);

  // Act
  account_->SetWallet(kWalletPaymentId, kWalletRecoverySeed);

  // Assert
  EXPECT_TRUE(wallet_was_created_);
  EXPECT_TRUE(wallet_did_update_);
  EXPECT_FALSE(wallet_did_change_);
  EXPECT_FALSE(invalid_wallet_);

  const absl::optional<IssuersInfo> issuers = GetIssuers();
  ASSERT_TRUE(issuers);

  EXPECT_EQ(BuildIssuers(), *issuers);
}

TEST_F(BraveAdsAccountTest,
       DoNotGetIssuersWhenWalletIsCreatedIfIssuersAlreadyExist) {
  // Arrange
  BuildAndSetIssuers();

  MockUrlResponses(ads_client_mock_, GetValidIssuersUrlResponses());

  privacy::SetUnblindedTokens(/*count*/ 50);

  // Act
  account_->SetWallet(kWalletPaymentId, kWalletRecoverySeed);

  // Assert
  EXPECT_TRUE(wallet_was_created_);
  EXPECT_TRUE(wallet_did_update_);
  EXPECT_FALSE(wallet_did_change_);
  EXPECT_FALSE(invalid_wallet_);

  const absl::optional<IssuersInfo> issuers = GetIssuers();
  ASSERT_TRUE(issuers);

  EXPECT_EQ(BuildIssuers(), *issuers);
}

TEST_F(BraveAdsAccountTest, GetIssuersIfAdsAreEnabled) {
  // Arrange
  MockUrlResponses(ads_client_mock_, GetValidIssuersUrlResponses());

  account_->Process();

  // Act
  const absl::optional<IssuersInfo> issuers = GetIssuers();
  ASSERT_TRUE(issuers);

  // Assert
  EXPECT_EQ(BuildIssuers(), *issuers);
}

TEST_F(BraveAdsAccountTest, DoNotGetIssuersIfAdsAreDisabled) {
  // Arrange
  ads_client_mock_.SetBooleanPref(prefs::kEnabled, false);

  MockUrlResponses(ads_client_mock_, GetValidIssuersUrlResponses());

  account_->Process();

  // Act
  const absl::optional<IssuersInfo> issuers = GetIssuers();
  ASSERT_TRUE(issuers);

  // Assert
  const IssuersInfo expected_issuers;
  EXPECT_EQ(expected_issuers, *issuers);
}

TEST_F(BraveAdsAccountTest, DoNotGetInvalidIssuers) {
  // Arrange
  MockUrlResponses(ads_client_mock_, GetInvalidIssuersUrlResponses());

  account_->Process();

  // Act
  const absl::optional<IssuersInfo> issuers = GetIssuers();
  ASSERT_TRUE(issuers);

  // Assert
  const IssuersInfo expected_issuers;
  EXPECT_EQ(expected_issuers, *issuers);
}

TEST_F(BraveAdsAccountTest, DoNotGetMissingIssuers) {
  // Arrange
  const URLResponseMap url_responses = {{// Get issuers request
                                         "/v3/issuers/",
                                         {{net::HTTP_OK, /*response_body*/ R"(
                                          {
                                            "ping": 7200000,
                                            "issuers": []
                                          }
                                         )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  account_->Process();

  // Act
  const absl::optional<IssuersInfo> issuers = GetIssuers();
  ASSERT_TRUE(issuers);

  // Assert
  const IssuersInfo expected_issuers;
  EXPECT_EQ(expected_issuers, *issuers);
}

TEST_F(BraveAdsAccountTest, DoNotGetIssuersFromInvalidResponse) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Get issuers request
       "/v3/issuers/",
       {{net::HTTP_OK, /*response_body*/ "INVALID"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  account_->Process();

  // Act
  const absl::optional<IssuersInfo> issuers = GetIssuers();
  ASSERT_TRUE(issuers);

  // Assert
  const IssuersInfo expected_issuers;
  EXPECT_EQ(expected_issuers, *issuers);
}

TEST_F(BraveAdsAccountTest, DepositForCash) {
  // Arrange
  BuildAndSetIssuers();

  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_CREATED, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.717Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6"
            }
          )"}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_OK, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.736Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
              "paymentToken" : {
                "publicKey" : "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=",
                "batchProof" : "FWTZ5fOYITYlMWMYaxg254QWs+Pmd0dHzoor0mzIlQ8tWHagc7jm7UVJykqIo+ZSM+iK29mPuWJxPHpG4HypBw==",
                "signedTokens" : [
                  "DHe4S37Cn1WaTbCC+ytiNTB2s5H0vcLzVcRgzRoO3lU="
                ]
              }
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  privacy::SetUnblindedTokens(/*count*/ 1);

  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ false);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act
  account_->Deposit(creative_ad.creative_instance_id, AdType::kNotificationAd,
                    kSegment, ConfirmationType::kViewed);

  // Assert
  EXPECT_TRUE(did_process_deposit_);
  EXPECT_FALSE(failed_to_process_deposit_);
  EXPECT_TRUE(statement_of_accounts_did_change_);

  TransactionList expected_transactions;
  TransactionInfo expected_transaction;
  expected_transaction.id = transaction_.id;
  expected_transaction.created_at = Now();
  expected_transaction.creative_instance_id = creative_ad.creative_instance_id;
  expected_transaction.value = 1.0;
  expected_transaction.segment = kSegment;
  expected_transaction.ad_type = AdType::kNotificationAd;
  expected_transaction.confirmation_type = ConfirmationType::kViewed;
  expected_transactions.push_back(expected_transaction);

  GetTransactionsForDateRange(
      DistantPast(), DistantFuture(),
      base::BindOnce(
          [](const TransactionList& expected_transactions, const bool success,
             const TransactionList& transactions) {
            ASSERT_TRUE(success);
            EXPECT_EQ(expected_transactions, transactions);
          },
          std::move(expected_transactions)));
}

TEST_F(BraveAdsAccountTest, DepositForNonCash) {
  // Arrange
  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  privacy::SetUnblindedTokens(/*count*/ 1);

  // Act
  account_->Deposit(kCreativeInstanceId, AdType::kNotificationAd, kSegment,
                    ConfirmationType::kClicked);

  // Assert
  EXPECT_TRUE(did_process_deposit_);
  EXPECT_FALSE(failed_to_process_deposit_);
  EXPECT_TRUE(statement_of_accounts_did_change_);

  TransactionList expected_transactions;
  TransactionInfo expected_transaction;
  expected_transaction.id = transaction_.id;
  expected_transaction.created_at = Now();
  expected_transaction.creative_instance_id = kCreativeInstanceId;
  expected_transaction.value = 0.0;
  expected_transaction.segment = kSegment;
  expected_transaction.ad_type = AdType::kNotificationAd;
  expected_transaction.confirmation_type = ConfirmationType::kClicked;
  expected_transactions.push_back(expected_transaction);

  GetTransactionsForDateRange(
      DistantPast(), DistantFuture(),
      base::BindOnce(
          [](const TransactionList& expected_transactions, const bool success,
             const TransactionList& transactions) {
            ASSERT_TRUE(success);
            EXPECT_EQ(expected_transactions, transactions);
          },
          std::move(expected_transactions)));
}

TEST_F(BraveAdsAccountTest, DoNotDepositCashIfCreativeInstanceIdDoesNotExist) {
  // Arrange
  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ false);
  database::SaveCreativeNotificationAds({creative_ad});

  // Act
  account_->Deposit(kMissingCreativeInstanceId, AdType::kNotificationAd,
                    kSegment, ConfirmationType::kViewed);

  // Assert
  EXPECT_FALSE(did_process_deposit_);
  EXPECT_TRUE(failed_to_process_deposit_);
  EXPECT_FALSE(statement_of_accounts_did_change_);

  GetTransactionsForDateRange(
      DistantPast(), DistantFuture(),
      base::BindOnce(
          [](const bool success, const TransactionList& transactions) {
            ASSERT_TRUE(success);
            EXPECT_TRUE(transactions.empty());
          }));
}

TEST_F(BraveAdsAccountTest, GetStatement) {
  // Arrange
  TransactionList transactions;

  AdvanceClockTo(TimeFromString("31 October 2020", /*is_local*/ true));

  const TransactionInfo transaction_1 =
      BuildTransaction(/*value*/ 0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 =
      BuildTransaction(/*value*/ 0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_2);

  AdvanceClockTo(TimeFromString("18 November 2020", /*is_local*/ true));

  const TransactionInfo transaction_3 =
      BuildTransaction(/*value*/ 0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_3);

  const TransactionInfo transaction_4 =
      BuildTransaction(/*value*/ 0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_4);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local*/ true));

  const TransactionInfo transaction_5 =
      BuildTransaction(/*value*/ 0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_5);

  const TransactionInfo transaction_6 =
      BuildTransaction(/*value*/ 0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_6);

  const TransactionInfo transaction_7 =
      BuildTransaction(/*value*/ 0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_7);

  SaveTransactions(transactions);

  // Act
  Account::GetStatement(base::BindOnce([](mojom::StatementInfoPtr statement) {
    ASSERT_TRUE(statement);

    mojom::StatementInfoPtr expected_statement = mojom::StatementInfo::New();
    expected_statement->earnings_last_month = 0.01;
    expected_statement->earnings_this_month = 0.05;
    expected_statement->next_payment_date =
        TimeFromString("7 January 2021 23:59:59.999", /*is_local*/ false);
    expected_statement->ads_received_this_month = 3;

    EXPECT_EQ(expected_statement, statement);
  }));

  // Assert
}

TEST_F(BraveAdsAccountTest, DoNotGetStatementIfAdsAreDisabled) {
  // Arrange
  ads_client_mock_.SetBooleanPref(prefs::kEnabled, false);

  // Act
  Account::GetStatement(base::BindOnce(
      [](mojom::StatementInfoPtr statement) { EXPECT_FALSE(statement); }));

  // Assert
}

}  // namespace brave_ads
