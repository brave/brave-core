/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/deprecated/confirmations/confirmations_state.h"

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
#include "base/values.h"
#include "bat/ads/internal/account/issuers/issuers_value_util.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
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

ConfirmationsState* g_confirmations_state_instance = nullptr;

constexpr char kConfirmationsFilename[] = "confirmations.json";

uint64_t GenerateHash(const std::string& value) {
  return static_cast<uint64_t>(base::PersistentHash(value));
}

void SetHash(const std::string& value) {
  AdsClientHelper::Get()->SetUint64Pref(prefs::kConfirmationsHash,
                                        GenerateHash(value));
}

bool IsMutated(const std::string& value) {
  return AdsClientHelper::Get()->GetUint64Pref(prefs::kConfirmationsHash) !=
         GenerateHash(value);
}

}  // namespace

ConfirmationsState::ConfirmationsState()
    : unblinded_tokens_(std::make_unique<privacy::UnblindedTokens>()),
      unblinded_payment_tokens_(
          std::make_unique<privacy::UnblindedPaymentTokens>()) {
  DCHECK(!g_confirmations_state_instance);
  g_confirmations_state_instance = this;
}

ConfirmationsState::~ConfirmationsState() {
  DCHECK_EQ(this, g_confirmations_state_instance);
  g_confirmations_state_instance = nullptr;
}

// static
ConfirmationsState* ConfirmationsState::Get() {
  DCHECK(g_confirmations_state_instance);
  return g_confirmations_state_instance;
}

// static
bool ConfirmationsState::HasInstance() {
  return !!g_confirmations_state_instance;
}

void ConfirmationsState::Initialize(InitializeCallback callback) {
  callback_ = callback;

  Load();
}

void ConfirmationsState::Load() {
  BLOG(3, "Loading confirmations state");

  AdsClientHelper::Get()->Load(
      kConfirmationsFilename, [=](const bool success, const std::string& json) {
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

        callback_(/* success */ true);
      });
}

void ConfirmationsState::Save() {
  if (!is_initialized_) {
    return;
  }

  BLOG(9, "Saving confirmations state");

  const std::string json = ToJson();

  SetHash(json);

  AdsClientHelper::Get()->Save(
      kConfirmationsFilename, json, [](const bool success) {
        if (!success) {
          BLOG(0, "Failed to save confirmations state");
          return;
        }

        BLOG(9, "Successfully saved confirmations state");
      });
}

ConfirmationList ConfirmationsState::GetFailedConfirmations() const {
  DCHECK(is_initialized_);
  return failed_confirmations_;
}

void ConfirmationsState::AppendFailedConfirmation(
    const ConfirmationInfo& confirmation) {
  DCHECK(confirmation.IsValid());

  DCHECK(is_initialized_);
  failed_confirmations_.push_back(confirmation);
}

bool ConfirmationsState::RemoveFailedConfirmation(
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

///////////////////////////////////////////////////////////////////////////////

std::string ConfirmationsState::ToJson() {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  // Issuers
  base::Value issuers = IssuerListToValue(issuers_);
  dictionary.SetKey("issuers", std::move(issuers));

  // Confirmations
  base::Value failed_confirmations =
      GetFailedConfirmationsAsDictionary(failed_confirmations_);
  dictionary.SetKey("confirmations", std::move(failed_confirmations));

  // Unblinded tokens
  base::Value unblinded_tokens = unblinded_tokens_->GetTokensAsList();
  dictionary.SetKey("unblinded_tokens", std::move(unblinded_tokens));

  // Unblinded payment tokens
  base::Value unblinded_payment_tokens =
      unblinded_payment_tokens_->GetTokensAsList();
  dictionary.SetKey("unblinded_payment_tokens",
                    std::move(unblinded_payment_tokens));

  // Write to JSON
  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

bool ConfirmationsState::FromJson(const std::string& json) {
  absl::optional<base::Value> value = base::JSONReader::Read(json);
  if (!value || !value->is_dict()) {
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return false;
  }

  if (!ParseIssuersFromDictionary(dictionary)) {
    BLOG(1, "Failed to parse issuers");
  }

  if (!ParseFailedConfirmationsFromDictionary(dictionary)) {
    BLOG(1, "Failed to parse failed confirmations");
  }

  if (!ParseUnblindedTokensFromDictionary(dictionary)) {
    BLOG(1, "Failed to parse unblinded tokens");
  }

  if (!ParseUnblindedPaymentTokensFromDictionary(dictionary)) {
    BLOG(1, "Failed to parse unblinded payment tokens");
  }

  return true;
}

void ConfirmationsState::SetIssuers(const IssuerList& issuers) {
  DCHECK(is_initialized_);
  issuers_ = issuers;
}

IssuerList ConfirmationsState::GetIssuers() const {
  DCHECK(is_initialized_);
  return issuers_;
}

base::Value ConfirmationsState::GetFailedConfirmationsAsDictionary(
    const ConfirmationList& confirmations) const {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  base::Value list(base::Value::Type::LIST);
  for (const auto& confirmation : confirmations) {
    DCHECK(confirmation.IsValid());

    base::Value confirmation_dictionary(base::Value::Type::DICTIONARY);

    confirmation_dictionary.SetStringKey("id", confirmation.id);

    confirmation_dictionary.SetStringKey("transaction_id",
                                         confirmation.transaction_id);

    confirmation_dictionary.SetStringKey("creative_instance_id",
                                         confirmation.creative_instance_id);

    confirmation_dictionary.SetStringKey("type", confirmation.type.ToString());

    confirmation_dictionary.SetStringKey("ad_type",
                                         confirmation.ad_type.ToString());

    base::Value token_info_dictionary(base::Value::Type::DICTIONARY);
    const absl::optional<std::string> unblinded_token_base64_optional =
        confirmation.unblinded_token.value.EncodeBase64();
    if (!unblinded_token_base64_optional) {
      NOTREACHED();
      continue;
    }
    token_info_dictionary.SetStringKey("unblinded_token",
                                       unblinded_token_base64_optional.value());
    const absl::optional<std::string> public_key_base64_optional =
        confirmation.unblinded_token.public_key.EncodeBase64();
    if (!public_key_base64_optional) {
      NOTREACHED();
      continue;
    }
    token_info_dictionary.SetStringKey("public_key",
                                       public_key_base64_optional.value());
    confirmation_dictionary.SetKey("token_info",
                                   std::move(token_info_dictionary));

    const absl::optional<std::string> payment_token_base64_optional =
        confirmation.payment_token.EncodeBase64();
    if (!payment_token_base64_optional) {
      NOTREACHED();
      continue;
    }
    confirmation_dictionary.SetStringKey("payment_token",
                                         payment_token_base64_optional.value());

    const absl::optional<std::string> blinded_payment_token_base64_optional =
        confirmation.blinded_payment_token.EncodeBase64();
    if (!blinded_payment_token_base64_optional) {
      NOTREACHED();
      continue;
    }
    confirmation_dictionary.SetStringKey(
        "blinded_payment_token", blinded_payment_token_base64_optional.value());

    confirmation_dictionary.SetStringKey("credential", confirmation.credential);

    absl::optional<base::Value> user_data =
        base::JSONReader::Read(confirmation.user_data);
    if (user_data && user_data->is_dict()) {
      base::DictionaryValue* user_data_dictionary = nullptr;
      if (user_data->GetAsDictionary(&user_data_dictionary)) {
        confirmation_dictionary.SetKey("user_data",
                                       user_data_dictionary->Clone());
      }
    }

    confirmation_dictionary.SetStringKey(
        "timestamp_in_seconds",
        base::NumberToString(confirmation.created_at.ToDoubleT()));

    confirmation_dictionary.SetBoolKey("created", confirmation.was_created);

    list.Append(std::move(confirmation_dictionary));
  }

  dictionary.SetKey("failed_confirmations", std::move(list));

  return dictionary;
}

bool ConfirmationsState::GetFailedConfirmationsFromDictionary(
    base::Value* dictionary,
    ConfirmationList* confirmations) {
  DCHECK(dictionary);
  DCHECK(confirmations);

  // Confirmations
  const base::Value* failed_confirmations =
      dictionary->FindListKey("failed_confirmations");
  if (!failed_confirmations) {
    BLOG(0, "Failed confirmations dictionary missing failed confirmations");
    return false;
  }

  ConfirmationList new_failed_confirmations;

  for (const auto& value : failed_confirmations->GetList()) {
    const base::DictionaryValue* confirmation_dictionary = nullptr;
    if (!value.GetAsDictionary(&confirmation_dictionary)) {
      BLOG(0, "Confirmation should be a dictionary");
      continue;
    }

    ConfirmationInfo confirmation;

    // Id
    const std::string* id = confirmation_dictionary->FindStringKey("id");
    if (!id) {
      // Id missing, skip confirmation
      BLOG(0, "Confirmation missing id");
      continue;
    }
    confirmation.id = *id;

    // Transaction id
    const std::string* transaction_id =
        confirmation_dictionary->FindStringKey("transaction_id");
    if (!transaction_id) {
      // Migrate legacy confirmations
      confirmation.transaction_id =
          base::GUID::GenerateRandomV4().AsLowercaseString();
    } else {
      confirmation.transaction_id = *transaction_id;
    }

    // Creative instance id
    const std::string* creative_instance_id =
        confirmation_dictionary->FindStringKey("creative_instance_id");
    if (!creative_instance_id) {
      // Creative instance id missing, skip confirmation
      BLOG(0, "Confirmation missing creative_instance_id");
      continue;
    }
    confirmation.creative_instance_id = *creative_instance_id;

    // Type
    const std::string* type = confirmation_dictionary->FindStringKey("type");
    if (!type) {
      // Type missing, skip confirmation
      BLOG(0, "Confirmation missing type");
      continue;
    }
    ConfirmationType confirmation_type(*type);
    confirmation.type = confirmation_type;

    // Ad type
    const std::string* ad_type =
        confirmation_dictionary->FindStringKey("ad_type");
    if (ad_type) {
      confirmation.ad_type = AdType(*ad_type);
    } else {
      // Migrate legacy confirmations, this value is not used right now so safe
      // to set to |kAdNotification|
      confirmation.ad_type = AdType::kAdNotification;
    }

    // Token info
    const base::Value* token_info_dictionary =
        confirmation_dictionary->FindDictKey("token_info");
    if (!token_info_dictionary) {
      BLOG(0, "Confirmation missing token_info");
      continue;
    }

    // Unblinded token
    const std::string* unblinded_token_base64 =
        token_info_dictionary->FindStringKey("unblinded_token");
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
        token_info_dictionary->FindStringKey("public_key");
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
    const std::string* payment_token_base64 =
        confirmation_dictionary->FindStringKey("payment_token");
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
        confirmation_dictionary->FindStringKey("blinded_payment_token");
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
    const std::string* credential =
        confirmation_dictionary->FindStringKey("credential");
    if (!credential) {
      // Credential missing, skip confirmation
      BLOG(0, "Confirmation missing credential");
      continue;
    }
    confirmation.credential = *credential;

    // User data
    const base::Value* user_data_dictionary =
        confirmation_dictionary->FindDictKey("user_data");
    if (user_data_dictionary) {
      std::string json;
      base::JSONWriter::Write(*user_data_dictionary, &json);
      confirmation.user_data = json;
    }

    // Timestamp
    const std::string* timestamp =
        confirmation_dictionary->FindStringKey("timestamp_in_seconds");
    if (timestamp) {
      double timestamp_as_double;
      if (!base::StringToDouble(*timestamp, &timestamp_as_double)) {
        continue;
      }

      confirmation.created_at = base::Time::FromDoubleT(timestamp_as_double);
    }

    // Created
    absl::optional<bool> created =
        confirmation_dictionary->FindBoolKey("created");
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

bool ConfirmationsState::ParseIssuersFromDictionary(
    base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  base::Value* value = dictionary->FindListKey("issuers");
  if (!value || !value->is_list()) {
    return false;
  }

  const absl::optional<IssuerList>& issuers = ValueToIssuerList(*value);
  if (!issuers) {
    return false;
  }

  issuers_ = issuers.value();

  return true;
}

bool ConfirmationsState::ParseFailedConfirmationsFromDictionary(
    base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  base::Value* failed_confirmations_dictionary =
      dictionary->FindDictKey("confirmations");
  if (!failed_confirmations_dictionary) {
    return false;
  }

  if (!GetFailedConfirmationsFromDictionary(failed_confirmations_dictionary,
                                            &failed_confirmations_)) {
    return false;
  }

  return true;
}

bool ConfirmationsState::ParseUnblindedTokensFromDictionary(
    base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  const base::Value* unblinded_tokens_list =
      dictionary->FindListKey("unblinded_tokens");
  if (!unblinded_tokens_list) {
    return false;
  }

  unblinded_tokens_->SetTokensFromList(*unblinded_tokens_list);

  return true;
}

bool ConfirmationsState::ParseUnblindedPaymentTokensFromDictionary(
    base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  const base::Value* unblinded_tokens_list =
      dictionary->FindListKey("unblinded_payment_tokens");
  if (!unblinded_tokens_list) {
    return false;
  }

  unblinded_payment_tokens_->SetTokensFromList(*unblinded_tokens_list);

  return true;
}

}  // namespace ads
