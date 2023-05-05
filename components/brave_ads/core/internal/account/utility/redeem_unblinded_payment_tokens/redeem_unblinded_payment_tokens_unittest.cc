/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens.h"

#include <memory>

#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;

class BraveAdsRedeemUnblindedPaymentTokensTest : public UnitTestBase {
 protected:
  BraveAdsRedeemUnblindedPaymentTokensTest()
      : redeem_unblinded_payment_tokens_(
            std::make_unique<RedeemUnblindedPaymentTokens>()) {
    redeem_unblinded_payment_tokens_->SetDelegate(
        &redeem_unblinded_payment_tokens_delegate_mock_);
  }

  std::unique_ptr<RedeemUnblindedPaymentTokens>
      redeem_unblinded_payment_tokens_;
  NiceMock<RedeemUnblindedPaymentTokensDelegateMock>
      redeem_unblinded_payment_tokens_delegate_mock_;
};

TEST_F(BraveAdsRedeemUnblindedPaymentTokensTest, RedeemUnblindedPaymentTokens) {
  // Arrange
  const URLResponseMap url_responses = {
      {BuildRedeemUnblindedPaymentTokensUrlPath(
           /*payment_id*/ kWalletPaymentId),
       {{net::HTTP_OK, BuildRedeemUnblindedPaymentTokensUrlResponseBody()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  ads_client_mock_.SetTimePref(prefs::kNextTokenRedemptionAt, Now());

  privacy::SetUnblindedPaymentTokens(/*count*/ 1);

  // Act
  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRedeemUnblindedPaymentTokens(_));

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnFailedToRedeemUnblindedPaymentTokens())
      .Times(0);

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidScheduleNextUnblindedPaymentTokensRedemption(_));

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnWillRetryRedeemingUnblindedPaymentTokens(_))
      .Times(0);

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRetryRedeemingUnblindedPaymentTokens())
      .Times(0);

  const WalletInfo wallet = GetWalletForTesting();
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);

  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_TRUE(privacy::UnblindedPaymentTokensIsEmpty());
}

TEST_F(BraveAdsRedeemUnblindedPaymentTokensTest,
       RedeemUnblindedPaymentTokensMultipleTimes) {
  // Arrange
  const URLResponseMap url_responses = {
      {BuildRedeemUnblindedPaymentTokensUrlPath(
           /*payment_id*/ kWalletPaymentId),
       {{net::HTTP_OK, BuildRedeemUnblindedPaymentTokensUrlResponseBody()},
        {net::HTTP_OK, BuildRedeemUnblindedPaymentTokensUrlResponseBody()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  ads_client_mock_.SetTimePref(prefs::kNextTokenRedemptionAt, Now());

  const privacy::UnblindedPaymentTokenList unblinded_payment_tokens =
      privacy::SetUnblindedPaymentTokens(/*count*/ 1);

  // Act
  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRedeemUnblindedPaymentTokens(_))
      .Times(2);

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnFailedToRedeemUnblindedPaymentTokens())
      .Times(0);

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidScheduleNextUnblindedPaymentTokensRedemption(_))
      .Times(2);

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnWillRetryRedeemingUnblindedPaymentTokens(_))
      .Times(0);

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRetryRedeemingUnblindedPaymentTokens())
      .Times(0);

  const WalletInfo wallet = GetWalletForTesting();
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);

  FastForwardClockToNextPendingTask();

  privacy::GetUnblindedPaymentTokens().SetTokens(unblinded_payment_tokens);

  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_EQ(1U, GetPendingTaskCount());
}

TEST_F(BraveAdsRedeemUnblindedPaymentTokensTest, ScheduleNextTokenRedemption) {
  // Arrange
  const URLResponseMap url_responses = {
      {BuildRedeemUnblindedPaymentTokensUrlPath(
           /*payment_id*/ kWalletPaymentId),
       {{net::HTTP_OK, BuildRedeemUnblindedPaymentTokensUrlResponseBody()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  ads_client_mock_.SetTimePref(prefs::kNextTokenRedemptionAt, Now());

  privacy::SetUnblindedPaymentTokens(/*count*/ 1);

  // Act
  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRedeemUnblindedPaymentTokens(_));

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnFailedToRedeemUnblindedPaymentTokens())
      .Times(0);

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidScheduleNextUnblindedPaymentTokensRedemption(_));

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnWillRetryRedeemingUnblindedPaymentTokens(_))
      .Times(0);

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRetryRedeemingUnblindedPaymentTokens())
      .Times(0);

  const WalletInfo wallet = GetWalletForTesting();
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);

  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_TRUE(privacy::UnblindedPaymentTokensIsEmpty());
}

TEST_F(BraveAdsRedeemUnblindedPaymentTokensTest, InvalidWallet) {
  // Arrange
  ads_client_mock_.SetTimePref(prefs::kNextTokenRedemptionAt, Now());

  privacy::SetUnblindedPaymentTokens(/*count*/ 1);

  // Act
  const InSequence seq;

  EXPECT_CALL(ads_client_mock_, UrlRequest(_, _)).Times(0);

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnFailedToRedeemUnblindedPaymentTokens());

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnWillRetryRedeemingUnblindedPaymentTokens(_))
      .Times(0);

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRetryRedeemingUnblindedPaymentTokens())
      .Times(0);

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRedeemUnblindedPaymentTokens(_))
      .Times(0);

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidScheduleNextUnblindedPaymentTokensRedemption(_))
      .Times(0);

  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(/*wallet*/ {});

  // Assert
  EXPECT_EQ(1U, privacy::GetUnblindedPaymentTokens().Count());
}

TEST_F(BraveAdsRedeemUnblindedPaymentTokensTest, NoUnblindedPaymentTokens) {
  // Arrange
  ads_client_mock_.SetTimePref(prefs::kNextTokenRedemptionAt, Now());

  // Act
  EXPECT_CALL(ads_client_mock_, UrlRequest(_, _)).Times(0);

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRedeemUnblindedPaymentTokens(_))
      .Times(0);

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnFailedToRedeemUnblindedPaymentTokens())
      .Times(0);

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidScheduleNextUnblindedPaymentTokensRedemption(_));

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnWillRetryRedeemingUnblindedPaymentTokens(_))
      .Times(0);

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRetryRedeemingUnblindedPaymentTokens())
      .Times(0);

  const WalletInfo wallet = GetWalletForTesting();
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);

  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_TRUE(privacy::UnblindedPaymentTokensIsEmpty());
}

TEST_F(BraveAdsRedeemUnblindedPaymentTokensTest, Retry) {
  // Arrange
  const URLResponseMap url_responses = {
      {BuildRedeemUnblindedPaymentTokensUrlPath(
           /*payment_id*/ kWalletPaymentId),
       {{net::HTTP_NOT_FOUND,
         /*response_body*/ net::GetHttpReasonPhrase(net::HTTP_NOT_FOUND)},
        {net::HTTP_OK, BuildRedeemUnblindedPaymentTokensUrlResponseBody()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  ads_client_mock_.SetTimePref(prefs::kNextTokenRedemptionAt, Now());

  privacy::SetUnblindedPaymentTokens(/*count*/ 1);

  // Act
  const InSequence seq;

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnFailedToRedeemUnblindedPaymentTokens());

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnWillRetryRedeemingUnblindedPaymentTokens(_));

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRetryRedeemingUnblindedPaymentTokens());

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidRedeemUnblindedPaymentTokens(_));

  EXPECT_CALL(redeem_unblinded_payment_tokens_delegate_mock_,
              OnDidScheduleNextUnblindedPaymentTokensRedemption(_));

  const WalletInfo wallet = GetWalletForTesting();
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);

  FastForwardClockToNextPendingTask();
  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_TRUE(privacy::UnblindedPaymentTokensIsEmpty());
}

}  // namespace brave_ads
