/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens.h"

#include <memory>

#include "bat/ads/internal/account/wallet/wallet.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens_unittest_util.h"
#include "bat/ads/internal/tokens/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_delegate_mock.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;

namespace ads {

class BatAdsRedeemUnblindedPaymentTokensTest : public UnitTestBase {
 protected:
  BatAdsRedeemUnblindedPaymentTokensTest()
      : redeem_unblinded_payment_tokens_(
            std::make_unique<RedeemUnblindedPaymentTokens>()),
        redeem_unblinded_payment_tokens_delegate_mock_(
            std::make_unique<
                NiceMock<RedeemUnblindedPaymentTokensDelegateMock>>()) {
    redeem_unblinded_payment_tokens_->set_delegate(
        redeem_unblinded_payment_tokens_delegate_mock_.get());
  }

  ~BatAdsRedeemUnblindedPaymentTokensTest() override = default;

  privacy::UnblindedTokens* get_unblinded_payment_tokens() {
    return ConfirmationsState::Get()->get_unblinded_payment_tokens();
  }

  WalletInfo GetWallet() {
    Wallet wallet;
    wallet.Set("27a39b2f-9b2e-4eb0-bbb2-2f84447496e7",
               "x5uBvgI5MTTVY6sjGv65e9EHr8v7i+UxkFB9qVc5fP0=");

    return wallet.Get();
  }

  std::unique_ptr<RedeemUnblindedPaymentTokens>
      redeem_unblinded_payment_tokens_;
  std::unique_ptr<RedeemUnblindedPaymentTokensDelegateMock>
      redeem_unblinded_payment_tokens_delegate_mock_;
};

TEST_F(BatAdsRedeemUnblindedPaymentTokensTest, RedeemUnblindedPaymentTokens) {
  // Arrange
  const URLEndpoints endpoints = {
      {R"(/v1/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)",
       {{net::HTTP_OK, R"(
            {
              "payload": "{\"paymentId\":\"27a39b2f-9b2e-4eb0-bbb2-2f84447496e7\"}",
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

  MockUrlRequest(ads_client_mock_, endpoints);

  const base::Time time = Now();
  ConfirmationsState::Get()->SetNextTokenRedemptionDate(time);

  const privacy::UnblindedTokenList unblinded_tokens =
      privacy::GetUnblindedTokens(1);
  get_unblinded_payment_tokens()->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRedeemUnblindedPaymentTokens(_))
      .Times(1);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnFailedToRedeemUnblindedPaymentTokens())
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidScheduleNextUnblindedPaymentTokensRedemption(_))
      .Times(1);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnWillRetryRedeemingUnblindedPaymentTokens())
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRetryRedeemingUnblindedPaymentTokens())
      .Times(0);

  const WalletInfo wallet = GetWallet();
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);

  FastForwardClockBy(NextPendingTaskDelay());

  // Assert
  EXPECT_EQ(0, get_unblinded_payment_tokens()->Count());
}

TEST_F(BatAdsRedeemUnblindedPaymentTokensTest,
       RedeemUnblindedPaymentTokensMultipleTimes) {
  // Arrange
  const URLEndpoints endpoints = {
      {R"(/v1/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)",
       {{net::HTTP_OK, R"(
            {
              "payload": "{\"paymentId\":\"27a39b2f-9b2e-4eb0-bbb2-2f84447496e7\"}",
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
              "payload": "{\"paymentId\":\"27a39b2f-9b2e-4eb0-bbb2-2f84447496e7\"}",
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

  MockUrlRequest(ads_client_mock_, endpoints);

  const base::Time time = Now();
  ConfirmationsState::Get()->SetNextTokenRedemptionDate(time);

  const privacy::UnblindedTokenList unblinded_tokens =
      privacy::GetUnblindedTokens(1);
  get_unblinded_payment_tokens()->SetTokens(unblinded_tokens);

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
              OnWillRetryRedeemingUnblindedPaymentTokens())
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRetryRedeemingUnblindedPaymentTokens())
      .Times(0);

  const WalletInfo wallet = GetWallet();
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);

  // Assert
  EXPECT_EQ(1UL, GetPendingTaskCount());
}

TEST_F(BatAdsRedeemUnblindedPaymentTokensTest, ScheduleNextTokenRedemption) {
  // Arrange
  const URLEndpoints endpoints = {
      {R"(/v1/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)",
       {{net::HTTP_OK, R"(
            {
              "payload": "{\"paymentId\":\"27a39b2f-9b2e-4eb0-bbb2-2f84447496e7\"}",
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

  MockUrlRequest(ads_client_mock_, endpoints);

  const base::Time time = Now();
  ConfirmationsState::Get()->SetNextTokenRedemptionDate(time);

  const privacy::UnblindedTokenList unblinded_tokens =
      privacy::GetUnblindedTokens(1);
  get_unblinded_payment_tokens()->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRedeemUnblindedPaymentTokens(_))
      .Times(1);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnFailedToRedeemUnblindedPaymentTokens())
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidScheduleNextUnblindedPaymentTokensRedemption(_))
      .Times(1);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnWillRetryRedeemingUnblindedPaymentTokens())
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRetryRedeemingUnblindedPaymentTokens())
      .Times(0);

  const WalletInfo wallet = GetWallet();
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);

  FastForwardClockBy(NextPendingTaskDelay());

  // Assert
  EXPECT_EQ(0, get_unblinded_payment_tokens()->Count());
}

TEST_F(BatAdsRedeemUnblindedPaymentTokensTest, InvalidWallet) {
  // Arrange
  const URLEndpoints endpoints = {
      {R"(/v1/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)",
       {{net::HTTP_OK, R"(
            {
              "payload": "{\"paymentId\":\"27a39b2f-9b2e-4eb0-bbb2-2f84447496e7\"}",
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

  MockUrlRequest(ads_client_mock_, endpoints);

  const base::Time time = Now();
  ConfirmationsState::Get()->SetNextTokenRedemptionDate(time);

  const privacy::UnblindedTokenList unblinded_tokens =
      privacy::GetUnblindedTokens(1);
  get_unblinded_payment_tokens()->SetTokens(unblinded_tokens);

  // Act
  InSequence seq;

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnFailedToRedeemUnblindedPaymentTokens())
      .Times(1);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnWillRetryRedeemingUnblindedPaymentTokens())
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

  FastForwardClockBy(NextPendingTaskDelay());

  // Assert
  EXPECT_EQ(1, get_unblinded_payment_tokens()->Count());
}

TEST_F(BatAdsRedeemUnblindedPaymentTokensTest, NoUnblindedPaymentTokens) {
  // Arrange
  const URLEndpoints endpoints = {
      {R"(/v1/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)",
       {{net::HTTP_OK, R"(
            {
              "payload": "{\"paymentId\":\"27a39b2f-9b2e-4eb0-bbb2-2f84447496e7\"}",
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

  MockUrlRequest(ads_client_mock_, endpoints);

  const base::Time time = Now();
  ConfirmationsState::Get()->SetNextTokenRedemptionDate(time);

  // Act
  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRedeemUnblindedPaymentTokens(_))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnFailedToRedeemUnblindedPaymentTokens())
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidScheduleNextUnblindedPaymentTokensRedemption(_))
      .Times(1);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnWillRetryRedeemingUnblindedPaymentTokens())
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRetryRedeemingUnblindedPaymentTokens())
      .Times(0);

  const WalletInfo wallet = GetWallet();
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);

  FastForwardClockBy(NextPendingTaskDelay());

  // Assert
  EXPECT_EQ(0, get_unblinded_payment_tokens()->Count());
}

TEST_F(BatAdsRedeemUnblindedPaymentTokensTest, Retry) {
  // Arrange
  const URLEndpoints endpoints = {
      {R"(/v1/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)",
       {{net::HTTP_NOT_FOUND, ""}, {net::HTTP_OK, R"(
            {
              "payload": "{\"paymentId\":\"27a39b2f-9b2e-4eb0-bbb2-2f84447496e7\"}",
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

  MockUrlRequest(ads_client_mock_, endpoints);

  const base::Time time = Now();
  ConfirmationsState::Get()->SetNextTokenRedemptionDate(time);

  const privacy::UnblindedTokenList unblinded_tokens =
      privacy::GetUnblindedTokens(1);
  get_unblinded_payment_tokens()->SetTokens(unblinded_tokens);

  // Act
  InSequence seq;

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnFailedToRedeemUnblindedPaymentTokens())
      .Times(1);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnWillRetryRedeemingUnblindedPaymentTokens())
      .Times(1);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRetryRedeemingUnblindedPaymentTokens())
      .Times(1);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRedeemUnblindedPaymentTokens(_))
      .Times(1);

  EXPECT_CALL(*redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidScheduleNextUnblindedPaymentTokensRedemption(_))
      .Times(1);

  const WalletInfo wallet = GetWallet();
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);

  FastForwardClockBy(NextPendingTaskDelay());
  FastForwardClockBy(NextPendingTaskDelay());

  // Assert
  EXPECT_EQ(0, get_unblinded_payment_tokens()->Count());
}

}  // namespace ads
