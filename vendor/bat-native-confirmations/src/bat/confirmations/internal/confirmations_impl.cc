/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/confirmations/confirmation_type.h"

#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/logging.h"
#include "bat/confirmations/internal/static_values.h"
#include "bat/confirmations/internal/refill_tokens.h"
#include "bat/confirmations/internal/redeem_token.h"
#include "bat/confirmations/internal/payout_tokens.h"
#include "bat/confirmations/internal/unblinded_tokens.h"
#include "bat/confirmations/internal/time.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/time/time.h"
#include "brave_base/random.h"

#include "third_party/re2/src/re2/re2.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace confirmations {

ConfirmationsImpl::ConfirmationsImpl(
    ConfirmationsClient* confirmations_client) :
    is_initialized_(false),
    unblinded_tokens_(std::make_unique<UnblindedTokens>(this)),
    unblinded_payment_tokens_(std::make_unique<UnblindedTokens>(this)),
    estimated_pending_rewards_(0.0),
    next_payment_date_in_seconds_(0),
    ads_rewards_(std::make_unique<AdsRewards>(this, confirmations_client)),
    refill_tokens_(std::make_unique<RefillTokens>(
        this, confirmations_client, unblinded_tokens_.get())),
    redeem_token_(std::make_unique<RedeemToken>(this, confirmations_client,
        unblinded_tokens_.get(), unblinded_payment_tokens_.get())),
    payout_tokens_(std::make_unique<PayoutTokens>(this, confirmations_client,
        unblinded_payment_tokens_.get())),
    state_has_loaded_(false),
    confirmations_client_(confirmations_client) {
}

ConfirmationsImpl::~ConfirmationsImpl() = default;

ConfirmationsClient* ConfirmationsImpl::get_client() const {
  return confirmations_client_;
}

void ConfirmationsImpl::Initialize(
    OnInitializeCallback callback) {
  BLOG(INFO) << "Initializing confirmations";

  initialize_callback_ = callback;

  if (is_initialized_) {
    BLOG(INFO) << "Already initialized confirmations";

    initialize_callback_(false);
    return;
  }

  LoadState();
}

void ConfirmationsImpl::MaybeStart() {
  DCHECK(state_has_loaded_);

  if (is_initialized_ ||
      !wallet_info_.IsValid() ||
      catalog_issuers_.empty()) {
    return;
  }

  is_initialized_ = true;
  BLOG(INFO) << "Successfully initialized confirmations";

  payout_tokens_->PayoutAfterDelay(wallet_info_);

  RefillTokensIfNecessary();

  StartRetryingFailedConfirmations();
}

void ConfirmationsImpl::NotifyAdsIfConfirmationsIsReady() {
  DCHECK(state_has_loaded_);

  bool is_ready = unblinded_tokens_->IsEmpty() ? false : true;
  confirmations_client_->SetConfirmationsIsReady(is_ready);
}

std::string ConfirmationsImpl::ToJSON() const {
  DCHECK(state_has_loaded_);

  base::Value dictionary(base::Value::Type::DICTIONARY);

  // Catalog issuers
  auto catalog_issuers =
      GetCatalogIssuersAsDictionary(public_key_, catalog_issuers_);
  dictionary.SetKey("catalog_issuers", base::Value(std::move(catalog_issuers)));

  // Next token redemption date
  auto token_redemption_timestamp_in_seconds =
      GetNextTokenRedemptionDateInSeconds();
  dictionary.SetKey("next_token_redemption_date_in_seconds", base::Value(
      std::to_string(token_redemption_timestamp_in_seconds)));

  // Confirmations
  auto confirmations = GetConfirmationsAsDictionary(confirmations_);
  dictionary.SetKey("confirmations", base::Value(std::move(confirmations)));

  // Ads rewards
  auto ads_rewards = ads_rewards_->GetAsDictionary();
  dictionary.SetKey("ads_rewards", base::Value(std::move(ads_rewards)));

  // Transaction history
  auto transaction_history =
      GetTransactionHistoryAsDictionary(transaction_history_);
  dictionary.SetKey("transaction_history", base::Value(
      std::move(transaction_history)));

  // Unblinded tokens
  auto unblinded_tokens = unblinded_tokens_->GetTokensAsList();
  dictionary.SetKey("unblinded_tokens", base::Value(
      std::move(unblinded_tokens)));

  // Unblinded payment tokens
  auto unblinded_payment_tokens = unblinded_payment_tokens_->GetTokensAsList();
  dictionary.SetKey("unblinded_payment_tokens", base::Value(
      std::move(unblinded_payment_tokens)));

  // Write to JSON
  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

base::Value ConfirmationsImpl::GetCatalogIssuersAsDictionary(
    const std::string& public_key,
    const std::map<std::string, std::string>& issuers) const {
  base::Value dictionary(base::Value::Type::DICTIONARY);
  dictionary.SetKey("public_key", base::Value(public_key));

  base::Value list(base::Value::Type::LIST);
  for (const auto& issuer : issuers) {
    base::Value issuer_dictionary(base::Value::Type::DICTIONARY);

    issuer_dictionary.SetKey("name", base::Value(issuer.second));
    issuer_dictionary.SetKey("public_key", base::Value(issuer.first));

    list.Append(std::move(issuer_dictionary));
  }

  dictionary.SetKey("issuers", base::Value(std::move(list)));

  return dictionary;
}

base::Value ConfirmationsImpl::GetConfirmationsAsDictionary(
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
    auto unblinded_token_base64 =
        confirmation.token_info.unblinded_token.encode_base64();
    token_info_dictionary.SetKey("unblinded_token",
        base::Value(unblinded_token_base64));
    auto public_key = confirmation.token_info.public_key;
    token_info_dictionary.SetKey("public_key", base::Value(public_key));
    confirmation_dictionary.SetKey("token_info",
        base::Value(std::move(token_info_dictionary)));

    auto payment_token_base64 = confirmation.payment_token.encode_base64();
    confirmation_dictionary.SetKey("payment_token",
        base::Value(payment_token_base64));

    auto blinded_payment_token_base64 =
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

base::Value ConfirmationsImpl::GetTransactionHistoryAsDictionary(
    const TransactionList& transaction_history) const {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  base::Value list(base::Value::Type::LIST);
  for (const auto& transaction : transaction_history) {
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

bool ConfirmationsImpl::FromJSON(const std::string& json) {
  DCHECK(state_has_loaded_);

  base::Optional<base::Value> value = base::JSONReader::Read(json);
  if (!value || !value->is_dict()) {
    BLOG(ERROR) << "Failed to parse JSON: " << json;
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(ERROR) << "Failed to get dictionary: " << json;
    return false;
  }

  if (!ParseCatalogIssuersFromJSON(dictionary)) {
    BLOG(WARNING) << "Failed to get catalog issuers from JSON: " << json;
  }

  if (!ParseNextTokenRedemptionDateInSecondsFromJSON(dictionary)) {
    BLOG(WARNING)
        << "Failed to get next token redemption date in seconds from JSON: "
        << json;
  }

  if (!ParseConfirmationsFromJSON(dictionary)) {
    BLOG(WARNING) << "Failed to get confirmations from JSON: " << json;
  }

  if (!ads_rewards_->SetFromDictionary(dictionary)) {
    BLOG(WARNING) << "Failed to get ads rewards from JSON: " << json;
  }

  if (!ParseTransactionHistoryFromJSON(dictionary)) {
    BLOG(WARNING) << "Failed to get transaction history from JSON: " << json;
  }

  if (!ParseUnblindedTokensFromJSON(dictionary)) {
    BLOG(WARNING) << "Failed to get unblinded tokens from JSON: " << json;
  }

  if (!ParseUnblindedPaymentTokensFromJSON(dictionary)) {
    BLOG(WARNING) <<
        "Failed to get unblinded payment tokens from JSON: " << json;
  }

  return true;
}

bool ConfirmationsImpl::ParseCatalogIssuersFromJSON(
    base::DictionaryValue* dictionary) {
  DCHECK(dictionary);
  if (!dictionary) {
    return false;
  }

  auto* catalog_issuers_value = dictionary->FindKey("catalog_issuers");
  if (!catalog_issuers_value) {
    return false;
  }

  base::DictionaryValue* catalog_issuers_dictionary;
  if (!catalog_issuers_value->GetAsDictionary(&catalog_issuers_dictionary)) {
    return false;
  }

  std::string public_key;
  std::map<std::string, std::string> catalog_issuers;
  if (!GetCatalogIssuersFromDictionary(catalog_issuers_dictionary, &public_key,
      &catalog_issuers)) {
    return false;
  }

  public_key_ = public_key;
  catalog_issuers_ = catalog_issuers;

  return true;
}

bool ConfirmationsImpl::GetCatalogIssuersFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* public_key,
    std::map<std::string, std::string>* issuers) const {
  DCHECK(dictionary);
  if (!dictionary) {
    return false;
  }

  DCHECK(public_key);
  if (!public_key) {
    return false;
  }

  DCHECK(issuers);
  if (!issuers) {
    return false;
  }

  // Public key
  auto* public_key_value = dictionary->FindKey("public_key");
  if (!public_key_value) {
    return false;
  }
  *public_key = public_key_value->GetString();

  // Issuers
  auto* issuers_value = dictionary->FindKey("issuers");
  if (!issuers_value) {
    return false;
  }

  issuers->clear();
  base::ListValue issuers_list_value(issuers_value->GetList());
  for (auto& issuer_value : issuers_list_value) {
    base::DictionaryValue* issuer_dictionary;
    if (!issuer_value.GetAsDictionary(&issuer_dictionary)) {
      return false;
    }

    // Public key
    auto* public_key_value = issuer_dictionary->FindKey("public_key");
    if (!public_key_value) {
      return false;
    }
    auto public_key = public_key_value->GetString();

    // Name
    auto* name_value = issuer_dictionary->FindKey("name");
    if (!name_value) {
      return false;
    }
    auto name = name_value->GetString();

    issuers->insert({public_key, name});
  }

  return true;
}

bool ConfirmationsImpl::ParseNextTokenRedemptionDateInSecondsFromJSON(
    base::DictionaryValue* dictionary) {
  DCHECK(dictionary);
  if (!dictionary) {
    return false;
  }

  auto* next_token_redemption_date_in_seconds_value =
      dictionary->FindKey("next_token_redemption_date_in_seconds");
  if (!next_token_redemption_date_in_seconds_value) {
    return false;
  }

  auto next_token_redemption_date_in_seconds =
      std::stoull(next_token_redemption_date_in_seconds_value->GetString());

  payout_tokens_->set_token_redemption_timestamp_in_seconds(
      Time::MigrateTimestampToDoubleT(next_token_redemption_date_in_seconds));

  return true;
}

bool ConfirmationsImpl::ParseConfirmationsFromJSON(
    base::DictionaryValue* dictionary) {
  DCHECK(dictionary);
  if (!dictionary) {
    return false;
  }

  auto* confirmations_value = dictionary->FindKey("confirmations");
  if (!confirmations_value) {
    return false;
  }

  base::DictionaryValue* confirmations_dictionary;
  if (!confirmations_value->GetAsDictionary(
        &confirmations_dictionary)) {
    return false;
  }

  ConfirmationList confirmations;
  if (!GetConfirmationsFromDictionary(confirmations_dictionary,
      &confirmations)) {
    return false;
  }

  confirmations_ = confirmations;

  return true;
}

bool ConfirmationsImpl::GetConfirmationsFromDictionary(
    base::DictionaryValue* dictionary,
    ConfirmationList* confirmations) {
  DCHECK(dictionary);
  if (!dictionary) {
    return false;
  }

  DCHECK(confirmations);
  if (!confirmations) {
    return false;
  }

  // Confirmations
  auto* confirmations_value = dictionary->FindKey("failed_confirmations");
  if (!confirmations_value) {
    DCHECK(false) << "Confirmations dictionary missing confirmations";
    return false;
  }

  confirmations->clear();
  base::ListValue confirmations_list_value(confirmations_value->GetList());
  for (auto& confirmation_value : confirmations_list_value) {
    base::DictionaryValue* confirmation_dictionary;
    if (!confirmation_value.GetAsDictionary(&confirmation_dictionary)) {
      DCHECK(false) << "Confirmation should be a dictionary";
      continue;
    }

    ConfirmationInfo confirmation_info;

    // Id
    auto* id_value = confirmation_dictionary->FindKey("id");
    if (id_value) {
      confirmation_info.id = id_value->GetString();
    } else {
      // Id missing, skip confirmation
      DCHECK(false) << "Confirmation missing id";
      continue;
    }

    // Creative instance id
    auto* creative_instance_id_value =
        confirmation_dictionary->FindKey("creative_instance_id");
    if (creative_instance_id_value) {
      confirmation_info.creative_instance_id =
          creative_instance_id_value->GetString();
    } else {
      // Creative instance id missing, skip confirmation
      DCHECK(false) << "Confirmation missing creative_instance_id";
      continue;
    }

    // Type
    auto* type_value = confirmation_dictionary->FindKey("type");
    if (type_value) {
      ConfirmationType type(type_value->GetString());
      confirmation_info.type = type;
    } else {
      // Type missing, skip confirmation
      DCHECK(false) << "Confirmation missing type";
      continue;
    }

    // Token info
    auto* token_info_value = confirmation_dictionary->FindKey("token_info");
    if (!token_info_value) {
      DCHECK(false) << "Confirmation missing token_info";
      continue;
    }

    base::DictionaryValue* token_info_dictionary;
    if (!token_info_value->GetAsDictionary(&token_info_dictionary)) {
      DCHECK(false) << "Token info should be a dictionary";
      continue;
    }

    auto* unblinded_token_value =
        token_info_dictionary->FindKey("unblinded_token");
    if (unblinded_token_value) {
      auto unblinded_token_base64 = unblinded_token_value->GetString();
      confirmation_info.token_info.unblinded_token =
          UnblindedToken::decode_base64(unblinded_token_base64);
    } else {
      // Unblinded token missing, skip confirmation
      DCHECK(false) << "Token info missing unblinded_token";
      continue;
    }

    auto* public_key_value = token_info_dictionary->FindKey("public_key");
    if (public_key_value) {
      confirmation_info.token_info.public_key = public_key_value->GetString();
    } else {
      // Public key missing, skip confirmation
      DCHECK(false) << "Token info missing public_key";
      continue;
    }

    // Payment token
    auto* payment_token_value =
        confirmation_dictionary->FindKey("payment_token");
    if (payment_token_value) {
      auto payment_token_base64 = payment_token_value->GetString();
      confirmation_info.payment_token =
          Token::decode_base64(payment_token_base64);
    } else {
      // Payment token missing, skip confirmation
      DCHECK(false) << "Confirmation missing payment_token";
      continue;
    }

    // Blinded payment token
    auto* blinded_payment_token_value =
        confirmation_dictionary->FindKey("blinded_payment_token");
    if (blinded_payment_token_value) {
      auto blinded_payment_token_base64 =
          blinded_payment_token_value->GetString();
      confirmation_info.blinded_payment_token =
          BlindedToken::decode_base64(blinded_payment_token_base64);
    } else {
      // Blinded payment token missing, skip confirmation
      DCHECK(false) << "Confirmation missing blinded_payment_token";
      continue;
    }

    // Credential
    auto* credential_value = confirmation_dictionary->FindKey("credential");
    if (credential_value) {
      confirmation_info.credential = credential_value->GetString();
    } else {
      // Credential missing, skip confirmation
      DCHECK(false) << "Confirmation missing credential";
      continue;
    }

    // Timestamp
    auto* timestamp_in_seconds_value =
        confirmation_dictionary->FindKey("timestamp_in_seconds");
    if (timestamp_in_seconds_value) {
      auto timestamp_in_seconds =
          std::stoull(timestamp_in_seconds_value->GetString());

      confirmation_info.timestamp_in_seconds = timestamp_in_seconds;
    }

    // Created
    auto* created_value = confirmation_dictionary->FindKey("created");
    if (created_value) {
      confirmation_info.created = created_value->GetBool();
    } else {
      confirmation_info.created = true;
    }

    confirmations->push_back(confirmation_info);
  }

  return true;
}

bool ConfirmationsImpl::ParseTransactionHistoryFromJSON(
    base::DictionaryValue* dictionary) {
  DCHECK(dictionary);
  if (!dictionary) {
    return false;
  }

  auto* transaction_history_value = dictionary->FindKey("transaction_history");
  if (!transaction_history_value) {
    return false;
  }

  base::DictionaryValue* transaction_history_dictionary;
  if (!transaction_history_value->GetAsDictionary(
        &transaction_history_dictionary)) {
    return false;
  }

  TransactionList transaction_history;
  if (!GetTransactionHistoryFromDictionary(transaction_history_dictionary,
      &transaction_history)) {
    return false;
  }

  transaction_history_ = transaction_history;

  return true;
}

bool ConfirmationsImpl::GetTransactionHistoryFromDictionary(
    base::DictionaryValue* dictionary,
    TransactionList* transaction_history) {
  DCHECK(dictionary);
  if (!dictionary) {
    return false;
  }

  DCHECK(transaction_history);
  if (!transaction_history) {
    return false;
  }

  // Transaction
  auto* transactions_value = dictionary->FindKey("transactions");
  if (!transactions_value) {
    DCHECK(false) << "Transactions history dictionary missing transactions";
    return false;
  }

  transaction_history->clear();
  base::ListValue transactions_list_value(transactions_value->GetList());
  for (auto& transaction_value : transactions_list_value) {
    base::DictionaryValue* transaction_dictionary;
    if (!transaction_value.GetAsDictionary(&transaction_dictionary)) {
      DCHECK(false) << "Transaction should be a dictionary";
      continue;
    }

    TransactionInfo info;

    // Timestamp
    auto* timestamp_in_seconds_value =
        transaction_dictionary->FindKey("timestamp_in_seconds");
    if (timestamp_in_seconds_value) {
      auto timestamp_in_seconds =
          std::stoull(timestamp_in_seconds_value->GetString());

      info.timestamp_in_seconds =
          Time::MigrateTimestampToDoubleT(timestamp_in_seconds);
    } else {
      // timestamp missing, fallback to default
      info.timestamp_in_seconds = Time::NowInSeconds();
    }

    // Estimated redemption value
    auto* estimated_redemption_value_value =
        transaction_dictionary->FindKey("estimated_redemption_value");
    if (estimated_redemption_value_value) {
      info.estimated_redemption_value =
          estimated_redemption_value_value->GetDouble();
    } else {
      // estimated redemption value missing, fallback to default
      info.estimated_redemption_value = 0.0;
    }

    // Confirmation type (>= 0.63.8)
    auto* confirmation_type_value =
        transaction_dictionary->FindKey("confirmation_type");
    if (confirmation_type_value) {
      info.confirmation_type = confirmation_type_value->GetString();
    } else {
      // confirmation type missing, fallback to default
      ConfirmationType type(ConfirmationType::kViewed);
      info.confirmation_type = std::string(type);
    }

    transaction_history->push_back(info);
  }

  return true;
}

bool ConfirmationsImpl::ParseUnblindedTokensFromJSON(
    base::DictionaryValue* dictionary) {
  DCHECK(dictionary);
  if (!dictionary) {
    return false;
  }

  auto* unblinded_tokens_value = dictionary->FindKey("unblinded_tokens");
  if (!unblinded_tokens_value) {
    return false;
  }

  base::ListValue unblinded_token_values(unblinded_tokens_value->GetList());

  unblinded_tokens_->SetTokensFromList(unblinded_token_values);

  return true;
}

bool ConfirmationsImpl::ParseUnblindedPaymentTokensFromJSON(
    base::DictionaryValue* dictionary) {
  DCHECK(dictionary);
  if (!dictionary) {
    return false;
  }

  auto* unblinded_payment_tokens_value =
      dictionary->FindKey("unblinded_payment_tokens");
  if (!unblinded_payment_tokens_value) {
    return false;
  }

  base::ListValue unblinded_payment_token_values(
      unblinded_payment_tokens_value->GetList());

  unblinded_payment_tokens_->SetTokensFromList(
      unblinded_payment_token_values);

  return true;
}

void ConfirmationsImpl::SaveState() {
  if (!state_has_loaded_) {
    NOTREACHED();
    return;
  }

  BLOG(INFO) << "Saving confirmations state";

  std::string json = ToJSON();
  auto callback = std::bind(&ConfirmationsImpl::OnStateSaved, this, _1);
  confirmations_client_->SaveState(_confirmations_resource_name, json,
      callback);

  NotifyAdsIfConfirmationsIsReady();
}

void ConfirmationsImpl::OnStateSaved(const Result result) {
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to save confirmations state";
    return;
  }

  BLOG(INFO) << "Successfully saved confirmations state";
}

void ConfirmationsImpl::LoadState() {
  BLOG(INFO) << "Loading confirmations state";

  auto callback = std::bind(&ConfirmationsImpl::OnStateLoaded, this, _1, _2);
  confirmations_client_->LoadState(_confirmations_resource_name, callback);
}

void ConfirmationsImpl::OnStateLoaded(
    Result result,
    const std::string& json) {
  state_has_loaded_ = true;

  auto confirmations_json = json;

  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to load confirmations state, resetting to default"
        << " values";

    confirmations_json = ToJSON();
  } else {
    BLOG(INFO) << "Successfully loaded confirmations state";
  }

  if (!FromJSON(confirmations_json)) {
    state_has_loaded_ = false;

    BLOG(ERROR) << "Failed to parse confirmations state: "
        << confirmations_json;

    confirmations_client_->ConfirmationsTransactionHistoryDidChange();

    initialize_callback_(false);

    return;
  }

  initialize_callback_(true);
}

void ConfirmationsImpl::ResetState() {
  DCHECK(state_has_loaded_);

  BLOG(INFO) << "Resetting confirmations to default state";

  auto callback = std::bind(&ConfirmationsImpl::OnStateReset, this, _1);
  confirmations_client_->ResetState(_confirmations_resource_name, callback);
}

void ConfirmationsImpl::OnStateReset(const Result result) {
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to reset confirmations state";

    return;
  }

  BLOG(INFO) << "Successfully reset confirmations state";
}

void ConfirmationsImpl::SetWalletInfo(std::unique_ptr<WalletInfo> info) {
  if (!state_has_loaded_) {
    return;
  }

  if (!info->IsValid()) {
    BLOG(ERROR) << "SetWalletInfo (Invalid wallet):";
    BLOG(ERROR) << "  Payment id: " << info->payment_id;
    BLOG(ERROR) << "  Private key: " << info->private_key;
    return;
  }

  if (*info == wallet_info_) {
    return;
  }

  wallet_info_ = WalletInfo(*info);

  BLOG(INFO) << "SetWalletInfo:";
  BLOG(INFO) << "  Payment id: " << wallet_info_.payment_id;
  BLOG(INFO) << "  Private key: ********";

  NotifyAdsIfConfirmationsIsReady();

  UpdateAdsRewards(true);

  MaybeStart();
}

void ConfirmationsImpl::SetCatalogIssuers(std::unique_ptr<IssuersInfo> info) {
  DCHECK(state_has_loaded_);
  if (!state_has_loaded_) {
    BLOG(ERROR) <<
        "Unable to set catalog issuers as Confirmations state is not ready";
    return;
  }

  BLOG(INFO) << "SetCatalogIssuers:";
  BLOG(INFO) << "  Public key: " << info->public_key;
  BLOG(INFO) << "  Issuers:";

  for (const auto& issuer : info->issuers) {
    BLOG(INFO) << "    Name: " << issuer.name;
    BLOG(INFO) << "    Public key: " << issuer.public_key;
  }

  const bool public_key_was_rotated =
      !public_key_.empty() && public_key_ != info->public_key;

  public_key_ = info->public_key;

  catalog_issuers_.clear();
  for (const auto& issuer : info->issuers) {
    catalog_issuers_.insert({issuer.public_key, issuer.name});
  }

  if (public_key_was_rotated) {
    unblinded_tokens_->RemoveAllTokens();
    if (is_initialized_) {
      RefillTokensIfNecessary();
    }
  }

  NotifyAdsIfConfirmationsIsReady();

  MaybeStart();
}

std::map<std::string, std::string> ConfirmationsImpl::GetCatalogIssuers()
    const {
  DCHECK(state_has_loaded_);

  return catalog_issuers_;
}

bool ConfirmationsImpl::IsValidPublicKeyForCatalogIssuers(
    const std::string& public_key) const {
  DCHECK(state_has_loaded_);

  auto it = catalog_issuers_.find(public_key);
  if (it == catalog_issuers_.end()) {
    return false;
  }

  return true;
}

void ConfirmationsImpl::AppendConfirmationToQueue(
    const ConfirmationInfo& confirmation_info) {
  DCHECK(state_has_loaded_);

  confirmations_.push_back(confirmation_info);

  SaveState();

  BLOG(INFO) << "Added " << confirmation_info.id
      << " confirmation id with " << confirmation_info.creative_instance_id
      << " creative instance id for " << std::string(confirmation_info.type)
      << " to the confirmations queue";

  StartRetryingFailedConfirmations();
}

void ConfirmationsImpl::RemoveConfirmationFromQueue(
    const ConfirmationInfo& confirmation_info) {
  DCHECK(state_has_loaded_);

  auto it = std::find_if(confirmations_.begin(), confirmations_.end(),
      [=](const ConfirmationInfo& info) {
        return (info.id == confirmation_info.id);
      });

  if (it == confirmations_.end()) {
    BLOG(WARNING) << "Failed to remove " << confirmation_info.id
        << " confirmation id with " << confirmation_info.creative_instance_id
        << " creative instance id for " << std::string(confirmation_info.type)
        << " from the confirmations queue";

    return;
  }

  BLOG(INFO) << "Removed " << confirmation_info.id
      << " confirmation id with " << confirmation_info.creative_instance_id
      << " creative instance id for " << std::string(confirmation_info.type)
      << " from the confirmations queue";

  confirmations_.erase(it);

  SaveState();
}

void ConfirmationsImpl::UpdateAdsRewards(const bool should_refresh) {
  DCHECK(state_has_loaded_);
  if (!state_has_loaded_) {
    BLOG(ERROR) <<
        "Unable to update ads rewards as Confirmations state is not ready";
    return;
  }

  ads_rewards_->Update(wallet_info_, should_refresh);
}

void ConfirmationsImpl::UpdateAdsRewards(
    const double estimated_pending_rewards,
    const uint64_t next_payment_date_in_seconds) {
  DCHECK(state_has_loaded_);

  estimated_pending_rewards_ = estimated_pending_rewards;
  next_payment_date_in_seconds_ = next_payment_date_in_seconds;

  SaveState();

  confirmations_client_->ConfirmationsTransactionHistoryDidChange();
}

void ConfirmationsImpl::GetTransactionHistory(
    OnGetTransactionHistoryCallback callback) {
  DCHECK(state_has_loaded_);
  if (!state_has_loaded_) {
    BLOG(ERROR) <<
        "Unable to get transaction history as Confirmations state is not ready";
    return;
  }

  auto unredeemed_transactions = GetUnredeemedTransactions();
  double unredeemed_estimated_pending_rewards =
      GetEstimatedPendingRewardsForTransactions(unredeemed_transactions);

  auto all_transactions = GetTransactions();
  uint64_t ad_notifications_received_this_month =
      GetAdNotificationsReceivedThisMonthForTransactions(all_transactions);

  auto transactions_info = std::make_unique<TransactionsInfo>();

  transactions_info->estimated_pending_rewards =
      estimated_pending_rewards_ + unredeemed_estimated_pending_rewards;

  transactions_info->next_payment_date_in_seconds =
      next_payment_date_in_seconds_;

  transactions_info->ad_notifications_received_this_month =
      ad_notifications_received_this_month;

  auto to_timestamp_in_seconds = Time::NowInSeconds();
  auto transactions = GetTransactionHistory(0, to_timestamp_in_seconds);
  transactions_info->transactions = transactions;

  callback(std::move(transactions_info));
}

void ConfirmationsImpl::AddUnredeemedTransactionsToPendingRewards() {
  auto unredeemed_transactions = GetUnredeemedTransactions();
  AddTransactionsToPendingRewards(unredeemed_transactions);
}

void ConfirmationsImpl::AddTransactionsToPendingRewards(
    const TransactionList& transactions) {
  estimated_pending_rewards_ +=
      GetEstimatedPendingRewardsForTransactions(transactions);

  confirmations_client_->ConfirmationsTransactionHistoryDidChange();
}

double ConfirmationsImpl::GetEstimatedPendingRewardsForTransactions(
    const TransactionList& transactions) const {
  double estimated_pending_rewards = 0.0;

  for (const auto& transaction : transactions) {
    auto estimated_redemption_value = transaction.estimated_redemption_value;
    if (estimated_redemption_value > 0.0) {
      estimated_pending_rewards += estimated_redemption_value;
    }
  }

  return estimated_pending_rewards;
}

uint64_t ConfirmationsImpl::GetAdNotificationsReceivedThisMonthForTransactions(
    const TransactionList& transactions) const {
  uint64_t ad_notifications_received_this_month = 0;

  auto now = base::Time::Now();
  base::Time::Exploded now_exploded;
  now.UTCExplode(&now_exploded);

  for (const auto& transaction : transactions) {
    auto transaction_timestamp =
        Time::FromDoubleT(transaction.timestamp_in_seconds);

    base::Time::Exploded transaction_timestamp_exploded;
    transaction_timestamp.UTCExplode(&transaction_timestamp_exploded);

    if (transaction_timestamp_exploded.year == now_exploded.year &&
        transaction_timestamp_exploded.month == now_exploded.month &&
        transaction.estimated_redemption_value > 0.0) {
      ad_notifications_received_this_month++;
    }
  }

  return ad_notifications_received_this_month;
}

TransactionList ConfirmationsImpl::GetTransactionHistory(
    const uint64_t from_timestamp_in_seconds,
    const uint64_t to_timestamp_in_seconds) {
  DCHECK(state_has_loaded_);

  TransactionList transactions(transaction_history_.size());

  auto it = std::copy_if(transaction_history_.begin(),
      transaction_history_.end(), transactions.begin(),
      [=](TransactionInfo& info) {
        return info.timestamp_in_seconds >= from_timestamp_in_seconds &&
            info.timestamp_in_seconds <= to_timestamp_in_seconds;
      });

  transactions.resize(std::distance(transactions.begin(), it));

  return transactions;
}

TransactionList ConfirmationsImpl::GetTransactions() const {
  DCHECK(state_has_loaded_);

  return transaction_history_;
}

TransactionList ConfirmationsImpl::GetUnredeemedTransactions() {
  DCHECK(state_has_loaded_);

  auto count = unblinded_payment_tokens_->Count();
  if (count == 0) {
    // There are no outstanding unblinded payment tokens to redeem
    return {};
  }

  // Unredeemed transactions are always at the end of the transaction history
  TransactionList transactions(transaction_history_.end() - count,
      transaction_history_.end());

  return transactions;
}

double ConfirmationsImpl::GetEstimatedRedemptionValue(
    const std::string& public_key) const {
  DCHECK(state_has_loaded_);

  double estimated_redemption_value = 0.0;

  auto it = catalog_issuers_.find(public_key);
  if (it != catalog_issuers_.end()) {
    auto name = it->second;
    if (!re2::RE2::Replace(&name, "BAT", "")) {
      BLOG(ERROR) << "Could not estimate redemption value due to catalog"
          << " issuer name missing BAT";
    }

    estimated_redemption_value = stod(name);
  }

  return estimated_redemption_value;
}

void ConfirmationsImpl::AppendTransactionToHistory(
    const double estimated_redemption_value,
    const ConfirmationType confirmation_type) {
  DCHECK(state_has_loaded_);

  TransactionInfo info;
  info.timestamp_in_seconds = Time::NowInSeconds();
  info.estimated_redemption_value = estimated_redemption_value;
  info.confirmation_type = std::string(confirmation_type);

  transaction_history_.push_back(info);

  SaveState();

  confirmations_client_->ConfirmationsTransactionHistoryDidChange();
}

void ConfirmationsImpl::ConfirmAd(
    const AdInfo& info,
    const ConfirmationType confirmation_type) {
  if (!state_has_loaded_) {
    BLOG(ERROR) << "Unable to confirm ad as Confirmations state is not ready";
    return;
  }

  BLOG(INFO) << "Confirm ad:"
      << std::endl << "  creativeInstanceId: " << info.creative_instance_id
      << std::endl << "  creativeSetId: " << info.creative_set_id
      << std::endl << "  category: " << info.category
      << std::endl << "  targetUrl: " << info.target_url
      << std::endl << "  geoTarget: " << info.geo_target
      << std::endl << "  confirmationType: " << std::string(confirmation_type);

  redeem_token_->Redeem(info, confirmation_type);
}

void ConfirmationsImpl::ConfirmAction(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const ConfirmationType confirmation_type) {
  DCHECK(state_has_loaded_);
  if (!state_has_loaded_) {
    BLOG(ERROR) <<
        "Unable to confirm action as Confirmations state is not ready";
    return;
  }

  BLOG(INFO) << "Confirm action:"
      << std::endl << "  creativeInstanceId: " << creative_instance_id
      << std::endl << "  creativeSetId: " << creative_set_id
      << std::endl << "  confirmationType: " << std::string(confirmation_type);

  redeem_token_->Redeem(creative_instance_id, creative_set_id,
      confirmation_type);
}

void ConfirmationsImpl::RefillTokensIfNecessary() const {
  DCHECK(wallet_info_.IsValid());

  refill_tokens_->Refill(wallet_info_, public_key_);
}

uint64_t ConfirmationsImpl::GetNextTokenRedemptionDateInSeconds() const {
  return payout_tokens_->get_token_redemption_timestamp_in_seconds();
}

void ConfirmationsImpl::StartRetryingFailedConfirmations() {
  if (failed_confirmations_timer_.IsRunning()) {
    return;
  }

  const base::Time time = failed_confirmations_timer_.StartWithPrivacy(
      kRetryFailedConfirmationsAfterSeconds,
          base::BindOnce(&ConfirmationsImpl::RetryFailedConfirmations,
              base::Unretained(this)));

  BLOG(INFO) << "Retry failed confirmations at " << time;
}

void ConfirmationsImpl::RetryFailedConfirmations() {
  if (confirmations_.empty()) {
    BLOG(INFO) << "No failed confirmations to retry";
    return;
  }

  ConfirmationInfo confirmation_info(confirmations_.front());
  RemoveConfirmationFromQueue(confirmation_info);

  redeem_token_->Redeem(confirmation_info);

  StartRetryingFailedConfirmations();
}

}  // namespace confirmations
