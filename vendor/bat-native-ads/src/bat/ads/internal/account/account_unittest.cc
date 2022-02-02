/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/account.h"

#include "bat/ads/ad_type.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/issuers/issuers_info.h"
#include "bat/ads/internal/account/issuers/issuers_unittest_util.h"
#include "bat/ads/internal/account/issuers/issuers_util.h"
#include "bat/ads/internal/account/transactions/transactions.h"
#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info_aliases.h"
#include "bat/ads/internal/database/tables/creative_ad_notifications_database_table.h"
#include "bat/ads/internal/privacy/tokens/token_generator_mock.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/pref_names.h"
#include "bat/ads/statement_info.h"
#include "bat/ads/transaction_info.h"
#include "bat/ads/transaction_info_aliases.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::NiceMock;

namespace ads {

namespace {

constexpr char kWalletId[] = "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7";
constexpr char kWalletSeed[] = "x5uBvgI5MTTVY6sjGv65e9EHr8v7i+UxkFB9qVc5fP0=";
constexpr char kInvalidWalletSeed[] =
    "y6vCwhJ6NUUWZ7tkHw76f0FIs9w8j-VylGC0rWd6gQ1=";

}  // namespace

class BatAdsAccountTest : public AccountObserver, public UnitTestBase {
 protected:
  BatAdsAccountTest()
      : token_generator_mock_(
            std::make_unique<NiceMock<privacy::TokenGeneratorMock>>()),
        account_(std::make_unique<Account>(token_generator_mock_.get())) {
    account_->AddObserver(this);
  }

  ~BatAdsAccountTest() override = default;

  void Save(const CreativeAdNotificationList& creative_ads) {
    database::table::CreativeAdNotifications database_table;
    database_table.Save(creative_ads,
                        [](const bool success) { ASSERT_TRUE(success); });
  }

  void OnWalletDidUpdate(const WalletInfo& wallet) override {
    wallet_did_update_ = true;
  }

  void OnWalletDidChange(const WalletInfo& wallet) override {
    wallet_did_change_ = true;
  }

  void OnInvalidWallet() override { invalid_wallet_ = true; }

  void OnDepositedFunds(const TransactionInfo& transaction) override {
    deposited_funds_ = true;
    transaction_ = transaction;
  }

  void OnFailedToDepositFunds(
      const CreativeAdInfo& creative_instance_id,
      const AdType& ad_type,
      const ConfirmationType& confirmation_type) override {
    failed_to_deposit_funds_ = true;
  }

  void OnStatementOfAccountsDidChange() override {
    statement_of_accounts_did_change_ = true;
  }

  std::unique_ptr<privacy::TokenGeneratorMock> token_generator_mock_;
  std::unique_ptr<Account> account_;

  bool wallet_did_update_ = false;
  bool wallet_did_change_ = false;
  bool invalid_wallet_ = false;

  TransactionInfo transaction_;
  bool deposited_funds_ = false;
  bool failed_to_deposit_funds_ = false;

  bool statement_of_accounts_did_change_ = false;
};

TEST_F(BatAdsAccountTest, SetWallet) {
  // Arrange

  // Act
  account_->SetWallet(kWalletId, kWalletSeed);

  // Assert
  EXPECT_TRUE(wallet_did_update_);
  EXPECT_FALSE(wallet_did_change_);
  EXPECT_FALSE(invalid_wallet_);
}

TEST_F(BatAdsAccountTest, SetInvalidWallet) {
  // Arrange

  // Act
  account_->SetWallet(kWalletId, kInvalidWalletSeed);

  // Assert
  EXPECT_FALSE(wallet_did_update_);
  EXPECT_FALSE(wallet_did_change_);
  EXPECT_TRUE(invalid_wallet_);
}

TEST_F(BatAdsAccountTest, ChangeWallet) {
  // Arrange
  account_->SetWallet(kWalletId, kWalletSeed);

  // Act
  account_->SetWallet("c1bf0a09-cac8-48eb-8c21-7ca6d995b0a3", kWalletSeed);

  // Assert
  EXPECT_TRUE(wallet_did_update_);
  EXPECT_TRUE(wallet_did_change_);
  EXPECT_FALSE(invalid_wallet_);
}

TEST_F(BatAdsAccountTest, GetWallet) {
  // Arrange
  account_->SetWallet(kWalletId, kWalletSeed);

  // Act
  const WalletInfo& wallet = account_->GetWallet();

  // Assert
  WalletInfo expected_wallet;
  expected_wallet.id = "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7";
  expected_wallet.secret_key =
      "93052310477323AAE423A84BA32C68B1AE3B66B71952F6D8A69026E33BD817980621BF8B"
      "7B5F34B49E380F59179AE43C21B286473B28245B412DDB54632F150D";

  EXPECT_EQ(expected_wallet, wallet);
}

TEST_F(BatAdsAccountTest, GetIssuersIfAdsAreEnabled) {
  // Arrange
  AdsClientHelper::Get()->SetBooleanPref(prefs::kEnabled, true);

  const URLEndpoints& endpoints = {{// Get issuers request
                                    R"(/v1/issuers/)",
                                    {{net::HTTP_OK, R"(
        {
          "ping": 7200000,
          "issuers": [
            {
              "name": "confirmations",
              "publicKeys": [
                {
                  "publicKey": "JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=",
                  "associatedValue": ""
                },
                {
                  "publicKey": "crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=",
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
                }
              ]
            }
          ]
        }
        )"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  account_->MaybeGetIssuers();

  // Act
  const IssuersInfo& issuers = GetIssuers();

  // Assert
  const IssuersInfo& expected_issuers =
      BuildIssuers(7200000,
                   {{"JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=", 0.0},
                    {"crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=", 0.0}},
                   {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
                    {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1},
                    {"XovQyvVWM8ez0mAzTtfqgPIbSpH5/idv8w0KJxhirwA=", 0.1},
                    {"wAcnJtb34Asykf+2jrTWrjFiaTqilklZ6bxLyR3LyFo=", 0.1}});

  EXPECT_EQ(expected_issuers, issuers);
}

TEST_F(BatAdsAccountTest, DoNotGetIssuersIfAdsAreDisabled) {
  // Arrange
  AdsClientHelper::Get()->SetBooleanPref(prefs::kEnabled, false);

  const URLEndpoints& endpoints = {{// Get issuers request
                                    R"(/v1/issuers/)",
                                    {{net::HTTP_OK, R"(
        {
          "ping": 7200000,
          "issuers": [
            {
              "name": "confirmations",
              "publicKeys": [
                {
                  "publicKey": "JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=",
                  "associatedValue": ""
                },
                {
                  "publicKey": "crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=",
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
                }
              ]
            }
          ]
        }
        )"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  account_->MaybeGetIssuers();

  // Act
  const IssuersInfo& issuers = GetIssuers();

  // Assert
  const IssuersInfo expected_issuers;

  EXPECT_EQ(expected_issuers, issuers);
}

TEST_F(BatAdsAccountTest, DoNotGetInvalidIssuers) {
  // Arrange
  AdsClientHelper::Get()->SetBooleanPref(prefs::kEnabled, true);

  BuildAndSetIssuers();

  const URLEndpoints& endpoints = {{// Get issuers request
                                    R"(/v1/issuers/)",
                                    {{net::HTTP_OK, R"(
        {
          "ping": 7200000,
          "issuers": [
            {
              "name": "confirmations",
              "publicKeys": [
                {
                  "publicKey": "JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=",
                  "associatedValue": ""
                },
                {
                  "publicKey": "crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=",
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
                }
              ]
            }
          ]
        }
        )"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  account_->MaybeGetIssuers();

  // Act
  const IssuersInfo& issuers = GetIssuers();

  // Assert
  const IssuersInfo& expected_issuers =
      BuildIssuers(7200000,
                   {{"JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=", 0.0},
                    {"crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=", 0.0}},
                   {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
                    {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}});

  EXPECT_EQ(expected_issuers, issuers);
}

TEST_F(BatAdsAccountTest, DoNotGetMissingPaymentIssuers) {
  // Arrange
  AdsClientHelper::Get()->SetBooleanPref(prefs::kEnabled, true);

  BuildAndSetIssuers();

  const URLEndpoints& endpoints = {{// Get issuers request
                                    R"(/v1/issuers/)",
                                    {{net::HTTP_OK, R"(
        {
          "ping": 7200000,
          "issuers": [
            {
              "name": "confirmations",
              "publicKeys": [
                {
                  "publicKey": "JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=",
                  "associatedValue": ""
                },
                {
                  "publicKey": "crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=",
                  "associatedValue": ""
                }
              ]
            }
          ]
        }
        )"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  account_->MaybeGetIssuers();

  // Act
  const IssuersInfo& issuers = GetIssuers();

  // Assert
  const IssuersInfo& expected_issuers =
      BuildIssuers(7200000,
                   {{"JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=", 0.0},
                    {"crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=", 0.0}},
                   {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
                    {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}});

  EXPECT_EQ(expected_issuers, issuers);
}

TEST_F(BatAdsAccountTest, DepositFunds) {
  // Arrange
  CreativeAdNotificationList creative_ads;
  CreativeDaypartInfo daypart_info;
  CreativeAdNotificationInfo info;
  info.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info.start_at = DistantPast();
  info.end_at = DistantFuture();
  info.daily_cap = 1;
  info.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info.priority = 2;
  info.per_day = 3;
  info.per_week = 4;
  info.per_month = 5;
  info.total_max = 6;
  info.value = 1.0;
  info.segment = "technology & computing-software";
  info.dayparts.push_back(daypart_info);
  info.geo_targets = {"US"};
  info.target_url = "https://brave.com";
  info.title = "Test Ad 1 Title";
  info.body = "Test Ad 1 Body";
  info.ptr = 1.0;
  creative_ads.push_back(info);

  Save(creative_ads);

  // Act
  account_->DepositFunds(info.creative_instance_id, AdType::kAdNotification,
                         ConfirmationType::kViewed);

  // Assert
  EXPECT_TRUE(deposited_funds_);
  EXPECT_FALSE(failed_to_deposit_funds_);
  EXPECT_TRUE(statement_of_accounts_did_change_);

  const TransactionList& expected_transactions = {transaction_};

  transactions::GetForDateRange(
      DistantPast(), DistantFuture(),
      [&expected_transactions](const bool success,
                               const TransactionList& transactions) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_transactions, transactions);
      });
}

TEST_F(BatAdsAccountTest, DoNotDepositFundsIfCreativeInstanceIdDoesNotExist) {
  // Arrange
  CreativeAdNotificationList creative_ads;
  CreativeDaypartInfo daypart_info;
  CreativeAdNotificationInfo info;
  info.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info.start_at = DistantPast();
  info.end_at = DistantFuture();
  info.daily_cap = 1;
  info.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info.priority = 2;
  info.per_day = 3;
  info.per_week = 4;
  info.per_month = 5;
  info.total_max = 6;
  info.value = 1.0;
  info.segment = "technology & computing-software";
  info.dayparts.push_back(daypart_info);
  info.geo_targets = {"US"};
  info.target_url = "https://brave.com";
  info.title = "Test Ad 1 Title";
  info.body = "Test Ad 1 Body";
  info.ptr = 1.0;
  creative_ads.push_back(info);

  Save(creative_ads);

  // Act
  account_->DepositFunds("eaa6224a-876d-4ef8-a384-9ac34f238631",
                         AdType::kAdNotification, ConfirmationType::kViewed);

  // Assert
  EXPECT_FALSE(deposited_funds_);
  EXPECT_TRUE(failed_to_deposit_funds_);
  EXPECT_FALSE(statement_of_accounts_did_change_);

  transactions::GetForDateRange(
      DistantPast(), DistantFuture(),
      [](const bool success, const TransactionList& transactions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(transactions.empty());
      });
}

TEST_F(BatAdsAccountTest, GetStatement) {
  // Arrange
  TransactionList transactions;

  AdvanceClock(TimeFromString("31 October 2020", /* is_local */ true));

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_2);

  AdvanceClock(TimeFromString("18 November 2020", /* is_local */ true));

  const TransactionInfo& transaction_3 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_3);

  const TransactionInfo& transaction_4 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_4);

  AdvanceClock(TimeFromString("25 December 2020", /* is_local */ true));

  const TransactionInfo& transaction_5 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_5);

  const TransactionInfo& transaction_6 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_6);

  const TransactionInfo& transaction_7 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_7);

  SaveTransactions(transactions);

  // Act
  account_->GetStatement(
      [](const bool success, const StatementInfo& statement) {
        ASSERT_TRUE(success);

        StatementInfo expected_statement;
        expected_statement.next_payment_date = TimestampFromString(
            "5 January 2021 23:59:59.999", /* is_local */ false);
        expected_statement.earnings_this_month = 0.05;
        expected_statement.earnings_last_month = 0.01;
        expected_statement.ads_received_this_month = 3;

        EXPECT_EQ(expected_statement, statement);
      });

  // Assert
}

}  // namespace ads
