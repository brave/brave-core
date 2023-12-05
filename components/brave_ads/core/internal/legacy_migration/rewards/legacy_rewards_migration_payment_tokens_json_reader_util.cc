/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/rewards/legacy_rewards_migration_payment_tokens_json_reader_util.h"

#include <string>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"

namespace brave_ads::rewards::json::reader {

namespace {

constexpr char kPaymentTokenListKey[] = "unblinded_payment_tokens";
constexpr char kUnblindedTokenKey[] = "unblinded_token";
constexpr char kPublicKeyKey[] = "public_key";

std::optional<PaymentTokenInfo> ParsePaymentToken(
    const base::Value::Dict& dict) {
  PaymentTokenInfo payment_token;

  // Public key
  const std::string* const public_key = dict.FindString(kPublicKeyKey);
  if (!public_key) {
    return std::nullopt;
  }
  payment_token.public_key = cbr::PublicKey(*public_key);
  if (!payment_token.public_key.has_value()) {
    return std::nullopt;
  }

  // Unblinded token
  const std::string* const unblinded_token =
      dict.FindString(kUnblindedTokenKey);
  if (!unblinded_token) {
    return std::nullopt;
  }
  payment_token.unblinded_token = cbr::UnblindedToken(*unblinded_token);
  if (!payment_token.unblinded_token.has_value()) {
    return std::nullopt;
  }

  return payment_token;
}

std::optional<PaymentTokenList> GetPaymentTokensFromList(
    const base::Value::List& list) {
  PaymentTokenList payment_tokens;

  for (const auto& item : list) {
    const auto* item_dict = item.GetIfDict();
    if (!item_dict) {
      return std::nullopt;
    }

    const std::optional<PaymentTokenInfo> payment_token =
        ParsePaymentToken(*item_dict);
    if (!payment_token) {
      return std::nullopt;
    }

    payment_tokens.push_back(*payment_token);
  }

  return payment_tokens;
}

}  // namespace

std::optional<PaymentTokenList> ParsePaymentTokens(
    const base::Value::Dict& dict) {
  const auto* const list = dict.FindList(kPaymentTokenListKey);
  if (!list) {
    return PaymentTokenList{};
  }

  return GetPaymentTokensFromList(*list);
}

}  // namespace brave_ads::rewards::json::reader
