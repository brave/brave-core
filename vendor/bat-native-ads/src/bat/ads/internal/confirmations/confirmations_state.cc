/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/confirmations/confirmations_state.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "wrapper.hpp"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/server/ad_rewards/ad_rewards.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/time_util.h"

namespace ads {

using challenge_bypass_ristretto::PublicKey;
using challenge_bypass_ristretto::UnblindedToken;

ConfirmationsState::ConfirmationsState(
    AdsImpl* ads)
    : ads_(ads),
      unblinded_tokens_(std::make_unique<privacy::UnblindedTokens>(ads_)),
      unblinded_payment_tokens_(std::make_unique<
          privacy::UnblindedTokens>(ads_)) {
  DCHECK(ads_);
}

ConfirmationsState::~ConfirmationsState() = default;

std::string ConfirmationsState::ToJson() {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  // Catalog issuers
  base::Value catalog_issuers_dictionary = catalog_issuers_.ToDictionary();
  dictionary.SetKey("catalog_issuers",
      base::Value(std::move(catalog_issuers_dictionary)));

  // Next token redemption date
  dictionary.SetKey("next_token_redemption_date_in_seconds",
      base::Value(std::to_string(static_cast<uint64_t>(
          next_token_redemption_date_.ToDoubleT()))));

  // Confirmations
  base::Value confirmations = GetConfirmationsAsDictionary(confirmations_);
  dictionary.SetKey("confirmations",
      base::Value(std::move(confirmations)));

  // Ad rewards
  base::Value ad_rewards = ads_->get_ad_rewards()->GetAsDictionary();
  dictionary.SetKey("ads_rewards",
      base::Value(std::move(ad_rewards)));

  // Transaction history
  base::Value transactions = GetTransactionsAsDictionary(transactions_);
  dictionary.SetKey("transaction_history",
      base::Value(std::move(transactions)));

  // Unblinded tokens
  base::Value unblinded_tokens = unblinded_tokens_->GetTokensAsList();
  dictionary.SetKey("unblinded_tokens",
      base::Value(std::move(unblinded_tokens)));

  // Unblinded payment tokens
  base::Value unblinded_payment_tokens =
      unblinded_payment_tokens_->GetTokensAsList();
  dictionary.SetKey("unblinded_payment_tokens",
      base::Value(std::move(unblinded_payment_tokens)));

  // Write to JSON
  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

bool ConfirmationsState::FromJson(
    const std::string& json) {
  base::Optional<base::Value> value = base::JSONReader::Read(json);
  if (!value || !value->is_dict()) {
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return false;
  }

  if (!ParseCatalogIssuersFromDictionary(dictionary)) {
    BLOG(1, "Failed to parse catalog issuers");
  }

  if (!ParseNextTokenRedemptionDateFromDictionary(dictionary)) {
    BLOG(1, "Failed to parse next token redemption date");
  }

  if (!ParseConfirmationsFromDictionary(dictionary)) {
    BLOG(1, "Failed to parse confirmations");
  }

  if (!ParseAdRewardsFromDictionary(dictionary)) {
    BLOG(1, "Failed to parse ad rewards");
  }

  if (!ParseTransactionsFromDictionary(dictionary)) {
    BLOG(1, "Failed to parse transactions");
  }

  if (!ParseUnblindedTokensFromDictionary(dictionary)) {
    BLOG(1, "Failed to parse unblinded tokens");
  }

  if (!ParseUnblindedPaymentTokensFromDictionary(dictionary)) {
    BLOG(1, "Failed to parse unblinded payment tokens");
  }

  return true;
}

CatalogIssuersInfo ConfirmationsState::get_catalog_issuers() const {
  return catalog_issuers_;
}

void ConfirmationsState::set_catalog_issuers(
    const CatalogIssuersInfo& catalog_issuers) {
  catalog_issuers_ = catalog_issuers;
}

ConfirmationList ConfirmationsState::get_confirmations() const {
  return confirmations_;
}

void ConfirmationsState::append_confirmation(
    const ConfirmationInfo& confirmation) {
  confirmations_.push_back(confirmation);
}

bool ConfirmationsState::remove_confirmation(
    const ConfirmationInfo& confirmation) {
  const auto iter = std::find_if(confirmations_.begin(), confirmations_.end(),
      [&confirmation](const ConfirmationInfo& info) {
    return (info.id == confirmation.id);
  });

  if (iter == confirmations_.end()) {
    return false;
  }

  confirmations_.erase(iter);

  return true;
}

TransactionList ConfirmationsState::get_transactions() const {
  return transactions_;
}

void ConfirmationsState::append_transaction(
    const TransactionInfo& transaction) {
  transactions_.push_back(transaction);
}

base::Time ConfirmationsState::get_next_token_redemption_date() const {
  return next_token_redemption_date_;
}

void ConfirmationsState::set_next_token_redemption_date(
    const base::Time& next_token_redemption_date) {
  next_token_redemption_date_ = next_token_redemption_date;
}

privacy::UnblindedTokens* ConfirmationsState::get_unblinded_tokens() const {
  return unblinded_tokens_.get();
}

privacy::UnblindedTokens*
ConfirmationsState::get_unblinded_payment_tokens() const {
  return unblinded_payment_tokens_.get();
}

///////////////////////////////////////////////////////////////////////////////

bool ConfirmationsState::ParseCatalogIssuersFromDictionary(
    base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  base::Value* catalog_issuers_dictionary =
      dictionary->FindDictKey("catalog_issuers");
  if (!catalog_issuers_dictionary) {
    return false;
  }

  if (!catalog_issuers_.FromDictionary(catalog_issuers_dictionary)) {
    return false;
  }

  return true;
}

base::Value ConfirmationsState::GetConfirmationsAsDictionary(
    const ConfirmationList& confirmations) const {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  base::Value list(base::Value::Type::LIST);
  for (const auto& confirmation : confirmations) {
    base::Value confirmation_dictionary(base::Value::Type::DICTIONARY);

    confirmation_dictionary.SetKey("id", base::Value(confirmation.id));

    confirmation_dictionary.SetKey("creative_instance_id",
        base::Value(confirmation.creative_instance_id));

    std::string type = std::string(confirmation.type);
    confirmation_dictionary.SetKey("type", base::Value(type));

    base::Value token_info_dictionary(base::Value::Type::DICTIONARY);
    const std::string unblinded_token_base64 =
        confirmation.unblinded_token.value.encode_base64();
    token_info_dictionary.SetKey("unblinded_token",
        base::Value(unblinded_token_base64));
    const std::string public_key_base64 =
        confirmation.unblinded_token.public_key.encode_base64();
    token_info_dictionary.SetKey("public_key", base::Value(public_key_base64));
    confirmation_dictionary.SetKey("token_info",
        base::Value(std::move(token_info_dictionary)));

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

    confirmation_dictionary.SetKey("timestamp_in_seconds",
        base::Value(std::to_string(confirmation.timestamp_in_seconds)));

    confirmation_dictionary.SetKey("created",
        base::Value(confirmation.created));

    list.Append(std::move(confirmation_dictionary));
  }

  dictionary.SetKey("failed_confirmations", base::Value(std::move(list)));

  return dictionary;
}

bool ConfirmationsState::GetConfirmationsFromDictionary(
    base::Value* dictionary,
    ConfirmationList* confirmations) {
  DCHECK(dictionary);
  DCHECK(confirmations);

  // Confirmations
  const base::Value* confirmations_list =
      dictionary->FindListKey("failed_confirmations");
  if (!confirmations_list) {
    BLOG(0, "Confirmations dictionary missing confirmations list");
    return false;
  }

  ConfirmationList new_confirmations;

  for (const auto& value : confirmations_list->GetList()) {
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

    // Token info
    const base::Value* token_info_dictionary =
        confirmation_dictionary->FindDictKey("token_info");
    if (!token_info_dictionary) {
      BLOG(0, "Confirmation missing token_info");
      continue;
    }

    const std::string* unblinded_token_base64 =
        token_info_dictionary->FindStringKey("unblinded_token");
    if (!unblinded_token_base64) {
      // Unblinded token missing, skip confirmation
      BLOG(0, "Token info missing unblinded_token");
      continue;
    }
    confirmation.unblinded_token.value =
        UnblindedToken::decode_base64(*unblinded_token_base64);

    const std::string* public_key_base64 =
        token_info_dictionary->FindStringKey("public_key");
    if (!public_key_base64) {
      // Public key missing, skip confirmation
      BLOG(0, "Token info missing public_key");
      continue;
    }
    confirmation.unblinded_token.public_key =
        PublicKey::decode_base64(*public_key_base64);

    // Payment token
    const std::string* payment_token_base64 =
        confirmation_dictionary->FindStringKey("payment_token");
    if (!payment_token_base64) {
      // Payment token missing, skip confirmation
      BLOG(0, "Confirmation missing payment_token");
      continue;
    }
    confirmation.payment_token = Token::decode_base64(*payment_token_base64);

    // Blinded payment token
    const std::string* blinded_payment_token_base64 =
        confirmation_dictionary->FindStringKey("blinded_payment_token");
    if (!blinded_payment_token_base64) {
      // Blinded payment token missing, skip confirmation
      BLOG(0, "Confirmation missing blinded_payment_token");
      continue;
    }
    confirmation.blinded_payment_token =
        BlindedToken::decode_base64(*blinded_payment_token_base64);

    // Credential
    const std::string* credential =
        confirmation_dictionary->FindStringKey("credential");
    if (!credential) {
      // Credential missing, skip confirmation
      BLOG(0, "Confirmation missing credential");
      continue;
    }
    confirmation.credential = *credential;

    // Timestamp
    const std::string* timestamp_in_seconds =
        confirmation_dictionary->FindStringKey("timestamp_in_seconds");
    if (timestamp_in_seconds) {
      uint64_t timestamp_in_seconds_as_uint64;
      if (!base::StringToUint64(*timestamp_in_seconds,
          &timestamp_in_seconds_as_uint64)) {
        continue;
      }
      confirmation.timestamp_in_seconds = timestamp_in_seconds_as_uint64;
    }

    // Created
    base::Optional<bool> created =
        confirmation_dictionary->FindBoolKey("created");
    confirmation.created = created.value_or(true);

    new_confirmations.push_back(confirmation);
  }

  *confirmations = new_confirmations;

  return true;
}

bool ConfirmationsState::ParseConfirmationsFromDictionary(
    base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  base::Value* confirmations_dictionary =
      dictionary->FindDictKey("confirmations");
  if (!confirmations_dictionary) {
    return false;
  }

  if (!GetConfirmationsFromDictionary(confirmations_dictionary,
      &confirmations_)) {
    return false;
  }

  return true;
}

base::Value ConfirmationsState::GetTransactionsAsDictionary(
    const TransactionList& transactions) const {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  base::Value list(base::Value::Type::LIST);
  for (const auto& transaction : transactions) {
    base::Value transaction_dictionary(base::Value::Type::DICTIONARY);

    transaction_dictionary.SetKey("timestamp_in_seconds",
        base::Value(std::to_string(transaction.timestamp_in_seconds)));

    transaction_dictionary.SetKey("estimated_redemption_value",
        base::Value(transaction.estimated_redemption_value));

    transaction_dictionary.SetKey("confirmation_type",
        base::Value(transaction.confirmation_type));

    list.Append(std::move(transaction_dictionary));
  }

  dictionary.SetKey("transactions", base::Value(std::move(list)));

  return dictionary;
}

bool ConfirmationsState::GetTransactionsFromDictionary(
    base::Value* dictionary,
    TransactionList* transactions) {
  DCHECK(dictionary);
  DCHECK(transactions);

  // Transaction
  const base::Value* transactions_list =
      dictionary->FindListKey("transactions");
  if (!transactions_list) {
    BLOG(0, "Transactions history dictionary missing transactions");
    return false;
  }

  TransactionList new_transactions;

  for (const auto& value : transactions_list->GetList()) {
    const base::DictionaryValue* transaction_dictionary = nullptr;
    if (!value.GetAsDictionary(&transaction_dictionary)) {
      BLOG(0, "Transaction should be a dictionary");
      continue;
    }

    TransactionInfo transaction;

    // Timestamp
    const std::string* timestamp_in_seconds =
        transaction_dictionary->FindStringKey("timestamp_in_seconds");
    if (timestamp_in_seconds) {
      uint64_t timestamp_in_seconds_as_uint64;
      if (!base::StringToUint64(*timestamp_in_seconds,
          &timestamp_in_seconds_as_uint64)) {
        continue;
      }

      transaction.timestamp_in_seconds =
          MigrateTimestampToDoubleT(timestamp_in_seconds_as_uint64);
    } else {
      // timestamp missing, fallback to default
      transaction.timestamp_in_seconds =
          static_cast<uint64_t>(base::Time::Now().ToDoubleT());
    }

    // Estimated redemption value
    const base::Optional<double> estimated_redemption_value =
        transaction_dictionary->FindDoubleKey("estimated_redemption_value");
    if (!estimated_redemption_value) {
      continue;
    }
    transaction.estimated_redemption_value = *estimated_redemption_value;

    // Confirmation type (>= 0.63.8)
    const std::string* confirmation_type =
        transaction_dictionary->FindStringKey("confirmation_type");
    if (confirmation_type) {
      transaction.confirmation_type = *confirmation_type;
    } else {
      // confirmation type missing, fallback to default
      ConfirmationType type(ConfirmationType::kViewed);
      transaction.confirmation_type = std::string(type);
    }

    new_transactions.push_back(transaction);
  }

  *transactions = new_transactions;

  return true;
}

bool ConfirmationsState::ParseTransactionsFromDictionary(
    base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  base::Value* transactions_dictionary =
      dictionary->FindDictKey("transaction_history");
  if (!transactions_dictionary) {
    return false;
  }

  if (!GetTransactionsFromDictionary(transactions_dictionary, &transactions_)) {
    return false;
  }

  return true;
}

bool ConfirmationsState::ParseNextTokenRedemptionDateFromDictionary(
    base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  const std::string* value =
      dictionary->FindStringKey("next_token_redemption_date_in_seconds");
  if (!value) {
    return false;
  }

  uint64_t value_as_uint64;
  if (!base::StringToUint64(*value, &value_as_uint64)) {
    return false;
  }

  next_token_redemption_date_ = base::Time::FromDoubleT(value_as_uint64);

  return true;
}

bool ConfirmationsState::ParseAdRewardsFromDictionary(
    base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  base::Value* ad_rewards_dictionary = dictionary->FindDictKey("ads_rewards");
  if (!ad_rewards_dictionary) {
    return false;
  }

  ads_->get_ad_rewards()->SetFromDictionary(ad_rewards_dictionary);

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
