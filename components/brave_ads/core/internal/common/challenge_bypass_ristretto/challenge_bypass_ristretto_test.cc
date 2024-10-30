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

namespace brave_ads {

namespace {
constexpr char kMessage[] = "The quick brown fox jumps over the lazy dog";
}  // namespace

TEST(BraveAdsChallengeBypassRistrettoTest, ProveAndVerifyUnblindedToken) {
  // The client creates a random token.
  cbr::Token token;
  EXPECT_TRUE(token.has_value());

  // The client blinds this token using a blinding scalar. Blinding ensures that
  // the token is not recognizable or linkable to the original value until it's
  // unblinded.
  const std::optional<cbr::BlindedToken> blinded_token = token.Blind();
  EXPECT_TRUE(blinded_token);

  // The server generates a random signing key.
  cbr::SigningKey signing_key;
  EXPECT_TRUE(signing_key.has_value());

  // The server signs the blinded token using its signing key. This signature
  // proves the server’s endorsement of the token.
  const std::optional<cbr::SignedToken> signed_token =
      signing_key.Sign(*blinded_token);
  EXPECT_TRUE(signed_token);

  // The client verifies the batch DLEQ proof using the public key provided by
  // the server. This step confirms that the signatures are valid and correspond
  // to the public key.
  const std::optional<cbr::PublicKey> public_key = signing_key.GetPublicKey();
  EXPECT_TRUE(public_key);

  const cbr::BlindedTokenList blinded_tokens = {*blinded_token};
  const cbr::SignedTokenList signed_tokens = {*signed_token};
  cbr::BatchDLEQProof batch_dleq_proof(blinded_tokens, signed_tokens,
                                       signing_key);
  EXPECT_TRUE(batch_dleq_proof.has_value());
  EXPECT_TRUE(
      batch_dleq_proof.Verify(blinded_tokens, signed_tokens, *public_key));

  // The client unblinds the signed tokens using the blinding scalar.
  const cbr::TokenList tokens = {token};
  const std::optional<cbr::UnblindedTokenList> unblinded_tokens =
      batch_dleq_proof.VerifyAndUnblind(tokens, blinded_tokens, signed_tokens,
                                        *public_key);
  EXPECT_TRUE(unblinded_tokens);

  // Unblinded tokens are redeemed with the server periodically.
  for (const auto& unblinded_token : *unblinded_tokens) {
    // The client derives a shared verification key from the unblinded token.
    std::optional<cbr::VerificationKey> shared_verification_key =
        unblinded_token.DeriveVerificationKey();
    EXPECT_TRUE(shared_verification_key);

    // The client signs the message using the shared verification key and sends
    // it to the server as a `signature` in the credential.
    const std::optional<cbr::VerificationSignature> verification_signature =
        shared_verification_key->Sign(kMessage);
    EXPECT_TRUE(verification_signature);

    // The client decodes the token preimage from the unblinded token and sends
    // it to the server as `t` in the credential.
    const std::optional<cbr::TokenPreimage> token_preimage =
        unblinded_token.GetTokenPreimage();
    EXPECT_TRUE(token_preimage);

    // The server rederives the unblinded token using the server signing key and
    // the token preimage.
    const std::optional<cbr::UnblindedToken> rederived_unblinded_token =
        signing_key.RederiveUnblindedToken(*token_preimage);
    EXPECT_TRUE(rederived_unblinded_token);

    // The server derives the shared verification key from the unblinded token.
    std::optional<cbr::VerificationKey> rederived_shared_verification_key =
        rederived_unblinded_token->DeriveVerificationKey();
    EXPECT_TRUE(rederived_shared_verification_key);

    // The server proves and verifies the message using the verification key and
    // signature.
    EXPECT_TRUE(rederived_shared_verification_key->Verify(
        *verification_signature, kMessage));
  }
}

}  // namespace brave_ads
