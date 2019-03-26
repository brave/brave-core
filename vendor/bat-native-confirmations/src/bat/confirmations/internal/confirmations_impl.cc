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

#include "base/rand_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/time/time.h"

#include "third_party/re2/src/re2/re2.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace confirmations {

ConfirmationsImpl::ConfirmationsImpl(
    ConfirmationsClient* confirmations_client) :
    is_initialized_(false),
    unblinded_tokens_(std::make_unique<UnblindedTokens>(this)),
    unblinded_payment_tokens_(std::make_unique<UnblindedTokens>(this)),
    retry_getting_signed_tokens_timer_id_(0),
    refill_tokens_(std::make_unique<RefillTokens>(
        this, confirmations_client, unblinded_tokens_.get())),
    redeem_token_(std::make_unique<RedeemToken>(this, confirmations_client,
        unblinded_tokens_.get(), unblinded_payment_tokens_.get())),
    payout_redeemed_tokens_timer_id_(0),
    payout_tokens_(std::make_unique<PayoutTokens>(this, confirmations_client,
        unblinded_payment_tokens_.get())),
    next_token_redemption_date_in_seconds_(0),
    state_has_loaded_(false),
    confirmations_client_(confirmations_client) {
}

ConfirmationsImpl::~ConfirmationsImpl() {
  BLOG(INFO) << "Deinitializing Confirmations";

  StopRetryingToGetRefillSignedTokens();
  StopPayingOutRedeemedTokens();
}

void ConfirmationsImpl::Initialize() {
  BLOG(INFO) << "Initializing Confirmations";

  if (is_initialized_) {
    BLOG(INFO) << "Already initialized";

    return;
  }

  LoadState();
}

void ConfirmationsImpl::CheckReady() {
  if (is_initialized_) {
    return;
  }

  if (!state_has_loaded_ ||
      !wallet_info_.IsValid() ||
      catalog_issuers_.empty()) {
    return;
  }

  is_initialized_ = true;
  BLOG(INFO) << "Successfully initialized";

  auto start_timer_in = CalculateTokenRedemptionTimeInSeconds();
  StartPayingOutRedeemedTokens(start_timer_in);

  RefillTokensIfNecessary();
}

void ConfirmationsImpl::NotifyAdsIfConfirmationsIsReady() {
  bool is_ready = unblinded_tokens_->IsEmpty() ? false : true;
  confirmations_client_->SetConfirmationsIsReady(is_ready);
}

std::string ConfirmationsImpl::ToJSON() const {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  // Catalog issuers
  auto catalog_issuers =
      GetCatalogIssuersAsDictionary(public_key_, catalog_issuers_);
  dictionary.SetKey("catalog_issuers", base::Value(std::move(catalog_issuers)));

  // Next token redemption date
  dictionary.SetKey("next_token_redemption_date_in_seconds", base::Value(
      std::to_string(next_token_redemption_date_in_seconds_)));

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

    list.GetList().push_back(std::move(issuer_dictionary));
  }

  dictionary.SetKey("issuers", base::Value(std::move(list)));

  return dictionary;
}

base::Value ConfirmationsImpl::GetTransactionHistoryAsDictionary(
    const std::vector<TransactionInfo>& transaction_history) const {
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

    list.GetList().push_back(std::move(transaction_dictionary));
  }

  dictionary.SetKey("transactions", base::Value(std::move(list)));

  return dictionary;
}

bool ConfirmationsImpl::FromJSON(const std::string& json) {
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

  if (!GetCatalogIssuersFromJSON(dictionary)) {
    BLOG(WARNING) << "Failed to get catalog issuers from JSON: " << json;
  }

  if (!GetNextTokenRedemptionDateInSecondsFromJSON(dictionary)) {
    BLOG(WARNING)
        << "Failed to get next token redemption date in seconds from JSON: "
        << json;
  }

  if (!GetTransactionHistoryFromJSON(dictionary)) {
    BLOG(WARNING) << "Failed to get transaction history from JSON: " << json;
  }

  if (!GetUnblindedTokensFromJSON(dictionary)) {
    BLOG(WARNING) << "Failed to get unblinded tokens from JSON: " << json;
  }

  if (!GetUnblindedPaymentTokensFromJSON(dictionary)) {
    BLOG(WARNING) <<
        "Failed to get unblinded payment tokens from JSON: " << json;
  }

  return true;
}

bool ConfirmationsImpl::GetCatalogIssuersFromJSON(
    base::DictionaryValue* dictionary) {
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
  DCHECK(public_key);
  DCHECK(issuers);

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

bool ConfirmationsImpl::GetNextTokenRedemptionDateInSecondsFromJSON(
    base::DictionaryValue* dictionary) {
  auto* next_token_redemption_date_in_seconds_value =
      dictionary->FindKey("next_token_redemption_date_in_seconds");
  if (!next_token_redemption_date_in_seconds_value) {
    return false;
  }

  next_token_redemption_date_in_seconds_ =
      std::stoull(next_token_redemption_date_in_seconds_value->GetString());

  return true;
}

bool ConfirmationsImpl::GetTransactionHistoryFromJSON(
    base::DictionaryValue* dictionary) {
  auto* transaction_history_value = dictionary->FindKey("transaction_history");
  if (!transaction_history_value) {
    return false;
  }

  base::DictionaryValue* transaction_history_dictionary;
  if (!transaction_history_value->GetAsDictionary(
        &transaction_history_dictionary)) {
    return false;
  }

  std::vector<TransactionInfo> transaction_history;
  if (!GetTransactionHistoryFromDictionary(transaction_history_dictionary,
      &transaction_history)) {
    return false;
  }

  transaction_history_ = transaction_history;

  return true;
}

bool ConfirmationsImpl::GetTransactionHistoryFromDictionary(
    base::DictionaryValue* dictionary,
    std::vector<TransactionInfo>* transaction_history) {
  DCHECK(dictionary);
  DCHECK(transaction_history);

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
      info.timestamp_in_seconds =
          std::stoull(timestamp_in_seconds_value->GetString());
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
      info.confirmation_type = kConfirmationTypeView;
    }

    transaction_history->push_back(info);
  }

  return true;
}

bool ConfirmationsImpl::GetUnblindedTokensFromJSON(
    base::DictionaryValue* dictionary) {
  auto* unblinded_tokens_value = dictionary->FindKey("unblinded_tokens");
  if (!unblinded_tokens_value) {
    return false;
  }

  base::ListValue unblinded_token_values(unblinded_tokens_value->GetList());

  unblinded_tokens_->SetTokensFromList(unblinded_token_values);

  return true;
}

bool ConfirmationsImpl::GetUnblindedPaymentTokensFromJSON(
    base::DictionaryValue* dictionary) {
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
  BLOG(INFO) << "Saving confirmations state";

  DCHECK(state_has_loaded_);

  std::string json = ToJSON();
  auto callback = std::bind(&ConfirmationsImpl::OnStateSaved, this, _1);
  confirmations_client_->SaveState(_confirmations_name, json, callback);

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
  confirmations_client_->LoadState(_confirmations_name, callback);
}

void ConfirmationsImpl::OnStateLoaded(
    const Result result,
    const std::string& json) {
  state_has_loaded_ = true;

  auto confirmations_json = json;

  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to load confirmations state, resetting to default"
        << " values";

    confirmations_json = ToJSON();
  }

  if (!FromJSON(confirmations_json)) {
    BLOG(ERROR) << "Failed to parse confirmations state: "
        << confirmations_json;
    return;
  }

  BLOG(INFO) << "Successfully loaded confirmations state";

  NotifyAdsIfConfirmationsIsReady();

  CheckReady();
}

void ConfirmationsImpl::ResetState() {
  BLOG(INFO) << "Resetting confirmations to default state";

  auto callback = std::bind(&ConfirmationsImpl::OnStateReset, this, _1);
  confirmations_client_->ResetState(_confirmations_name, callback);
}

void ConfirmationsImpl::OnStateReset(const Result result) {
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to reset confirmations state";

    return;
  }

  BLOG(INFO) << "Successfully reset confirmations state";
}

void ConfirmationsImpl::SetWalletInfo(std::unique_ptr<WalletInfo> info) {
  if (info->payment_id.empty() || info->public_key.empty()) {
    return;
  }

  wallet_info_ = WalletInfo(*info);

  BLOG(INFO) << "SetWalletInfo:";
  BLOG(INFO) << "  Payment id: " << wallet_info_.payment_id;
  BLOG(INFO) << "  Public key: " << wallet_info_.public_key;

  CheckReady();
}

void ConfirmationsImpl::SetCatalogIssuers(std::unique_ptr<IssuersInfo> info) {
  BLOG(INFO) << "SetCatalogIssuers:";
  BLOG(INFO) << "  Public key: " << info->public_key;
  BLOG(INFO) << "  Issuers:";

  for (const auto& issuer : info->issuers) {
    BLOG(INFO) << "    Name: " << issuer.name;
    BLOG(INFO) << "    Public key: " << issuer.public_key;
  }

  public_key_ = info->public_key;

  catalog_issuers_.clear();
  for (const auto& issuer : info->issuers) {
    catalog_issuers_.insert({issuer.public_key, issuer.name});
  }

  CheckReady();
}

std::map<std::string, std::string> ConfirmationsImpl::GetCatalogIssuers()
    const {
  return catalog_issuers_;
}

bool ConfirmationsImpl::IsValidPublicKeyForCatalogIssuers(
    const std::string& public_key) const {
  auto it = catalog_issuers_.find(public_key);
  if (it == catalog_issuers_.end()) {
    return false;
  }

  return true;
}

void ConfirmationsImpl::GetTransactionHistory(
    const uint64_t from_timestamp_in_seconds,
    const uint64_t to_timestamp_in_seconds,
    OnGetTransactionHistoryCallback callback) {
  std::vector<TransactionInfo> transactions(transaction_history_.size());

  auto it = std::copy_if(transaction_history_.begin(),
      transaction_history_.end(), transactions.begin(),
      [=](TransactionInfo& info) {
        return info.timestamp_in_seconds >= from_timestamp_in_seconds &&
            info.timestamp_in_seconds <= to_timestamp_in_seconds;
      });

  transactions.resize(std::distance(transactions.begin(), it));

  auto transactions_info = std::make_unique<TransactionsInfo>();
  transactions_info->transactions = transactions;

  callback(std::move(transactions_info));
}

double ConfirmationsImpl::GetEstimatedRedemptionValue(
    const std::string& public_key) const {
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

void ConfirmationsImpl::AppendTransactionToTransactionHistory(
    const double estimated_redemption_value,
    const ConfirmationType confirmation_type) {
  TransactionInfo info;
  info.timestamp_in_seconds = Time::NowInSeconds();
  info.estimated_redemption_value = estimated_redemption_value;

  switch (confirmation_type) {
    case ConfirmationType::UNKNOWN: {
      DCHECK(false) << "Invalid confirmation type";
      break;
    }

    case ConfirmationType::CLICK: {
      info.confirmation_type = kConfirmationTypeClick;
      break;
    }

    case ConfirmationType::DISMISS: {
      info.confirmation_type = kConfirmationTypeDismiss;
      break;
    }

    case ConfirmationType::VIEW: {
      info.confirmation_type = kConfirmationTypeView;
      break;
    }

    case ConfirmationType::LANDED: {
      info.confirmation_type = kConfirmationTypeLanded;
      break;
    }
  }

  transaction_history_.push_back(info);

  confirmations_client_->ConfirmationsTransactionHistoryDidChange();

  SaveState();
}

void ConfirmationsImpl::ConfirmAd(std::unique_ptr<NotificationInfo> info) {
  BLOG(INFO) << "ConfirmAd:";
  BLOG(INFO) << "  creativeSetId: " << info->creative_set_id;
  BLOG(INFO) << "  category: " << info->category;
  BLOG(INFO) << "  notificationUrl: " << info->url;
  BLOG(INFO) << "  notificationText: " << info->text;
  BLOG(INFO) << "  advertiser: " << info->advertiser;
  BLOG(INFO) << "  uuid: " << info->uuid;
  switch (info->type) {
    case ConfirmationType::UNKNOWN: {
      DCHECK(false) << "Invalid confirmation type";
      break;
    }

    case ConfirmationType::CLICK: {
      BLOG(INFO) << "  confirmationType: click";
      break;
    }

    case ConfirmationType::DISMISS: {
      BLOG(INFO) << "  confirmationType: dismiss";
      break;
    }

    case ConfirmationType::VIEW: {
      BLOG(INFO) << "  confirmationType: view";
      break;
    }

    case ConfirmationType::LANDED: {
      BLOG(INFO) << "  confirmationType: landed";
      break;
    }
  }

  redeem_token_->Redeem(info->uuid, info->type);
}

bool ConfirmationsImpl::OnTimer(const uint32_t timer_id) {
  BLOG(INFO) << "OnTimer:" << std::endl
      << "  timer_id: "
      << timer_id << std::endl
      << "  retry_getting_signed_tokens_timer_id_: "
      << retry_getting_signed_tokens_timer_id_ << std::endl
      << "  payout_redeemed_tokens_timer_id_: "
      << payout_redeemed_tokens_timer_id_;

  if (timer_id == retry_getting_signed_tokens_timer_id_) {
    RetryGettingRefillSignedTokens();
    return true;
  } else if (timer_id == payout_redeemed_tokens_timer_id_) {
    PayoutRedeemedTokens();
    return true;
  }

  return false;
}

void ConfirmationsImpl::RefillTokensIfNecessary() const {
  refill_tokens_->Refill(wallet_info_, public_key_);
}

uint64_t ConfirmationsImpl::CalculateTokenRedemptionTimeInSeconds() {
  auto now_in_seconds = Time::NowInSeconds();

  uint64_t start_timer_in;

  if (_is_debug) {
    if (next_token_redemption_date_in_seconds_ - now_in_seconds >=
        kDebugNextTokenRedemptionAfterSeconds) {
      UpdateNextTokenRedemptionDate();
      SaveState();
    }
  }

  if (next_token_redemption_date_in_seconds_ == 0) {
    UpdateNextTokenRedemptionDate();
    SaveState();
  }

  if (now_in_seconds >= next_token_redemption_date_in_seconds_) {
    // Browser was launched after the token redemption date
    start_timer_in = base::RandInt(0, 5 * base::Time::kSecondsPerMinute);
  } else {
    start_timer_in = next_token_redemption_date_in_seconds_ - now_in_seconds;
  }

  auto rand_delay = base::RandInt(0, start_timer_in / 10);
  start_timer_in += rand_delay;

  return start_timer_in;
}

void ConfirmationsImpl::UpdateNextTokenRedemptionDate() {
  next_token_redemption_date_in_seconds_ = Time::NowInSeconds();

  if (!_is_debug) {
    next_token_redemption_date_in_seconds_ +=
        kNextTokenRedemptionAfterSeconds;
  } else {
    next_token_redemption_date_in_seconds_ +=
        kDebugNextTokenRedemptionAfterSeconds;
  }
}

void ConfirmationsImpl::StartPayingOutRedeemedTokens(
    const uint64_t start_timer_in) {
  StopPayingOutRedeemedTokens();

  confirmations_client_->SetTimer(start_timer_in,
      &payout_redeemed_tokens_timer_id_);
  if (payout_redeemed_tokens_timer_id_ == 0) {
    BLOG(ERROR)
        << "Failed to start paying out redeemed tokens due to an invalid timer";
    return;
  }

  BLOG(INFO) << "Start paying out redeemed tokens in " << start_timer_in
      << " seconds";
}

void ConfirmationsImpl::PayoutRedeemedTokens() const {
  payout_tokens_->Payout(wallet_info_);
}

void ConfirmationsImpl::StopPayingOutRedeemedTokens() {
  if (!IsPayingOutRedeemedTokens()) {
    return;
  }

  BLOG(INFO) << "Stopped paying out redeemed tokens";

  confirmations_client_->KillTimer(payout_redeemed_tokens_timer_id_);
  payout_redeemed_tokens_timer_id_ = 0;
}

bool ConfirmationsImpl::IsPayingOutRedeemedTokens() const {
  if (payout_redeemed_tokens_timer_id_ == 0) {
    return false;
  }

  return true;
}

void ConfirmationsImpl::StartRetryingToGetRefillSignedTokens(
    const uint64_t start_timer_in) {
  StopRetryingToGetRefillSignedTokens();

  confirmations_client_->SetTimer(start_timer_in,
      &retry_getting_signed_tokens_timer_id_);
  if (retry_getting_signed_tokens_timer_id_ == 0) {
    BLOG(ERROR)
        << "Failed to start getting signed tokens due to an invalid timer";

    return;
  }

  BLOG(INFO) << "Start getting signed tokens in " << start_timer_in
      << " seconds";
}

void ConfirmationsImpl::RetryGettingRefillSignedTokens() const {
  refill_tokens_->RetryGettingSignedTokens();
}

void ConfirmationsImpl::StopRetryingToGetRefillSignedTokens() {
  if (!IsRetryingToGetRefillSignedTokens()) {
    return;
  }

  BLOG(INFO) << "Stopped getting signed tokens";

  confirmations_client_->KillTimer(retry_getting_signed_tokens_timer_id_);
  retry_getting_signed_tokens_timer_id_ = 0;
}

bool ConfirmationsImpl::IsRetryingToGetRefillSignedTokens() const {
  if (retry_getting_signed_tokens_timer_id_ == 0) {
    return false;
  }

  return true;
}

}  // namespace confirmations
