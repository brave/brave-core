/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/account.h"

#include <utility>

#include "absl/types/optional.h"
#include "base/functional/bind.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/issuers/issuers_info.h"
#include "bat/ads/internal/account/issuers/issuers_unittest_util.h"
#include "bat/ads/internal/account/issuers/issuers_util.h"
#include "bat/ads/internal/account/transactions/transaction_info.h"
#include "bat/ads/internal/account/transactions/transactions.h"
#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/account/wallet/wallet_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_mock_util.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "bat/ads/internal/privacy/tokens/token_generator_mock.h"
#include "bat/ads/internal/privacy/tokens/token_generator_unittest_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "net/http/http_status_code.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class BatAdsAccountTest : public AccountObserver, public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    token_generator_mock_ =
        std::make_unique<NiceMock<privacy::TokenGeneratorMock>>();
    account_ = std::make_unique<Account>(token_generator_mock_.get());
    account_->AddObserver(this);
  }

  void TearDown() override {
    account_->RemoveObserver(this);

    UnitTestBase::TearDown();
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
  account_->SetWallet(GetWalletPaymentIdForTesting(),
                      GetWalletRecoverySeedForTesting());

  // Assert
  EXPECT_TRUE(wallet_did_update_);
  EXPECT_FALSE(wallet_did_change_);
  EXPECT_FALSE(invalid_wallet_);
}

TEST_F(BatAdsAccountTest, SetInvalidWallet) {
  // Arrange

  // Act
  account_->SetWallet(GetWalletPaymentIdForTesting(),
                      GetInvalidWalletRecoverySeedForTesting());

  // Assert
  EXPECT_FALSE(wallet_did_update_);
  EXPECT_FALSE(wallet_did_change_);
  EXPECT_TRUE(invalid_wallet_);
}

TEST_F(BatAdsAccountTest, ChangeWallet) {
  // Arrange
  account_->SetWallet(GetWalletPaymentIdForTesting(),
                      GetWalletRecoverySeedForTesting());

  // Act
  account_->SetWallet(/*payment_id*/ "c1bf0a09-cac8-48eb-8c21-7ca6d995b0a3",
                      GetWalletRecoverySeedForTesting());

  // Assert
  EXPECT_TRUE(wallet_did_update_);
  EXPECT_TRUE(wallet_did_change_);
  EXPECT_FALSE(invalid_wallet_);
}

TEST_F(BatAdsAccountTest, GetWallet) {
  // Arrange
  account_->SetWallet(GetWalletPaymentIdForTesting(),
                      GetWalletRecoverySeedForTesting());

  // Act
  const WalletInfo& wallet = account_->GetWallet();

  // Assert
  WalletInfo expected_wallet;
  expected_wallet.payment_id = "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7";
  expected_wallet.public_key = "BiG/i3tfNLSeOA9ZF5rkPCGyhkc7KCRbQS3bVGMvFQ0=";
  expected_wallet.secret_key =
      "kwUjEEdzI6rkI6hLoyxosa47ZrcZUvbYppAm4zvYF5gGIb+"
      "Le180tJ44D1kXmuQ8IbKGRzsoJFtBLdtUYy8VDQ==";

  EXPECT_EQ(expected_wallet, wallet);
}

TEST_F(BatAdsAccountTest, GetIssuersIfAdsAreEnabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  const URLResponseMap url_responses = {{// Get issuers request
                                         "/v3/issuers/",
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
                },
                {
                  "publicKey": "JlOezORiqLkFkvapoNRGWcMH3/g09/7M2UPEwMjRpFE=",
                  "associatedValue": "0.1"
                },
                {
                  "publicKey": "hJP1nDjTdHcVDw347oH0XO+XBPPh5wZA2xWZE8QUSSA=",
                  "associatedValue": "0.1"
                }
              ]
            }
          ]
        }
        )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  account_->Process();

  // Act
  const absl::optional<IssuersInfo> issuers = GetIssuers();
  ASSERT_TRUE(issuers);

  // Assert
  const IssuersInfo expected_issuers = BuildIssuers(
      /*ping*/ 7'200'000,
      /*confirmation_public_keys*/
      {{"JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=", 0.0},
       {"crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=", 0.0}},
      /*payments_public_keys*/
      {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
       {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1},
       {"XovQyvVWM8ez0mAzTtfqgPIbSpH5/idv8w0KJxhirwA=", 0.1},
       {"wAcnJtb34Asykf+2jrTWrjFiaTqilklZ6bxLyR3LyFo=", 0.1},
       {"ZvzeYOT1geUQXfOsYXBxZj/H26IfiBUVodHl51j68xI=", 0.1},
       {"JlOezORiqLkFkvapoNRGWcMH3/g09/7M2UPEwMjRpFE=", 0.1},
       {"hJP1nDjTdHcVDw347oH0XO+XBPPh5wZA2xWZE8QUSSA=", 0.1}});

  EXPECT_EQ(expected_issuers, *issuers);
}

TEST_F(BatAdsAccountTest, DoNotGetIssuersIfAdsAreDisabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, false);

  const URLResponseMap url_responses = {{// Get issuers request
                                         "/v3/issuers/",
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
                },
                {
                  "publicKey": "JlOezORiqLkFkvapoNRGWcMH3/g09/7M2UPEwMjRpFE=",
                  "associatedValue": "0.1"
                },
                {
                  "publicKey": "hJP1nDjTdHcVDw347oH0XO+XBPPh5wZA2xWZE8QUSSA=",
                  "associatedValue": "0.1"
                }
              ]
            }
          ]
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

TEST_F(BatAdsAccountTest, DoNotGetInvalidIssuers) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  const URLResponseMap url_responses = {{// Get issuers request
                                         "/v3/issuers/",
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

TEST_F(BatAdsAccountTest, DoNotGetMissingIssuers) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  const URLResponseMap url_responses = {{// Get issuers request
                                         "/v3/issuers/",
                                         {{net::HTTP_OK, R"(
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

TEST_F(BatAdsAccountTest, DoNotGetIssuersFromInvalidResponse) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  const URLResponseMap url_responses = {{// Get issuers request
                                         "/v3/issuers/",
                                         {{net::HTTP_OK, "INVALID"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  account_->Process();

  // Act
  const absl::optional<IssuersInfo> issuers = GetIssuers();
  ASSERT_TRUE(issuers);

  // Assert
  const IssuersInfo expected_issuers;

  EXPECT_EQ(expected_issuers, *issuers);
}

TEST_F(BatAdsAccountTest, DepositForCash) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

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
       {{net::HTTP_CREATED, R"(
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
       {{net::HTTP_OK, R"(
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

  BuildAndSetIssuers();

  ON_CALL(*token_generator_mock_, Generate(_))
      .WillByDefault(Return(privacy::GetTokens(1)));

  privacy::SetUnblindedTokens(1);

  CreativeNotificationAdList creative_ads;
  const CreativeDaypartInfo daypart_info;
  CreativeNotificationAdInfo info;
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

  SaveCreativeAds(creative_ads);

  // Act
  account_->Deposit(info.creative_instance_id, AdType::kNotificationAd,
                    ConfirmationType::kViewed);

  // Assert
  EXPECT_TRUE(did_process_deposit_);
  EXPECT_FALSE(failed_to_process_deposit_);
  EXPECT_TRUE(statement_of_accounts_did_change_);

  TransactionList expected_transactions;
  TransactionInfo expected_transaction;
  expected_transaction.id = transaction_.id;
  expected_transaction.created_at = Now();
  expected_transaction.creative_instance_id = info.creative_instance_id;
  expected_transaction.value = 1.0;
  expected_transaction.ad_type = AdType::kNotificationAd;
  expected_transaction.confirmation_type = ConfirmationType::kViewed;
  expected_transactions.push_back(expected_transaction);

  transactions::GetForDateRange(
      DistantPast(), DistantFuture(),
      base::BindOnce(
          [](const TransactionList& expected_transactions, const bool success,
             const TransactionList& transactions) {
            ASSERT_TRUE(success);
            EXPECT_EQ(expected_transactions, transactions);
          },
          std::move(expected_transactions)));
}

TEST_F(BatAdsAccountTest, DepositForNonCash) {
  // Arrange
  ON_CALL(*token_generator_mock_, Generate(_))
      .WillByDefault(Return(privacy::GetTokens(1)));

  privacy::SetUnblindedTokens(1);

  // Act
  account_->Deposit("3519f52c-46a4-4c48-9c2b-c264c0067f04",
                    AdType::kNotificationAd, ConfirmationType::kClicked);

  // Assert
  EXPECT_TRUE(did_process_deposit_);
  EXPECT_FALSE(failed_to_process_deposit_);
  EXPECT_TRUE(statement_of_accounts_did_change_);

  TransactionList expected_transactions;
  TransactionInfo expected_transaction;
  expected_transaction.id = transaction_.id;
  expected_transaction.created_at = Now();
  expected_transaction.creative_instance_id =
      "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  expected_transaction.value = 0.0;
  expected_transaction.ad_type = AdType::kNotificationAd;
  expected_transaction.confirmation_type = ConfirmationType::kClicked;
  expected_transactions.push_back(expected_transaction);

  transactions::GetForDateRange(
      DistantPast(), DistantFuture(),
      base::BindOnce(
          [](const TransactionList& expected_transactions, const bool success,
             const TransactionList& transactions) {
            ASSERT_TRUE(success);
            EXPECT_EQ(expected_transactions, transactions);
          },
          std::move(expected_transactions)));
}

TEST_F(BatAdsAccountTest, DoNotDepositCashIfCreativeInstanceIdDoesNotExist) {
  // Arrange
  ON_CALL(*token_generator_mock_, Generate(_))
      .WillByDefault(Return(privacy::GetTokens(1)));

  CreativeNotificationAdList creative_ads;
  const CreativeDaypartInfo daypart_info;
  CreativeNotificationAdInfo info;
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

  SaveCreativeAds(creative_ads);

  // Act
  account_->Deposit("eaa6224a-876d-4ef8-a384-9ac34f238631",
                    AdType::kNotificationAd, ConfirmationType::kViewed);

  // Assert
  EXPECT_FALSE(did_process_deposit_);
  EXPECT_TRUE(failed_to_process_deposit_);
  EXPECT_FALSE(statement_of_accounts_did_change_);

  transactions::GetForDateRange(
      DistantPast(), DistantFuture(),
      base::BindOnce(
          [](const bool success, const TransactionList& transactions) {
            ASSERT_TRUE(success);
            EXPECT_TRUE(transactions.empty());
          }));
}

TEST_F(BatAdsAccountTest, GetStatement) {
  // Arrange
  TransactionList transactions;

  AdvanceClockTo(TimeFromString("31 October 2020", /*is_local*/ true));

  const TransactionInfo transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_2);

  AdvanceClockTo(TimeFromString("18 November 2020", /*is_local*/ true));

  const TransactionInfo transaction_3 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_3);

  const TransactionInfo transaction_4 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_4);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local*/ true));

  const TransactionInfo transaction_5 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_5);

  const TransactionInfo transaction_6 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_6);

  const TransactionInfo transaction_7 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
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

}  // namespace ads
