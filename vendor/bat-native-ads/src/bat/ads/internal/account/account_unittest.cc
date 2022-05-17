/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/account.h"

#include <vector>

#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/issuers/issuers_info.h"
#include "bat/ads/internal/account/issuers/issuers_unittest_util.h"
#include "bat/ads/internal/account/issuers/issuers_util.h"
#include "bat/ads/internal/account/transactions/transactions.h"
#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/ad_server/catalog/bundle/creative_ad_notification_info_aliases.h"
#include "bat/ads/internal/ad_server/catalog/bundle/creative_ad_notifications_database_table.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/privacy/tokens/token_generator_mock.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"
#include "bat/ads/pref_names.h"
#include "bat/ads/statement_info.h"
#include "bat/ads/transaction_info.h"
#include "bat/ads/transaction_info_aliases.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

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

  void OnDidProcessDeposit(const TransactionInfo& transaction) override {
    did_process_deposit_ = true;
    transaction_ = transaction;
  }

  void OnFailedToProcessDeposit(
      const std::string& creative_instance_id,
      const AdType& ad_type,
      const ConfirmationType& confirmation_type) override {
    failed_to_process_deposit_ = true;
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
  bool did_process_deposit_ = false;
  bool failed_to_process_deposit_ = false;

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

TEST_F(BatAdsAccountTest, DepositForCash) {
  // Arrange
  AdsClientHelper::Get()->SetBooleanPref(prefs::kEnabled, true);

  const URLEndpoints& endpoints = {
      {// Create confirmation request
       R"(/v2/confirmation/9fd71bc4-1b8e-4c1e-8ddc-443193a09f91/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVRxeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidGVzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjMC1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIsXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVURyR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYmJmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TWGpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3Zz09In0=)",
       {{net::HTTP_CREATED, R"(
            {
              "id" : "9fd71bc4-1b8e-4c1e-8ddc-443193a09f91",
              "payload" : {},
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.717Z",
              "creativeInstanceId" : "70829d71-ce2e-4483-a4c0-e1e2bee96520"
            }
          )"}}},
      {// Fetch payment token request
       R"(/v2/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/paymentToken)",
       {{net::HTTP_OK, R"(
            {
              "id" : "d990ed8d-d739-49fb-811b-c2e02158fb60",
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

  MockUrlRequest(ads_client_mock_, endpoints);

  BuildAndSetIssuers();

  const std::vector<std::string> tokens_base64 = {
      R"(nDM8XFo2GzY/ekTtHm3MYTK9Rs80rot3eS1n+WAuzmRvf64rHFMAcMUydrqKi2pUhgjthd8SM9BW3ituHudFNC5fS1c1Z+pe1oW2P5UxNOb8KurYGGQj/OHsG8jWhGMD)"};
  std::vector<privacy::cbr::Token> tokens;
  for (const auto& token_base64 : tokens_base64) {
    const privacy::cbr::Token token = privacy::cbr::Token(token_base64);
    ASSERT_TRUE(token.has_value());
    tokens.push_back(token);
  }
  ON_CALL(*token_generator_mock_, Generate(_)).WillByDefault(Return(tokens));

  privacy::SetUnblindedTokens(1);

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
  info.target_url = GURL("https://brave.com");
  info.title = "Test Ad 1 Title";
  info.body = "Test Ad 1 Body";
  info.ptr = 1.0;
  creative_ads.push_back(info);

  Save(creative_ads);

  // Act
  account_->Deposit(info.creative_instance_id, AdType::kAdNotification,
                    ConfirmationType::kViewed);

  // Assert
  EXPECT_TRUE(did_process_deposit_);
  EXPECT_FALSE(failed_to_process_deposit_);
  EXPECT_TRUE(statement_of_accounts_did_change_);

  TransactionList expected_transactions;
  TransactionInfo expected_transaction;
  expected_transaction.id = transaction_.id;
  expected_transaction.created_at = NowAsTimestamp();
  expected_transaction.creative_instance_id = info.creative_instance_id;
  expected_transaction.value = 1.0;
  expected_transaction.ad_type = AdType::kAdNotification;
  expected_transaction.confirmation_type = ConfirmationType::kViewed;
  expected_transactions.push_back(expected_transaction);

  transactions::GetForDateRange(
      DistantPast(), DistantFuture(),
      [&expected_transactions](const bool success,
                               const TransactionList& transactions) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_transactions, transactions);
      });
}

TEST_F(BatAdsAccountTest, DepositForNonCash) {
  // Arrange
  const std::vector<std::string> tokens_base64 = {
      R"(nDM8XFo2GzY/ekTtHm3MYTK9Rs80rot3eS1n+WAuzmRvf64rHFMAcMUydrqKi2pUhgjthd8SM9BW3ituHudFNC5fS1c1Z+pe1oW2P5UxNOb8KurYGGQj/OHsG8jWhGMD)"};
  std::vector<privacy::cbr::Token> tokens;
  for (const auto& token_base64 : tokens_base64) {
    const privacy::cbr::Token token = privacy::cbr::Token(token_base64);
    ASSERT_TRUE(token.has_value());
    tokens.push_back(token);
  }
  ON_CALL(*token_generator_mock_, Generate(_)).WillByDefault(Return(tokens));

  privacy::SetUnblindedTokens(1);

  // Act
  account_->Deposit("3519f52c-46a4-4c48-9c2b-c264c0067f04",
                    AdType::kAdNotification, ConfirmationType::kClicked);

  // Assert
  EXPECT_TRUE(did_process_deposit_);
  EXPECT_FALSE(failed_to_process_deposit_);
  EXPECT_TRUE(statement_of_accounts_did_change_);

  TransactionList expected_transactions;
  TransactionInfo expected_transaction;
  expected_transaction.id = transaction_.id;
  expected_transaction.created_at = NowAsTimestamp();
  expected_transaction.creative_instance_id =
      "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  expected_transaction.value = 0.0;
  expected_transaction.ad_type = AdType::kAdNotification;
  expected_transaction.confirmation_type = ConfirmationType::kClicked;
  expected_transactions.push_back(expected_transaction);

  transactions::GetForDateRange(
      DistantPast(), DistantFuture(),
      [&expected_transactions](const bool success,
                               const TransactionList& transactions) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_transactions, transactions);
      });
}

TEST_F(BatAdsAccountTest, DoNotDepositCashIfCreativeInstanceIdDoesNotExist) {
  // Arrange
  const std::vector<std::string> tokens_base64 = {
      R"(nDM8XFo2GzY/ekTtHm3MYTK9Rs80rot3eS1n+WAuzmRvf64rHFMAcMUydrqKi2pUhgjthd8SM9BW3ituHudFNC5fS1c1Z+pe1oW2P5UxNOb8KurYGGQj/OHsG8jWhGMD)"};
  std::vector<privacy::cbr::Token> tokens;
  for (const auto& token_base64 : tokens_base64) {
    const privacy::cbr::Token token = privacy::cbr::Token(token_base64);
    ASSERT_TRUE(token.has_value());
    tokens.push_back(token);
  }
  ON_CALL(*token_generator_mock_, Generate(_)).WillByDefault(Return(tokens));

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
  info.target_url = GURL("https://brave.com");
  info.title = "Test Ad 1 Title";
  info.body = "Test Ad 1 Body";
  info.ptr = 1.0;
  creative_ads.push_back(info);

  Save(creative_ads);

  // Act
  account_->Deposit("eaa6224a-876d-4ef8-a384-9ac34f238631",
                    AdType::kAdNotification, ConfirmationType::kViewed);

  // Assert
  EXPECT_FALSE(did_process_deposit_);
  EXPECT_TRUE(failed_to_process_deposit_);
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
