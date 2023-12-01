/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/tokens_util.h"

#include <optional>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
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

class BraveAdsSignedTokensUtilTest : public UnitTestBase {};

TEST_F(BraveAdsSignedTokensUtilTest, ParsePublicKey) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(cbr::PublicKey(cbr::kPublicKeyBase64),
            ParsePublicKey(BuildUrlResponseBody()));
}

TEST_F(BraveAdsSignedTokensUtilTest, DoNotParseInvalidPublicKey) {
  // Arrange
  base::Value::Dict dict = BuildUrlResponseBody();
  dict.Set("publicKey", cbr::kInvalidBase64);

  // Act

  // Assert
  EXPECT_FALSE(ParsePublicKey(dict));
}

TEST_F(BraveAdsSignedTokensUtilTest, ParseSignedTokens) {
  // Arrange

  // Act
  const std::optional<std::vector<cbr::SignedToken>> signed_tokens =
      ParseSignedTokens(BuildUrlResponseBody());

  // Assert
  const std::vector<cbr::SignedToken> expected_signed_tokens = {
      cbr::SignedToken(cbr::kSignedTokenBase64)};
  EXPECT_EQ(expected_signed_tokens, signed_tokens);
}

TEST_F(BraveAdsSignedTokensUtilTest, DoNotParseSignedTokensIfMissingKey) {
  // Arrange
  base::Value::Dict dict = BuildUrlResponseBody();
  dict.Remove("signedTokens");

  // Act

  // Assert
  EXPECT_FALSE(ParseSignedTokens(dict));
}

TEST_F(BraveAdsSignedTokensUtilTest, DoNotParseInvalidSignedTokens) {
  // Arrange
  base::Value::Dict dict = BuildUrlResponseBody();
  dict.Set("signedTokens", cbr::kInvalidBase64);

  // Act

  // Assert
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
  base::Value::Dict dict = BuildUrlResponseBody();
  dict.Set("signedTokens", base::Value::List().Append(/*invalid*/ 0));

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
  base::Value::Dict dict = BuildUrlResponseBody();
  dict.Set("signedTokens", base::Value::List().Append(cbr::kInvalidBase64));

  // Act
  const auto result = ParseVerifyAndUnblindTokens(dict, cbr::test::GetTokens(),
                                                  cbr::test::GetBlindedTokens(),
                                                  cbr::test::GetPublicKey());

  // Assert
  EXPECT_FALSE(result.has_value());
}

}  // namespace brave_ads
