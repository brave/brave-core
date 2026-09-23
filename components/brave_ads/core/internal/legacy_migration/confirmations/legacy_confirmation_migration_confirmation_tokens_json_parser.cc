/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration_confirmation_tokens_json_parser.h"

#include <optional>
#include <string>
#include <string_view>

#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"

namespace brave_ads::json::reader {

namespace {

constexpr char kUnblindedTokensKey[] = "unblinded_tokens";
constexpr char kUnblindedTokenKey[] = "unblinded_token";
constexpr char kPublicKeyKey[] = "public_key";
constexpr char kSignatureKey[] = "signature";

ConfirmationTokenList ParseConfirmationTokensFromList(
    const base::ListValue& list,
    const WalletInfo& wallet) {
  ConfirmationTokenList confirmation_tokens;
  confirmation_tokens.reserve(list.size());

  for (const auto& value : list) {
    const auto* const dict = value.GetIfDict();
    if (!dict) {
      BLOG(0, "Confirmation token should be a dictionary");
      continue;
    }

    ConfirmationTokenInfo confirmation_token;

    const auto* const unblinded_token = dict->FindString(kUnblindedTokenKey);
    if (!unblinded_token) {
      BLOG(0, "Missing confirmation unblinded token");
      continue;
    }
    confirmation_token.unblinded_token = cbr::UnblindedToken(*unblinded_token);
    if (!confirmation_token.unblinded_token.has_value()) {
      BLOG(0, "Invalid confirmation unblinded token");
      continue;
    }

    const auto* const public_key = dict->FindString(kPublicKeyKey);
    if (!public_key) {
      BLOG(0, "Missing confirmation token public key");
      continue;
    }
    confirmation_token.public_key = cbr::PublicKey(*public_key);
    if (!confirmation_token.public_key.has_value()) {
      BLOG(0, "Invalid confirmation token public key");
      continue;
    }

    const auto* const signature = dict->FindString(kSignatureKey);
    if (!signature) {
      BLOG(0, "Missing confirmation token signature");
      continue;
    }
    confirmation_token.signature_base64 = *signature;

    // The server will reject tokens with invalid signatures on redemption, but
    // drop them here too to avoid storing known-bad tokens.
    std::optional<std::string> unblinded_token_base64 =
        confirmation_token.unblinded_token.EncodeBase64();
    if (!unblinded_token_base64 ||
        !crypto::Verify(*unblinded_token_base64, wallet.public_key_base64,
                        confirmation_token.signature_base64)) {
      BLOG(1, "Dropping confirmation token with invalid signature");
      continue;
    }

    if (!confirmation_token.IsValid()) {
      BLOG(0, "Invalid confirmation token");
      continue;
    }

    confirmation_tokens.push_back(confirmation_token);
  }

  return confirmation_tokens;
}

}  // namespace

std::optional<ConfirmationTokenList> ParseConfirmationTokens(
    std::string_view json,
    const WalletInfo& wallet) {
  std::optional<base::DictValue> dict =
      base::JSONReader::ReadDict(json, base::JSON_PARSE_RFC);
  if (!dict) {
    return std::nullopt;
  }

  const auto* const list = dict->FindList(kUnblindedTokensKey);
  if (!list) {
    return std::nullopt;
  }

  return ParseConfirmationTokensFromList(*list, wallet);
}

}  // namespace brave_ads::json::reader
