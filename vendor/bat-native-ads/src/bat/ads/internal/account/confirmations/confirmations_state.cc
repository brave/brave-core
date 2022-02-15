/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/confirmations/confirmations_state.h"

#include <utility>

#include "base/check_op.h"
#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/account/issuers/issuers_value_util.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto_util.h"
#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_tokens.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "wrapper.hpp"

namespace ads {

using challenge_bypass_ristretto::PublicKey;
using challenge_bypass_ristretto::UnblindedToken;

namespace {

ConfirmationsState* g_confirmations_state = nullptr;

const char kConfirmationsFilename[] = "confirmations.json";

}  // namespace

ConfirmationsState::ConfirmationsState()
    : unblinded_tokens_(std::make_unique<privacy::UnblindedTokens>()),
      unblinded_payment_tokens_(
          std::make_unique<privacy::UnblindedPaymentTokens>()) {
  DCHECK_EQ(g_confirmations_state, nullptr);

  g_confirmations_state = this;
}

ConfirmationsState::~ConfirmationsState() {
  DCHECK(g_confirmations_state);
  g_confirmations_state = nullptr;
}

// static
ConfirmationsState* ConfirmationsState::Get() {
  DCHECK(g_confirmations_state);
  return g_confirmations_state;
}

// static
bool ConfirmationsState::HasInstance() {
  return g_confirmations_state;
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

        callback_(/* success */ true);
      });
}

void ConfirmationsState::Save() {
  if (!is_initialized_) {
    return;
  }

  BLOG(9, "Saving confirmations state");

  const std::string json = ToJson();
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

    confirmation_dictionary.SetKey("id", base::Value(confirmation.id));

    confirmation_dictionary.SetKey("transaction_id",
                                   base::Value(confirmation.transaction_id));

    confirmation_dictionary.SetKey(
        "creative_instance_id", base::Value(confirmation.creative_instance_id));

    std::string type = std::string(confirmation.type);
    confirmation_dictionary.SetKey("type", base::Value(type));

    std::string ad_type = std::string(confirmation.ad_type);
    confirmation_dictionary.SetKey("ad_type", base::Value(ad_type));

    base::Value token_info_dictionary(base::Value::Type::DICTIONARY);
    const std::string unblinded_token_base64 =
        confirmation.unblinded_token.value.encode_base64();
    token_info_dictionary.SetKey("unblinded_token",
                                 base::Value(unblinded_token_base64));
    const std::string public_key_base64 =
        confirmation.unblinded_token.public_key.encode_base64();
    token_info_dictionary.SetKey("public_key", base::Value(public_key_base64));
    confirmation_dictionary.SetKey("token_info",
                                   std::move(token_info_dictionary));

    const std::string payment_token_base64 =
        confirmation.payment_token.encode_base64();
    confirmation_dictionary.SetKey("payment_token",
                                   base::Value(payment_token_base64));

    const std::string blinded_payment_token_base64 =
        confirmation.blinded_payment_token.encode_base64();
    confirmation_dictionary.SetKey("blinded_payment_token",
                                   base::Value(blinded_payment_token_base64));

    confirmation_dictionary.SetKey("credential",
                                   base::Value(confirmation.credential));

    absl::optional<base::Value> user_data =
        base::JSONReader::Read(confirmation.user_data);
    if (user_data && user_data->is_dict()) {
      base::DictionaryValue* user_data_dictionary = nullptr;
      if (user_data->GetAsDictionary(&user_data_dictionary)) {
        confirmation_dictionary.SetKey("user_data",
                                       user_data_dictionary->Clone());
      }
    }

    confirmation_dictionary.SetKey(
        "timestamp_in_seconds",
        base::Value(base::NumberToString(confirmation.created_at.ToDoubleT())));

    confirmation_dictionary.SetKey("created",
                                   base::Value(confirmation.was_created));

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

  for (const auto& value : failed_confirmations->GetListDeprecated()) {
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
      confirmation.transaction_id = base::GenerateGUID();
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
    if (!unblinded_token_base64->empty()) {
      confirmation.unblinded_token.value =
          UnblindedToken::decode_base64(*unblinded_token_base64);
      if (privacy::ExceptionOccurred()) {
        BLOG(0, "Invalid unblinded token");
        NOTREACHED();
        continue;
      }
    }

    const std::string* public_key_base64 =
        token_info_dictionary->FindStringKey("public_key");
    if (!public_key_base64) {
      // Public key missing, skip confirmation
      BLOG(0, "Token info missing public_key");
      continue;
    }
    if (!public_key_base64->empty()) {
      confirmation.unblinded_token.public_key =
          PublicKey::decode_base64(*public_key_base64);
      if (privacy::ExceptionOccurred()) {
        BLOG(0, "Invalid public key");
        NOTREACHED();
        continue;
      }
    }

    // Payment token
    const std::string* payment_token_base64 =
        confirmation_dictionary->FindStringKey("payment_token");
    if (!payment_token_base64) {
      // Payment token missing, skip confirmation
      BLOG(0, "Confirmation missing payment_token");
      continue;
    }
    if (!payment_token_base64->empty()) {
      confirmation.payment_token = Token::decode_base64(*payment_token_base64);
      if (privacy::ExceptionOccurred()) {
        BLOG(0, "Invalid payment token");
        NOTREACHED();
        continue;
      }
    }

    // Blinded payment token
    const std::string* blinded_payment_token_base64 =
        confirmation_dictionary->FindStringKey("blinded_payment_token");
    if (!blinded_payment_token_base64) {
      // Blinded payment token missing, skip confirmation
      BLOG(0, "Confirmation missing blinded_payment_token");
      continue;
    }
    if (!blinded_payment_token_base64->empty()) {
      confirmation.blinded_payment_token =
          BlindedToken::decode_base64(*blinded_payment_token_base64);
      if (privacy::ExceptionOccurred()) {
        BLOG(0, "Invalid blinded payment token");
        NOTREACHED();
        continue;
      }
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
