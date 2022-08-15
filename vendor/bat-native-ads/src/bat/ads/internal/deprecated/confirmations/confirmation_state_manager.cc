/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager.h"

#include <algorithm>
#include <cstdint>
#include <utility>

#include "base/check_op.h"
#include "base/guid.h"
#include "base/hash/hash.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager_constants.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens.h"
#include "bat/ads/pref_names.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

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
  callback_ = callback;

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

      callback_(/* success */ false);
      return;
    }

    BLOG(3, "Successfully loaded confirmations state");

    is_initialized_ = true;
  }

  is_mutated_ = IsMutated(ToJson());
  if (is_mutated_) {
    BLOG(9, "Confirmation state is mutated");
  }

  callback_(/* success */ true);
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
  DCHECK(confirmation.IsValid());

  DCHECK(is_initialized_);
  failed_confirmations_.push_back(confirmation);
}

bool ConfirmationStateManager::RemoveFailedConfirmation(
    const ConfirmationInfo& confirmation) {
  DCHECK(confirmation.IsValid());

  DCHECK(is_initialized_);

  const auto iter =
      std::find_if(failed_confirmations_.cbegin(), failed_confirmations_.cend(),
                   [&confirmation](const ConfirmationInfo& info) {
                     return info.id == confirmation.id;
                   });

  if (iter == failed_confirmations_.end()) {
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
  dict.Set("unblinded_tokens", unblinded_tokens_->GetTokensAsList());

  // Unblinded payment tokens
  dict.Set("unblinded_payment_tokens",
           unblinded_payment_tokens_->GetTokensAsList());

  // Write to JSON
  std::string json;
  base::JSONWriter::Write(dict, &json);
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
    DCHECK(confirmation.IsValid());

    const absl::optional<std::string> unblinded_token_base64 =
        confirmation.unblinded_token.value.EncodeBase64();
    if (!unblinded_token_base64) {
      NOTREACHED();
      continue;
    }

    const absl::optional<std::string> public_key_base64 =
        confirmation.unblinded_token.public_key.EncodeBase64();
    if (!public_key_base64) {
      NOTREACHED();
      continue;
    }

    const absl::optional<std::string> payment_token_base64 =
        confirmation.payment_token.EncodeBase64();
    if (!payment_token_base64) {
      NOTREACHED();
      continue;
    }

    const absl::optional<std::string> blinded_payment_token_base64 =
        confirmation.blinded_payment_token.EncodeBase64();
    if (!blinded_payment_token_base64) {
      NOTREACHED();
      continue;
    }

    base::Value::Dict confirmation_dict;
    confirmation_dict.Set("id", confirmation.id);
    confirmation_dict.Set("transaction_id", confirmation.transaction_id);
    confirmation_dict.Set("creative_instance_id",
                          confirmation.creative_instance_id);
    confirmation_dict.Set("type", confirmation.type.ToString());
    confirmation_dict.Set("ad_type", confirmation.ad_type.ToString());
    confirmation_dict.Set("payment_token", *payment_token_base64);
    confirmation_dict.Set("blinded_payment_token",
                          *blinded_payment_token_base64);
    confirmation_dict.Set("credential", confirmation.credential);
    confirmation_dict.Set(
        "timestamp_in_seconds",
        base::NumberToString(confirmation.created_at.ToDoubleT()));
    confirmation_dict.Set("created", confirmation.was_created);

    base::Value::Dict unblinded_token;
    unblinded_token.Set("unblinded_token", *unblinded_token_base64);
    unblinded_token.Set("public_key", *public_key_base64);
    confirmation_dict.Set("token_info", std::move(unblinded_token));

    absl::optional<base::Value> user_data =
        base::JSONReader::Read(confirmation.user_data);
    if (user_data && user_data->is_dict())
      confirmation_dict.Set("user_data", std::move(*user_data));

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
    const base::Value::Dict* dict = item.GetIfDict();
    if (!dict) {
      BLOG(0, "Confirmation should be a dictionary");
      continue;
    }

    ConfirmationInfo confirmation;

    // Id
    const std::string* id = dict->FindString("id");
    if (!id) {
      // Id missing, skip confirmation
      BLOG(0, "Confirmation missing id");
      continue;
    }
    confirmation.id = *id;

    // Transaction id
    const std::string* transaction_id = dict->FindString("transaction_id");
    if (!transaction_id) {
      // Migrate legacy confirmations
      confirmation.transaction_id =
          base::GUID::GenerateRandomV4().AsLowercaseString();
    } else {
      confirmation.transaction_id = *transaction_id;
    }

    // Creative instance id
    const std::string* creative_instance_id =
        dict->FindString("creative_instance_id");
    if (!creative_instance_id) {
      // Creative instance id missing, skip confirmation
      BLOG(0, "Confirmation missing creative_instance_id");
      continue;
    }
    confirmation.creative_instance_id = *creative_instance_id;

    // Type
    const std::string* type = dict->FindString("type");
    if (!type) {
      // Type missing, skip confirmation
      BLOG(0, "Confirmation missing type");
      continue;
    }
    ConfirmationType confirmation_type(*type);
    confirmation.type = confirmation_type;

    // Ad type
    const std::string* ad_type = dict->FindString("ad_type");
    if (ad_type) {
      confirmation.ad_type = AdType(*ad_type);
    } else {
      // Migrate legacy confirmations, this value is not used right now so safe
      // to set to |kNotificationAd|
      confirmation.ad_type = AdType::kNotificationAd;
    }

    // Token info
    const base::Value::Dict* unblinded_token = dict->FindDict("token_info");
    if (!unblinded_token) {
      BLOG(0, "Confirmation missing token_info");
      continue;
    }

    // Unblinded token
    const std::string* unblinded_token_base64 =
        unblinded_token->FindString("unblinded_token");
    if (!unblinded_token_base64) {
      // Unblinded token missing, skip confirmation
      BLOG(0, "Token info missing unblinded_token");
      continue;
    }
    confirmation.unblinded_token.value =
        privacy::cbr::UnblindedToken(*unblinded_token_base64);
    if (!confirmation.unblinded_token.value.has_value()) {
      BLOG(0, "Invalid unblinded token");
      NOTREACHED();
      continue;
    }

    const std::string* public_key_base64 =
        unblinded_token->FindString("public_key");
    if (!public_key_base64) {
      // Public key missing, skip confirmation
      BLOG(0, "Token info missing public_key");
      continue;
    }
    confirmation.unblinded_token.public_key =
        privacy::cbr::PublicKey(*public_key_base64);
    if (!confirmation.unblinded_token.public_key.has_value()) {
      BLOG(0, "Invalid public key");
      NOTREACHED();
      continue;
    }

    // Payment token
    const std::string* payment_token_base64 = dict->FindString("payment_token");
    if (!payment_token_base64) {
      // Payment token missing, skip confirmation
      BLOG(0, "Confirmation missing payment_token");
      continue;
    }
    confirmation.payment_token = privacy::cbr::Token(*payment_token_base64);
    if (!confirmation.payment_token.has_value()) {
      BLOG(0, "Invalid payment token");
      NOTREACHED();
      continue;
    }

    // Blinded payment token
    const std::string* blinded_payment_token_base64 =
        dict->FindString("blinded_payment_token");
    if (!blinded_payment_token_base64) {
      // Blinded payment token missing, skip confirmation
      BLOG(0, "Confirmation missing blinded_payment_token");
      continue;
    }
    confirmation.blinded_payment_token =
        privacy::cbr::BlindedToken(*blinded_payment_token_base64);
    if (!confirmation.blinded_payment_token.has_value()) {
      BLOG(0, "Invalid blinded payment token");
      NOTREACHED();
      continue;
    }

    // Credential
    const std::string* credential = dict->FindString("credential");
    if (!credential) {
      // Credential missing, skip confirmation
      BLOG(0, "Confirmation missing credential");
      continue;
    }
    confirmation.credential = *credential;

    // User data
    const base::Value::Dict* user_data = dict->FindDict("user_data");
    if (user_data) {
      std::string json;
      base::JSONWriter::Write(*user_data, &json);
      confirmation.user_data = json;
    }

    // Timestamp
    const std::string* timestamp = dict->FindString("timestamp_in_seconds");
    if (timestamp) {
      double timestamp_as_double;
      if (!base::StringToDouble(*timestamp, &timestamp_as_double)) {
        continue;
      }

      confirmation.created_at = base::Time::FromDoubleT(timestamp_as_double);
    }

    // Created
    absl::optional<bool> created = dict->FindBool("created");
    confirmation.was_created = created.value_or(true);

    if (!confirmation.IsValid()) {
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

  unblinded_tokens_->SetTokensFromList(*unblinded_tokens);

  return true;
}

bool ConfirmationStateManager::ParseUnblindedPaymentTokensFromDictionary(
    const base::Value::Dict& dict) {
  const base::Value::List* unblinded_tokens =
      dict.FindList("unblinded_payment_tokens");
  if (!unblinded_tokens) {
    return false;
  }

  unblinded_payment_tokens_->SetTokensFromList(*unblinded_tokens);

  return true;
}

}  // namespace ads
