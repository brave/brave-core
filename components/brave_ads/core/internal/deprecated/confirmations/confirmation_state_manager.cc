/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/json/values_util.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_value_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_value_util.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager_constants.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

namespace brave_ads {

namespace {

base::Value::Dict GetConfirmationsAsDictionary(
    const ConfirmationList& confirmations) {
  base::Value::List list;

  for (const auto& confirmation : confirmations) {
    CHECK(IsValid(confirmation));

    base::Value::Dict dict;

    dict.Set("transaction_id", confirmation.transaction_id);

    dict.Set("creative_instance_id", confirmation.creative_instance_id);

    dict.Set("type", confirmation.type.ToString());

    dict.Set("ad_type", confirmation.ad_type.ToString());

    dict.Set("created_at", base::TimeToValue(confirmation.created_at));

    if (confirmation.reward) {
      // Token
      const absl::optional<std::string> token_base64 =
          confirmation.reward->token.EncodeBase64();
      if (!token_base64) {
        continue;
      }
      dict.Set("payment_token", *token_base64);

      // Blinded token
      const absl::optional<std::string> blinded_token_base64 =
          confirmation.reward->blinded_token.EncodeBase64();
      if (!blinded_token_base64) {
        continue;
      }
      dict.Set("blinded_payment_token", *blinded_token_base64);

      // Unblinded token
      base::Value::Dict unblinded_token;
      const absl::optional<std::string> unblinded_token_base64 =
          confirmation.reward->unblinded_token.EncodeBase64();
      if (!unblinded_token_base64) {
        continue;
      }
      unblinded_token.Set("unblinded_token", *unblinded_token_base64);

      const absl::optional<std::string> public_key_base64 =
          confirmation.reward->public_key.EncodeBase64();
      if (!public_key_base64) {
        continue;
      }
      unblinded_token.Set("public_key", *public_key_base64);

      const std::string signature = confirmation.reward->signature;
      unblinded_token.Set("signature", signature);

      dict.Set("token_info", std::move(unblinded_token));

      // User data
      dict.Set("user_data", confirmation.user_data.fixed.Clone());

      // Credential
      if (confirmation.reward->credential_base64url.empty()) {
        continue;
      }

      // Dynamic user data is not persisted for confirmations because the
      // dynamic user data is rebuilt when redeeming a confirmation token, so we
      // must create a new credential excluding the dynamic user data.
      ConfirmationInfo mutable_confirmation(confirmation);
      mutable_confirmation.user_data.dynamic = {};
      const absl::optional<std::string> reward_credential_base64url =
          BuildRewardCredential(mutable_confirmation);
      if (!reward_credential_base64url) {
        continue;
      }

      dict.Set("credential", *reward_credential_base64url);
    }

    list.Append(std::move(dict));
  }

  base::Value::Dict dict;
  dict.Set("queue", std::move(list));

  return dict;
}

}  // namespace

ConfirmationStateManager::ConfirmationStateManager() = default;

ConfirmationStateManager::~ConfirmationStateManager() = default;

// static
ConfirmationStateManager& ConfirmationStateManager::GetInstance() {
  return GlobalState::GetInstance()->GetConfirmationStateManager();
}

void ConfirmationStateManager::LoadState(
    const absl::optional<WalletInfo>& wallet,
    InitializeCallback callback) {
  BLOG(3, "Loading confirmation state");

  wallet_ = wallet;

  Load(kConfirmationStateFilename,
       base::BindOnce(&ConfirmationStateManager::LoadCallback,
                      weak_factory_.GetWeakPtr(), std::move(callback)));
}

void ConfirmationStateManager::LoadCallback(
    InitializeCallback callback,
    const absl::optional<std::string>& json) {
  if (!json) {
    BLOG(3, "Confirmation state does not exist, creating default state");

    is_initialized_ = true;

    SaveState();
  } else {
    if (!FromJson(*json)) {
      BLOG(0, "Failed to load confirmation state");
      BLOG(3, "Failed to parse confirmation state: " << *json);

      return std::move(callback).Run(/*success=*/false);
    }

    BLOG(3, "Successfully loaded confirmation state");

    is_initialized_ = true;
  }

  std::move(callback).Run(/*success=*/true);
}

void ConfirmationStateManager::SaveState() {
  if (!is_initialized_) {
    return;
  }

  BLOG(9, "Saving confirmation state");

  Save(kConfirmationStateFilename, ToJson(),
       base::BindOnce([](const bool success) {
         if (!success) {
           return BLOG(0, "Failed to save confirmation state");
         }

         BLOG(9, "Successfully saved confirmation state");
       }));
}

absl::optional<RewardInfo> ConfirmationStateManager::GetReward(
    const base::Value::Dict& dict) const {
  RewardInfo reward;

  // Token
  if (const auto* const value = dict.FindString("payment_token")) {
    reward.token = cbr::Token(*value);
  } else {
    return absl::nullopt;
  }

  // Blinded token
  if (const auto* const value = dict.FindString("blinded_payment_token")) {
    reward.blinded_token = cbr::BlindedToken(*value);
  } else {
    return absl::nullopt;
  }

  if (const auto* const unblinded_token_dict = dict.FindDict("token_info")) {
    // Unblinded token
    if (const auto* const value =
            unblinded_token_dict->FindString("unblinded_token")) {
      reward.unblinded_token = cbr::UnblindedToken(*value);
    } else {
      return absl::nullopt;
    }

    // Public key
    if (const auto* const value =
            unblinded_token_dict->FindString("public_key")) {
      reward.public_key = cbr::PublicKey(*value);
    } else {
      return absl::nullopt;
    }

    // Signature
    if (const auto* const value =
            unblinded_token_dict->FindString("signature")) {
      reward.signature = *value;
    } else {
      // Legacy migration.
      if (!wallet_) {
        return absl::nullopt;
      }

      const absl::optional<std::string> unblinded_token_base64 =
          reward.unblinded_token.EncodeBase64();
      if (!unblinded_token_base64) {
        return absl::nullopt;
      }

      const absl::optional<std::string> signature =
          crypto::Sign(*unblinded_token_base64, wallet_->secret_key);
      if (!signature) {
        return absl::nullopt;
      }

      reward.signature = *signature;
    }
  }

  // Credential
  if (const auto* const value = dict.FindString("credential")) {
    reward.credential_base64url = *value;
  } else {
    return absl::nullopt;
  }

  return reward;
}

bool ConfirmationStateManager::GetConfirmationsFromDictionary(
    const base::Value::Dict& dict,
    ConfirmationList* confirmations) const {
  CHECK(confirmations);

  // Confirmations
  const auto* list = dict.FindList("queue");
  if (!list) {
    // Legacy migration.
    list = dict.FindList("failed_confirmations");
    if (!list) {
      BLOG(0, "Missing confirmations");
      return false;
    }
  }

  ConfirmationList new_confirmations;

  for (const auto& item : *list) {
    const auto* const item_dict = item.GetIfDict();
    if (!item_dict) {
      BLOG(0, "Confirmation should be a dictionary");
      continue;
    }

    ConfirmationInfo confirmation;

    // Transaction id
    if (const auto* const value = item_dict->FindString("transaction_id")) {
      confirmation.transaction_id = *value;
    } else {
      BLOG(0, "Missing confirmation transaction id");
      continue;
    }

    // Creative instance id
    if (const auto* const value =
            item_dict->FindString("creative_instance_id")) {
      confirmation.creative_instance_id = *value;
    } else {
      BLOG(0, "Missing confirmation creative instance id");
      continue;
    }

    // Type
    if (const auto* const value = item_dict->FindString("type")) {
      confirmation.type = ConfirmationType(*value);
    } else {
      BLOG(0, "Missing confirmation type");
      continue;
    }

    // Ad type
    if (const auto* const value = item_dict->FindString("ad_type")) {
      confirmation.ad_type = AdType(*value);
    } else {
      BLOG(0, "Missing confirmation ad type");
      continue;
    }

    // Created at
    if (const auto* const value = item_dict->Find("created_at")) {
      confirmation.created_at = base::ValueToTime(value).value_or(base::Time());
    } else {
      BLOG(0, "Missing confirmation created at");
      continue;
    }

    // User data
    if (const auto* const value = item_dict->FindDict("user_data")) {
      confirmation.user_data.fixed = value->Clone();
    }

    // Opted-in
    confirmation.reward = GetReward(*item_dict);

    if (!IsValid(confirmation)) {
      BLOG(0, "Invalid confirmation");
      continue;
    }

    new_confirmations.push_back(confirmation);
  }

  *confirmations = new_confirmations;

  return true;
}

ConfirmationList ConfirmationStateManager::GetConfirmations() const {
  CHECK(is_initialized_);

  return confirmations_;
}

void ConfirmationStateManager::AddConfirmation(
    const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));

  CHECK(is_initialized_);
  confirmations_.push_back(confirmation);
}

bool ConfirmationStateManager::RemoveConfirmation(
    const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));

  CHECK(is_initialized_);

  const auto iter =
      base::ranges::find(confirmations_, confirmation.transaction_id,
                         &ConfirmationInfo::transaction_id);

  if (iter == confirmations_.cend()) {
    return false;
  }

  confirmations_.erase(iter);

  return true;
}

std::string ConfirmationStateManager::ToJson() {
  base::Value::Dict dict;

  // Confirmations
  dict.Set("confirmations", GetConfirmationsAsDictionary(confirmations_));

  // Unblinded tokens
  dict.Set("unblinded_tokens",
           ConfirmationTokensToValue(confirmation_tokens_.GetAllTokens()));

  // Payment tokens
  dict.Set("unblinded_payment_tokens",
           PaymentTokensToValue(payment_tokens_.GetAllTokens()));

  // Write to JSON
  std::string json;
  CHECK(base::JSONWriter::Write(dict, &json));
  return json;
}

bool ConfirmationStateManager::FromJson(const std::string& json) {
  const absl::optional<base::Value> root = base::JSONReader::Read(json);
  if (!root || !root->is_dict()) {
    return false;
  }
  const base::Value::Dict& dict = root->GetDict();

  if (!ParseConfirmationsFromDictionary(dict)) {
    BLOG(1, "Failed to parse confirmations");
  }

  if (!ParseConfirmationTokensFromDictionary(dict)) {
    BLOG(1, "Failed to parse confirmation tokens");
  }

  if (!ParsePaymentTokensFromDictionary(dict)) {
    BLOG(1, "Failed to parse payment tokens");
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool ConfirmationStateManager::ParseConfirmationsFromDictionary(
    const base::Value::Dict& dict) {
  const auto* const confirmations_dict = dict.FindDict("confirmations");
  if (!confirmations_dict) {
    return false;
  }

  return GetConfirmationsFromDictionary(*confirmations_dict, &confirmations_);
}

bool ConfirmationStateManager::ParseConfirmationTokensFromDictionary(
    const base::Value::Dict& dict) {
  const auto* const list = dict.FindList("unblinded_tokens");
  if (!list) {
    return false;
  }

  ConfirmationTokenList filtered_confirmation_tokens =
      ConfirmationTokensFromValue(*list);

  if (wallet_ && !filtered_confirmation_tokens.empty()) {
    const std::string public_key = wallet_->public_key;

    filtered_confirmation_tokens.erase(
        base::ranges::remove_if(
            filtered_confirmation_tokens,
            [&public_key](const ConfirmationTokenInfo& confirmation_token) {
              const absl::optional<std::string> unblinded_token_base64 =
                  confirmation_token.unblinded_token.EncodeBase64();
              return !unblinded_token_base64 ||
                     !crypto::Verify(*unblinded_token_base64, public_key,
                                     confirmation_token.signature);
            }),
        filtered_confirmation_tokens.cend());
  }

  confirmation_tokens_.SetTokens(filtered_confirmation_tokens);

  return true;
}

bool ConfirmationStateManager::ParsePaymentTokensFromDictionary(
    const base::Value::Dict& dict) {
  const auto* const list = dict.FindList("unblinded_payment_tokens");
  if (!list) {
    return false;
  }

  payment_tokens_.SetTokens(PaymentTokensFromValue(*list));

  return true;
}

}  // namespace brave_ads
