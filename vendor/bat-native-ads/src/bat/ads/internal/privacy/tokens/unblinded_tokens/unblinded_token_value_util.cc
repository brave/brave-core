/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_value_util.h"

#include <string>
#include <utility>

#include "absl/types/optional.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"

namespace ads::privacy {

namespace {

constexpr char kUnblindedTokenKey[] = "unblinded_token";
constexpr char kPublicKey[] = "public_key";
constexpr char kSignature[] = "signature";

}  // namespace

base::Value::List UnblindedTokensToValue(
    const UnblindedTokenList& unblinded_tokens) {
  base::Value::List list;

  for (const auto& unblinded_token : unblinded_tokens) {
    const absl::optional<std::string> unblinded_token_base64 =
        unblinded_token.value.EncodeBase64();
    if (!unblinded_token_base64) {
      continue;
    }

    const absl::optional<std::string> public_key_base64 =
        unblinded_token.public_key.EncodeBase64();
    if (!public_key_base64) {
      continue;
    }

    base::Value::Dict dict;
    dict.Set(kUnblindedTokenKey, *unblinded_token_base64);
    dict.Set(kPublicKey, *public_key_base64);
    dict.Set(kSignature, unblinded_token.signature);
    list.Append(std::move(dict));
  }

  return list;
}

UnblindedTokenList UnblindedTokensFromValue(const base::Value::List& list) {
  UnblindedTokenList unblinded_tokens;

  for (const auto& item : list) {
    const base::Value::Dict* const dict = item.GetIfDict();
    if (!dict) {
      BLOG(0, "Unblinded token should be a dictionary");
      continue;
    }

    UnblindedTokenInfo unblinded_token;

    // Unblinded token
    if (const std::string* const value = dict->FindString(kUnblindedTokenKey)) {
      unblinded_token.value = cbr::UnblindedToken(*value);
      if (!unblinded_token.value.has_value()) {
        BLOG(0, "Invalid unblinded token");
        continue;
      }
    } else {
      BLOG(0, "Missing unblinded token");
      continue;
    }

    // Public key
    if (const std::string* const value = dict->FindString(kPublicKey)) {
      unblinded_token.public_key = cbr::PublicKey(*value);
      if (!unblinded_token.public_key.has_value()) {
        BLOG(0, "Invalid unblinded token public key");
        continue;
      }
    } else {
      BLOG(0, "Missing unblinded token public key");
      continue;
    }

    // Signature
    if (const std::string* const value = dict->FindString(kSignature)) {
      unblinded_token.signature = *value;
    } else {
      BLOG(0, "Missing unblinded token signature");
      continue;
    }

    unblinded_tokens.push_back(unblinded_token);
  }

  return unblinded_tokens;
}

}  // namespace ads::privacy
