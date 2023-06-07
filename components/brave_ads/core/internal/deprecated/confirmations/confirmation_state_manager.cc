/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"

#include <cstdint>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/json/values_util.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_ads/core/internal/account/account_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/opted_in_info.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager_constants.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_value_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_token_value_util.h"

namespace brave_ads {

namespace {

base::Value::Dict GetFailedConfirmationsAsDictionary(
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

    dict.Set("created", confirmation.was_created);

    if (confirmation.opted_in) {
      // Token
      const absl::optional<std::string> token_base64 =
          confirmation.opted_in->token.EncodeBase64();
      if (!token_base64) {
        continue;
      }
      dict.Set("payment_token", *token_base64);

      // Blinded token
      const absl::optional<std::string> blinded_token_base64 =
          confirmation.opted_in->blinded_token.EncodeBase64();
      if (!blinded_token_base64) {
        continue;
      }
      dict.Set("blinded_payment_token", *blinded_token_base64);

      // Unblinded token
      base::Value::Dict unblinded_token;
      const absl::optional<std::string> unblinded_token_base64 =
          confirmation.opted_in->unblinded_token.value.EncodeBase64();
      if (!unblinded_token_base64) {
        continue;
      }
      unblinded_token.Set("unblinded_token", *unblinded_token_base64);

      const absl::optional<std::string> public_key_base64 =
          confirmation.opted_in->unblinded_token.public_key.EncodeBase64();
      if (!public_key_base64) {
        continue;
      }
      unblinded_token.Set("public_key", *public_key_base64);

      const std::string signature =
          confirmation.opted_in->unblinded_token.signature;
      unblinded_token.Set("signature", signature);

      dict.Set("token_info", std::move(unblinded_token));

      // User data
      dict.Set("user_data", confirmation.opted_in->user_data.fixed.Clone());

      // Credential
      if (!confirmation.opted_in->credential_base64url) {
        continue;
      }

      // Dynamic user data is not persisted for failed confirmations because the
      // user data is recreated when redeeming the confirmation tokens, so we
      // must create a new credential excluding the dynamic user data.
      ConfirmationInfo confirmation_copy(confirmation);
      confirmation_copy.opted_in->user_data.dynamic = {};
      const absl::optional<std::string> opted_in_credential =
          CreateOptedInCredential(confirmation_copy);
      if (!opted_in_credential) {
        continue;
      }

      dict.Set("credential", *opted_in_credential);
    }

    list.Append(std::move(dict));
  }

  base::Value::Dict dict;
  dict.Set("failed_confirmations", std::move(list));

  return dict;
}

}  // namespace

ConfirmationStateManager::ConfirmationStateManager() = default;

ConfirmationStateManager::~ConfirmationStateManager() = default;

// static
ConfirmationStateManager& ConfirmationStateManager::GetInstance() {
  return GlobalState::GetInstance()->GetConfirmationStateManager();
}

void ConfirmationStateManager::Load(const absl::optional<WalletInfo>& wallet,
                                    InitializeCallback callback) {
  BLOG(3, "Loading confirmations state");

  wallet_ = wallet;

  AdsClientHelper::GetInstance()->Load(
      kConfirmationStateFilename,
      base::BindOnce(&ConfirmationStateManager::LoadCallback,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void ConfirmationStateManager::LoadCallback(
    InitializeCallback callback,
    const absl::optional<std::string>& json) {
  if (!json) {
    BLOG(3, "Confirmations state does not exist, creating default state");

    is_initialized_ = true;

    Save();
  } else {
    if (!FromJson(*json)) {
      BLOG(0, "Failed to load confirmations state");
      BLOG(3, "Failed to parse confirmations state: " << *json);

      return std::move(callback).Run(/*success*/ false);
    }

    BLOG(3, "Successfully loaded confirmations state");

    is_initialized_ = true;
  }

  std::move(callback).Run(/*success*/ true);
}

void ConfirmationStateManager::Save() {
  if (!is_initialized_) {
    return;
  }

  BLOG(9, "Saving confirmations state");

  AdsClientHelper::GetInstance()->Save(
      kConfirmationStateFilename, ToJson(),
      base::BindOnce([](const bool success) {
        if (!success) {
          return BLOG(0, "Failed to save confirmations state");
        }

        BLOG(9, "Successfully saved confirmations state");
      }));
}

absl::optional<OptedInInfo> ConfirmationStateManager::GetOptedIn(
    const base::Value::Dict& dict) const {
  if (!wallet_) {
    return absl::nullopt;
  }

  OptedInInfo opted_in;

  // Token
  if (const auto* const value = dict.FindString("payment_token")) {
    opted_in.token = privacy::cbr::Token(*value);
  } else {
    return absl::nullopt;
  }

  // Blinded token
  if (const auto* const value = dict.FindString("blinded_payment_token")) {
    opted_in.blinded_token = privacy::cbr::BlindedToken(*value);
  } else {
    return absl::nullopt;
  }

  // Unblinded token
  if (const auto* const unblinded_token_dict = dict.FindDict("token_info")) {
    // Value
    if (const auto* const value =
            unblinded_token_dict->FindString("unblinded_token")) {
      opted_in.unblinded_token.value = privacy::cbr::UnblindedToken(*value);
    } else {
      return absl::nullopt;
    }

    // Public key
    if (const auto* const value =
            unblinded_token_dict->FindString("public_key")) {
      opted_in.unblinded_token.public_key = privacy::cbr::PublicKey(*value);
    } else {
      return absl::nullopt;
    }

    // Signature
    if (const auto* const value =
            unblinded_token_dict->FindString("signature")) {
      opted_in.unblinded_token.signature = *value;
    } else {
      const absl::optional<std::string> unblinded_token_base64 =
          opted_in.unblinded_token.value.EncodeBase64();
      if (!unblinded_token_base64) {
        return absl::nullopt;
      }

      const absl::optional<std::string> signature =
          crypto::Sign(*unblinded_token_base64, wallet_->secret_key);
      if (!signature) {
        return absl::nullopt;
      }

      opted_in.unblinded_token.signature = *signature;
    }
  }

  // User data (opted_in.user_data.dynamic is recreated when redeeming a token)
  if (const auto* const value = dict.FindDict("user_data")) {
    opted_in.user_data.fixed = value->Clone();
  } else {
    return absl::nullopt;
  }

  // Credential
  if (const auto* const value = dict.FindString("credential")) {
    opted_in.credential_base64url = *value;
  } else {
    return absl::nullopt;
  }

  return opted_in;
}

bool ConfirmationStateManager::GetFailedConfirmationsFromDictionary(
    const base::Value::Dict& dict,
    ConfirmationList* confirmations) const {
  CHECK(confirmations);

  // Confirmations
  const auto* const list = dict.FindList("failed_confirmations");
  if (!list) {
    BLOG(0, "Failed confirmations dictionary missing failed confirmations");
    return false;
  }

  ConfirmationList new_failed_confirmations;

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
      // Migrate legacy confirmations
      confirmation.transaction_id =
          base::GUID::GenerateRandomV4().AsLowercaseString();
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
      // Migrate legacy confirmations, this value is not used right now so safe
      // to set to |kNotificationAd|
      confirmation.ad_type = AdType::kNotificationAd;
    }

    // Created at
    if (const auto* const value = item_dict->Find("created_at")) {
      confirmation.created_at = base::ValueToTime(value).value_or(base::Time());
    } else if (const auto* const legacy_string_value =
                   item_dict->FindString("timestamp_in_seconds")) {
      double value_as_double;
      if (base::StringToDouble(*legacy_string_value, &value_as_double)) {
        confirmation.created_at = base::Time::FromDoubleT(value_as_double);
      }
    }

    // Was created
    confirmation.was_created = item_dict->FindBool("created").value_or(true);

    // Opted-in
    confirmation.opted_in = GetOptedIn(*item_dict);

    if (!IsValid(confirmation)) {
      BLOG(0, "Invalid confirmation");
      continue;
    }

    new_failed_confirmations.push_back(confirmation);
  }

  *confirmations = new_failed_confirmations;

  return true;
}

ConfirmationList ConfirmationStateManager::GetFailedConfirmations() const {
  CHECK(is_initialized_);

  if (ShouldRewardUser()) {
    return failed_confirmations_;
  }

  // User is not opted-in to Brave Private Ads so only return opted-out
  // confirmations, opted-in confirmations will be redeemed if and when the user
  // rejoins Brave Private Ads.
  ConfirmationList opted_out_confirmations;

  base::ranges::copy_if(failed_confirmations_,
                        std::back_inserter(opted_out_confirmations),
                        [](const ConfirmationInfo& confirmation) {
                          return !confirmation.opted_in;
                        });

  return opted_out_confirmations;
}

void ConfirmationStateManager::AppendFailedConfirmation(
    const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));

  CHECK(is_initialized_);
  failed_confirmations_.push_back(confirmation);
}

bool ConfirmationStateManager::RemoveFailedConfirmation(
    const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));

  CHECK(is_initialized_);

  const auto iter =
      base::ranges::find(failed_confirmations_, confirmation.transaction_id,
                         &ConfirmationInfo::transaction_id);

  if (iter == failed_confirmations_.cend()) {
    return false;
  }

  failed_confirmations_.erase(iter);

  return true;
}

std::string ConfirmationStateManager::ToJson() {
  base::Value::Dict dict;

  // Confirmations
  dict.Set("confirmations",
           GetFailedConfirmationsAsDictionary(failed_confirmations_));

  // Unblinded tokens
  dict.Set("unblinded_tokens",
           privacy::UnblindedTokensToValue(unblinded_tokens_.GetAllTokens()));

  // Unblinded payment tokens
  dict.Set("unblinded_payment_tokens",
           privacy::UnblindedPaymentTokensToValue(
               unblinded_payment_tokens_.GetAllTokens()));

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

  if (!ParseFailedConfirmationsFromDictionary(dict)) {
    BLOG(1, "Failed to parse failed confirmations");
  }

  if (!ParseUnblindedTokensFromDictionary(dict)) {
    BLOG(1, "Failed to parse unblinded tokens");
  }

  if (!ParseUnblindedPaymentTokensFromDictionary(dict)) {
    BLOG(1, "Failed to parse unblinded payment tokens");
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool ConfirmationStateManager::ParseFailedConfirmationsFromDictionary(
    const base::Value::Dict& dict) {
  const auto* const confirmations_dict = dict.FindDict("confirmations");
  if (!confirmations_dict) {
    return false;
  }

  return GetFailedConfirmationsFromDictionary(*confirmations_dict,
                                              &failed_confirmations_);
}

bool ConfirmationStateManager::ParseUnblindedTokensFromDictionary(
    const base::Value::Dict& dict) {
  const auto* const list = dict.FindList("unblinded_tokens");
  if (!list) {
    return false;
  }

  privacy::UnblindedTokenList filtered_unblinded_tokens =
      privacy::UnblindedTokensFromValue(*list);

  if (wallet_ && !filtered_unblinded_tokens.empty()) {
    const std::string public_key = wallet_->public_key;

    filtered_unblinded_tokens.erase(
        base::ranges::remove_if(
            filtered_unblinded_tokens,
            [&public_key](const privacy::UnblindedTokenInfo& unblinded_token) {
              const absl::optional<std::string> unblinded_token_base64 =
                  unblinded_token.value.EncodeBase64();
              return !unblinded_token_base64 ||
                     !crypto::Verify(*unblinded_token_base64, public_key,
                                     unblinded_token.signature);
            }),
        filtered_unblinded_tokens.cend());
  }

  unblinded_tokens_.SetTokens(filtered_unblinded_tokens);

  return true;
}

bool ConfirmationStateManager::ParseUnblindedPaymentTokensFromDictionary(
    const base::Value::Dict& dict) {
  const auto* const list = dict.FindList("unblinded_payment_tokens");
  if (!list) {
    return false;
  }

  unblinded_payment_tokens_.SetTokens(
      privacy::UnblindedPaymentTokensFromValue(*list));

  return true;
}

}  // namespace brave_ads
