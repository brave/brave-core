/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration_confirmations_json_parser.h"

#include <optional>
#include <string_view>

#include "base/json/json_reader.h"
#include "base/json/values_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_type.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"

namespace brave_ads::json::reader {

namespace {

constexpr char kConfirmationRewardPaymentTokenKey[] = "payment_token";
constexpr char kConfirmationRewardBlindedPaymentTokenKey[] =
    "blinded_payment_token";
constexpr char kConfirmationRewardTokenInfoKey[] = "token_info";
constexpr char kConfirmationRewardUnblindedTokenKey[] = "unblinded_token";
constexpr char kConfirmationRewardPublicKeyKey[] = "public_key";
constexpr char kConfirmationRewardSignatureKey[] = "signature";
constexpr char kConfirmationRewardCredentialKey[] = "credential";

constexpr char kConfirmationsKey[] = "confirmations";
constexpr char kConfirmationQueueListKey[] = "queue";
constexpr char kConfirmationTransactionIdKey[] = "transaction_id";
constexpr char kConfirmationCreativeInstanceIdKey[] = "creative_instance_id";
constexpr char kConfirmationTypeKey[] = "type";
constexpr char kConfirmationAdTypeKey[] = "ad_type";
constexpr char kConfirmationCreatedAtKey[] = "created_at";
constexpr char kConfirmationUserDataKey[] = "user_data";

// Legacy ad types that were removed and must be dropped on migration.
constexpr char kLegacyInlineContentAdType[] = "inline_content_ad";
constexpr char kLegacyPromotedContentAdType[] = "promoted_content_ad";

std::optional<RewardInfo> ParseConfirmationReward(const base::DictValue& dict) {
  RewardInfo reward;

  const auto* const token = dict.FindString(kConfirmationRewardPaymentTokenKey);
  if (!token) {
    return std::nullopt;
  }
  reward.token = cbr::Token(*token);

  const auto* const blinded_token =
      dict.FindString(kConfirmationRewardBlindedPaymentTokenKey);
  if (!blinded_token) {
    return std::nullopt;
  }
  reward.blinded_token = cbr::BlindedToken(*blinded_token);

  const auto* const unblinded_token_dict =
      dict.FindDict(kConfirmationRewardTokenInfoKey);
  if (unblinded_token_dict) {
    const auto* const unblinded_token =
        unblinded_token_dict->FindString(kConfirmationRewardUnblindedTokenKey);
    if (!unblinded_token) {
      return std::nullopt;
    }
    reward.unblinded_token = cbr::UnblindedToken(*unblinded_token);

    const auto* const public_key =
        unblinded_token_dict->FindString(kConfirmationRewardPublicKeyKey);
    if (!public_key) {
      return std::nullopt;
    }
    reward.public_key = cbr::PublicKey(*public_key);

    const auto* const signature =
        unblinded_token_dict->FindString(kConfirmationRewardSignatureKey);
    if (!signature) {
      return std::nullopt;
    }
    reward.signature = *signature;
  }

  const auto* const credential =
      dict.FindString(kConfirmationRewardCredentialKey);
  if (!credential) {
    return std::nullopt;
  }
  reward.credential_base64url = *credential;

  return reward;
}

std::optional<ConfirmationInfo> ParseConfirmation(const base::DictValue& dict) {
  ConfirmationInfo confirmation;

  const auto* const transaction_id =
      dict.FindString(kConfirmationTransactionIdKey);
  if (!transaction_id) {
    return std::nullopt;
  }
  confirmation.transaction_id = *transaction_id;

  const auto* const creative_instance_id =
      dict.FindString(kConfirmationCreativeInstanceIdKey);
  if (!creative_instance_id) {
    return std::nullopt;
  }
  confirmation.creative_instance_id = *creative_instance_id;

  const auto* const type = dict.FindString(kConfirmationTypeKey);
  if (!type) {
    return std::nullopt;
  }
  confirmation.type = ToMojomConfirmationType(*type);

  const auto* const ad_type = dict.FindString(kConfirmationAdTypeKey);
  if (!ad_type) {
    return std::nullopt;
  }
  if (*ad_type == kLegacyInlineContentAdType ||
      *ad_type == kLegacyPromotedContentAdType) {
    return std::nullopt;
  }
  confirmation.ad_type = ToMojomAdType(*ad_type);

  const auto* const created_at = dict.Find(kConfirmationCreatedAtKey);
  if (!created_at) {
    return std::nullopt;
  }
  confirmation.created_at =
      base::ValueToTime(created_at).value_or(base::Time());

  const auto* const user_data = dict.FindDict(kConfirmationUserDataKey);
  if (!user_data) {
    return std::nullopt;
  }
  confirmation.user_data.fixed = user_data->Clone();

  confirmation.reward = ParseConfirmationReward(dict);

  if (!IsValid(confirmation)) {
    return std::nullopt;
  }

  return confirmation;
}

std::optional<ConfirmationList> ParseConfirmationsFromDict(
    const base::DictValue& dict) {
  const auto* const confirmations_dict = dict.FindDict(kConfirmationsKey);
  if (!confirmations_dict) {
    return std::nullopt;
  }

  const auto* const list =
      confirmations_dict->FindList(kConfirmationQueueListKey);
  if (!list) {
    return std::nullopt;
  }

  ConfirmationList confirmations;
  confirmations.reserve(list->size());

  for (const auto& value : *list) {
    const auto* const confirmation_dict = value.GetIfDict();
    if (!confirmation_dict) {
      continue;
    }

    if (std::optional<ConfirmationInfo> confirmation =
            ParseConfirmation(*confirmation_dict)) {
      confirmations.push_back(*confirmation);
    }
  }

  return confirmations;
}

}  // namespace

std::optional<ConfirmationList> ParseConfirmations(std::string_view json) {
  std::optional<base::DictValue> dict =
      base::JSONReader::ReadDict(json, base::JSON_PARSE_RFC);
  if (!dict) {
    return std::nullopt;
  }

  return ParseConfirmationsFromDict(*dict);
}

}  // namespace brave_ads::json::reader
