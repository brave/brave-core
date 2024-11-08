/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_rewards/user_rewards.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_test_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/url_request/issuers_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/redeem_payment_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/url_request_builders/redeem_payment_tokens_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/refill_confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/get_signed_tokens/get_signed_tokens_url_request_builder_test_constants.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/get_signed_tokens/get_signed_tokens_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/request_signed_tokens/request_signed_tokens_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_test_constants.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_test_util.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_mock.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/profile_pref_value_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds

namespace brave_ads {

class BraveAdsUserRewardsTest : public AdsClientMock, public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    user_rewards_ = std::make_unique<UserRewards>(test::Wallet());
  }

  std::unique_ptr<UserRewards> user_rewards_;
};

TEST_F(BraveAdsUserRewardsTest, FetchIssuers) {
  // Arrange
  test::MockTokenGenerator(/*count=*/50);

  const test::URLResponseMap url_responses = {
      {BuildIssuersUrlPath(),
       {{net::HTTP_OK, test::BuildIssuersUrlResponseBody()}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  user_rewards_->FetchIssuers();

  // Assert
  EXPECT_TRUE(HasIssuers());
}

TEST_F(BraveAdsUserRewardsTest, DoNotFetchInvalidIssuers) {
  // Arrange
  test::MockTokenGenerator(/*count=*/50);

  const test::URLResponseMap url_responses = {
      {BuildIssuersUrlPath(), {{net::HTTP_OK, /*response_body=*/R"(
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
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  user_rewards_->FetchIssuers();

  // Assert
  EXPECT_FALSE(HasIssuers());
}

TEST_F(BraveAdsUserRewardsTest, DoNotFetchMissingIssuers) {
  // Arrange
  test::BuildAndSetIssuers();

  const test::URLResponseMap url_responses = {
      {BuildIssuersUrlPath(), {{net::HTTP_OK, /*response_body=*/R"(
          {
            "ping": 7200000,
            "issuers": []
          })"}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  user_rewards_->FetchIssuers();

  // Assert
  const IssuersInfo issuers = test::BuildIssuers();
  EXPECT_FALSE(HasIssuersChanged(issuers));
}

TEST_F(BraveAdsUserRewardsTest, RefillConfirmationTokens) {
  // Arrange
  test::BuildAndSetIssuers();

  test::MockTokenGenerator(/*count=*/50);

  const test::URLResponseMap url_responses = {
      {BuildRequestSignedTokensUrlPath(test::kWalletPaymentId),
       {{net::HTTP_CREATED, test::BuildRequestSignedTokensUrlResponseBody()}}},
      {BuildGetSignedTokensUrlPath(test::kWalletPaymentId,
                                   test::kRequestSignedTokensNonce),
       {{net::HTTP_OK, test::BuildGetSignedTokensUrlResponseBody()}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  user_rewards_->MaybeRefillConfirmationTokens();

  // Assert
  EXPECT_EQ(50U, ConfirmationTokenCount());
}

TEST_F(BraveAdsUserRewardsTest, RedeemPaymentTokens) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildRedeemPaymentTokensUrlPath(test::kWalletPaymentId),
       {{net::HTTP_OK, test::BuildRedeemPaymentTokensUrlResponseBody()}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  test::SetProfileTimePrefValue(prefs::kNextPaymentTokenRedemptionAt,
                                test::Now());

  test::SetPaymentTokens(/*count=*/1);

  // Act
  user_rewards_->MaybeRedeemPaymentTokens();
  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_TRUE(PaymentTokensIsEmpty());
}

TEST_F(BraveAdsUserRewardsTest,
       RequireCaptchaToRefillConfirmationTokensIfCaptchaIdExists) {
  // Arrange
  test::BuildAndSetIssuers();

  test::MockTokenGenerator(/*count=*/50);

  const test::URLResponseMap url_responses = {
      {BuildRequestSignedTokensUrlPath(test::kWalletPaymentId),
       {{net::HTTP_CREATED, test::BuildRequestSignedTokensUrlResponseBody()}}},
      {BuildGetSignedTokensUrlPath(test::kWalletPaymentId,
                                   test::kRequestSignedTokensNonce),
       {{net::HTTP_UNAUTHORIZED, /*response_body=*/R"(
            {
              "captcha_id": "daf85dc8-164e-4eb9-a4d4-1836055004b3"
            }
          )"}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  EXPECT_CALL(ads_client_mock_,
              ShowScheduledCaptcha(
                  test::kWalletPaymentId,
                  /*captcha_id=*/"daf85dc8-164e-4eb9-a4d4-1836055004b3"));

  // Act & Assert
  user_rewards_->MaybeRefillConfirmationTokens();
}

TEST_F(BraveAdsUserRewardsTest,
       DoNotRequireCaptchaToRefillConfirmationTokensIfCaptchaIdIsEmpty) {
  // Arrange
  test::BuildAndSetIssuers();

  test::MockTokenGenerator(/*count=*/50);

  const test::URLResponseMap url_responses = {
      {BuildRequestSignedTokensUrlPath(test::kWalletPaymentId),
       {{net::HTTP_CREATED, test::BuildRequestSignedTokensUrlResponseBody()}}},
      {BuildGetSignedTokensUrlPath(test::kWalletPaymentId,
                                   test::kRequestSignedTokensNonce),
       {{net::HTTP_UNAUTHORIZED, /*response_body=*/R"(
            {
              "captcha_id": ""
            }
          )"}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  EXPECT_CALL(ads_client_mock_, ShowScheduledCaptcha).Times(0);

  // Act & Assert
  user_rewards_->MaybeRefillConfirmationTokens();
}

TEST_F(BraveAdsUserRewardsTest,
       DoNotRequireCaptchaToRefillConfirmationTokensIfCaptchaIdDoesNotExist) {
  // Arrange
  test::BuildAndSetIssuers();

  test::MockTokenGenerator(/*count=*/50);

  const test::URLResponseMap url_responses = {
      {BuildRequestSignedTokensUrlPath(test::kWalletPaymentId),
       {{net::HTTP_CREATED, test::BuildRequestSignedTokensUrlResponseBody()}}},
      {BuildGetSignedTokensUrlPath(test::kWalletPaymentId,
                                   test::kRequestSignedTokensNonce),
       {{net::HTTP_OK, test::BuildGetSignedTokensUrlResponseBody()}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  EXPECT_CALL(ads_client_mock_, ShowScheduledCaptcha).Times(0);

  // Act & Assert
  user_rewards_->MaybeRefillConfirmationTokens();
}

}  // namespace brave_ads
