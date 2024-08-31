/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/refill_confirmation_tokens.h"

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_test_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/refill_confirmation_tokens_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/refill_confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/get_signed_tokens/get_signed_tokens_url_request_builder_test_constants.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/get_signed_tokens/get_signed_tokens_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/request_signed_tokens/request_signed_tokens_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_test_constants.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_test_util.h"
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
       RefillConfirmationTokensCaptchaRequired) {
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

TEST_F(BraveAdsRefillConfirmationTokensTest, IssuersPublicKeyMismatch) {
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
       {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}});

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
         /*response_body=*/net::GetHttpReasonPhrase(
             net::HTTP_INTERNAL_SERVER_ERROR)},
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

TEST_F(BraveAdsRefillConfirmationTokensTest, RequestSignedTokensMissingNonce) {
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
         /*response_body=*/net::GetHttpReasonPhrase(
             net::HTTP_INTERNAL_SERVER_ERROR)},
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

TEST_F(BraveAdsRefillConfirmationTokensTest, GetSignedTokensInvalidResponse) {
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

TEST_F(BraveAdsRefillConfirmationTokensTest, GetSignedTokensMissingPublicKey) {
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
              "batchProof": "nZBm4sojuuKV91w9/Hcevh3r0SSmx7Cu26oeUko7hwIXYQJXjuFabmJ40nNToVm7UAkiaQvoKDFViqfpHcwxAA==",
              "signedTokens": [
                "DA+IEWxDIW91q3JItW8D3xIYMgO0TDyP/TKXFQExLEs=",
                "3KQ0o6IgVLHpKLzH01cGxA89om8CvOrW+anxorhJISk=",
                "XlzLlvOPl3XqQAYF19ly95wnAu0IA5GNk9IIUVsen0Q=",
                "VIAZP4U2QYJt/uUHLf0rfTVmMqTbytvqEGWx5eZLYDs=",
                "rk+ollPWD+y8NlqiftGmr/riBJfsg34DM/M3Fi9BFA0=",
                "xn/IDC6/wUuag5XIHR34lnnMRTi1GDVb96MYmUBUgmo=",
                "iibC1sEFbkOqGvZuQgYG1vdr6QMfLYfvFWbybmo8Myo=",
                "GAw6J637OVCz7iKN1WSg5LbLem/CMGcIz/iqmcVEvBc=",
                "Vvr5eN/Wtk5papD2urom0iOZtInck8MCBFPKKn2yuEc=",
                "1unbpoVjusErAw5cM03aBAtVUS1rKz21Z+/qro4fKjY=",
                "/Efsmh2qQNviLW47/F7wjMYnvXsS3sNvoShIxg7tsCM=",
                "VrWO44mtMyBHi3vwG9WFJcnJAueZtqCcUsnMnaH8x38=",
                "yvr5gFkC70TOQuoPdc+1UUpL8TDvKgTHNIpIBqSVcic=",
                "EsbmSuk0n/bNwwg9w09+vcABAXyJWOJHllEJam0li1I=",
                "kpyU2LZomANA/sSiAmlhITmaSnUCF4k3ZbL8tC8bCBs=",
                "snY5KcxsrH4YtPtuATwrqFEsI8s41CwFwzD9NQPRYDM=",
                "Os0UZsMkLd2zjSvtlZXNONoC9lW88ub/am/m6M5JyA0=",
                "yhgSTuGXe/7rEOn71eJFsIs2yenE4yAqqQLTxZvj9AE=",
                "hswDh0BfDSFVC5IgVRo48u4DmNoVGXBguN0MvOcxFwQ=",
                "WKkneVbAf05kwIWiH8wN3jhNjwyJLYi9zH+wkNBo5nw=",
                "HH26LkAhxJxIFthoLyKD/OA67AzmyPW59ady3nYcnj4=",
                "5EPMh4OW09U3M4d8oPYv4OPtOZPiN0+cLTWIRc486lw=",
                "lhYrEba/qFw6hKUOhBJO6KVRUj4yQwXNhkhdNWvAFjg=",
                "XKkrY+E8P2c3vBLa2967Q1uP3o0bsz6ElDbetbiPuUE=",
                "/GgluIwAIj2zfuiY4YpfF+cLTByPNM37DBlld746bxY=",
                "dMwnd60OVo3jF/DkkZIO9Y68vvGoL0yfO9qlLsdTfwo=",
                "op4WXEs052uAvEvqvGWkr7PATcigxwwKExuIqMUoVQ4=",
                "VJrlz3pP+VNeBivSYUYOLqKEP2x9vr75AUkBcazpMhg=",
                "UJuKnh5+jVwrRctKm29v4FBLoLqy7Hq4ddr7Ci9o6X0=",
                "pEQVS63QXhWHHOIClAxKJFevczY2YcTnS5SnzxX/AAE=",
                "bjAvdetnrggnuQ4nUAG+SCUpMRhsnstXW/m9kIVwUys=",
                "tNcuHnWkzovoG/CQ/Uz8LwqojM9se+WxfdArBrMEjTE=",
                "lDT++JAPo/S5R4S9R9c/WBnrNq/qznsb121cybxDcFI=",
                "yIRJoQT0DErGmFNLt2DraTRqbiScbAh07TsAqHLAzlk=",
                "Vtt/1xciT3GzPVEqWS321kcD37JPW/gfA0i5A4jHSFA=",
                "WF1TDYuaZaah9xuS0PRonsMtwS/YYATyzrsB525OpEw=",
                "yq2LhUEeBFD2eaKu7Wmjyb1W2OyB482PwPKj7HjUaWg=",
                "JNbzAbJS/LWdUdWbd/epPHy935mpeo+1yBZrPQuH4k0=",
                "PPucf/FQzL7mU6Ec8gUtOiO+1V1sabDX76rqv/XjkFk=",
                "dH/pwaRwwyg9kSDUmGyfRSV94slSYXIbrl8ZhGkGiBQ=",
                "dj45NGx3tM2/t5hfu7shUxPKd1XmKpucopYBooxR/SE=",
                "emn0UWvwqfSZeR8uBqjT629UeP+7PXyfg4WXYIIk9BM=",
                "Gpiwdsv6YlBqgcWCA1oenPaX8VkvT2eXxvGC60SUC08=",
                "0m4tWlAVWh7OA1scMdbbXRLGM2ui72DbiSh3tgZ56Xk=",
                "uCYqm0IjumhZH7yBzZl+W2FbHwq3Qc0tV0CQTnJSV04=",
                "8OJEKXDcCoDEnzggFHaQpemkfPmHJ5u+isQzE8wnXkE=",
                "hHJHWTpnWUexvq8J218KqBqmblpimQR021/GbPFKT1Q=",
                "+MZDcE6B+WykiTN4ArTMQob21auW8/6PVAc4O62fq2s=",
                "MNm3EDUMslVioVqCDb5PkW8op+cHFyw8qo3BKxVZLF4=",
                "2LEoAWcTHMMFFV+/BYOGjPhdAtNL8asRwdliyaGQnRA="
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
       GetSignedTokensMissingBatchProofDleq) {
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
                "3KQ0o6IgVLHpKLzH01cGxA89om8CvOrW+anxorhJISk=",
                "XlzLlvOPl3XqQAYF19ly95wnAu0IA5GNk9IIUVsen0Q=",
                "VIAZP4U2QYJt/uUHLf0rfTVmMqTbytvqEGWx5eZLYDs=",
                "rk+ollPWD+y8NlqiftGmr/riBJfsg34DM/M3Fi9BFA0=",
                "xn/IDC6/wUuag5XIHR34lnnMRTi1GDVb96MYmUBUgmo=",
                "iibC1sEFbkOqGvZuQgYG1vdr6QMfLYfvFWbybmo8Myo=",
                "GAw6J637OVCz7iKN1WSg5LbLem/CMGcIz/iqmcVEvBc=",
                "Vvr5eN/Wtk5papD2urom0iOZtInck8MCBFPKKn2yuEc=",
                "1unbpoVjusErAw5cM03aBAtVUS1rKz21Z+/qro4fKjY=",
                "/Efsmh2qQNviLW47/F7wjMYnvXsS3sNvoShIxg7tsCM=",
                "VrWO44mtMyBHi3vwG9WFJcnJAueZtqCcUsnMnaH8x38=",
                "yvr5gFkC70TOQuoPdc+1UUpL8TDvKgTHNIpIBqSVcic=",
                "EsbmSuk0n/bNwwg9w09+vcABAXyJWOJHllEJam0li1I=",
                "kpyU2LZomANA/sSiAmlhITmaSnUCF4k3ZbL8tC8bCBs=",
                "snY5KcxsrH4YtPtuATwrqFEsI8s41CwFwzD9NQPRYDM=",
                "Os0UZsMkLd2zjSvtlZXNONoC9lW88ub/am/m6M5JyA0=",
                "yhgSTuGXe/7rEOn71eJFsIs2yenE4yAqqQLTxZvj9AE=",
                "hswDh0BfDSFVC5IgVRo48u4DmNoVGXBguN0MvOcxFwQ=",
                "WKkneVbAf05kwIWiH8wN3jhNjwyJLYi9zH+wkNBo5nw=",
                "HH26LkAhxJxIFthoLyKD/OA67AzmyPW59ady3nYcnj4=",
                "5EPMh4OW09U3M4d8oPYv4OPtOZPiN0+cLTWIRc486lw=",
                "lhYrEba/qFw6hKUOhBJO6KVRUj4yQwXNhkhdNWvAFjg=",
                "XKkrY+E8P2c3vBLa2967Q1uP3o0bsz6ElDbetbiPuUE=",
                "/GgluIwAIj2zfuiY4YpfF+cLTByPNM37DBlld746bxY=",
                "dMwnd60OVo3jF/DkkZIO9Y68vvGoL0yfO9qlLsdTfwo=",
                "op4WXEs052uAvEvqvGWkr7PATcigxwwKExuIqMUoVQ4=",
                "VJrlz3pP+VNeBivSYUYOLqKEP2x9vr75AUkBcazpMhg=",
                "UJuKnh5+jVwrRctKm29v4FBLoLqy7Hq4ddr7Ci9o6X0=",
                "pEQVS63QXhWHHOIClAxKJFevczY2YcTnS5SnzxX/AAE=",
                "bjAvdetnrggnuQ4nUAG+SCUpMRhsnstXW/m9kIVwUys=",
                "tNcuHnWkzovoG/CQ/Uz8LwqojM9se+WxfdArBrMEjTE=",
                "lDT++JAPo/S5R4S9R9c/WBnrNq/qznsb121cybxDcFI=",
                "yIRJoQT0DErGmFNLt2DraTRqbiScbAh07TsAqHLAzlk=",
                "Vtt/1xciT3GzPVEqWS321kcD37JPW/gfA0i5A4jHSFA=",
                "WF1TDYuaZaah9xuS0PRonsMtwS/YYATyzrsB525OpEw=",
                "yq2LhUEeBFD2eaKu7Wmjyb1W2OyB482PwPKj7HjUaWg=",
                "JNbzAbJS/LWdUdWbd/epPHy935mpeo+1yBZrPQuH4k0=",
                "PPucf/FQzL7mU6Ec8gUtOiO+1V1sabDX76rqv/XjkFk=",
                "dH/pwaRwwyg9kSDUmGyfRSV94slSYXIbrl8ZhGkGiBQ=",
                "dj45NGx3tM2/t5hfu7shUxPKd1XmKpucopYBooxR/SE=",
                "emn0UWvwqfSZeR8uBqjT629UeP+7PXyfg4WXYIIk9BM=",
                "Gpiwdsv6YlBqgcWCA1oenPaX8VkvT2eXxvGC60SUC08=",
                "0m4tWlAVWh7OA1scMdbbXRLGM2ui72DbiSh3tgZ56Xk=",
                "uCYqm0IjumhZH7yBzZl+W2FbHwq3Qc0tV0CQTnJSV04=",
                "8OJEKXDcCoDEnzggFHaQpemkfPmHJ5u+isQzE8wnXkE=",
                "hHJHWTpnWUexvq8J218KqBqmblpimQR021/GbPFKT1Q=",
                "+MZDcE6B+WykiTN4ArTMQob21auW8/6PVAc4O62fq2s=",
                "MNm3EDUMslVioVqCDb5PkW8op+cHFyw8qo3BKxVZLF4=",
                "2LEoAWcTHMMFFV+/BYOGjPhdAtNL8asRwdliyaGQnRA="
              ],
              "publicKey": "bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg="
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
       GetSignedTokensMissingSignedTokens) {
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
              "batchProof": "emZH1a/Y+Q/ub1n7FuWZdkS/989iw4wNl2AnSNMmCQgy4f4ocjDJbHsWMnXWJUAIDPMHilVNJ1SXVctZ8TleBw==",
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

TEST_F(BraveAdsRefillConfirmationTokensTest, GetInvalidSignedTokens) {
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
              "batchProof": "nZBm4sojuuKV91w9/Hcevh3r0SSmx7Cu26oeUko7hwIXYQJXjuFabmJ40nNToVm7UAkiaQvoKDFViqfpHcwxAA==",
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
              "publicKey": "bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg="
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
       DoNotRefillIfAboveTheMinimumThreshold) {
  // Arrange
  test::BuildAndSetIssuers();

  test::RefillConfirmationTokens(/*count=*/50);

  const WalletInfo wallet = test::Wallet();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidRefillConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnFailedToRefillConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnWillRetryRefillingConfirmationTokens).Times(0);
  EXPECT_CALL(delegate_mock_, OnDidRetryRefillingConfirmationTokens).Times(0);
  refill_confirmation_tokens_.MaybeRefill(wallet);

  EXPECT_EQ(50U, ConfirmationTokenCount());
}

TEST_F(BraveAdsRefillConfirmationTokensTest, RefillIfBelowTheMinimumThreshold) {
  // Arrange
  test::BuildAndSetIssuers();

  test::MockTokenGenerator(/*count=*/31);
  test::RefillConfirmationTokens(/*count=*/19);

  const test::URLResponseMap url_responses = {
      {BuildRequestSignedTokensUrlPath(test::kWalletPaymentId),
       {{net::HTTP_CREATED, test::BuildRequestSignedTokensUrlResponseBody()}}},
      {BuildGetSignedTokensUrlPath(test::kWalletPaymentId,
                                   test::kRequestSignedTokensNonce),
       {{net::HTTP_OK, /*response_body=*/R"(
            {
              "batchProof": "gCF+6wrxT9bRFX+yjbdwPeWNZ8/wuDg6uZPy0Ea0yA2cfq5yEMpMfYMoH4BHW6EBbWR5QaOo1mVg+O9syYD3CA==",
              "signedTokens": [
                "DA+IEWxDIW91q3JItW8D3xIYMgO0TDyP/TKXFQExLEs=",
                "3KQ0o6IgVLHpKLzH01cGxA89om8CvOrW+anxorhJISk=",
                "XlzLlvOPl3XqQAYF19ly95wnAu0IA5GNk9IIUVsen0Q=",
                "VIAZP4U2QYJt/uUHLf0rfTVmMqTbytvqEGWx5eZLYDs=",
                "rk+ollPWD+y8NlqiftGmr/riBJfsg34DM/M3Fi9BFA0=",
                "xn/IDC6/wUuag5XIHR34lnnMRTi1GDVb96MYmUBUgmo=",
                "iibC1sEFbkOqGvZuQgYG1vdr6QMfLYfvFWbybmo8Myo=",
                "GAw6J637OVCz7iKN1WSg5LbLem/CMGcIz/iqmcVEvBc=",
                "Vvr5eN/Wtk5papD2urom0iOZtInck8MCBFPKKn2yuEc=",
                "1unbpoVjusErAw5cM03aBAtVUS1rKz21Z+/qro4fKjY=",
                "/Efsmh2qQNviLW47/F7wjMYnvXsS3sNvoShIxg7tsCM=",
                "VrWO44mtMyBHi3vwG9WFJcnJAueZtqCcUsnMnaH8x38=",
                "yvr5gFkC70TOQuoPdc+1UUpL8TDvKgTHNIpIBqSVcic=",
                "EsbmSuk0n/bNwwg9w09+vcABAXyJWOJHllEJam0li1I=",
                "kpyU2LZomANA/sSiAmlhITmaSnUCF4k3ZbL8tC8bCBs=",
                "snY5KcxsrH4YtPtuATwrqFEsI8s41CwFwzD9NQPRYDM=",
                "Os0UZsMkLd2zjSvtlZXNONoC9lW88ub/am/m6M5JyA0=",
                "yhgSTuGXe/7rEOn71eJFsIs2yenE4yAqqQLTxZvj9AE=",
                "hswDh0BfDSFVC5IgVRo48u4DmNoVGXBguN0MvOcxFwQ=",
                "WKkneVbAf05kwIWiH8wN3jhNjwyJLYi9zH+wkNBo5nw=",
                "HH26LkAhxJxIFthoLyKD/OA67AzmyPW59ady3nYcnj4=",
                "5EPMh4OW09U3M4d8oPYv4OPtOZPiN0+cLTWIRc486lw=",
                "lhYrEba/qFw6hKUOhBJO6KVRUj4yQwXNhkhdNWvAFjg=",
                "XKkrY+E8P2c3vBLa2967Q1uP3o0bsz6ElDbetbiPuUE=",
                "/GgluIwAIj2zfuiY4YpfF+cLTByPNM37DBlld746bxY=",
                "dMwnd60OVo3jF/DkkZIO9Y68vvGoL0yfO9qlLsdTfwo=",
                "op4WXEs052uAvEvqvGWkr7PATcigxwwKExuIqMUoVQ4=",
                "VJrlz3pP+VNeBivSYUYOLqKEP2x9vr75AUkBcazpMhg=",
                "UJuKnh5+jVwrRctKm29v4FBLoLqy7Hq4ddr7Ci9o6X0=",
                "pEQVS63QXhWHHOIClAxKJFevczY2YcTnS5SnzxX/AAE=",
                "bjAvdetnrggnuQ4nUAG+SCUpMRhsnstXW/m9kIVwUys="
              ],
              "publicKey": "bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg="
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
