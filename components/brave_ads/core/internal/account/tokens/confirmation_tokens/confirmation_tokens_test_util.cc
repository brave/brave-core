/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_test_util.h"

#include <cstddef>
#include <optional>
#include <string>

#include "base/check.h"
#include "base/check_op.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_util.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_test_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/batch_dleq_proof.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signing_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signing_key_test_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token_test_util.h"
#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"

namespace brave_ads::test {

namespace {

std::optional<cbr::UnblindedTokenList> GenerateUnblindedTokens(size_t count) {
  // The server generates a random signing key.
  cbr::SigningKey signing_key = cbr::test::GetSigningKey();
  std::optional<cbr::PublicKey> public_key = signing_key.GetPublicKey();
  CHECK(public_key);

  // The client generates random tokens.
  cbr::TokenList tokens(count);

  // The client blinds the tokens using a blinding scalar. Blinding ensures that
  // the tokens are not recognizable or linkable to the original value until
  // they are unblinded.
  cbr::BlindedTokenList blinded_tokens;
  for (auto& token : tokens) {
    std::optional<cbr::BlindedToken> blinded_token = token.Blind();
    CHECK(blinded_token);
    blinded_tokens.push_back(*blinded_token);
  }

  // The server signs the blinded tokens using its signing key. This signature
  // proves the server’s endorsement of the tokens.
  cbr::SignedTokenList signed_tokens;
  for (const auto& blinded_token : blinded_tokens) {
    std::optional<cbr::SignedToken> signed_token =
        signing_key.Sign(blinded_token);
    CHECK(signed_token);
    signed_tokens.push_back(*signed_token);
  }

  // The client verifies the batch DLEQ proof using the public key provided by
  // the server. This step confirms that the signatures are valid and correspond
  // to the public key.
  cbr::BatchDLEQProof batch_dleq_proof(blinded_tokens, signed_tokens,
                                       signing_key);
  CHECK(batch_dleq_proof.has_value());
  CHECK(batch_dleq_proof.Verify(blinded_tokens, signed_tokens, *public_key));

  // The client unblinds the signed tokens using the blinding scalar.
  return batch_dleq_proof.VerifyAndUnblind(tokens, blinded_tokens,
                                           signed_tokens, *public_key);
}

ConfirmationTokenInfo BuildConfirmationToken(
    const cbr::UnblindedToken& unblinded_token,
    const WalletInfo& wallet) {
  ConfirmationTokenInfo confirmation_token;

  confirmation_token.unblinded_token = unblinded_token;
  std::optional<std::string> unblinded_token_base64 =
      unblinded_token.EncodeBase64();
  CHECK(unblinded_token_base64);

  confirmation_token.public_key =
      cbr::PublicKey("OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM=");

  std::optional<std::string> signature_base64 =
      crypto::Sign(*unblinded_token_base64, wallet.secret_key_base64);
  CHECK(signature_base64);
  confirmation_token.signature_base64 = *signature_base64;

  CHECK(IsValid(confirmation_token));

  return confirmation_token;
}

}  // namespace

ConfirmationTokenList RefillRandomConfirmationTokens(size_t count) {
  CHECK_GT(count, 0U);

  std::optional<cbr::UnblindedTokenList> unblinded_tokens =
      GenerateUnblindedTokens(count);

  const WalletInfo wallet = Wallet();

  ConfirmationTokenList confirmation_tokens;
  for (const auto& unblinded_token : *unblinded_tokens) {
    const ConfirmationTokenInfo confirmation_token =
        BuildConfirmationToken(unblinded_token, wallet);
    confirmation_tokens.push_back(confirmation_token);
  }

  GetConfirmationTokens().Add(confirmation_tokens);

  return confirmation_tokens;
}

ConfirmationTokenList RefillConfirmationTokens(size_t count) {
  CHECK_GT(count, 0U);

  ConfirmationTokenList confirmation_tokens = BuildConfirmationTokens(count);
  GetConfirmationTokens().Add(confirmation_tokens);

  return confirmation_tokens;
}

ConfirmationTokenInfo BuildConfirmationToken() {
  const cbr::UnblindedToken unblinded_token =
      cbr::test::UnblindedTokens().front();

  const WalletInfo wallet = Wallet();

  return BuildConfirmationToken(unblinded_token, wallet);
}

ConfirmationTokenList BuildConfirmationTokens(size_t count) {
  CHECK_GT(count, 0U);
  CHECK_LE(count, 50U);

  const WalletInfo wallet = Wallet();

  ConfirmationTokenList confirmation_tokens;

  for (size_t i = 0; i < count; ++i) {
    const cbr::UnblindedToken unblinded_token =
        cbr::test::UnblindedTokens().at(i);

    const ConfirmationTokenInfo confirmation_token =
        BuildConfirmationToken(unblinded_token, wallet);
    confirmation_tokens.push_back(confirmation_token);
  }

  return confirmation_tokens;
}

}  // namespace brave_ads::test
