/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_unblinded_payment_tokens_json_reader_util.h"

#include <string>

#include "base/values.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace rewards {
namespace JSONReader {

namespace {

constexpr char kUnblindedPaymentTokenListKey[] = "unblinded_payment_tokens";
constexpr char kPublicKeyKey[] = "public_key";
constexpr char kUnblindedTokenKey[] = "unblinded_token";

absl::optional<privacy::UnblindedPaymentTokenInfo> ParseUnblindedPaymentToken(
    const base::Value& value) {
  privacy::UnblindedPaymentTokenInfo unblinded_payment_token;

  // Public key
  const std::string* const public_key = value.FindStringKey(kPublicKeyKey);
  if (!public_key) {
    return absl::nullopt;
  }
  unblinded_payment_token.public_key = privacy::cbr::PublicKey(*public_key);
  if (!unblinded_payment_token.public_key.has_value()) {
    return absl::nullopt;
  }

  // Unblinded token
  const std::string* unblinded_token = value.FindStringKey(kUnblindedTokenKey);
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
GetUnblindedPaymentTokensFromList(const base::Value& value) {
  if (!value.is_list()) {
    return absl::nullopt;
  }

  privacy::UnblindedPaymentTokenList unblinded_payment_tokens;

  for (const auto& unblinded_payment_token_value : value.GetList()) {
    if (!unblinded_payment_token_value.is_dict()) {
      return absl::nullopt;
    }

    const absl::optional<privacy::UnblindedPaymentTokenInfo>&
        unblinded_payment_token_optional =
            ParseUnblindedPaymentToken(unblinded_payment_token_value);
    if (!unblinded_payment_token_optional) {
      return absl::nullopt;
    }
    const privacy::UnblindedPaymentTokenInfo& unblinded_payment_token =
        unblinded_payment_token_optional.value();

    unblinded_payment_tokens.push_back(unblinded_payment_token);
  }

  return unblinded_payment_tokens;
}

}  // namespace

absl::optional<privacy::UnblindedPaymentTokenList> ParseUnblindedPaymentTokens(
    const base::Value& value) {
  const base::Value* const unblinded_payment_tokens_value =
      value.FindListKey(kUnblindedPaymentTokenListKey);
  if (!unblinded_payment_tokens_value) {
    return absl::nullopt;
  }

  const absl::optional<privacy::UnblindedPaymentTokenList>&
      unblinded_payment_tokens_optional =
          GetUnblindedPaymentTokensFromList(*unblinded_payment_tokens_value);
  if (!unblinded_payment_tokens_optional) {
    return absl::nullopt;
  }

  return unblinded_payment_tokens_optional.value();
}

}  // namespace JSONReader
}  // namespace rewards
}  // namespace ads
