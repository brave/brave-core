/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_value_util.h"

#include <optional>
#include <string>

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"

namespace brave_ads {

namespace {

constexpr char kUnblindedTokenKey[] = "unblinded_token";
constexpr char kPublicKey[] = "public_key";
constexpr char kSignature[] = "signature";

}  // namespace

base::Value::List ConfirmationTokensToValue(
    const ConfirmationTokenList& confirmation_tokens) {
  base::Value::List list;

  for (const auto& confirmation_token : confirmation_tokens) {
    const std::optional<std::string> unblinded_token_base64 =
        confirmation_token.unblinded_token.EncodeBase64();
    if (!unblinded_token_base64) {
      continue;
    }

    const std::optional<std::string> public_key_base64 =
        confirmation_token.public_key.EncodeBase64();
    if (!public_key_base64) {
      continue;
    }

    list.Append(base::Value::Dict()
                    .Set(kUnblindedTokenKey, *unblinded_token_base64)
                    .Set(kPublicKey, *public_key_base64)
                    .Set(kSignature, confirmation_token.signature_base64));
  }

  return list;
}

ConfirmationTokenList ConfirmationTokensFromValue(
    const base::Value::List& list) {
  ConfirmationTokenList confirmation_tokens;
  confirmation_tokens.reserve(list.size());

  for (const auto& value : list) {
    const auto* const dict = value.GetIfDict();
    if (!dict) {
      BLOG(0, "Confirmation token should be a dictionary");
      continue;
    }

    ConfirmationTokenInfo confirmation_token;

    // Unblinded token
    if (const auto* const unblinded_token =
            dict->FindString(kUnblindedTokenKey)) {
      confirmation_token.unblinded_token =
          cbr::UnblindedToken(*unblinded_token);
      if (!confirmation_token.unblinded_token.has_value()) {
        BLOG(0, "Invalid confirmation unblinded token");
        continue;
      }
    } else {
      BLOG(0, "Missing confirmation unblinded token");
      continue;
    }

    // Public key
    if (const auto* const public_key = dict->FindString(kPublicKey)) {
      confirmation_token.public_key = cbr::PublicKey(*public_key);
      if (!confirmation_token.public_key.has_value()) {
        BLOG(0, "Invalid confirmation token public key");
        continue;
      }
    } else {
      BLOG(0, "Missing confirmation token public key");
      continue;
    }

    // Signature
    if (const auto* const signature_base64 = dict->FindString(kSignature)) {
      confirmation_token.signature_base64 = *signature_base64;
    } else {
      BLOG(0, "Missing confirmation token signature");
      continue;
    }

    confirmation_tokens.push_back(confirmation_token);
  }

  return confirmation_tokens;
}

}  // namespace brave_ads
