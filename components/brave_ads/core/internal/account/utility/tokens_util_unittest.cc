/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/tokens_util.h"

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_test_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token_test_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key_test_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

base::Value::Dict BuildUrlResponseBody() {
  return base::Value::Dict()
      .Set("batchProof", cbr::test::kBatchDLEQProofBase64)
      .Set("signedTokens",
           base::Value::List().Append(cbr::test::kSignedTokenBase64))
      .Set("publicKey", cbr::test::kPublicKeyBase64);
}

}  // namespace

class BraveAdsSignedTokensUtilTest : public test::TestBase {};

TEST_F(BraveAdsSignedTokensUtilTest, ParsePublicKey) {
  // Act & Assert
  EXPECT_EQ(cbr::PublicKey(cbr::test::kPublicKeyBase64),
            ParsePublicKey(BuildUrlResponseBody()));
}

TEST_F(BraveAdsSignedTokensUtilTest, DoNotParseInvalidPublicKey) {
  // Arrange
  const base::Value::Dict dict =
      BuildUrlResponseBody().Set("publicKey", cbr::test::kInvalidBase64);

  // Act & Assert
  EXPECT_FALSE(ParsePublicKey(dict));
}

TEST_F(BraveAdsSignedTokensUtilTest, ParseSignedTokens) {
  // Act
  const std::optional<std::vector<cbr::SignedToken>> signed_tokens =
      ParseSignedTokens(BuildUrlResponseBody());
  ASSERT_TRUE(signed_tokens);

  // Assert
  const std::vector<cbr::SignedToken> expected_signed_tokens = {
      cbr::SignedToken(cbr::test::kSignedTokenBase64)};
  EXPECT_EQ(expected_signed_tokens, signed_tokens);
}

TEST_F(BraveAdsSignedTokensUtilTest, DoNotParseSignedTokensIfMissingKey) {
  // Arrange
  base::Value::Dict dict = BuildUrlResponseBody();
  dict.Remove("signedTokens");

  // Act & Assert
  EXPECT_FALSE(ParseSignedTokens(dict));
}

TEST_F(BraveAdsSignedTokensUtilTest, DoNotParseInvalidSignedTokens) {
  // Arrange
  const base::Value::Dict dict =
      BuildUrlResponseBody().Set("signedTokens", cbr::test::kInvalidBase64);

  // Act & Assert
  EXPECT_FALSE(ParseSignedTokens(dict));
}

TEST_F(BraveAdsSignedTokensUtilTest, ParseVerifyAndUnblindTokens) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act
  const auto result = ParseVerifyAndUnblindTokens(
      BuildUrlResponseBody(), cbr::test::GetTokens(),
      cbr::test::GetBlindedTokens(), cbr::test::GetPublicKey());

  // Assert
  EXPECT_TRUE(result.has_value());
}

TEST_F(BraveAdsSignedTokensUtilTest,
       DoNotParseVerifyAndUnblindTokensIfMissingBatchDLEQProof) {
  // Arrange
  base::Value::Dict dict = BuildUrlResponseBody();
  dict.Remove("batchProof");

  // Act
  const auto result = ParseVerifyAndUnblindTokens(dict, cbr::test::GetTokens(),
                                                  cbr::test::GetBlindedTokens(),
                                                  cbr::test::GetPublicKey());

  // Assert
  EXPECT_FALSE(result.has_value());
}

TEST_F(BraveAdsSignedTokensUtilTest,
       DoNotParseVerifyAndUnblindMissingSignedTokens) {
  // Arrange
  base::Value::Dict dict = BuildUrlResponseBody();
  dict.Remove("signedTokens");

  // Act
  const auto result = ParseVerifyAndUnblindTokens(dict, cbr::test::GetTokens(),
                                                  cbr::test::GetBlindedTokens(),
                                                  cbr::test::GetPublicKey());

  // Assert
  EXPECT_FALSE(result.has_value());
}

TEST_F(BraveAdsSignedTokensUtilTest,
       DoNoParseVerifyAndUnblindMalformedSignedTokens) {
  // Arrange
  const base::Value::Dict dict = BuildUrlResponseBody().Set(
      "signedTokens", base::Value::List().Append(/*invalid*/ 0));

  // Act
  const auto result = ParseVerifyAndUnblindTokens(dict, cbr::test::GetTokens(),
                                                  cbr::test::GetBlindedTokens(),
                                                  cbr::test::GetPublicKey());

  // Assert
  EXPECT_FALSE(result.has_value());
}

TEST_F(BraveAdsSignedTokensUtilTest,
       DoNotParseVerifyAndUnblindInvalidSignedTokens) {
  // Arrange
  const base::Value::Dict dict = BuildUrlResponseBody().Set(
      "signedTokens", base::Value::List().Append(cbr::test::kInvalidBase64));

  // Act
  const auto result = ParseVerifyAndUnblindTokens(dict, cbr::test::GetTokens(),
                                                  cbr::test::GetBlindedTokens(),
                                                  cbr::test::GetPublicKey());

  // Assert
  EXPECT_FALSE(result.has_value());
}

}  // namespace brave_ads
