/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration_confirmations_json_reader_util.h"

#include "base/json/values_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

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

std::optional<RewardInfo> ParseConfirmationReward(
    const base::Value::Dict& dict) {
  RewardInfo reward;

  // Token
  if (const auto* const value =
          dict.FindString(kConfirmationRewardPaymentTokenKey)) {
    reward.token = cbr::Token(*value);
  } else {
    return std::nullopt;
  }

  // Blinded token
  if (const auto* const value =
          dict.FindString(kConfirmationRewardBlindedPaymentTokenKey)) {
    reward.blinded_token = cbr::BlindedToken(*value);
  } else {
    return std::nullopt;
  }

  if (const auto* const unblinded_token_dict =
          dict.FindDict(kConfirmationRewardTokenInfoKey)) {
    // Unblinded token
    if (const auto* const value = unblinded_token_dict->FindString(
            kConfirmationRewardUnblindedTokenKey)) {
      reward.unblinded_token = cbr::UnblindedToken(*value);
    } else {
      return std::nullopt;
    }

    // Public key
    if (const auto* const value =
            unblinded_token_dict->FindString(kConfirmationRewardPublicKeyKey)) {
      reward.public_key = cbr::PublicKey(*value);
    } else {
      return std::nullopt;
    }

    // Signature
    if (const auto* const value =
            unblinded_token_dict->FindString(kConfirmationRewardSignatureKey)) {
      reward.signature = *value;
    } else {
      return std::nullopt;
    }
  }

  // Credential
  if (const auto* const value =
          dict.FindString(kConfirmationRewardCredentialKey)) {
    reward.credential_base64url = *value;
  } else {
    return std::nullopt;
  }

  return reward;
}

std::optional<ConfirmationInfo> ParseConfirmation(
    const base::Value::Dict& dict) {
  ConfirmationInfo confirmation;

  // Transaction id
  if (const auto* const value =
          dict.FindString(kConfirmationTransactionIdKey)) {
    confirmation.transaction_id = *value;
  } else {
    return std::nullopt;
  }

  // Creative instance id
  if (const auto* const value =
          dict.FindString(kConfirmationCreativeInstanceIdKey)) {
    confirmation.creative_instance_id = *value;
  } else {
    return std::nullopt;
  }

  // Type
  if (const auto* const value = dict.FindString(kConfirmationTypeKey)) {
    confirmation.type = ToMojomConfirmationType(*value);
  } else {
    return std::nullopt;
  }

  // Ad type
  if (const auto* const value = dict.FindString(kConfirmationAdTypeKey)) {
    confirmation.ad_type = ToMojomAdType(*value);
  } else {
    return std::nullopt;
  }

  // Created at
  if (const auto* const value = dict.Find(kConfirmationCreatedAtKey)) {
    confirmation.created_at = base::ValueToTime(value).value_or(base::Time());
  } else {
    return std::nullopt;
  }

  // User data
  if (const auto* const value = dict.FindDict(kConfirmationUserDataKey)) {
    confirmation.user_data.fixed = value->Clone();
  } else {
    return std::nullopt;
  }

  // Reward
  confirmation.reward = ParseConfirmationReward(dict);

  if (!IsValid(confirmation)) {
    return std::nullopt;
  }

  return confirmation;
}

}  // namespace

std::optional<ConfirmationList> ParseConfirmations(
    const base::Value::Dict& dict) {
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

    if (const std::optional<ConfirmationInfo> confirmation =
            ParseConfirmation(*confirmation_dict)) {
      confirmations.push_back(*confirmation);
    }
  }

  return confirmations;
}

}  // namespace brave_ads::json::reader
