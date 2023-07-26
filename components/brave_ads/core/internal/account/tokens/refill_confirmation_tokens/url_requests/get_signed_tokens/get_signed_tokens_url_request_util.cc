/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/refill_confirmation_tokens/url_requests/get_signed_tokens/get_signed_tokens_url_request_util.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuer_types.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/batch_dleq_proof.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/confirmation_tokens/confirmation_token_info.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/confirmation_tokens/confirmation_tokens_util.h"

namespace brave_ads {

namespace {

constexpr char kSignedTokensKey[] = "signedTokens";
constexpr char kCaptchaIdKey[] = "captcha_id";
constexpr char kBatchDLEQProofKey[] = "batchProof";
constexpr char kPublicKeyKey[] = "publicKey";

absl::optional<std::string> Sign(
    const privacy::cbr::UnblindedToken& unblinded_token,
    const WalletInfo& wallet) {
  const absl::optional<std::string> unblinded_token_base64 =
      unblinded_token.EncodeBase64();
  if (!unblinded_token_base64) {
    return absl::nullopt;
  }

  return crypto::Sign(*unblinded_token_base64, wallet.secret_key);
}

absl::optional<std::vector<privacy::cbr::SignedToken>> ParseSignedTokens(
    const base::Value::Dict& dict) {
  const base::Value::List* list = dict.FindList(kSignedTokensKey);
  if (!list) {
    return absl::nullopt;
  }

  std::vector<privacy::cbr::SignedToken> signed_tokens;

  for (const auto& item : *list) {
    const std::string* const signed_token_base64 = item.GetIfString();
    if (!signed_token_base64) {
      return absl::nullopt;
    }

    const privacy::cbr::SignedToken signed_token =
        privacy::cbr::SignedToken(*signed_token_base64);
    if (!signed_token.has_value()) {
      return absl::nullopt;
    }

    signed_tokens.push_back(signed_token);
  }

  return signed_tokens;
}

privacy::ConfirmationTokenList BuildConfirmationTokens(
    const std::vector<privacy::cbr::UnblindedToken>& unblinded_tokens,
    const privacy::cbr::PublicKey& public_key,
    const WalletInfo& wallet) {
  privacy::ConfirmationTokenList confirmation_tokens;

  for (const privacy::cbr::UnblindedToken& unblinded_token : unblinded_tokens) {
    const absl::optional<std::string> signature = Sign(unblinded_token, wallet);
    CHECK(signature);

    privacy::ConfirmationTokenInfo confirmation_token;
    confirmation_token.unblinded_token = unblinded_token;
    confirmation_token.public_key = public_key;
    confirmation_token.signature = *signature;
    CHECK(IsValid(confirmation_token));

    confirmation_tokens.push_back(confirmation_token);
  }

  return confirmation_tokens;
}

}  // namespace

absl::optional<std::string> ParseCaptchaId(const base::Value::Dict& dict) {
  const std::string* const captcha_id = dict.FindString(kCaptchaIdKey);
  if (!captcha_id || captcha_id->empty()) {
    return absl::nullopt;
  }

  return *captcha_id;
}

base::expected<std::tuple<std::vector<privacy::cbr::UnblindedToken>,
                          privacy::cbr::PublicKey>,
               std::string>
ParseAndUnblindSignedTokens(
    const base::Value::Dict& dict,
    const std::vector<privacy::cbr::Token>& tokens,
    const std::vector<privacy::cbr::BlindedToken>& blinded_tokens) {
  const std::string* const batch_dleq_proof_base64 =
      dict.FindString(kBatchDLEQProofKey);
  if (!batch_dleq_proof_base64) {
    return base::unexpected("Failed to parse batch DLEQ proof");
  }
  privacy::cbr::BatchDLEQProof batch_dleq_proof =
      privacy::cbr::BatchDLEQProof(*batch_dleq_proof_base64);
  if (!batch_dleq_proof.has_value()) {
    return base::unexpected("Invalid batch DLEQ proof");
  }

  const absl::optional<std::vector<privacy::cbr::SignedToken>> signed_tokens =
      ParseSignedTokens(dict);
  if (!signed_tokens) {
    return base::unexpected("Failed to parse signed tokens");
  }

  const std::string* const public_key_base64 = dict.FindString(kPublicKeyKey);
  if (!public_key_base64) {
    return base::unexpected("Failed to parse public key");
  }

  if (!PublicKeyExistsForIssuerType(IssuerType::kConfirmations,
                                    *public_key_base64)) {
    return base::unexpected("Missing public key");
  }

  const privacy::cbr::PublicKey public_key =
      privacy::cbr::PublicKey(*public_key_base64);
  if (!public_key.has_value()) {
    return base::unexpected("Invalid public key");
  }

  const absl::optional<std::vector<privacy::cbr::UnblindedToken>>
      unblinded_tokens = batch_dleq_proof.VerifyAndUnblind(
          tokens, blinded_tokens, *signed_tokens, public_key);
  if (!unblinded_tokens || unblinded_tokens->empty()) {
    return base::unexpected("Failed to verify and unblind tokens");
  }

  return std::make_tuple(*unblinded_tokens, public_key);
}

void BuildAndAddConfirmationTokens(
    const std::vector<privacy::cbr::UnblindedToken>& unblinded_tokens,
    const privacy::cbr::PublicKey& public_key,
    const WalletInfo& wallet) {
  const privacy::ConfirmationTokenList confirmation_tokens =
      BuildConfirmationTokens(unblinded_tokens, public_key, wallet);

  AddConfirmationTokens(confirmation_tokens);

  BLOG(1, "Added " << confirmation_tokens.size()
                   << " confirmation tokens, you now have "
                   << privacy::ConfirmationTokenCount()
                   << " confirmation tokens");
}

}  // namespace brave_ads
