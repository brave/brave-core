/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/types/expected.h"
#include "brave/components/challenge_bypass_ristretto/batch_dleq_proof.h"
#include "brave/components/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/challenge_bypass_ristretto/public_key.h"
#include "brave/components/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/challenge_bypass_ristretto/signing_key.h"
#include "brave/components/challenge_bypass_ristretto/token.h"
#include "brave/components/challenge_bypass_ristretto/token_preimage.h"
#include "brave/components/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/challenge_bypass_ristretto/verification_key.h"
#include "brave/components/challenge_bypass_ristretto/verification_signature.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace challenge_bypass_ristretto {

namespace {
constexpr char kMessage[] = "The quick brown fox jumps over the lazy dog";
}  // namespace

TEST(ChallengeBypassRistrettoTest, ProveAndVerifyUnblindedToken) {
  // Client prepares a random token.
  Token token = Token::Random();

  // Client prepares a blinding scalar.
  const BlindedToken blinded_token = token.Blind();

  // Server prepares a random signing key.
  SigningKey signing_key = SigningKey::Random();

  // Server signs the blinded token.
  const base::expected<SignedToken, std::string> signed_token =
      signing_key.Sign(blinded_token);
  EXPECT_TRUE(signed_token.has_value());

  // Server returns the associated public key for the signing key.
  const PublicKey public_key = signing_key.GetPublicKey();

  // Server signs and returns a batch DLEQ (Discrete Log Equivalence) proof.
  const std::vector<BlindedToken> blinded_tokens = {blinded_token};
  const std::vector<SignedToken> signed_tokens = {*signed_token};
  base::expected<BatchDLEQProof, std::string> batch_dleq_proof =
      BatchDLEQProof::Create(blinded_tokens, signed_tokens, signing_key);
  EXPECT_TRUE(batch_dleq_proof.has_value());

  // Client verifies the batch DLEQ proof received from the server using the
  // received public key.
  const base::expected<bool, std::string> client_verification_result =
      batch_dleq_proof->Verify(blinded_tokens, signed_tokens, public_key);
  EXPECT_TRUE(client_verification_result.has_value());
  EXPECT_TRUE(*client_verification_result);

  // Client verifies the batch DLEQ proof received from the server and uses the
  // blinding scalar to unblind the returned signed tokens.
  const std::vector<Token> tokens = {token};
  const base::expected<std::vector<UnblindedToken>, std::string>
      unblinded_tokens = batch_dleq_proof->VerifyAndUnblind(
          tokens, blinded_tokens, signed_tokens, public_key);
  EXPECT_TRUE(unblinded_tokens.has_value());

  // Redeem unblinded tokens.
  for (const auto& unblinded_token : *unblinded_tokens) {
    // Client derives a shared verification key from the unblinded token.
    VerificationKey verification_key = unblinded_token.DeriveVerificationKey();

    // Client signs the message using the shared verification key and sends it
    // to the server as a `signature` in the credential.
    const base::expected<VerificationSignature, std::string>
        verification_signature = verification_key.Sign(kMessage);
    EXPECT_TRUE(verification_signature.has_value());

    // Client decodes the token preimage from the unblinded token and sends it
    // to the server as `t` in the credential.
    const TokenPreimage token_preimage = unblinded_token.Preimage();

    // Server rederives the unblinded token using the server signing key and the
    // token preimage.
    const UnblindedToken rederived_unblinded_token =
        signing_key.RederiveUnblindedToken(token_preimage);

    // Server derives the shared verification key from the unblinded token.
    VerificationKey shared_verification_key =
        rederived_unblinded_token.DeriveVerificationKey();

    // Server proves and verifies the message using the verification signature.
    const base::expected<bool, std::string> server_verification_result =
        shared_verification_key.Verify(*verification_signature, kMessage);
    EXPECT_TRUE(server_verification_result.has_value());
    EXPECT_TRUE(*server_verification_result);
  }
}

}  // namespace challenge_bypass_ristretto
