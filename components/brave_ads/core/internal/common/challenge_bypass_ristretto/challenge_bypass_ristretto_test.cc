/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/batch_dleq_proof.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signing_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token_preimage.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/verification_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/verification_signature.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::cbr {

namespace {
constexpr char kMessage[] = "The quick brown fox jumps over the lazy dog";
}  // namespace

TEST(BraveAdsChallengeBypassRistrettoTest, ProveAndVerifyUnblindedToken) {
  // Client prepares a random token.
  Token token;
  EXPECT_TRUE(token.has_value());

  // Client prepares a blinding scalar.
  const std::optional<BlindedToken> blinded_token = token.Blind();
  EXPECT_TRUE(blinded_token);

  // Server prepares a random signing key.
  SigningKey signing_key;
  EXPECT_TRUE(signing_key.has_value());

  // Server signs the blinded token.
  const std::optional<SignedToken> signed_token =
      signing_key.Sign(*blinded_token);
  EXPECT_TRUE(signed_token);

  // Server returns the associated public key for the signing key.
  const std::optional<PublicKey> public_key = signing_key.GetPublicKey();
  EXPECT_TRUE(public_key);

  // Server signs and returns a batch DLEQ (Discrete Log Equivalence) proof.
  const std::vector<BlindedToken> blinded_tokens = {*blinded_token};
  const std::vector<SignedToken> signed_tokens = {*signed_token};
  BatchDLEQProof batch_dleq_proof(blinded_tokens, signed_tokens, signing_key);
  EXPECT_TRUE(batch_dleq_proof.has_value());

  // Client verifies the batch DLEQ proof received from the server using the
  // received public key.
  EXPECT_TRUE(
      batch_dleq_proof.Verify(blinded_tokens, signed_tokens, *public_key));

  // Client verifies the batch DLEQ proof received from the server and uses the
  // blinding scalar to unblind the returned signed tokens.
  const std::vector<Token> tokens = {token};
  const std::optional<std::vector<UnblindedToken>> unblinded_tokens =
      batch_dleq_proof.VerifyAndUnblind(tokens, blinded_tokens, signed_tokens,
                                        *public_key);
  EXPECT_TRUE(unblinded_tokens);

  // Redeem unblinded tokens.
  for (const auto& unblinded_token : *unblinded_tokens) {
    // Client derives a shared verification key from the unblinded token.
    std::optional<VerificationKey> verification_key =
        unblinded_token.DeriveVerificationKey();
    EXPECT_TRUE(verification_key);

    // Client signs the message using the shared verification key and sends it
    // to the server as a `signature` in the credential.
    const std::optional<VerificationSignature> verification_signature =
        verification_key->Sign(kMessage);
    EXPECT_TRUE(verification_signature);

    // Client decodes the token preimage from the unblinded token and sends it
    // to the server as `t` in the credential.
    const std::optional<TokenPreimage> token_preimage =
        unblinded_token.GetTokenPreimage();
    EXPECT_TRUE(token_preimage);

    // Server rederives the unblinded token using the server signing key and the
    // token preimage.
    const std::optional<UnblindedToken> rederived_unblinded_token =
        signing_key.RederiveUnblindedToken(*token_preimage);
    EXPECT_TRUE(rederived_unblinded_token);

    // Server derives the shared verification key from the unblinded token.
    std::optional<VerificationKey> shared_verification_key =
        rederived_unblinded_token->DeriveVerificationKey();
    EXPECT_TRUE(shared_verification_key);

    // Server proves and verifies the message using the verification signature.
    EXPECT_TRUE(
        shared_verification_key->Verify(*verification_signature, kMessage));
  }
}

}  // namespace brave_ads::cbr
