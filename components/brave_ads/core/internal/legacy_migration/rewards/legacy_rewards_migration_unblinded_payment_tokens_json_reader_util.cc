/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/rewards/legacy_rewards_migration_unblinded_payment_tokens_json_reader_util.h"

#include <string>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"

namespace brave_ads::rewards::json::reader {

namespace {

constexpr char kUnblindedPaymentTokenListKey[] = "unblinded_payment_tokens";
constexpr char kPublicKeyKey[] = "public_key";
constexpr char kUnblindedTokenKey[] = "unblinded_token";

absl::optional<privacy::UnblindedPaymentTokenInfo> ParseUnblindedPaymentToken(
    const base::Value::Dict& dict) {
  privacy::UnblindedPaymentTokenInfo unblinded_payment_token;

  // Public key
  const std::string* const public_key = dict.FindString(kPublicKeyKey);
  if (!public_key) {
    return absl::nullopt;
  }
  unblinded_payment_token.public_key = privacy::cbr::PublicKey(*public_key);
  if (!unblinded_payment_token.public_key.has_value()) {
    return absl::nullopt;
  }

  // Unblinded token
  const std::string* const unblinded_token =
      dict.FindString(kUnblindedTokenKey);
  if (!unblinded_token) {
    return absl::nullopt;
  }
  unblinded_payment_token.value =
      privacy::cbr::UnblindedToken(*unblinded_token);
  if (!unblinded_payment_token.value.has_value()) {
    return absl::nullopt;
  }

  return unblinded_payment_token;
}

absl::optional<privacy::UnblindedPaymentTokenList>
GetUnblindedPaymentTokensFromList(const base::Value::List& list) {
  privacy::UnblindedPaymentTokenList unblinded_payment_tokens;

  for (const auto& item : list) {
    const auto* item_dict = item.GetIfDict();
    if (!item_dict) {
      return absl::nullopt;
    }

    const absl::optional<privacy::UnblindedPaymentTokenInfo>
        unblinded_payment_token = ParseUnblindedPaymentToken(*item_dict);
    if (!unblinded_payment_token) {
      return absl::nullopt;
    }

    unblinded_payment_tokens.push_back(*unblinded_payment_token);
  }

  return unblinded_payment_tokens;
}

}  // namespace

absl::optional<privacy::UnblindedPaymentTokenList> ParseUnblindedPaymentTokens(
    const base::Value::Dict& dict) {
  const auto* const list = dict.FindList(kUnblindedPaymentTokenListKey);
  if (!list) {
    return privacy::UnblindedPaymentTokenList{};
  }

  return GetUnblindedPaymentTokensFromList(*list);
}

}  // namespace brave_ads::rewards::json::reader
