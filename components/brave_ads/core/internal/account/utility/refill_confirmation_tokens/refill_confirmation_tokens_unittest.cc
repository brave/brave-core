/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/refill_confirmation_tokens.h"

#include <string>

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/test/issuers_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/test/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/test/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/test/refill_confirmation_tokens_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/test/refill_confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/get_signed_tokens/get_signed_tokens_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/get_signed_tokens/test/get_signed_tokens_url_request_builder_test_constants.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/request_signed_tokens/request_signed_tokens_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/wallet/test/wallet_test_constants.h"
#include "brave/components/brave_ads/core/internal/account/wallet/test/wallet_test_util.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/test_constants.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsRefillConfirmationTokensTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    refill_confirmation_tokens_.SetDelegate(&delegate_mock_);
  }

  RefillConfirmationTokens refill_confirmation_tokens_;
  RefillConfirmationTokensDelegateMock delegate_mock_;
};

TEST_F(BraveAdsRefillConfirmationTokensTest, RefillConfirmationTokens) {
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

  const WalletInfo wallet = test::Wallet();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidRefillConfirmationTokens);
  EXPECT_CALL(delegate_mock_, OnFailedToRefillConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnWillRetryRefillingConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnDidRetryRefillingConfirmationTokens).Times(0);
  refill_confirmation_tokens_.MaybeRefill(wallet);

  EXPECT_EQ(50U, ConfirmationTokenCount());
}

TEST_F(BraveAdsRefillConfirmationTokensTest,
       DoNotRefillConfirmationTokensIfCaptchaIsRequired) {
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

  const WalletInfo wallet = test::Wallet();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidRefillConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnFailedToRefillConfirmationTokens);
  EXPECT_CALL(delegate_mock_, OnWillRetryRefillingConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnDidRetryRefillingConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_,
              OnCaptchaRequiredToRefillConfirmationTokens(
                  /*captcha_id=*/"daf85dc8-164e-4eb9-a4d4-1836055004b3"));
  refill_confirmation_tokens_.MaybeRefill(wallet);

  EXPECT_EQ(0U, ConfirmationTokenCount());
}

TEST_F(BraveAdsRefillConfirmationTokensTest,
       DoNotRefillConfirmationTokensIfMismatchedIssuersPublicKey) {
  // Arrange
  test::MockTokenGenerator(/*count=*/50);

  const test::URLResponseMap url_responses = {
      {BuildRequestSignedTokensUrlPath(test::kWalletPaymentId),
       {{net::HTTP_CREATED, test::BuildRequestSignedTokensUrlResponseBody()}}},
      {BuildGetSignedTokensUrlPath(test::kWalletPaymentId,
                                   test::kRequestSignedTokensNonce),
       {{net::HTTP_OK, test::BuildGetSignedTokensUrlResponseBody()}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  const IssuersInfo issuers = test::BuildIssuers(
      /*ping=*/7'200'000,
      /*confirmation_token_issuer_public_keys=*/
      {{"qiy6l/i2WXc8AkDFt/QDxXoT0XKgL0xRSZ3Db+k2u3A=", 0.0},
       {"hKjGQd7WAXs0lcdf+SCHCTKsBLWtKaEubwlK4YA1NkA=", 0.0}},
      /*payment_token_issuer_public_keys=*/
      {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
       {"OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM=", 0.1}});

  SetIssuers(issuers);

  const WalletInfo wallet = test::Wallet();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidRefillConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnFailedToRefillConfirmationTokens);
  EXPECT_CALL(delegate_mock_, OnWillRetryRefillingConfirmationTokens);
  EXPECT_CALL(delegate_mock_, OnDidRetryRefillingConfirmationTokens).Times(0);
  refill_confirmation_tokens_.MaybeRefill(wallet);
}

TEST_F(BraveAdsRefillConfirmationTokensTest,
       RetryRequestSignedTokensAfterInternalServerError) {
  // Arrange
  test::BuildAndSetIssuers();

  test::MockTokenGenerator(/*count=*/50);

  const test::URLResponseMap url_responses = {
      {BuildRequestSignedTokensUrlPath(test::kWalletPaymentId),
       {{net::HTTP_INTERNAL_SERVER_ERROR,
         /*response_body=*/std::string(
             net::GetHttpReasonPhrase(net::HTTP_INTERNAL_SERVER_ERROR))},
        {net::HTTP_CREATED, test::BuildRequestSignedTokensUrlResponseBody()}}},
      {BuildGetSignedTokensUrlPath(test::kWalletPaymentId,
                                   test::kRequestSignedTokensNonce),
       {{net::HTTP_OK, test::BuildGetSignedTokensUrlResponseBody()}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  const WalletInfo wallet = test::Wallet();

  // Act & Assert
  const ::testing::InSequence s;
  EXPECT_CALL(delegate_mock_, OnFailedToRefillConfirmationTokens);
  EXPECT_CALL(delegate_mock_, OnWillRetryRefillingConfirmationTokens);
  EXPECT_CALL(delegate_mock_, OnDidRetryRefillingConfirmationTokens);
  EXPECT_CALL(delegate_mock_, OnDidRefillConfirmationTokens);
  refill_confirmation_tokens_.MaybeRefill(wallet);
  FastForwardClockToNextPendingTask();

  EXPECT_EQ(50U, ConfirmationTokenCount());
}

TEST_F(BraveAdsRefillConfirmationTokensTest,
       DoNotRefillConfirmationTokensIfRequestSignedTokensIsMissingNonce) {
  // Arrange
  test::BuildAndSetIssuers();

  test::MockTokenGenerator(/*count=*/50);

  const test::URLResponseMap url_responses = {
      {BuildRequestSignedTokensUrlPath(test::kWalletPaymentId),
       {{net::HTTP_CREATED, /*response_body=*/"{}"}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  const WalletInfo wallet = test::Wallet();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidRefillConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnFailedToRefillConfirmationTokens);
  EXPECT_CALL(delegate_mock_, OnWillRetryRefillingConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnDidRetryRefillingConfirmationTokens).Times(0);
  refill_confirmation_tokens_.MaybeRefill(wallet);

  EXPECT_EQ(0U, ConfirmationTokenCount());
}

TEST_F(BraveAdsRefillConfirmationTokensTest,
       RetryGetSignedTokensAfterInternalServerError) {
  // Arrange
  test::BuildAndSetIssuers();

  test::MockTokenGenerator(/*count=*/50);

  const test::URLResponseMap url_responses = {
      {BuildRequestSignedTokensUrlPath(test::kWalletPaymentId),
       {{net::HTTP_CREATED, test::BuildRequestSignedTokensUrlResponseBody()},
        {net::HTTP_CREATED, test::BuildRequestSignedTokensUrlResponseBody()}}},
      {BuildGetSignedTokensUrlPath(test::kWalletPaymentId,
                                   test::kRequestSignedTokensNonce),
       {{net::HTTP_INTERNAL_SERVER_ERROR,
         /*response_body=*/std::string(
             net::GetHttpReasonPhrase(net::HTTP_INTERNAL_SERVER_ERROR))},
        {net::HTTP_OK, test::BuildGetSignedTokensUrlResponseBody()}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  const WalletInfo wallet = test::Wallet();

  // Act & Assert
  const ::testing::InSequence s;
  EXPECT_CALL(delegate_mock_, OnFailedToRefillConfirmationTokens);
  EXPECT_CALL(delegate_mock_, OnWillRetryRefillingConfirmationTokens);
  EXPECT_CALL(delegate_mock_, OnDidRetryRefillingConfirmationTokens);
  EXPECT_CALL(delegate_mock_, OnDidRefillConfirmationTokens);
  refill_confirmation_tokens_.MaybeRefill(wallet);
  FastForwardClockToNextPendingTask();

  EXPECT_EQ(50U, ConfirmationTokenCount());
}

TEST_F(BraveAdsRefillConfirmationTokensTest,
       DoNotRefillConfirmationTokensIfGetSignedTokensReturnsAnInvalidResponse) {
  // Arrange
  test::BuildAndSetIssuers();

  test::MockTokenGenerator(/*count=*/50);

  const test::URLResponseMap url_responses = {
      {BuildRequestSignedTokensUrlPath(test::kWalletPaymentId),
       {{net::HTTP_CREATED, test::BuildRequestSignedTokensUrlResponseBody()}}},
      {BuildGetSignedTokensUrlPath(test::kWalletPaymentId,
                                   test::kRequestSignedTokensNonce),
       {{net::HTTP_OK, /*response_body=*/test::kMalformedJson}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  const WalletInfo wallet = test::Wallet();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidRefillConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnFailedToRefillConfirmationTokens);
  EXPECT_CALL(delegate_mock_, OnWillRetryRefillingConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnDidRetryRefillingConfirmationTokens).Times(0);
  refill_confirmation_tokens_.MaybeRefill(wallet);

  EXPECT_EQ(0U, ConfirmationTokenCount());
}

TEST_F(BraveAdsRefillConfirmationTokensTest,
       DoNotRefillConfirmationTokensIfGetSignedTokensIsMissingPublicKey) {
  // Arrange
  test::BuildAndSetIssuers();

  test::MockTokenGenerator(/*count=*/50);

  const test::URLResponseMap url_responses = {
      {BuildRequestSignedTokensUrlPath(test::kWalletPaymentId),
       {{net::HTTP_CREATED, test::BuildRequestSignedTokensUrlResponseBody()}}},
      {BuildGetSignedTokensUrlPath(test::kWalletPaymentId,
                                   test::kRequestSignedTokensNonce),
       {{net::HTTP_OK, /*response_body=*/R"(
            {
              "batchProof": "flsRyY/zcE6V1ymow+hJW9DdVjGlEAGyVr3QN7kc1Qf6eeYOSK93thF+IZehEXrsRFjFoV3reEm60a/pEq7XAA==",
              "signedTokens": [
                "2g0WjgYZfADeoAYI0kkXNVCcXCpfg5lv5yRdCHigkGs=",
                "XuBEdiNS8cOMKApsIiHygcnVOVssvaoudyd1wMI+O1Y=",
                "sr9ISUbtRozfF8IXslvveZSCE/NIZzkHr4WkcizZims=",
                "jDpLjMtRm6OjPwMmaH3IUP9zuYHvn1PkUamYrtuaCRs=",
                "5u947b1ODsUaTT1tU1Vh1qN24HSAaaCt0MapVNIiM2Y=",
                "XD25BcXUS4YJEGQdedCrli6wnufmFdKQmbAkKCJMnBo=",
                "QKcG8fsNPwWzmlZUZbfww0A+iBS8FM+a76pz/R4b2HQ=",
                "zFD7PmAnw/CP5Tq1qxw8iSbrjBH036al2dvb1fRZz0o=",
                "qMm/EqdSREFbFD4i6ezy+evFaGYp7Untq0iULvm4mwU=",
                "sjJQ9T5rFJcw1+ZlyeOZHY7Kmmlv9Po3FaBcZ1hL1hI=",
                "cOTDHJEx2ZLBZKk8W56KJq3fgsSj5o0mRaVCYQZXkxo=",
                "0CIrR2Ng3oOlJyOKtMwGSnfTXJwXjh5E/VBwkBLAbhg=",
                "IrcpnN7p4xwGbGGpqCMRh1G1c2ujilXuYfuYKIbxahc=",
                "HhtASybmIMS+HwgKsbvzUeM5TnLsYReixf8WnTCIvx0=",
                "9FpVkri+eN+D7sXzVjwObjRSx6ADewQ66SEuFCRlRx8=",
                "aJ7DnFP13RXo7GItmAvKPH5CvY2dITtc149PAr5WtBw=",
                "8DcOKuGIlMx00d+g4ixlz4VMxjKRQ2CJEyyf/fwEDTk=",
                "BPGAp/+/RFBTwm2Lx7+MxyYgkvzRdJqkWBw4TSRhPlE=",
                "RirDth6yyg9Zq0MCoiRiibFvaMVaFyyiQC/SDJZemiI=",
                "pImA5vt8XlSd7xWc0DcheI1pnIBRIiMKltplBn8vI0o=",
                "tCEMf+D18pRR6rWzWdHDyQlAjZ6/465GnUg6pXlcYkM=",
                "nJRbR/uZQTIKLQF6Yhxx/bywQ6SvjP7YJJz5a1k69xc=",
                "cLKRj8+1KR0lk2w1uUysR5Go87WgLfiJ+9NXD91AHl4=",
                "WPvZHt4qITOFMitv/NTXOx6M+GUf3H5ud+uuCLacrgs=",
                "XHCtobEsiESgGDgqCn4I6FmrU9vnEhXRNrzw5O0bdl0=",
                "yIDV8B7/fv1t8RcNLm93N0siuMX5AXAll5QHaOyAxj0=",
                "7pBWOhSj1cl2DlsCHC3/Dapd6bknZ+urewK6mtOKFlg=",
                "KhNVrYoxTyth9xDMSXLAyO1MyObuYQh1NyFeMshBUA4=",
                "xGqTbNWAJREpd2lKO3/pqqBmazx28VEtSwpdHJpqZzE=",
                "hkLhf4xPTDClmUoYrQZCXnP5jILU097Z5mULuez9/ns=",
                "IgJBYMaNLixag1yoXg6lNLuMjNoNE03m3456d3ekKE8=",
                "Rj1BEQ5Zb8OJAOhIFUkHatzcZPkiD71BQdLU7VQV2HA=",
                "yOKGTPFF3f64H1IVpUCs/I9uw/ZYQGlOsMnmLiieplg=",
                "4D9T1jXnerXvKC3hfERLC462Bg2fXDon+SPgyB59oBw=",
                "xLeSjxjpLs2g5nppceuPir5LaLdvU/ZwrQ6j9tgEklc=",
                "qGHBPL0PEyK3S190hPOJnMU8ptUmnfY8T81oEtoH4F0=",
                "XpO85gRmr7HKoAmLPqx+Ne2KWS9ewqclj9l865cnokc=",
                "yotMG+lPwhKxJrSDn0XXVF7+50m/WyjJmUszCEs1BhE=",
                "BNz99P06znT/ABsf0M3ga6AAPPIGoxWGhURRIMPqdmg=",
                "QP+g04U4LkLADAomyQui7+31al/UwEhsq9goa26ndm4=",
                "tmV4MEvcoaoY1LJvehD/57duhcbK3Y9+ULfuZGzHLWw=",
                "kjeRJnyBLsdetp8pR5uDxn/to4i4Y8XwXqTWmbEhsWo=",
                "IjlKYdnbqNacAzV8BUR+REHmj8V2EOcurU9afpATF1g=",
                "xJcKsn/r/mBJHOrHwKAlvdYIMZKvZcB4N7OtkCd4q3w=",
                "uDbbJC4fqBitdgvGbn2M1ksMXG7XxO4XEpzWqR9VIhw=",
                "MCmJPVJMzXTVl+7bR57iH+CRUWMrUUjcBJ6P8LdD0wc=",
                "4vfcsB7coDvhHP+HR71AJ5jYTAhuiWOdQN207dVEqAc=",
                "ZvXt0CyDwdHXN2p3t9xrXkb9Y6yjSRBC5L5EwIWlwGs=",
                "imsSQ9dFxDIoWexM877Bx19Elo4qFA2Vgds761PbMEQ=",
                "XsxhvO3k44prGYyJydyi2VSXqrWEX9AHREy8YfMyhEA="
              ]
            })"}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  const WalletInfo wallet = test::Wallet();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidRefillConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnFailedToRefillConfirmationTokens);
  EXPECT_CALL(delegate_mock_, OnWillRetryRefillingConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnDidRetryRefillingConfirmationTokens).Times(0);
  refill_confirmation_tokens_.MaybeRefill(wallet);

  EXPECT_EQ(0U, ConfirmationTokenCount());
}

TEST_F(BraveAdsRefillConfirmationTokensTest,
       DoNotRefillConfirmationTokensIfGetSignedTokensIsMissingBatchProofDleq) {
  // Arrange
  test::BuildAndSetIssuers();

  test::MockTokenGenerator(/*count=*/50);

  const test::URLResponseMap url_responses = {
      {BuildRequestSignedTokensUrlPath(test::kWalletPaymentId),
       {{net::HTTP_CREATED, test::BuildRequestSignedTokensUrlResponseBody()}}},
      {BuildGetSignedTokensUrlPath(test::kWalletPaymentId,
                                   test::kRequestSignedTokensNonce),
       {{net::HTTP_OK, /*response_body=*/R"(
            {
              "signedTokens": [
                "2g0WjgYZfADeoAYI0kkXNVCcXCpfg5lv5yRdCHigkGs=",
                "XuBEdiNS8cOMKApsIiHygcnVOVssvaoudyd1wMI+O1Y=",
                "sr9ISUbtRozfF8IXslvveZSCE/NIZzkHr4WkcizZims=",
                "jDpLjMtRm6OjPwMmaH3IUP9zuYHvn1PkUamYrtuaCRs=",
                "5u947b1ODsUaTT1tU1Vh1qN24HSAaaCt0MapVNIiM2Y=",
                "XD25BcXUS4YJEGQdedCrli6wnufmFdKQmbAkKCJMnBo=",
                "QKcG8fsNPwWzmlZUZbfww0A+iBS8FM+a76pz/R4b2HQ=",
                "zFD7PmAnw/CP5Tq1qxw8iSbrjBH036al2dvb1fRZz0o=",
                "qMm/EqdSREFbFD4i6ezy+evFaGYp7Untq0iULvm4mwU=",
                "sjJQ9T5rFJcw1+ZlyeOZHY7Kmmlv9Po3FaBcZ1hL1hI=",
                "cOTDHJEx2ZLBZKk8W56KJq3fgsSj5o0mRaVCYQZXkxo=",
                "0CIrR2Ng3oOlJyOKtMwGSnfTXJwXjh5E/VBwkBLAbhg=",
                "IrcpnN7p4xwGbGGpqCMRh1G1c2ujilXuYfuYKIbxahc=",
                "HhtASybmIMS+HwgKsbvzUeM5TnLsYReixf8WnTCIvx0=",
                "9FpVkri+eN+D7sXzVjwObjRSx6ADewQ66SEuFCRlRx8=",
                "aJ7DnFP13RXo7GItmAvKPH5CvY2dITtc149PAr5WtBw=",
                "8DcOKuGIlMx00d+g4ixlz4VMxjKRQ2CJEyyf/fwEDTk=",
                "BPGAp/+/RFBTwm2Lx7+MxyYgkvzRdJqkWBw4TSRhPlE=",
                "RirDth6yyg9Zq0MCoiRiibFvaMVaFyyiQC/SDJZemiI=",
                "pImA5vt8XlSd7xWc0DcheI1pnIBRIiMKltplBn8vI0o=",
                "tCEMf+D18pRR6rWzWdHDyQlAjZ6/465GnUg6pXlcYkM=",
                "nJRbR/uZQTIKLQF6Yhxx/bywQ6SvjP7YJJz5a1k69xc=",
                "cLKRj8+1KR0lk2w1uUysR5Go87WgLfiJ+9NXD91AHl4=",
                "WPvZHt4qITOFMitv/NTXOx6M+GUf3H5ud+uuCLacrgs=",
                "XHCtobEsiESgGDgqCn4I6FmrU9vnEhXRNrzw5O0bdl0=",
                "yIDV8B7/fv1t8RcNLm93N0siuMX5AXAll5QHaOyAxj0=",
                "7pBWOhSj1cl2DlsCHC3/Dapd6bknZ+urewK6mtOKFlg=",
                "KhNVrYoxTyth9xDMSXLAyO1MyObuYQh1NyFeMshBUA4=",
                "xGqTbNWAJREpd2lKO3/pqqBmazx28VEtSwpdHJpqZzE=",
                "hkLhf4xPTDClmUoYrQZCXnP5jILU097Z5mULuez9/ns=",
                "IgJBYMaNLixag1yoXg6lNLuMjNoNE03m3456d3ekKE8=",
                "Rj1BEQ5Zb8OJAOhIFUkHatzcZPkiD71BQdLU7VQV2HA=",
                "yOKGTPFF3f64H1IVpUCs/I9uw/ZYQGlOsMnmLiieplg=",
                "4D9T1jXnerXvKC3hfERLC462Bg2fXDon+SPgyB59oBw=",
                "xLeSjxjpLs2g5nppceuPir5LaLdvU/ZwrQ6j9tgEklc=",
                "qGHBPL0PEyK3S190hPOJnMU8ptUmnfY8T81oEtoH4F0=",
                "XpO85gRmr7HKoAmLPqx+Ne2KWS9ewqclj9l865cnokc=",
                "yotMG+lPwhKxJrSDn0XXVF7+50m/WyjJmUszCEs1BhE=",
                "BNz99P06znT/ABsf0M3ga6AAPPIGoxWGhURRIMPqdmg=",
                "QP+g04U4LkLADAomyQui7+31al/UwEhsq9goa26ndm4=",
                "tmV4MEvcoaoY1LJvehD/57duhcbK3Y9+ULfuZGzHLWw=",
                "kjeRJnyBLsdetp8pR5uDxn/to4i4Y8XwXqTWmbEhsWo=",
                "IjlKYdnbqNacAzV8BUR+REHmj8V2EOcurU9afpATF1g=",
                "xJcKsn/r/mBJHOrHwKAlvdYIMZKvZcB4N7OtkCd4q3w=",
                "uDbbJC4fqBitdgvGbn2M1ksMXG7XxO4XEpzWqR9VIhw=",
                "MCmJPVJMzXTVl+7bR57iH+CRUWMrUUjcBJ6P8LdD0wc=",
                "4vfcsB7coDvhHP+HR71AJ5jYTAhuiWOdQN207dVEqAc=",
                "ZvXt0CyDwdHXN2p3t9xrXkb9Y6yjSRBC5L5EwIWlwGs=",
                "imsSQ9dFxDIoWexM877Bx19Elo4qFA2Vgds761PbMEQ=",
                "XsxhvO3k44prGYyJydyi2VSXqrWEX9AHREy8YfMyhEA="
              ],
              "publicKey": "OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM="
            })"}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  const WalletInfo wallet = test::Wallet();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidRefillConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnFailedToRefillConfirmationTokens);
  EXPECT_CALL(delegate_mock_, OnWillRetryRefillingConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnDidRetryRefillingConfirmationTokens).Times(0);
  refill_confirmation_tokens_.MaybeRefill(wallet);

  EXPECT_EQ(0U, ConfirmationTokenCount());
}

TEST_F(BraveAdsRefillConfirmationTokensTest,
       DoNotRefillConfirmationTokensIfGetSignedTokensIsMissingSignedTokens) {
  // Arrange
  test::BuildAndSetIssuers();

  test::MockTokenGenerator(/*count=*/50);

  const test::URLResponseMap url_responses = {
      {BuildRequestSignedTokensUrlPath(test::kWalletPaymentId),
       {{net::HTTP_CREATED, test::BuildRequestSignedTokensUrlResponseBody()}}},
      {BuildGetSignedTokensUrlPath(test::kWalletPaymentId,
                                   test::kRequestSignedTokensNonce),
       {{net::HTTP_OK, /*response_body=*/R"(
            {
              "batchProof": "flsRyY/zcE6V1ymow+hJW9DdVjGlEAGyVr3QN7kc1Qf6eeYOSK93thF+IZehEXrsRFjFoV3reEm60a/pEq7XAA==",
              "publicKey": "QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g="
            })"}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  const WalletInfo wallet = test::Wallet();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidRefillConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnFailedToRefillConfirmationTokens);
  EXPECT_CALL(delegate_mock_, OnWillRetryRefillingConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnDidRetryRefillingConfirmationTokens).Times(0);
  refill_confirmation_tokens_.MaybeRefill(wallet);

  EXPECT_EQ(0U, ConfirmationTokenCount());
}

TEST_F(BraveAdsRefillConfirmationTokensTest,
       DoNotRefillConfirmationTokensIfGetSignedTokensAreInvalid) {
  // Arrange
  test::BuildAndSetIssuers();

  test::MockTokenGenerator(/*count=*/50);

  const test::URLResponseMap url_responses = {
      {BuildRequestSignedTokensUrlPath(test::kWalletPaymentId),
       {{net::HTTP_CREATED, test::BuildRequestSignedTokensUrlResponseBody()}}},
      {BuildGetSignedTokensUrlPath(test::kWalletPaymentId,
                                   test::kRequestSignedTokensNonce),
       {{net::HTTP_OK, /*response_body=*/R"(
            {
            {
              "batchProof": "flsRyY/zcE6V1ymow+hJW9DdVjGlEAGyVr3QN7kc1Qf6eeYOSK93thF+IZehEXrsRFjFoV3reEm60a/pEq7XAA==",
              "signedTokens": [
                "gD5YfqudgGrfn+oHpwPsF7COcPrCTLsYX70wa+EE+gg=",
                "OOPCQu4K+hfE7YaYnI4SyNI1KTIfNR71rIuZKs/9rE8=",
                "4kCHwIqcMuptlWqHNqGVpSBB5og8h5ooIQkno+qV0j4=",
                "/lNHOB5ISVVNvoTkS0n4PhDynjYJxKYwXnaDVfzmGSI=",
                "+ADYC6BAjtbrULLhXoBJM6mK7RPAyYUBA37Dfz223A8=",
                "ipBrQYPynDtfMVH4COUqZTUm/7Cs5j+4f2v+w1s0H20=",
                "Jrmctnj+ixdK3xUq+0eLklQsyofptcf9paHQrVD20QE=",
                "MMxS2Hdx3y6l2jWcBf1fMKxwAWN215S4CD/BPJ57oTA=",
                "oPI2nQ8Xu5cS8dmLfDynFjWaxxGgLzYX++qUdgLWxxU=",
                "mk+RCIjgRyqsFDG6Sukg7Sqq9ke7DheF8ID3QJqdCi8=",
                "OlKDho69Ulh+s/6KF8eS9LG3I58Aq3mgfPErr8AEo1s=",
                "pnZk5XlLuED7I/sYNYOedBqLvg9KAC1Tw4poxfojFBg=",
                "2mL4YIz3VFtdrHBpBUQLIPlsXkvfpqneMCneVDqDgBI=",
                "QPG8e94mNMUgeueC2h+ANRfnkjkG5yli/hpPw8mFwRk=",
                "2OiY14D3B9nKW1ai/ACOx/VO+y/xWFcrXwGPvlGQGwY=",
                "hNe+AZ+QIkbkwfnkYKmuq4LFjJez9c8QXCONIHMa2yI=",
                "lhXQa087T1T8yt32rwlO0Y9K9i6A6ysJxaeoCpQsUXk=",
                "2BVub545mBdHJIZnotoHP2QIrSstOdAGeHkTk8PbsA4=",
                "cvsy/fUIwOYgbTvxWoAH+RjRjdBKvjpC0yS8V7TTAzo=",
                "UsWm27QlfxDFAXUKOyQd+QbzFniAo8KMAcb8ogQn3zk=",
                "LO9hDP7KfQFIFuw4y6qKolzZCQAjVUtGa6SEJ0WtH28=",
                "oLrrrpgKoz/L8cEG4J2VV9VSJF8QG4Gactshr1WwZXQ=",
                "DrtwKP5kQEey3uOZvQzjqCTT30elIrLRvw3PIBqSdg4=",
                "mBxJCg3ClDS2IiJePXsv6KK6eQCY1yXvOi8m0/54uRg=",
                "9p4vrVEEIEnmreI1gy2JHvVtunHJjqT+oxUmwidJDlQ=",
                "VBMfinFy5m7jXqv1LPVqSvAn4mhntpFZ/PyS4eoJmiQ=",
                "om0eBmPqhiswq66mRdfgyzyPG/n/1jJXS5vLRMB1zTA=",
                "vs1t2qaE0RptGUHoc6CC1yNJAHJhs7g5Plwpk2hhwgQ=",
                "GLtViGiHvY6DnWT3OQ65JTBoCu4uv+S0MCvm97VJWkA=",
                "0tKtV02T7yomO6tb3D5rYr/UHQy6rITYVygqUMF+1Hk=",
                "SG4OS7WthG8Toff8NHIfBafHTB/8stW+bGrnt9ZUCWQ=",
                "/JaxZ/fXY8/bZdhL33sorUof6qDfhRHqJn7FGXNg5Wg=",
                "8vZlB2XPZF4vMn4K6FSNjvk5aZ4G6iCVSoU+Rh6Kgx0=",
                "xIbWr9fuB2qr1Xr6r5vMIzeOraIiLB338MSWl8RjATE=",
                "xDYuZfPQiVA5sW75Z4M+1fmtYvifXTEYX/BWsA701ks=",
                "2l6UgMUlJBEY2R+CTJBX5M2l552bkEPECu7YMP2OAy0=",
                "uLrkxPY2eBn3FJ4fkuklZimz455rCzCzvcFYBmVWFUQ=",
                "4EbkdgBc1IvhlGfaXuQxthQl3+wtM/qMdmnyfJE/MVc=",
                "RAlXUOypctgZ+EIBiqOVmnSW5VroQfT1aGqk0o/wR0s=",
                "tEehxSWHMtdBzl5mZWNSx9CmGzu1vrWm+YwdjvnNcUw=",
                "NF8qNh56/nXBPITAakis/FBUbNYlJQZ9ngR34VjJkiE=",
                "qrPGZKEmgnLMON6akKR2GR3omiPNBLnvB0f5Mh8EMVY=",
                "2A0rAiadKERas5Nb4d7UpBEMd15H8CF6R4a+E7QnPCk=",
                "MnS9QD/JJfsMWqZgXceAFDo/E60YQyd52Km+3jPCzhg=",
                "0rTQsecKlhLU9v6SBZuJbrUU+Yd5hx97EanqrZw6UV8=",
                "qIwAZMezVrm7ufJoTqSF+DEwOBXVdwf4zm0GMQZiZzI=",
                "6pYOa+9Kht35CGvrGEsbFqu3mxgzVTZzFJWytq0MpjU=",
                "xGd6OV9+IPhKkXgmn7AP6TcTZSANmweCS+PlgZLjQRA=",
                "tlX/IqPpfSvJfwCZzIZonVx3hln15RZpsifkiMxr53s=",
                "mML4eqBLA9XjZTqhoxVA6lVbMcjL54GqluGGPmMhWQA="
              ],
              "publicKey": "OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM="
            })"}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  const WalletInfo wallet = test::Wallet();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidRefillConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnFailedToRefillConfirmationTokens);
  EXPECT_CALL(delegate_mock_, OnWillRetryRefillingConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnDidRetryRefillingConfirmationTokens).Times(0);
  refill_confirmation_tokens_.MaybeRefill(wallet);

  EXPECT_EQ(0U, ConfirmationTokenCount());
}

TEST_F(BraveAdsRefillConfirmationTokensTest,
       DoNotRefillConfirmationTokensIfAboveTheMinimumThreshold) {
  // Arrange
  test::BuildAndSetIssuers();

  test::RefillConfirmationTokens(/*count=*/20);

  const WalletInfo wallet = test::Wallet();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidRefillConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnFailedToRefillConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnWillRetryRefillingConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnDidRetryRefillingConfirmationTokens).Times(0);
  refill_confirmation_tokens_.MaybeRefill(wallet);

  EXPECT_EQ(20U, ConfirmationTokenCount());
}

TEST_F(BraveAdsRefillConfirmationTokensTest,
       RefillConfirmationTokensIfBelowTheMinimumThreshold) {
  // Arrange
  test::BuildAndSetIssuers();

  test::RefillRandomConfirmationTokens(/*count=*/19);

  test::MockTokenGenerator(/*count=*/31);

  const test::URLResponseMap url_responses = {
      {BuildRequestSignedTokensUrlPath(test::kWalletPaymentId),
       {{net::HTTP_CREATED, test::BuildRequestSignedTokensUrlResponseBody()}}},
      {BuildGetSignedTokensUrlPath(test::kWalletPaymentId,
                                   test::kRequestSignedTokensNonce),
       {{net::HTTP_OK, /*response_body=*/R"(
            {
              "batchProof": "flsRyY/zcE6V1ymow+hJW9DdVjGlEAGyVr3QN7kc1Qf6eeYOSK93thF+IZehEXrsRFjFoV3reEm60a/pEq7XAA==",
              "signedTokens": [
                "2g0WjgYZfADeoAYI0kkXNVCcXCpfg5lv5yRdCHigkGs=",
                "XuBEdiNS8cOMKApsIiHygcnVOVssvaoudyd1wMI+O1Y=",
                "sr9ISUbtRozfF8IXslvveZSCE/NIZzkHr4WkcizZims=",
                "jDpLjMtRm6OjPwMmaH3IUP9zuYHvn1PkUamYrtuaCRs=",
                "5u947b1ODsUaTT1tU1Vh1qN24HSAaaCt0MapVNIiM2Y=",
                "XD25BcXUS4YJEGQdedCrli6wnufmFdKQmbAkKCJMnBo=",
                "QKcG8fsNPwWzmlZUZbfww0A+iBS8FM+a76pz/R4b2HQ=",
                "zFD7PmAnw/CP5Tq1qxw8iSbrjBH036al2dvb1fRZz0o=",
                "qMm/EqdSREFbFD4i6ezy+evFaGYp7Untq0iULvm4mwU=",
                "sjJQ9T5rFJcw1+ZlyeOZHY7Kmmlv9Po3FaBcZ1hL1hI=",
                "cOTDHJEx2ZLBZKk8W56KJq3fgsSj5o0mRaVCYQZXkxo=",
                "0CIrR2Ng3oOlJyOKtMwGSnfTXJwXjh5E/VBwkBLAbhg=",
                "IrcpnN7p4xwGbGGpqCMRh1G1c2ujilXuYfuYKIbxahc=",
                "HhtASybmIMS+HwgKsbvzUeM5TnLsYReixf8WnTCIvx0=",
                "9FpVkri+eN+D7sXzVjwObjRSx6ADewQ66SEuFCRlRx8=",
                "aJ7DnFP13RXo7GItmAvKPH5CvY2dITtc149PAr5WtBw=",
                "8DcOKuGIlMx00d+g4ixlz4VMxjKRQ2CJEyyf/fwEDTk=",
                "BPGAp/+/RFBTwm2Lx7+MxyYgkvzRdJqkWBw4TSRhPlE=",
                "RirDth6yyg9Zq0MCoiRiibFvaMVaFyyiQC/SDJZemiI=",
                "pImA5vt8XlSd7xWc0DcheI1pnIBRIiMKltplBn8vI0o=",
                "tCEMf+D18pRR6rWzWdHDyQlAjZ6/465GnUg6pXlcYkM=",
                "nJRbR/uZQTIKLQF6Yhxx/bywQ6SvjP7YJJz5a1k69xc=",
                "cLKRj8+1KR0lk2w1uUysR5Go87WgLfiJ+9NXD91AHl4=",
                "WPvZHt4qITOFMitv/NTXOx6M+GUf3H5ud+uuCLacrgs=",
                "XHCtobEsiESgGDgqCn4I6FmrU9vnEhXRNrzw5O0bdl0=",
                "yIDV8B7/fv1t8RcNLm93N0siuMX5AXAll5QHaOyAxj0=",
                "7pBWOhSj1cl2DlsCHC3/Dapd6bknZ+urewK6mtOKFlg=",
                "KhNVrYoxTyth9xDMSXLAyO1MyObuYQh1NyFeMshBUA4=",
                "xGqTbNWAJREpd2lKO3/pqqBmazx28VEtSwpdHJpqZzE=",
                "hkLhf4xPTDClmUoYrQZCXnP5jILU097Z5mULuez9/ns=",
                "IgJBYMaNLixag1yoXg6lNLuMjNoNE03m3456d3ekKE8="
              ],
              "publicKey": "OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM="
            })"}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  const WalletInfo wallet = test::Wallet();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidRefillConfirmationTokens);
  EXPECT_CALL(delegate_mock_, OnFailedToRefillConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnWillRetryRefillingConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnDidRetryRefillingConfirmationTokens).Times(0);
  refill_confirmation_tokens_.MaybeRefill(wallet);

  EXPECT_EQ(50U, ConfirmationTokenCount());
}

}  // namespace brave_ads
