/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/get_signed_tokens/get_signed_tokens_url_request_util.h"

#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

base::Value::Dict BuildUrlResponseBody() {
  return base::Value::Dict()
      .Set("batchProof", cbr::kBatchDLEQProofBase64)
      .Set("signedTokens", base::Value::List().Append(cbr::kSignedTokenBase64))
      .Set("publicKey", cbr::kPublicKeyBase64);
}

}  // namespace

class BraveAdsGetSignedTokensUrlRequestUtilTest : public UnitTestBase {
 protected:
  TokenGeneratorMock token_generator_mock_;
};

TEST_F(BraveAdsGetSignedTokensUrlRequestUtilTest, ParseCaptchaId) {
  // Arrange
  const base::Value::Dict dict = base::test::ParseJsonDict(
      R"(
          {
            "captcha_id": "daf85dc8-164e-4eb9-a4d4-1836055004b3"
          })");

  // Act & Assert
  EXPECT_EQ("daf85dc8-164e-4eb9-a4d4-1836055004b3", ParseCaptchaId(dict));
}

TEST_F(BraveAdsGetSignedTokensUrlRequestUtilTest, DoNotParseMissingCaptchaId) {
  // Arrange
  const base::Value::Dict dict = base::test::ParseJsonDict("{}");

  // Act & Assert
  EXPECT_FALSE(ParseCaptchaId(dict));
}

TEST_F(BraveAdsGetSignedTokensUrlRequestUtilTest, ParseAndUnblindSignedTokens) {
  // Arrange
  test::BuildAndSetIssuers();

  const base::Value::Dict dict = BuildUrlResponseBody();

  // Act
  const auto result = ParseAndUnblindSignedTokens(
      dict, cbr::test::GetTokens(), cbr::test::GetBlindedTokens());

  // Assert
  EXPECT_TRUE(result.has_value());
}

TEST_F(BraveAdsGetSignedTokensUrlRequestUtilTest,
       DoNotParseAndUnblindSignedTokensIfMissingBatchDLEQProof) {
  // Arrange
  base::Value::Dict dict = BuildUrlResponseBody();
  dict.Remove("batchProof");

  // Act
  const auto result = ParseAndUnblindSignedTokens(
      dict, cbr::test::GetTokens(), cbr::test::GetBlindedTokens());

  // Assert
  EXPECT_FALSE(result.has_value());
}

TEST_F(BraveAdsGetSignedTokensUrlRequestUtilTest,
       DoNotParseAndUnblindMissingSignedTokens) {
  // Arrange
  base::Value::Dict dict = BuildUrlResponseBody();
  dict.Remove("signedTokens");

  // Act
  const auto result = ParseAndUnblindSignedTokens(
      dict, cbr::test::GetTokens(), cbr::test::GetBlindedTokens());

  // Assert
  EXPECT_FALSE(result.has_value());
}

TEST_F(BraveAdsGetSignedTokensUrlRequestUtilTest,
       DoNotParseAndUnblindInvalidSignedTokens) {
  // Arrange
  base::Value::Dict dict = BuildUrlResponseBody();
  dict.Set("signedTokens", base::Value::List().Append(/*invalid*/ 0));

  // Act
  const auto result = ParseAndUnblindSignedTokens(
      dict, cbr::test::GetTokens(), cbr::test::GetBlindedTokens());

  // Assert
  EXPECT_FALSE(result.has_value());
}

TEST_F(BraveAdsGetSignedTokensUrlRequestUtilTest,
       DoNotParseAndUnblindSignedTokensIfMissingPublicKey) {
  // Arrange
  base::Value::Dict dict = BuildUrlResponseBody();
  dict.Remove("publicKey");

  // Act
  const auto result = ParseAndUnblindSignedTokens(
      dict, cbr::test::GetTokens(), cbr::test::GetBlindedTokens());

  // Assert
  EXPECT_FALSE(result.has_value());
}

TEST_F(BraveAdsGetSignedTokensUrlRequestUtilTest,
       DoNotVerifyAndUnblindInvalidSignedTokens) {
  // Arrange
  base::Value::Dict dict = BuildUrlResponseBody();
  dict.Set("signedTokens", base::Value::List().Append(cbr::kInvalidBase64));

  // Act
  const auto result = ParseAndUnblindSignedTokens(
      dict, cbr::test::GetTokens(), cbr::test::GetBlindedTokens());

  // Assert
  EXPECT_FALSE(result.has_value());
}

TEST_F(BraveAdsGetSignedTokensUrlRequestUtilTest,
       BuildAndAddConfirmationTokens) {
  // Act
  BuildAndAddConfirmationTokens(cbr::test::GetUnblindedTokens(),
                                cbr::test::GetPublicKey(), test::GetWallet());

  // Act & Assert
  EXPECT_FALSE(test::GetConfirmationTokens().IsEmpty());
}

}  // namespace brave_ads
