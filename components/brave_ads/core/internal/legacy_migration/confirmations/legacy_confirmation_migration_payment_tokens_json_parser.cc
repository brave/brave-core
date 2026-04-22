/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration_payment_tokens_json_parser.h"

#include <optional>
#include <string_view>

#include "base/json/json_reader.h"
#include "base/uuid.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_type.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"

namespace brave_ads::json::reader {

namespace {

constexpr char kUnblindedPaymentTokensKey[] = "unblinded_payment_tokens";
constexpr char kTransactionIdKey[] = "transaction_id";
constexpr char kUnblindedTokenKey[] = "unblinded_token";
constexpr char kPublicKeyKey[] = "public_key";
constexpr char kConfirmationTypeKey[] = "confirmation_type";
constexpr char kAdTypeKey[] = "ad_type";

// Legacy ad types that were removed and must be dropped on migration.
constexpr char kLegacyInlineContentAdType[] = "inline_content_ad";
constexpr char kLegacyPromotedContentAdType[] = "promoted_content_ad";

PaymentTokenList ParsePaymentTokensFromList(const base::ListValue& list) {
  PaymentTokenList payment_tokens;

  for (const auto& value : list) {
    const auto* const dict = value.GetIfDict();
    if (!dict) {
      BLOG(0, "Payment token should be a dictionary");
      continue;
    }

    PaymentTokenInfo payment_token;

    const auto* const transaction_id = dict->FindString(kTransactionIdKey);
    if (transaction_id) {
      payment_token.transaction_id = *transaction_id;
    } else {
      // Assign a new id for payment tokens written before transaction ids were
      // stored in the JSON.
      payment_token.transaction_id =
          base::Uuid::GenerateRandomV4().AsLowercaseString();
    }

    const auto* const unblinded_token = dict->FindString(kUnblindedTokenKey);
    if (!unblinded_token) {
      BLOG(0, "Missing payment unblinded token");
      continue;
    }
    payment_token.unblinded_token = cbr::UnblindedToken(*unblinded_token);
    if (!payment_token.unblinded_token.has_value()) {
      BLOG(0, "Invalid payment unblinded token");
      continue;
    }

    const auto* const public_key = dict->FindString(kPublicKeyKey);
    if (!public_key) {
      BLOG(0, "Missing payment token public key");
      continue;
    }
    payment_token.public_key = cbr::PublicKey(*public_key);
    if (!payment_token.public_key.has_value()) {
      BLOG(0, "Invalid payment token public key");
      continue;
    }

    const auto* const confirmation_type =
        dict->FindString(kConfirmationTypeKey);
    if (confirmation_type) {
      payment_token.confirmation_type =
          ToMojomConfirmationType(*confirmation_type);
    }

    const auto* const ad_type = dict->FindString(kAdTypeKey);
    if (ad_type) {
      if (*ad_type == kLegacyInlineContentAdType ||
          *ad_type == kLegacyPromotedContentAdType) {
        BLOG(0, "Dropping legacy ad type payment token");
        continue;
      }
      payment_token.ad_type = ToMojomAdType(*ad_type);
    }

    if (!payment_token.IsValid()) {
      BLOG(0, "Invalid payment token");
      continue;
    }

    payment_tokens.push_back(payment_token);
  }

  return payment_tokens;
}

}  // namespace

std::optional<PaymentTokenList> ParsePaymentTokens(std::string_view json) {
  std::optional<base::DictValue> dict =
      base::JSONReader::ReadDict(json, base::JSON_PARSE_RFC);
  if (!dict) {
    return std::nullopt;
  }

  const auto* const list = dict->FindList(kUnblindedPaymentTokensKey);
  if (!list) {
    return std::nullopt;
  }

  return ParsePaymentTokensFromList(*list);
}

}  // namespace brave_ads::json::reader
