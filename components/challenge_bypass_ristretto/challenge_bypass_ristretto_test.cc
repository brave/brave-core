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
  // The client creates a random token.
  Token token = Token::Random();

  // The client blinds this token using a blinding scalar. Blinding ensures that
  // the token is not recognizable or linkable to the original value until it's
  // unblinded.
  const BlindedToken blinded_token = token.Blind();

  // The server generates a random signing key.
  SigningKey signing_key = SigningKey::Random();

  // The server signs the blinded token using its signing key. This signature
  // proves the server’s endorsement of the token.
  const base::expected<SignedToken, std::string> signed_token =
      signing_key.Sign(blinded_token);
  EXPECT_TRUE(signed_token.has_value());

  // The client verifies the batch DLEQ proof using the public key provided by
  // the server. This step confirms that the signatures are valid and correspond
  // to the public key.
  const PublicKey public_key = signing_key.GetPublicKey();

  const std::vector<BlindedToken> blinded_tokens = {blinded_token};
  const std::vector<SignedToken> signed_tokens = {*signed_token};
  base::expected<BatchDLEQProof, std::string> batch_dleq_proof =
      BatchDLEQProof::Create(blinded_tokens, signed_tokens, signing_key);
  EXPECT_TRUE(batch_dleq_proof.has_value());
  const base::expected<bool, std::string> batch_dleq_proof_verification_result =
      batch_dleq_proof->Verify(blinded_tokens, signed_tokens, public_key);
  EXPECT_TRUE(batch_dleq_proof_verification_result.has_value());
  EXPECT_TRUE(*batch_dleq_proof_verification_result);

  // The client unblinds the signed tokens using the blinding scalar.
  const std::vector<Token> tokens = {token};
  const base::expected<std::vector<UnblindedToken>, std::string>
      unblinded_tokens = batch_dleq_proof->VerifyAndUnblind(
          tokens, blinded_tokens, signed_tokens, public_key);
  EXPECT_TRUE(unblinded_tokens.has_value());

  // Unblinded tokens are redeemed with the server periodically.
  for (const auto& unblinded_token : *unblinded_tokens) {
    // The client derives a shared verification key from the unblinded token.
    VerificationKey shared_verification_key =
        unblinded_token.DeriveVerificationKey();

    // The client signs the message using the shared verification key and sends
    // it to the server as a `signature` in the credential.
    const base::expected<VerificationSignature, std::string>
        verification_signature = shared_verification_key.Sign(kMessage);
    EXPECT_TRUE(verification_signature.has_value());

    // The client decodes the token preimage from the unblinded token and sends
    // it to the server as `t` in the credential.
    const TokenPreimage token_preimage = unblinded_token.Preimage();

    // The server rederives the unblinded token using the server signing key and
    // the token preimage.
    const UnblindedToken rederived_unblinded_token =
        signing_key.RederiveUnblindedToken(token_preimage);

    // The server derives the shared verification key from the unblinded token.
    VerificationKey rederived_shared_verification_key =
        rederived_unblinded_token.DeriveVerificationKey();

    // The server proves and verifies the message using the verification key and
    // signature.
    const base::expected<bool, std::string>
        rederived_shared_verification_key_verification_result =
            rederived_shared_verification_key.Verify(*verification_signature,
                                                     kMessage);
    EXPECT_TRUE(
        rederived_shared_verification_key_verification_result.has_value());
    EXPECT_TRUE(*rederived_shared_verification_key_verification_result);
  }
}

}  // namespace challenge_bypass_ristretto
