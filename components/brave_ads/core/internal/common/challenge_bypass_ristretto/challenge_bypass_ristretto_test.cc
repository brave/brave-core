/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>
#include <string>

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
  std::optional<cbr::BlindedToken> blinded_token = token.Blind();
  EXPECT_TRUE(blinded_token);

  // The server generates a random signing key.
  cbr::SigningKey signing_key;
  EXPECT_TRUE(signing_key.has_value());

  // The server signs the blinded token using its signing key. This signature
  // proves the server’s endorsement of the token.
  std::optional<cbr::SignedToken> signed_token =
      signing_key.Sign(*blinded_token);
  EXPECT_TRUE(signed_token);

  // The client verifies the batch DLEQ proof using the public key provided by
  // the server. This step confirms that the signatures are valid and correspond
  // to the public key.
  std::optional<cbr::PublicKey> public_key = signing_key.GetPublicKey();
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
  std::optional<cbr::UnblindedTokenList> unblinded_tokens =
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
    std::optional<cbr::VerificationSignature> verification_signature =
        shared_verification_key->Sign(kMessage);
    EXPECT_TRUE(verification_signature);

    // The client decodes the token preimage from the unblinded token and sends
    // it to the server as `t` in the credential.
    std::optional<cbr::TokenPreimage> token_preimage =
        unblinded_token.GetTokenPreimage();
    EXPECT_TRUE(token_preimage);

    // The server rederives the unblinded token using the server signing key and
    // the token preimage.
    std::optional<cbr::UnblindedToken> rederived_unblinded_token =
        signing_key.RederiveUnblindedToken(*token_preimage);
    EXPECT_TRUE(rederived_unblinded_token);

    // The server derives the shared verification key from the unblinded token.
    std::optional<cbr::VerificationKey> rederived_shared_verification_key =
        rederived_unblinded_token->DeriveVerificationKey();
    EXPECT_TRUE(rederived_shared_verification_key);

    // The server verifies the client's signature with the rederived key. This
    // succeeds only if the client's blinding and both derivations agree, i.e.
    // the token passes server-side validation.
    EXPECT_TRUE(rederived_shared_verification_key->Verify(
        *verification_signature, kMessage));

    // Deriving a verification key from this unblinded token selects the
    // derivation with the `rfc` flag. Reconstruct the same token bytes both
    // ways to show the same token validates only when the key is generated with
    // the matching (RFC 9497) derivation.
    std::optional<std::string> unblinded_token_base64 =
        unblinded_token.EncodeBase64();
    ASSERT_TRUE(unblinded_token_base64);

    // Correct derivation (RFC 9497 finalization, matching how the token was
    // blinded): the same token still validates.
    std::optional<cbr::VerificationKey> correct_verification_key =
        cbr::UnblindedToken(*unblinded_token_base64, /*rfc=*/true)
            .DeriveVerificationKey();
    ASSERT_TRUE(correct_verification_key);
    std::optional<cbr::VerificationSignature> correct_verification_signature =
        correct_verification_key->Sign(kMessage);
    ASSERT_TRUE(correct_verification_signature);
    EXPECT_TRUE(rederived_shared_verification_key->Verify(
        *correct_verification_signature, kMessage));

    // Wrong derivation (legacy finalization on the RFC unblinded point): the
    // signature pairs with neither server verification method.
    std::optional<cbr::VerificationKey> wrong_verification_key =
        cbr::UnblindedToken(*unblinded_token_base64, /*rfc=*/false)
            .DeriveVerificationKey();
    ASSERT_TRUE(wrong_verification_key);
    std::optional<cbr::VerificationSignature> wrong_verification_signature =
        wrong_verification_key->Sign(kMessage);
    ASSERT_TRUE(wrong_verification_signature);

    // The RFC 9497 verification method (RFC rederive + RFC finalize) rejects
    // it: same unblinded point, different finalization.
    EXPECT_FALSE(rederived_shared_verification_key->Verify(
        *wrong_verification_signature, kMessage));

    // The legacy verification method (legacy rederive + legacy finalize)
    // rejects it too: the legacy rederivation yields a different unblinded
    // point, so even the matching finalization does not agree.
    std::optional<cbr::UnblindedToken> legacy_rederived_unblinded_token =
        signing_key.RederiveUnblindedTokenDeprecated(*token_preimage);
    ASSERT_TRUE(legacy_rederived_unblinded_token);
    std::optional<std::string> legacy_rederived_unblinded_token_base64 =
        legacy_rederived_unblinded_token->EncodeBase64();
    ASSERT_TRUE(legacy_rederived_unblinded_token_base64);
    std::optional<cbr::VerificationKey> legacy_shared_verification_key =
        cbr::UnblindedToken(*legacy_rederived_unblinded_token_base64,
                            /*rfc=*/false)
            .DeriveVerificationKey();
    ASSERT_TRUE(legacy_shared_verification_key);
    EXPECT_FALSE(legacy_shared_verification_key->Verify(
        *wrong_verification_signature, kMessage));
  }
}

// Legacy (non-RFC) counterpart of ProveAndVerifyUnblindedToken. The browser
// blinds with the RFC 9497 derivation, so a legacy unblinded token is obtained
// the way a pre-existing stored token is handled: by rederiving with the legacy
// derivation. The token validates when its verification key uses the legacy
// finalization, and fails under both verification methods when the key is built
// with the RFC 9497 finalization instead.
TEST(BraveAdsChallengeBypassRistrettoTest, ProveAndVerifyUnblindedTokenNonRfc) {
  cbr::SigningKey signing_key;
  ASSERT_TRUE(signing_key.has_value());

  // Obtain a genuine token preimage via the normal issuance flow.
  cbr::Token token;
  ASSERT_TRUE(token.has_value());
  std::optional<cbr::BlindedToken> blinded_token = token.Blind();
  ASSERT_TRUE(blinded_token);
  std::optional<cbr::SignedToken> signed_token =
      signing_key.Sign(*blinded_token);
  ASSERT_TRUE(signed_token);
  std::optional<cbr::PublicKey> public_key = signing_key.GetPublicKey();
  ASSERT_TRUE(public_key);
  const cbr::BlindedTokenList blinded_tokens = {*blinded_token};
  const cbr::SignedTokenList signed_tokens = {*signed_token};
  cbr::BatchDLEQProof batch_dleq_proof(blinded_tokens, signed_tokens,
                                       signing_key);
  ASSERT_TRUE(batch_dleq_proof.has_value());
  const cbr::TokenList tokens = {token};
  std::optional<cbr::UnblindedTokenList> unblinded_tokens =
      batch_dleq_proof.VerifyAndUnblind(tokens, blinded_tokens, signed_tokens,
                                        *public_key);
  ASSERT_TRUE(unblinded_tokens);
  ASSERT_FALSE(unblinded_tokens->empty());
  std::optional<cbr::TokenPreimage> token_preimage =
      unblinded_tokens->front().GetTokenPreimage();
  ASSERT_TRUE(token_preimage);

  // The legacy (non-RFC) unblinded token, whose point uses the legacy
  // derivation: W = k * legacy_hash_to_group(preimage).
  std::optional<cbr::UnblindedToken> legacy_unblinded_token =
      signing_key.RederiveUnblindedTokenDeprecated(*token_preimage);
  ASSERT_TRUE(legacy_unblinded_token);
  std::optional<std::string> legacy_unblinded_token_base64 =
      legacy_unblinded_token->EncodeBase64();
  ASSERT_TRUE(legacy_unblinded_token_base64);

  // The server's legacy verification method (legacy rederive + legacy finalize).
  std::optional<cbr::VerificationKey> legacy_shared_verification_key =
      cbr::UnblindedToken(*legacy_unblinded_token_base64, /*rfc=*/false)
          .DeriveVerificationKey();
  ASSERT_TRUE(legacy_shared_verification_key);

  // The server's RFC verification method (RFC rederive + RFC finalize) yields a
  // different unblinded point.
  std::optional<cbr::UnblindedToken> rfc_rederived_unblinded_token =
      signing_key.RederiveUnblindedToken(*token_preimage);
  ASSERT_TRUE(rfc_rederived_unblinded_token);
  std::optional<cbr::VerificationKey> rfc_shared_verification_key =
      rfc_rederived_unblinded_token->DeriveVerificationKey();
  ASSERT_TRUE(rfc_shared_verification_key);

  // Correct derivation (legacy finalization, matching the legacy point): the
  // same token validates.
  std::optional<cbr::VerificationKey> correct_verification_key =
      cbr::UnblindedToken(*legacy_unblinded_token_base64, /*rfc=*/false)
          .DeriveVerificationKey();
  ASSERT_TRUE(correct_verification_key);
  std::optional<cbr::VerificationSignature> correct_verification_signature =
      correct_verification_key->Sign(kMessage);
  ASSERT_TRUE(correct_verification_signature);
  EXPECT_TRUE(legacy_shared_verification_key->Verify(
      *correct_verification_signature, kMessage));

  // Wrong derivation (RFC finalization on the legacy unblinded point): the
  // signature pairs with neither verification method.
  std::optional<cbr::VerificationKey> wrong_verification_key =
      cbr::UnblindedToken(*legacy_unblinded_token_base64, /*rfc=*/true)
          .DeriveVerificationKey();
  ASSERT_TRUE(wrong_verification_key);
  std::optional<cbr::VerificationSignature> wrong_verification_signature =
      wrong_verification_key->Sign(kMessage);
  ASSERT_TRUE(wrong_verification_signature);
  EXPECT_FALSE(legacy_shared_verification_key->Verify(
      *wrong_verification_signature, kMessage));
  EXPECT_FALSE(rfc_shared_verification_key->Verify(
      *wrong_verification_signature, kMessage));
}

}  // namespace brave_ads
