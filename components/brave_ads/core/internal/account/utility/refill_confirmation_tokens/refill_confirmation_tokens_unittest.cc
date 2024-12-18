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
              "batchProof": "zFKE/klGjgSJjgrc74uzy2cdc0dXPD3WTIbxraoQegTVi+Q0/YpI1olCDXoLf6FPMPwktEt1e3YWaMFlXn0vCw==",
              "signedTokens": [
                "Ktsii4pOKOjfmqziUsdlV3lJETaGXG0rksRujn1i1Qs=",
                "Mv5M6UhufCEmF7u9pBaGQJAckuwpkSpOJvJq1F+y9VE=",
                "MpysVJMYUI4n0NcY2CdZyZxQxADOHJal1pZI2uPiqWs=",
                "5stC+5v3eo3tADEkTtxESlCBIkoYI8aRDLJ8VVtcLyE=",
                "MBSxmUfpp5p8FI+86A4+0zFYw5dPhre1YYw+sRFWkTs=",
                "TPtzCHdDbaW3vAApNFc6nyC806fmdNqG9HaoZMQWGx8=",
                "dt0Xmblg7vU0TEzE4yaBtNnQ8igZJcdlNj1CDVxXVHA=",
                "RBJGJT9Pf2T2yzi+hfPXnXIjxZ6WbJ7UhZ1f1BGDpHc=",
                "NN/FZ/sbjvhGAKzBQdtUrbjkDuCKbpwNCT3cKURvzB8=",
                "BCoCHDIFRVyZqgGfB1SJpTsHa8tJ9VFOq0PUPubLBk8=",
                "EAjQxthZt6mR2+wgMV35B4Bq61e16PCyX5JkhRwdxXM=",
                "3LPp0kUrPjeKVkBFJSOL9b7/qjfUCc8ylqZyjcSFhVs=",
                "zpjmculV506uDhkXJ4k6lCCFhVja5EkA2uXkefE3ZlY=",
                "nEzbQkRVPl6H9+AF/OBcfLf61o1xO/jbB+7a1amJ3Eg=",
                "NgSQDmucmraSK8QZve64DvMv1l01jQEpK9RjSR7J91Q=",
                "vCB5ksvHTXCYtbCdBe+xynPo3T6ybVgLfHf0Od8PEEQ=",
                "PlkLA8DFzpMQQaCsKICK7zxDRJ10YEdFGvFFfx9Fn08=",
                "jm2usDGsU+/KSy2+6Q8X5cwhFr7pZ+tJ6GRCue2ywl0=",
                "SD8rfdIV3Hp3PbiQ29KmNN4eYd0kMA6fVr4ZKb6McRg=",
                "DKPADls0HZ0m0MtQb2qk5A/w/bUWAgf5gQI+F/9JMQQ=",
                "5phPdMP/HWQCMGkXvgW8BDLgKZRTni2eQft8oDOlUhY=",
                "LDkKHokr0ydfPuj3cFaXsI1Il1pFEZUrX3G2IM30Il8=",
                "FFbRg7fgraPLTOBKkiepCl0ukp6fYHrh/E715Gsxn0Q=",
                "aNBW1Qn9y0WejDJJzU/4FmfBnfOipeDOD0qo2u8u2xQ=",
                "KGIhrGVhTah7Liwgphh6EidivlOVKbnWMQRrKItcnVY=",
                "Ag+u6UoZ/AJnVmLH659K8zLlaFVgEV4TSWiCIo/1RGk=",
                "Bjsr97vTdcPRFb8rVZ96yGKGEWqBRobalK09hSjnckk=",
                "TLdnTFqBqDBI4LhGQrQLFIP9hYBXjvRIAj58YuCkrG8=",
                "nLTmiuYHyKudXYxXSwT5fOLmIJOiuB5nsrvzRr+STlI=",
                "sBYEmkom7ay5PR0JI4J9pohdAsT00BneDcBlk9XJAXc=",
                "loZ/wr35u1XaMQXPi2KDCXbMoqCR4qSrihf8hGFGFnk=",
                "Ethq6jJOlvciMq9CxnvCvPiS6dpQTSJAfGG3JAMTcn0=",
                "uKGcjL8+ja5g+kg1GXmoKqB1UxsJjHFYyJsErIxRTDI=",
                "zGNHYNpUX9fdDENSlT5a0MFzssn9nJwRs9Fh5NM1GDw=",
                "xHnOdiSKGvOr7qzIXgP0epsFPy0STQy2+6nd6NZEeXo=",
                "wuFVfI9geQ+U+R7LxM+SFiz15Fj7rpIRMOMNSt43PAM=",
                "agMSIk+buHMb4qUc0veGzAUXdcVkHqc+Oy+Zt7IOKzA=",
                "mj7RPYebPDz859w61Ce5HEQh3NGQ1xYyp5xthZtpym8=",
                "jkmYEUw+dU8ypn03nXl6Fj+OzctOLlG8CSYEI15zNAg=",
                "UAuc8dnFor4mva7EGD5bV/nqZz8wq2/RAte8r0iuNUY=",
                "9OxHQt1/VR9JpMB4XrC0Yf7MScmHANkpSzcb1XujtkM=",
                "Mv5V7+I0BBwNrVYf/012S+esEnqh3gAXZtlE8ClbOg0=",
                "doMba7NOQ+4doCwCQdBGWbvI+qofpgNr1VHqrM8vMCw=",
                "QIl+CO4G3g6Tbfre2g6TOkKOgZ4TgIoXeOoRqSV4jGU=",
                "LBjAPlawmwRldmRDpKk0wJRQBpkNqhzDiDNBNYmzg1E=",
                "QlKUaOO/znQjVtwXNGiwUDrV+GW3/a9wZXvvjvib+3I=",
                "CAHB+5LsyE/ntr8d4Ll1eFnowotytI+ZksaSdiiIa00=",
                "PorTCjYCypszf7/fs4eXgrgV4yi/3qIEmffYmaJ9hXI=",
                "Gt95JWkzrdcSw41DCdoTs8umsC7CzH1y0rJ7m/2vGX4=",
                "IjtwdzooaSvEr/V+e5vESRhUoJgvLnaqdbTL+dnKyR4="
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
                "Ktsii4pOKOjfmqziUsdlV3lJETaGXG0rksRujn1i1Qs=",
                "Mv5M6UhufCEmF7u9pBaGQJAckuwpkSpOJvJq1F+y9VE=",
                "MpysVJMYUI4n0NcY2CdZyZxQxADOHJal1pZI2uPiqWs=",
                "5stC+5v3eo3tADEkTtxESlCBIkoYI8aRDLJ8VVtcLyE=",
                "MBSxmUfpp5p8FI+86A4+0zFYw5dPhre1YYw+sRFWkTs=",
                "TPtzCHdDbaW3vAApNFc6nyC806fmdNqG9HaoZMQWGx8=",
                "dt0Xmblg7vU0TEzE4yaBtNnQ8igZJcdlNj1CDVxXVHA=",
                "RBJGJT9Pf2T2yzi+hfPXnXIjxZ6WbJ7UhZ1f1BGDpHc=",
                "NN/FZ/sbjvhGAKzBQdtUrbjkDuCKbpwNCT3cKURvzB8=",
                "BCoCHDIFRVyZqgGfB1SJpTsHa8tJ9VFOq0PUPubLBk8=",
                "EAjQxthZt6mR2+wgMV35B4Bq61e16PCyX5JkhRwdxXM=",
                "3LPp0kUrPjeKVkBFJSOL9b7/qjfUCc8ylqZyjcSFhVs=",
                "zpjmculV506uDhkXJ4k6lCCFhVja5EkA2uXkefE3ZlY=",
                "nEzbQkRVPl6H9+AF/OBcfLf61o1xO/jbB+7a1amJ3Eg=",
                "NgSQDmucmraSK8QZve64DvMv1l01jQEpK9RjSR7J91Q=",
                "vCB5ksvHTXCYtbCdBe+xynPo3T6ybVgLfHf0Od8PEEQ=",
                "PlkLA8DFzpMQQaCsKICK7zxDRJ10YEdFGvFFfx9Fn08=",
                "jm2usDGsU+/KSy2+6Q8X5cwhFr7pZ+tJ6GRCue2ywl0=",
                "SD8rfdIV3Hp3PbiQ29KmNN4eYd0kMA6fVr4ZKb6McRg=",
                "DKPADls0HZ0m0MtQb2qk5A/w/bUWAgf5gQI+F/9JMQQ=",
                "5phPdMP/HWQCMGkXvgW8BDLgKZRTni2eQft8oDOlUhY=",
                "LDkKHokr0ydfPuj3cFaXsI1Il1pFEZUrX3G2IM30Il8=",
                "FFbRg7fgraPLTOBKkiepCl0ukp6fYHrh/E715Gsxn0Q=",
                "aNBW1Qn9y0WejDJJzU/4FmfBnfOipeDOD0qo2u8u2xQ=",
                "KGIhrGVhTah7Liwgphh6EidivlOVKbnWMQRrKItcnVY=",
                "Ag+u6UoZ/AJnVmLH659K8zLlaFVgEV4TSWiCIo/1RGk=",
                "Bjsr97vTdcPRFb8rVZ96yGKGEWqBRobalK09hSjnckk=",
                "TLdnTFqBqDBI4LhGQrQLFIP9hYBXjvRIAj58YuCkrG8=",
                "nLTmiuYHyKudXYxXSwT5fOLmIJOiuB5nsrvzRr+STlI=",
                "sBYEmkom7ay5PR0JI4J9pohdAsT00BneDcBlk9XJAXc=",
                "loZ/wr35u1XaMQXPi2KDCXbMoqCR4qSrihf8hGFGFnk=",
                "Ethq6jJOlvciMq9CxnvCvPiS6dpQTSJAfGG3JAMTcn0=",
                "uKGcjL8+ja5g+kg1GXmoKqB1UxsJjHFYyJsErIxRTDI=",
                "zGNHYNpUX9fdDENSlT5a0MFzssn9nJwRs9Fh5NM1GDw=",
                "xHnOdiSKGvOr7qzIXgP0epsFPy0STQy2+6nd6NZEeXo=",
                "wuFVfI9geQ+U+R7LxM+SFiz15Fj7rpIRMOMNSt43PAM=",
                "agMSIk+buHMb4qUc0veGzAUXdcVkHqc+Oy+Zt7IOKzA=",
                "mj7RPYebPDz859w61Ce5HEQh3NGQ1xYyp5xthZtpym8=",
                "jkmYEUw+dU8ypn03nXl6Fj+OzctOLlG8CSYEI15zNAg=",
                "UAuc8dnFor4mva7EGD5bV/nqZz8wq2/RAte8r0iuNUY=",
                "9OxHQt1/VR9JpMB4XrC0Yf7MScmHANkpSzcb1XujtkM=",
                "Mv5V7+I0BBwNrVYf/012S+esEnqh3gAXZtlE8ClbOg0=",
                "doMba7NOQ+4doCwCQdBGWbvI+qofpgNr1VHqrM8vMCw=",
                "QIl+CO4G3g6Tbfre2g6TOkKOgZ4TgIoXeOoRqSV4jGU=",
                "LBjAPlawmwRldmRDpKk0wJRQBpkNqhzDiDNBNYmzg1E=",
                "QlKUaOO/znQjVtwXNGiwUDrV+GW3/a9wZXvvjvib+3I=",
                "CAHB+5LsyE/ntr8d4Ll1eFnowotytI+ZksaSdiiIa00=",
                "PorTCjYCypszf7/fs4eXgrgV4yi/3qIEmffYmaJ9hXI=",
                "Gt95JWkzrdcSw41DCdoTs8umsC7CzH1y0rJ7m/2vGX4=",
                "IjtwdzooaSvEr/V+e5vESRhUoJgvLnaqdbTL+dnKyR4="
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
              "batchProof": "zFKE/klGjgSJjgrc74uzy2cdc0dXPD3WTIbxraoQegTVi+Q0/YpI1olCDXoLf6FPMPwktEt1e3YWaMFlXn0vCw==",
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
              "batchProof": "zFKE/klGjgSJjgrc74uzy2cdc0dXPD3WTIbxraoQegTVi+Q0/YpI1olCDXoLf6FPMPwktEt1e3YWaMFlXn0vCw==",
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
              "batchProof": "9YSTyoJ64yGBkMUfhr9AHNOHWVUsInyBTTrfdgfUCwyLTHE/TbPc/uwbGbVX0Y84QnOx/2dLXszDn3srCJMlCQ==",
              "signedTokens": [
                "Ktsii4pOKOjfmqziUsdlV3lJETaGXG0rksRujn1i1Qs=",
                "Mv5M6UhufCEmF7u9pBaGQJAckuwpkSpOJvJq1F+y9VE=",
                "MpysVJMYUI4n0NcY2CdZyZxQxADOHJal1pZI2uPiqWs=",
                "5stC+5v3eo3tADEkTtxESlCBIkoYI8aRDLJ8VVtcLyE=",
                "MBSxmUfpp5p8FI+86A4+0zFYw5dPhre1YYw+sRFWkTs=",
                "TPtzCHdDbaW3vAApNFc6nyC806fmdNqG9HaoZMQWGx8=",
                "dt0Xmblg7vU0TEzE4yaBtNnQ8igZJcdlNj1CDVxXVHA=",
                "RBJGJT9Pf2T2yzi+hfPXnXIjxZ6WbJ7UhZ1f1BGDpHc=",
                "NN/FZ/sbjvhGAKzBQdtUrbjkDuCKbpwNCT3cKURvzB8=",
                "BCoCHDIFRVyZqgGfB1SJpTsHa8tJ9VFOq0PUPubLBk8=",
                "EAjQxthZt6mR2+wgMV35B4Bq61e16PCyX5JkhRwdxXM=",
                "3LPp0kUrPjeKVkBFJSOL9b7/qjfUCc8ylqZyjcSFhVs=",
                "zpjmculV506uDhkXJ4k6lCCFhVja5EkA2uXkefE3ZlY=",
                "nEzbQkRVPl6H9+AF/OBcfLf61o1xO/jbB+7a1amJ3Eg=",
                "NgSQDmucmraSK8QZve64DvMv1l01jQEpK9RjSR7J91Q=",
                "vCB5ksvHTXCYtbCdBe+xynPo3T6ybVgLfHf0Od8PEEQ=",
                "PlkLA8DFzpMQQaCsKICK7zxDRJ10YEdFGvFFfx9Fn08=",
                "jm2usDGsU+/KSy2+6Q8X5cwhFr7pZ+tJ6GRCue2ywl0=",
                "SD8rfdIV3Hp3PbiQ29KmNN4eYd0kMA6fVr4ZKb6McRg=",
                "DKPADls0HZ0m0MtQb2qk5A/w/bUWAgf5gQI+F/9JMQQ=",
                "5phPdMP/HWQCMGkXvgW8BDLgKZRTni2eQft8oDOlUhY=",
                "LDkKHokr0ydfPuj3cFaXsI1Il1pFEZUrX3G2IM30Il8=",
                "FFbRg7fgraPLTOBKkiepCl0ukp6fYHrh/E715Gsxn0Q=",
                "aNBW1Qn9y0WejDJJzU/4FmfBnfOipeDOD0qo2u8u2xQ=",
                "KGIhrGVhTah7Liwgphh6EidivlOVKbnWMQRrKItcnVY=",
                "Ag+u6UoZ/AJnVmLH659K8zLlaFVgEV4TSWiCIo/1RGk=",
                "Bjsr97vTdcPRFb8rVZ96yGKGEWqBRobalK09hSjnckk=",
                "TLdnTFqBqDBI4LhGQrQLFIP9hYBXjvRIAj58YuCkrG8=",
                "nLTmiuYHyKudXYxXSwT5fOLmIJOiuB5nsrvzRr+STlI=",
                "sBYEmkom7ay5PR0JI4J9pohdAsT00BneDcBlk9XJAXc=",
                "loZ/wr35u1XaMQXPi2KDCXbMoqCR4qSrihf8hGFGFnk="
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
