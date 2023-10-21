/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_value_util.h"

#include <string>

#include "base/uuid.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

namespace {

constexpr char kTransactionIdKey[] = "transaction_id";
constexpr char kUnblindedTokenKey[] = "unblinded_token";
constexpr char kPublicKey[] = "public_key";
constexpr char kConfirmationTypeKey[] = "confirmation_type";
constexpr char kAdTypeKey[] = "ad_type";

}  // namespace

base::Value::List PaymentTokensToValue(const PaymentTokenList& payment_tokens) {
  base::Value::List list;

  for (const auto& payment_token : payment_tokens) {
    const absl::optional<std::string> unblinded_token_base64 =
        payment_token.unblinded_token.EncodeBase64();
    if (!unblinded_token_base64) {
      continue;
    }

    const absl::optional<std::string> public_key_base64 =
        payment_token.public_key.EncodeBase64();
    if (!public_key_base64) {
      continue;
    }

    list.Append(base::Value::Dict()
                    .Set(kTransactionIdKey, payment_token.transaction_id)
                    .Set(kUnblindedTokenKey, *unblinded_token_base64)
                    .Set(kPublicKey, *public_key_base64)
                    .Set(kConfirmationTypeKey,
                         payment_token.confirmation_type.ToString())
                    .Set(kAdTypeKey, payment_token.ad_type.ToString()));
  }

  return list;
}

PaymentTokenList PaymentTokensFromValue(const base::Value::List& list) {
  PaymentTokenList payment_tokens;

  for (const auto& item : list) {
    const auto* const item_dict = item.GetIfDict();
    if (!item_dict) {
      BLOG(0, "Payment token should be a dictionary");
      continue;
    }

    PaymentTokenInfo payment_token;

    // Transaction id
    if (const auto* const value = item_dict->FindString(kTransactionIdKey)) {
      payment_token.transaction_id = *value;
    } else {
      // Migrate legacy confirmations
      payment_token.transaction_id =
          base::Uuid::GenerateRandomV4().AsLowercaseString();
    }

    // Unblinded token
    if (const auto* const value = item_dict->FindString(kUnblindedTokenKey)) {
      payment_token.unblinded_token = cbr::UnblindedToken(*value);
      if (!payment_token.unblinded_token.has_value()) {
        BLOG(0, "Invalid unblinded token");
        continue;
      }
    } else {
      BLOG(0, "Missing unblinded token");
      continue;
    }

    // Public key
    if (const auto* const value = item_dict->FindString(kPublicKey)) {
      payment_token.public_key = cbr::PublicKey(*value);
      if (!payment_token.public_key.has_value()) {
        BLOG(0, "Invalid payment token public key");
        continue;
      }
    } else {
      BLOG(0, "Missing payment token public key");
      continue;
    }

    // Confirmation type
    if (const auto* const value = item_dict->FindString(kConfirmationTypeKey)) {
      payment_token.confirmation_type = ConfirmationType(*value);
    }

    // Ad type
    if (const auto* const value = item_dict->FindString(kAdTypeKey)) {
      payment_token.ad_type = AdType(*value);
    }

    payment_tokens.push_back(payment_token);
  }

  return payment_tokens;
}

}  // namespace brave_ads
