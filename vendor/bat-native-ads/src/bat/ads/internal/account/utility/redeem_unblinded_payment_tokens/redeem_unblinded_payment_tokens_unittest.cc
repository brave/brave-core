/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens.h"

#include <memory>

#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_delegate_mock.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/account/wallet/wallet_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_mock_util.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;

class BatAdsRedeemUnblindedPaymentTokensTest : public UnitTestBase {
 protected:
  BatAdsRedeemUnblindedPaymentTokensTest()
      : redeem_unblinded_payment_tokens_(
            std::make_unique<RedeemUnblindedPaymentTokens>()),
        redeem_unblinded_payment_tokens_delegate_mock_(
            std::make_unique<
                NiceMock<RedeemUnblindedPaymentTokensDelegateMock>>()) {
    redeem_unblinded_payment_tokens_->SetDelegate(
        redeem_unblinded_payment_tokens_delegate_mock_.get());
  }

  std::unique_ptr<RedeemUnblindedPaymentTokens>
      redeem_unblinded_payment_tokens_;
  std::unique_ptr<RedeemUnblindedPaymentTokensDelegateMock>
      redeem_unblinded_payment_tokens_delegate_mock_;
};

TEST_F(BatAdsRedeemUnblindedPaymentTokensTest, RedeemUnblindedPaymentTokens) {
  // Arrange
  const URLResponseMap url_responses = {
      {"/v3/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7",
       {{net::HTTP_OK, R"(
            {
              "payload": "{"paymentId":"27a39b2f-9b2e-4eb0-bbb2-2f84447496e7"}",
              "paymentCredentials": [
                {
                  "credential": {
                    "signature": "J6Lnoz1Ho5P4YDkcufA+WKUdR4C4f8QJARaT3Cko8RZ6dc777od9NQEaetU+xK3LXmQtmA6jfIUcLR3SCIJl0g==",
                    "t": "Z0GXil+GIQLOSSLHJV78jUE8cMxtwXtoROmv3uW8Qecpvx7L076GNI3TN44uF4uleOo2ZTpeKHzM2eeFHO2K6w=="
                  },
                  "publicKey": "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU="
                }
              ]
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  AdsClientHelper::GetInstance()->SetTimePref(prefs::kNextTokenRedemptionAt,
                                              Now());

  const privacy::UnblindedPaymentTokenList unblinded_payment_tokens =
      privacy::GetUnblindedPaymentTokens(1);
  privacy::GetUnblindedPaymentTokens()->SetTokens(unblinded_payment_tokens);

  // Act
  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRedeemUnblindedPaymentTokens(_));

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnFailedToRedeemUnblindedPaymentTokens())
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidScheduleNextUnblindedPaymentTokensRedemption(_));

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnWillRetryRedeemingUnblindedPaymentTokens(_))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRetryRedeemingUnblindedPaymentTokens())
      .Times(0);

  const WalletInfo wallet = GetWalletForTesting();
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);

  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_TRUE(privacy::UnblindedPaymentTokensIsEmpty());
}

TEST_F(BatAdsRedeemUnblindedPaymentTokensTest,
       RedeemUnblindedPaymentTokensMultipleTimes) {
  // Arrange
  const URLResponseMap url_responses = {
      {"/v3/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7",
       {{net::HTTP_OK, R"(
            {
              "payload": "{"paymentId":"27a39b2f-9b2e-4eb0-bbb2-2f84447496e7"}",
              "paymentCredentials": [
                {
                  "credential": {
                    "signature": "J6Lnoz1Ho5P4YDkcufA+WKUdR4C4f8QJARaT3Cko8RZ6dc777od9NQEaetU+xK3LXmQtmA6jfIUcLR3SCIJl0g==",
                    "t": "Z0GXil+GIQLOSSLHJV78jUE8cMxtwXtoROmv3uW8Qecpvx7L076GNI3TN44uF4uleOo2ZTpeKHzM2eeFHO2K6w=="
                  },
                  "publicKey": "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU="
                }
              ]
            }
          )"},
        {net::HTTP_OK, R"(
            {
              "payload": "{"paymentId":"27a39b2f-9b2e-4eb0-bbb2-2f84447496e7"}",
              "paymentCredentials": [
                {
                  "credential": {
                    "signature": "J6Lnoz1Ho5P4YDkcufA+WKUdR4C4f8QJARaT3Cko8RZ6dc777od9NQEaetU+xK3LXmQtmA6jfIUcLR3SCIJl0g==",
                    "t": "Z0GXil+GIQLOSSLHJV78jUE8cMxtwXtoROmv3uW8Qecpvx7L076GNI3TN44uF4uleOo2ZTpeKHzM2eeFHO2K6w=="
                  },
                  "publicKey": "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU="
                }
              ]
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  AdsClientHelper::GetInstance()->SetTimePref(prefs::kNextTokenRedemptionAt,
                                              Now());

  const privacy::UnblindedPaymentTokenList unblinded_payment_tokens =
      privacy::GetUnblindedPaymentTokens(1);
  privacy::GetUnblindedPaymentTokens()->SetTokens(unblinded_payment_tokens);

  // Act
  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRedeemUnblindedPaymentTokens(_))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnFailedToRedeemUnblindedPaymentTokens())
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidScheduleNextUnblindedPaymentTokensRedemption(_))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnWillRetryRedeemingUnblindedPaymentTokens(_))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRetryRedeemingUnblindedPaymentTokens())
      .Times(0);

  const WalletInfo wallet = GetWalletForTesting();
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);

  // Assert
  EXPECT_EQ(1UL, GetPendingTaskCount());
}

TEST_F(BatAdsRedeemUnblindedPaymentTokensTest, ScheduleNextTokenRedemption) {
  // Arrange
  const URLResponseMap url_responses = {
      {"/v3/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7",
       {{net::HTTP_OK, R"(
            {
              "payload": "{"paymentId":"27a39b2f-9b2e-4eb0-bbb2-2f84447496e7"}",
              "paymentCredentials": [
                {
                  "credential": {
                    "signature": "J6Lnoz1Ho5P4YDkcufA+WKUdR4C4f8QJARaT3Cko8RZ6dc777od9NQEaetU+xK3LXmQtmA6jfIUcLR3SCIJl0g==",
                    "t": "Z0GXil+GIQLOSSLHJV78jUE8cMxtwXtoROmv3uW8Qecpvx7L076GNI3TN44uF4uleOo2ZTpeKHzM2eeFHO2K6w=="
                  },
                  "publicKey": "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU="
                }
              ]
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  AdsClientHelper::GetInstance()->SetTimePref(prefs::kNextTokenRedemptionAt,
                                              Now());

  const privacy::UnblindedPaymentTokenList unblinded_payment_tokens =
      privacy::GetUnblindedPaymentTokens(1);
  privacy::GetUnblindedPaymentTokens()->SetTokens(unblinded_payment_tokens);

  // Act
  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRedeemUnblindedPaymentTokens(_));

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnFailedToRedeemUnblindedPaymentTokens())
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidScheduleNextUnblindedPaymentTokensRedemption(_));

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnWillRetryRedeemingUnblindedPaymentTokens(_))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRetryRedeemingUnblindedPaymentTokens())
      .Times(0);

  const WalletInfo wallet = GetWalletForTesting();
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);

  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_TRUE(privacy::UnblindedPaymentTokensIsEmpty());
}

TEST_F(BatAdsRedeemUnblindedPaymentTokensTest, InvalidWallet) {
  // Arrange
  const URLResponseMap url_responses = {
      {"/v3/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7",
       {{net::HTTP_OK, R"(
            {
              "payload": "{"paymentId":"27a39b2f-9b2e-4eb0-bbb2-2f84447496e7"}",
              "paymentCredentials": [
                {
                  "credential": {
                    "signature": "J6Lnoz1Ho5P4YDkcufA+WKUdR4C4f8QJARaT3Cko8RZ6dc777od9NQEaetU+xK3LXmQtmA6jfIUcLR3SCIJl0g==",
                    "t": "Z0GXil+GIQLOSSLHJV78jUE8cMxtwXtoROmv3uW8Qecpvx7L076GNI3TN44uF4uleOo2ZTpeKHzM2eeFHO2K6w=="
                  },
                  "publicKey": "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU="
                }
              ]
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  AdsClientHelper::GetInstance()->SetTimePref(prefs::kNextTokenRedemptionAt,
                                              Now());

  const privacy::UnblindedPaymentTokenList unblinded_payment_tokens =
      privacy::GetUnblindedPaymentTokens(1);
  privacy::GetUnblindedPaymentTokens()->SetTokens(unblinded_payment_tokens);

  // Act
  const InSequence seq;

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnFailedToRedeemUnblindedPaymentTokens());

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnWillRetryRedeemingUnblindedPaymentTokens(_))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRetryRedeemingUnblindedPaymentTokens())
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRedeemUnblindedPaymentTokens(_))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidScheduleNextUnblindedPaymentTokensRedemption(_))
      .Times(0);

  const WalletInfo invalid_wallet;
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(invalid_wallet);

  // Assert
  EXPECT_EQ(1, privacy::GetUnblindedPaymentTokens()->Count());
}

TEST_F(BatAdsRedeemUnblindedPaymentTokensTest, NoUnblindedPaymentTokens) {
  // Arrange
  const URLResponseMap url_responses = {
      {"/v3/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7",
       {{net::HTTP_OK, R"(
            {
              "payload": "{"paymentId":"27a39b2f-9b2e-4eb0-bbb2-2f84447496e7"}",
              "paymentCredentials": [
                {
                  "credential": {
                    "signature": "J6Lnoz1Ho5P4YDkcufA+WKUdR4C4f8QJARaT3Cko8RZ6dc777od9NQEaetU+xK3LXmQtmA6jfIUcLR3SCIJl0g==",
                    "t": "Z0GXil+GIQLOSSLHJV78jUE8cMxtwXtoROmv3uW8Qecpvx7L076GNI3TN44uF4uleOo2ZTpeKHzM2eeFHO2K6w=="
                  },
                  "publicKey": "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU="
                }
              ]
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  AdsClientHelper::GetInstance()->SetTimePref(prefs::kNextTokenRedemptionAt,
                                              Now());

  // Act
  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRedeemUnblindedPaymentTokens(_))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnFailedToRedeemUnblindedPaymentTokens())
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidScheduleNextUnblindedPaymentTokensRedemption(_));

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnWillRetryRedeemingUnblindedPaymentTokens(_))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRetryRedeemingUnblindedPaymentTokens())
      .Times(0);

  const WalletInfo wallet = GetWalletForTesting();
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);

  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_TRUE(privacy::UnblindedPaymentTokensIsEmpty());
}

TEST_F(BatAdsRedeemUnblindedPaymentTokensTest, Retry) {
  // Arrange
  const URLResponseMap url_responses = {
      {"/v3/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7",
       {{net::HTTP_NOT_FOUND, {}}, {net::HTTP_OK, R"(
            {
              "payload": "{"paymentId":"27a39b2f-9b2e-4eb0-bbb2-2f84447496e7"}",
              "paymentCredentials": [
                {
                  "credential": {
                    "signature": "J6Lnoz1Ho5P4YDkcufA+WKUdR4C4f8QJARaT3Cko8RZ6dc777od9NQEaetU+xK3LXmQtmA6jfIUcLR3SCIJl0g==",
                    "t": "Z0GXil+GIQLOSSLHJV78jUE8cMxtwXtoROmv3uW8Qecpvx7L076GNI3TN44uF4uleOo2ZTpeKHzM2eeFHO2K6w=="
                  },
                  "publicKey": "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU="
                }
              ]
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  AdsClientHelper::GetInstance()->SetTimePref(prefs::kNextTokenRedemptionAt,
                                              Now());

  const privacy::UnblindedPaymentTokenList unblinded_payment_tokens =
      privacy::GetUnblindedPaymentTokens(1);
  privacy::GetUnblindedPaymentTokens()->SetTokens(unblinded_payment_tokens);

  // Act
  const InSequence seq;

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnFailedToRedeemUnblindedPaymentTokens());

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnWillRetryRedeemingUnblindedPaymentTokens(_));

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRetryRedeemingUnblindedPaymentTokens());

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRedeemUnblindedPaymentTokens(_));

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidScheduleNextUnblindedPaymentTokensRedemption(_));

  const WalletInfo wallet = GetWalletForTesting();
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);

  FastForwardClockToNextPendingTask();
  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_TRUE(privacy::UnblindedPaymentTokensIsEmpty());
}

}  // namespace ads
