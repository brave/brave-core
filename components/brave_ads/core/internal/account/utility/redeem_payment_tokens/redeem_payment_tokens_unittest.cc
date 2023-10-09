/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/redeem_payment_tokens.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/redeem_payment_tokens_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/redeem_payment_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/url_request_builders/redeem_payment_tokens_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_profile_pref_value.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsRedeemPaymentTokensTest : public UnitTestBase {
 protected:
  BraveAdsRedeemPaymentTokensTest()
      : redeem_payment_tokens_(std::make_unique<RedeemPaymentTokens>()) {
    redeem_payment_tokens_->SetDelegate(&delegate_mock_);
  }

  std::unique_ptr<RedeemPaymentTokens> redeem_payment_tokens_;
  RedeemPaymentTokensDelegateMock delegate_mock_;
};

TEST_F(BraveAdsRedeemPaymentTokensTest, RedeemPaymentTokens) {
  // Arrange
  const URLResponseMap url_responses = {
      {BuildRedeemPaymentTokensUrlPath(
           /*payment_id=*/kWalletPaymentId),
       {{net::HTTP_OK, test::BuildRedeemPaymentTokensUrlResponseBody()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  SetProfileTimePrefValue(prefs::kNextTokenRedemptionAt, Now());

  test::SetPaymentTokens(/*count=*/1);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidRedeemPaymentTokens);

  EXPECT_CALL(delegate_mock_, OnFailedToRedeemPaymentTokens).Times(0);

  EXPECT_CALL(delegate_mock_, OnDidScheduleNextPaymentTokenRedemption);

  EXPECT_CALL(delegate_mock_, OnWillRetryRedeemingPaymentTokens).Times(0);

  EXPECT_CALL(delegate_mock_, OnDidRetryRedeemingPaymentTokens).Times(0);

  const WalletInfo wallet = test::GetWallet();
  redeem_payment_tokens_->MaybeRedeemAfterDelay(wallet);
  FastForwardClockToNextPendingTask();

  EXPECT_TRUE(PaymentTokensIsEmpty());
}

TEST_F(BraveAdsRedeemPaymentTokensTest, RedeemPaymentTokensMultipleTimes) {
  // Arrange
  const URLResponseMap url_responses = {
      {BuildRedeemPaymentTokensUrlPath(
           /*payment_id=*/kWalletPaymentId),
       {{net::HTTP_OK, test::BuildRedeemPaymentTokensUrlResponseBody()},
        {net::HTTP_OK, test::BuildRedeemPaymentTokensUrlResponseBody()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  SetProfileTimePrefValue(prefs::kNextTokenRedemptionAt, Now());

  const PaymentTokenList payment_tokens = test::SetPaymentTokens(/*count=*/1);

  EXPECT_CALL(delegate_mock_, OnDidRedeemPaymentTokens);
  EXPECT_CALL(delegate_mock_, OnFailedToRedeemPaymentTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnDidScheduleNextPaymentTokenRedemption);
  EXPECT_CALL(delegate_mock_, OnWillRetryRedeemingPaymentTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnDidRetryRedeemingPaymentTokens).Times(0);
  const WalletInfo wallet = test::GetWallet();
  redeem_payment_tokens_->MaybeRedeemAfterDelay(wallet);
  FastForwardClockToNextPendingTask();

  test::GetPaymentTokens().SetTokens(payment_tokens);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidRedeemPaymentTokens);
  EXPECT_CALL(delegate_mock_, OnDidScheduleNextPaymentTokenRedemption);
  FastForwardClockToNextPendingTask();

  EXPECT_EQ(1U, GetPendingTaskCount());
}

TEST_F(BraveAdsRedeemPaymentTokensTest, ScheduleNextTokenRedemption) {
  // Arrange
  const URLResponseMap url_responses = {
      {BuildRedeemPaymentTokensUrlPath(
           /*payment_id=*/kWalletPaymentId),
       {{net::HTTP_OK, test::BuildRedeemPaymentTokensUrlResponseBody()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  SetProfileTimePrefValue(prefs::kNextTokenRedemptionAt, Now());

  test::SetPaymentTokens(/*count=*/1);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidRedeemPaymentTokens);
  EXPECT_CALL(delegate_mock_, OnFailedToRedeemPaymentTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnDidScheduleNextPaymentTokenRedemption);
  EXPECT_CALL(delegate_mock_, OnWillRetryRedeemingPaymentTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnDidRetryRedeemingPaymentTokens).Times(0);
  const WalletInfo wallet = test::GetWallet();
  redeem_payment_tokens_->MaybeRedeemAfterDelay(wallet);
  FastForwardClockToNextPendingTask();

  EXPECT_TRUE(PaymentTokensIsEmpty());
}

TEST_F(BraveAdsRedeemPaymentTokensTest, NoPaymentTokens) {
  // Arrange
  SetProfileTimePrefValue(prefs::kNextTokenRedemptionAt, Now());

  // Act & Assert
  EXPECT_CALL(ads_client_mock_, UrlRequest).Times(0);
  EXPECT_CALL(delegate_mock_, OnDidRedeemPaymentTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnFailedToRedeemPaymentTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnDidScheduleNextPaymentTokenRedemption);
  EXPECT_CALL(delegate_mock_, OnWillRetryRedeemingPaymentTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnDidRetryRedeemingPaymentTokens).Times(0);
  const WalletInfo wallet = test::GetWallet();
  redeem_payment_tokens_->MaybeRedeemAfterDelay(wallet);
  FastForwardClockToNextPendingTask();

  EXPECT_TRUE(PaymentTokensIsEmpty());
}

TEST_F(BraveAdsRedeemPaymentTokensTest, Retry) {
  // Arrange
  const URLResponseMap url_responses = {
      {BuildRedeemPaymentTokensUrlPath(
           /*payment_id=*/kWalletPaymentId),
       {{net::HTTP_NOT_FOUND,
         /*response_body=*/net::GetHttpReasonPhrase(net::HTTP_NOT_FOUND)},
        {net::HTTP_OK, test::BuildRedeemPaymentTokensUrlResponseBody()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  SetProfileTimePrefValue(prefs::kNextTokenRedemptionAt, Now());

  test::SetPaymentTokens(/*count=*/1);

  // Act & Assert
  const ::testing::InSequence s;
  EXPECT_CALL(delegate_mock_, OnFailedToRedeemPaymentTokens);
  EXPECT_CALL(delegate_mock_, OnWillRetryRedeemingPaymentTokens);
  EXPECT_CALL(delegate_mock_, OnDidRetryRedeemingPaymentTokens);
  EXPECT_CALL(delegate_mock_, OnDidRedeemPaymentTokens);
  EXPECT_CALL(delegate_mock_, OnDidScheduleNextPaymentTokenRedemption);
  const WalletInfo wallet = test::GetWallet();
  redeem_payment_tokens_->MaybeRedeemAfterDelay(wallet);
  FastForwardClockToNextPendingTask();
  FastForwardClockToNextPendingTask();

  EXPECT_TRUE(PaymentTokensIsEmpty());
}

}  // namespace brave_ads
