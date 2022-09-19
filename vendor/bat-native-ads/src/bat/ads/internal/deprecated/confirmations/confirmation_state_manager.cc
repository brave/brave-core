/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager.h"

#include <cstdint>
#include <utility>

#include "absl/types/optional.h"
#include "base/bind.h"
#include "base/check_op.h"
#include "base/guid.h"
#include "base/hash/hash.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ads/internal/account/confirmations/confirmation_util.h"
#include "bat/ads/internal/account/confirmations/opted_in_info.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager_constants.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_value_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_value_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens.h"
#include "bat/ads/pref_names.h"

namespace ads {

namespace {

ConfirmationStateManager* g_confirmation_state_manager_instance = nullptr;

uint64_t GenerateHash(const std::string& value) {
  return static_cast<uint64_t>(base::PersistentHash(value));
}

void SetHash(const std::string& value) {
  AdsClientHelper::GetInstance()->SetUint64Pref(prefs::kConfirmationsHash,
                                                GenerateHash(value));
}

bool IsMutated(const std::string& value) {
  return AdsClientHelper::GetInstance()->GetUint64Pref(
             prefs::kConfirmationsHash) != GenerateHash(value);
}

absl::optional<OptedInInfo> GetOptedIn(const base::Value::Dict& dict) {
  OptedInInfo opted_in;

  // Token
  if (const std::string* value = dict.FindString("payment_token")) {
    opted_in.token = privacy::cbr::Token(*value);
  } else {
    return absl::nullopt;
  }

  // Blinded token
  if (const std::string* value = dict.FindString("blinded_payment_token")) {
    opted_in.blinded_token = privacy::cbr::BlindedToken(*value);
  } else {
    return absl::nullopt;
  }

  // Unblinded token
  if (const base::Value::Dict* unblinded_token = dict.FindDict("token_info")) {
    // Value
    if (const std::string* value =
            unblinded_token->FindString("unblinded_token")) {
      opted_in.unblinded_token.value = privacy::cbr::UnblindedToken(*value);
    } else {
      return absl::nullopt;
    }

    // Public key
    if (const std::string* value = unblinded_token->FindString("public_key")) {
      opted_in.unblinded_token.public_key = privacy::cbr::PublicKey(*value);
    } else {
      return absl::nullopt;
    }
  }

  // User data
  if (const base::Value::Dict* value = dict.FindDict("user_data")) {
    opted_in.user_data = value->Clone();
  } else {
    return absl::nullopt;
  }

  // Credential
  if (const std::string* value = dict.FindString("credential")) {
    opted_in.credential_base64url = *value;
  } else {
    return absl::nullopt;
  }

  return opted_in;
}

}  // namespace

ConfirmationStateManager::ConfirmationStateManager()
    : unblinded_tokens_(std::make_unique<privacy::UnblindedTokens>()),
      unblinded_payment_tokens_(
          std::make_unique<privacy::UnblindedPaymentTokens>()) {
  DCHECK(!g_confirmation_state_manager_instance);
  g_confirmation_state_manager_instance = this;
}

ConfirmationStateManager::~ConfirmationStateManager() {
  DCHECK_EQ(this, g_confirmation_state_manager_instance);
  g_confirmation_state_manager_instance = nullptr;
}

// static
ConfirmationStateManager* ConfirmationStateManager::GetInstance() {
  DCHECK(g_confirmation_state_manager_instance);
  return g_confirmation_state_manager_instance;
}

// static
bool ConfirmationStateManager::HasInstance() {
  return !!g_confirmation_state_manager_instance;
}

void ConfirmationStateManager::Initialize(InitializeCallback callback) {
  callback_ = std::move(callback);

  Load();
}

bool ConfirmationStateManager::IsInitialized() const {
  return is_initialized_;
}

void ConfirmationStateManager::Load() {
  BLOG(3, "Loading confirmations state");

  AdsClientHelper::GetInstance()->Load(
      kConfirmationStateFilename,
      base::BindOnce(&ConfirmationStateManager::OnLoaded,
                     base::Unretained(this)));
}

void ConfirmationStateManager::OnLoaded(const bool success,
                                        const std::string& json) {
  if (!success) {
    BLOG(3, "Confirmations state does not exist, creating default state");

    is_initialized_ = true;

    Save();
  } else {
    if (!FromJson(json)) {
      BLOG(0, "Failed to load confirmations state");

      BLOG(3, "Failed to parse confirmations state: " << json);

      callback_(/*success*/ false);
      return;
    }

    BLOG(3, "Successfully loaded confirmations state");

    is_initialized_ = true;
  }

  is_mutated_ = IsMutated(ToJson());
  if (is_mutated_) {
    BLOG(9, "Confirmation state is mutated");
  }

  callback_(/*success*/ true);
}

void ConfirmationStateManager::Save() {
  if (!is_initialized_) {
    return;
  }

  BLOG(9, "Saving confirmations state");

  const std::string json = ToJson();

  if (!is_mutated_) {
    SetHash(json);
  }

  AdsClientHelper::GetInstance()->Save(
      kConfirmationStateFilename, json, base::BindOnce([](const bool success) {
        if (!success) {
          BLOG(0, "Failed to save confirmations state");
          return;
        }

        BLOG(9, "Successfully saved confirmations state");
      }));
}

const ConfirmationList& ConfirmationStateManager::GetFailedConfirmations()
    const {
  DCHECK(is_initialized_);
  return failed_confirmations_;
}

void ConfirmationStateManager::AppendFailedConfirmation(
    const ConfirmationInfo& confirmation) {
  DCHECK(IsValid(confirmation));

  DCHECK(is_initialized_);
  failed_confirmations_.push_back(confirmation);
}

bool ConfirmationStateManager::RemoveFailedConfirmation(
    const ConfirmationInfo& confirmation) {
  DCHECK(IsValid(confirmation));

  DCHECK(is_initialized_);

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
           privacy::UnblindedTokensToValue(unblinded_tokens_->GetAllTokens()));

  // Unblinded payment tokens
  dict.Set("unblinded_payment_tokens",
           privacy::UnblindedPaymentTokensToValue(
               unblinded_payment_tokens_->GetAllTokens()));

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

base::Value::Dict ConfirmationStateManager::GetFailedConfirmationsAsDictionary(
    const ConfirmationList& confirmations) const {
  base::Value::Dict dict;

  base::Value::List list;
  for (const auto& confirmation : confirmations) {
    DCHECK(IsValid(confirmation));

    base::Value::Dict confirmation_dict;

    confirmation_dict.Set("transaction_id", confirmation.transaction_id);

    confirmation_dict.Set("creative_instance_id",
                          confirmation.creative_instance_id);

    confirmation_dict.Set("type", confirmation.type.ToString());

    confirmation_dict.Set("ad_type", confirmation.ad_type.ToString());

    confirmation_dict.Set(
        "timestamp_in_seconds",
        base::NumberToString(confirmation.created_at.ToDoubleT()));

    confirmation_dict.Set("created", confirmation.was_created);

    if (confirmation.opted_in) {
      // Token
      const absl::optional<std::string> token_base64 =
          confirmation.opted_in->token.EncodeBase64();
      if (!token_base64) {
        continue;
      }
      confirmation_dict.Set("payment_token", *token_base64);

      // Blinded token
      const absl::optional<std::string> blinded_token_base64 =
          confirmation.opted_in->blinded_token.EncodeBase64();
      if (!blinded_token_base64) {
        continue;
      }
      confirmation_dict.Set("blinded_payment_token", *blinded_token_base64);

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

      confirmation_dict.Set("token_info", std::move(unblinded_token));

      // User data
      confirmation_dict.Set("user_data",
                            confirmation.opted_in->user_data.Clone());

      // Credential
      DCHECK(confirmation.opted_in->credential_base64url);
      confirmation_dict.Set("credential",
                            *confirmation.opted_in->credential_base64url);
    }

    list.Append(std::move(confirmation_dict));
  }

  dict.Set("failed_confirmations", std::move(list));

  return dict;
}

bool ConfirmationStateManager::GetFailedConfirmationsFromDictionary(
    const base::Value::Dict& dict,
    ConfirmationList* confirmations) {
  DCHECK(confirmations);

  // Confirmations
  const base::Value::List* failed_confirmations =
      dict.FindList("failed_confirmations");
  if (!failed_confirmations) {
    BLOG(0, "Failed confirmations dictionary missing failed confirmations");
    return false;
  }

  ConfirmationList new_failed_confirmations;

  for (const auto& item : *failed_confirmations) {
    const base::Value::Dict* failed_confirmation_dict = item.GetIfDict();
    if (!failed_confirmation_dict) {
      BLOG(0, "Confirmation should be a dictionary");
      continue;
    }

    ConfirmationInfo confirmation;

    // Transaction id
    if (const std::string* value =
            failed_confirmation_dict->FindString("transaction_id")) {
      confirmation.transaction_id = *value;
    } else {
      // Migrate legacy confirmations
      confirmation.transaction_id =
          base::GUID::GenerateRandomV4().AsLowercaseString();
    }

    // Creative instance id
    if (const std::string* value =
            failed_confirmation_dict->FindString("creative_instance_id")) {
      confirmation.creative_instance_id = *value;
    } else {
      BLOG(0, "Missing confirmation creative instance id");
      continue;
    }

    // Type
    if (const std::string* value =
            failed_confirmation_dict->FindString("type")) {
      confirmation.type = ConfirmationType(*value);
    } else {
      BLOG(0, "Missing confirmation type");
      continue;
    }

    // Ad type
    if (const std::string* value =
            failed_confirmation_dict->FindString("ad_type")) {
      confirmation.ad_type = AdType(*value);
    } else {
      // Migrate legacy confirmations, this value is not used right now so safe
      // to set to |kNotificationAd|
      confirmation.ad_type = AdType::kNotificationAd;
    }

    // Created at
    if (const std::string* value =
            failed_confirmation_dict->FindString("timestamp_in_seconds")) {
      double timestamp_as_double;
      if (!base::StringToDouble(*value, &timestamp_as_double)) {
        continue;
      }

      confirmation.created_at = base::Time::FromDoubleT(timestamp_as_double);
    }

    // Was created
    const absl::optional<bool> was_created =
        failed_confirmation_dict->FindBool("created");
    confirmation.was_created = was_created.value_or(true);

    // Opted-in
    confirmation.opted_in = GetOptedIn(*failed_confirmation_dict);

    if (!IsValid(confirmation)) {
      BLOG(0, "Invalid confirmation");
      continue;
    }

    new_failed_confirmations.push_back(confirmation);
  }

  *confirmations = new_failed_confirmations;

  return true;
}

bool ConfirmationStateManager::ParseFailedConfirmationsFromDictionary(
    const base::Value::Dict& dict) {
  const base::Value::Dict* confirmations = dict.FindDict("confirmations");
  if (!confirmations) {
    return false;
  }

  if (!GetFailedConfirmationsFromDictionary(*confirmations,
                                            &failed_confirmations_)) {
    return false;
  }

  return true;
}

bool ConfirmationStateManager::ParseUnblindedTokensFromDictionary(
    const base::Value::Dict& dict) {
  const base::Value::List* unblinded_tokens = dict.FindList("unblinded_tokens");
  if (!unblinded_tokens) {
    return false;
  }

  unblinded_tokens_->SetTokens(
      privacy::UnblindedTokensFromValue(*unblinded_tokens));

  return true;
}

bool ConfirmationStateManager::ParseUnblindedPaymentTokensFromDictionary(
    const base::Value::Dict& dict) {
  const base::Value::List* unblinded_tokens =
      dict.FindList("unblinded_payment_tokens");
  if (!unblinded_tokens) {
    return false;
  }

  unblinded_payment_tokens_->SetTokens(
      privacy::UnblindedPaymentTokensFromValue(*unblinded_tokens));

  return true;
}

}  // namespace ads
