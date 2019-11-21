/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <limits>
#include <map>

#include "bat/ledger/internal/state/client_state.h"
#include "bat/ledger/internal/state/ballot_state.h"
#include "bat/ledger/internal/state/publisher_votes_state.h"
#include "bat/ledger/internal/state/transaction_state.h"
#include "bat/ledger/internal/state/current_reconcile_state.h"
#include "bat/ledger/internal/state/wallet_info_state.h"
#include "bat/ledger/internal/state/wallet_state.h"
#include "base/json/json_reader.h"
#include "base/logging.h"

namespace ledger {

namespace {

// Do not change these values as they are required to transition legacy state
const char kAutoContributeKey[] = "auto_contribute";
const char kBallotsKey[] = "ballots";
const char kPublisherVotesKey[] = "batch";
const char kBootTimestampKey[] = "bootStamp";
const char kCurrentReconcilesKey[] = "current_reconciles";
const char kDaysKey[] = "days";
const char kFeeAmountKey[] = "fee_amount";
const char kFeeCurrencyKey[] = "fee_currency";
const char kInlineTipsKey[] = "inlineTip";
const char kMasterUserTokenKey[] = "masterUserToken";
const char kPersonaIdKey[] = "personaId";
const char kPreFlightKey[] = "preFlight";
const char kReconcileTimestampKey[] = "reconcileStamp";
const char kRegistrarVkKey[] = "registrarVK";
const char kRewardsEnabledKey[] = "rewards_enabled";
const char kSettingsKey[] = "settings";
const char kTransactionsKey[] = "transactions";
const char kUserChangedFeeKey[] = "user_changed_fee";
const char kUserIdKey[] = "userId";
const char kWalletInfoKey[] = "walletInfo";
const char kWalletKey[] = "walletProperties";

}  // namespace

ClientState::ClientState() = default;

ClientState::~ClientState() = default;

bool ClientState::FromJson(
    const std::string& json,
    ClientProperties* properties) const {
  DCHECK(properties);
  if (!properties) {
    NOTREACHED();
    return false;
  }

  auto json_value = base::JSONReader::Read(json);
  if (!json_value) {
    NOTREACHED();
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  json_value->GetAsDictionary(&dictionary);
  if (!dictionary) {
    NOTREACHED();
    return false;
  }

  return FromDict(dictionary, properties);
}

bool ClientState::FromDict(
    const base::DictionaryValue* dictionary,
    ClientProperties* properties) const {
  DCHECK(dictionary);
  if (!dictionary) {
    NOTREACHED();
    return false;
  }

  DCHECK(properties);
  if (!properties) {
    NOTREACHED();
    return false;
  }

  ClientProperties client_properties;

  // Wallet Info
  const auto* wallet_info_value = dictionary->FindKey(kWalletInfoKey);
  if (!wallet_info_value || !wallet_info_value->is_dict()) {
    NOTREACHED();
    return false;
  }

  const base::DictionaryValue* wallet_info_dictionary = nullptr;
  wallet_info_value->GetAsDictionary(&wallet_info_dictionary);
  if (!wallet_info_dictionary) {
    NOTREACHED();
    return false;
  }

  const WalletInfoState wallet_info_state;
  WalletInfoProperties wallet_info;
  if (wallet_info_state.FromDict(wallet_info_dictionary, &wallet_info)) {
    client_properties.wallet_info = wallet_info;
  }

  // Boot Timestamp (There is no support for uint64_t. Writing JSON with such
  // types violates the spec. As we need a uint64_t, we need to use an unsigned
  // int and cast to a uint64_t)
  const auto boot_timestamp = dictionary->FindDoubleKey(kBootTimestampKey);
  if (!boot_timestamp) {
    NOTREACHED();
    return false;
  }
  client_properties.boot_timestamp = static_cast<uint64_t>(*boot_timestamp);

  // Reconcile Timestamp (There is no support for uint64_t. Writing JSON with
  // such types violates the spec. As we need a uint64_t, we need to use an
  // unsigned int and cast to a uint64_t)
  const auto reconcile_timestamp =
      dictionary->FindDoubleKey(kReconcileTimestampKey);
  if (!reconcile_timestamp) {
    NOTREACHED();
    return false;
  }
  client_properties.reconcile_timestamp =
      static_cast<uint64_t>(*reconcile_timestamp);

  // Persona Id
  const auto* persona_id = dictionary->FindStringKey(kPersonaIdKey);
  if (!persona_id) {
    NOTREACHED();
    return false;
  }
  client_properties.persona_id = *persona_id;

  // User Id
  const auto* user_id = dictionary->FindStringKey(kUserIdKey);
  if (!user_id) {
    NOTREACHED();
    return false;
  }
  client_properties.user_id = *user_id;

  // Registrar VK
  const auto* registrar_vk = dictionary->FindStringKey(kRegistrarVkKey);
  if (!registrar_vk) {
    NOTREACHED();
    return false;
  }
  client_properties.registrar_vk = *registrar_vk;

  // Master User Token
  const auto* master_user_token =
      dictionary->FindStringKey(kMasterUserTokenKey);
  if (!master_user_token) {
    NOTREACHED();
    return false;
  }
  client_properties.master_user_token = *master_user_token;

  // Pre Flight
  const auto* pre_flight = dictionary->FindStringKey(kPreFlightKey);
  if (!pre_flight) {
    NOTREACHED();
    return false;
  }
  client_properties.pre_flight = *pre_flight;

  // Fee Currency
  const auto* fee_currency = dictionary->FindStringKey(kFeeCurrencyKey);
  if (!fee_currency) {
    NOTREACHED();
    return false;
  }
  client_properties.fee_currency = *fee_currency;

  // Settings
  const auto* settings = dictionary->FindStringKey(kSettingsKey);
  if (!settings) {
    NOTREACHED();
    return false;
  }
  client_properties.settings = *settings;

  // Fee Amount
  const auto fee_amount = dictionary->FindDoubleKey(kFeeAmountKey);
  if (!fee_amount) {
    NOTREACHED();
    return false;
  }
  client_properties.fee_amount = *fee_amount;

  // User Changed Fee
  const auto user_changed_fee = dictionary->FindBoolKey(kUserChangedFeeKey);
  if (!user_changed_fee) {
    NOTREACHED();
    return false;
  }
  client_properties.user_changed_fee = *user_changed_fee;

  // Days (There is no support for unsigned int. Writing JSON with such types
  // violates the spec. As we need an unsigned int, we need to a double and cast
  // to an unsigned int)
  const auto days = dictionary->FindDoubleKey(kDaysKey);
  if (!days) {
    NOTREACHED();
    return false;
  }
  client_properties.days = static_cast<unsigned int>(*days);

  // Auto Contribute
  const auto auto_contribute = dictionary->FindBoolKey(kAutoContributeKey);
  if (!auto_contribute) {
    NOTREACHED();
    return false;
  }
  client_properties.auto_contribute = *auto_contribute;

  // Rewards Enabled
  const auto rewards_enabled = dictionary->FindBoolKey(kRewardsEnabledKey);
  if (!rewards_enabled) {
    NOTREACHED();
    return false;
  }
  client_properties.rewards_enabled = *rewards_enabled;

  // Transactions
  const auto* transaction_list = dictionary->FindListKey(kTransactionsKey);
  if (!transaction_list) {
    NOTREACHED();
    return false;
  }

  const TransactionState transaction_state;
  for (const auto& transaction_value : transaction_list->GetList()) {
    if (!transaction_value.is_dict()) {
      NOTREACHED();
      continue;
    }

    const base::DictionaryValue* transaction_dictionary = nullptr;
    transaction_value.GetAsDictionary(&transaction_dictionary);
    if (!transaction_dictionary) {
      NOTREACHED();
      continue;
    }

    TransactionProperties transaction;
    if (!transaction_state.FromDict(transaction_dictionary, &transaction)) {
      continue;
    }

    client_properties.transactions.push_back(transaction);
  }

  // Ballots
  const auto* ballot_list = dictionary->FindListKey(kBallotsKey);
  if (!ballot_list) {
    NOTREACHED();
    return false;
  }

  const BallotState ballot_state;
  for (const auto& ballot_value : ballot_list->GetList()) {
    if (!ballot_value.is_dict()) {
      NOTREACHED();
      continue;
    }

    const base::DictionaryValue* ballot_dictionary = nullptr;
    ballot_value.GetAsDictionary(&ballot_dictionary);
    if (!ballot_dictionary) {
      NOTREACHED();
      continue;
    }

    BallotProperties ballot;
    if (!ballot_state.FromDict(ballot_dictionary, &ballot)) {
      continue;
    }

    client_properties.ballots.push_back(ballot);
  }

  // Publisher Votes
  const auto* publisher_votes_list =
      dictionary->FindListKey(kPublisherVotesKey);
  if (!publisher_votes_list) {
    NOTREACHED();
    return false;
  }

  const PublisherVotesState publisher_votes_state;
  for (const auto& publisher_votes_value : publisher_votes_list->GetList()) {
    if (!publisher_votes_value.is_dict()) {
      NOTREACHED();
      continue;
    }

    const base::DictionaryValue* publisher_votes_dictionary = nullptr;
    publisher_votes_value.GetAsDictionary(&publisher_votes_dictionary);
    if (!publisher_votes_dictionary) {
      NOTREACHED();
      continue;
    }

    PublisherVotesProperties publisher_votes;
    if (!publisher_votes_state.FromDict(publisher_votes_dictionary,
        &publisher_votes)) {
      continue;
    }

    client_properties.publisher_votes.push_back(publisher_votes);
  }

  // Current Reconciles
  const auto* current_reconciles_value =
      dictionary->FindKey(kCurrentReconcilesKey);
  if (current_reconciles_value) {
    if (!current_reconciles_value->is_dict()) {
      NOTREACHED();
      return false;
    }

    const CurrentReconcileState current_reconcile_state;
    for (const auto& current_reconcile_value :
        current_reconciles_value->DictItems()) {
      const auto& key = current_reconcile_value.first;
      const auto& value = current_reconcile_value.second;

      if (!value.is_dict()) {
        NOTREACHED();
        continue;
      }

      const base::DictionaryValue* current_reconcile_dictionary = nullptr;
      value.GetAsDictionary(&current_reconcile_dictionary);
      if (!current_reconcile_dictionary) {
        NOTREACHED();
        continue;
      }

      CurrentReconcileProperties current_reconcile;
      if (!current_reconcile_state.FromDict(current_reconcile_dictionary,
          &current_reconcile)) {
        continue;
      }

      client_properties.current_reconciles.insert({key, current_reconcile});
    }
  }

  // Wallet
  const auto* wallet_value = dictionary->FindKey(kWalletKey);
  if (!wallet_value || !wallet_value->is_dict()) {
    NOTREACHED();
    return false;
  }

  const base::DictionaryValue* wallet_dictionary = nullptr;
  wallet_value->GetAsDictionary(&wallet_dictionary);
  if (!wallet_dictionary) {
    NOTREACHED();
    return false;
  }

  WalletProperties wallet;
  const WalletState wallet_state;
  if (!wallet_state.FromDict(wallet_dictionary, &wallet)) {
    return false;
  }
  client_properties.wallet = wallet;

  // Inline Tips
  const auto* inline_tips_value = dictionary->FindKey(kInlineTipsKey);
  if (inline_tips_value) {
    if (!inline_tips_value->is_dict()) {
      NOTREACHED();
      return false;
    }

    for (const auto& inline_tip_value : inline_tips_value->DictItems()) {
      const auto& key = inline_tip_value.first;
      const auto& value = inline_tip_value.second;

      if (!value.is_bool()) {
        NOTREACHED();
        continue;
      }

      client_properties.inline_tips.insert({key, value.GetBool()});
    }
  }

  *properties = client_properties;

  return true;
}

bool ClientState::ToJson(
    JsonWriter* writer,
    const ClientProperties& properties) const {
  DCHECK(writer);
  if (!writer) {
    NOTREACHED();
    return false;
  }

  writer->StartObject();

  writer->String(kWalletInfoKey);
  const WalletInfoState wallet_info_state;
  wallet_info_state.ToJson(writer, properties.wallet_info);

  writer->String(kBootTimestampKey);
  writer->Uint64(properties.boot_timestamp);

  writer->String(kReconcileTimestampKey);
  writer->Uint64(properties.reconcile_timestamp);

  writer->String(kPersonaIdKey);
  writer->String(properties.persona_id.c_str());

  writer->String(kUserIdKey);
  writer->String(properties.user_id.c_str());

  writer->String(kRegistrarVkKey);
  writer->String(properties.registrar_vk.c_str());

  writer->String(kMasterUserTokenKey);
  writer->String(properties.master_user_token.c_str());

  writer->String(kPreFlightKey);
  writer->String(properties.pre_flight.c_str());

  writer->String(kFeeCurrencyKey);
  writer->String(properties.fee_currency.c_str());

  writer->String(kSettingsKey);
  writer->String(properties.settings.c_str());

  writer->String(kFeeAmountKey);
  writer->Double(properties.fee_amount);

  writer->String(kUserChangedFeeKey);
  writer->Bool(properties.user_changed_fee);

  writer->String(kDaysKey);
  writer->Uint(properties.days);

  writer->String(kRewardsEnabledKey);
  writer->Bool(properties.rewards_enabled);

  writer->String(kAutoContributeKey);
  writer->Bool(properties.auto_contribute);

  writer->String(kTransactionsKey);
  writer->StartArray();
  const TransactionState transaction_state;
  for (const auto& transaction : properties.transactions) {
    transaction_state.ToJson(writer, transaction);
  }
  writer->EndArray();

  writer->String(kBallotsKey);
  writer->StartArray();
  const BallotState ballot_state;
  for (const auto& ballot : properties.ballots) {
    if (!ballot_state.ToJson(writer, ballot)) {
      NOTREACHED();
      continue;
    }
  }
  writer->EndArray();

  writer->String(kPublisherVotesKey);
  writer->StartArray();
  const PublisherVotesState publisher_votes_state;
  for (const auto& publisher_vote : properties.publisher_votes) {
    if (!publisher_votes_state.ToJson(writer, publisher_vote)) {
      NOTREACHED();
      continue;
    }
  }
  writer->EndArray();

  writer->String(kCurrentReconcilesKey);
  writer->StartObject();
  const CurrentReconcileState current_reconcile_state;
  for (const auto& current_reconcile : properties.current_reconciles) {
    writer->Key(current_reconcile.first.c_str());
    if (!current_reconcile_state.ToJson(writer, current_reconcile.second)) {
      NOTREACHED();
      continue;
    }
  }
  writer->EndObject();

  writer->String(kWalletKey);
  const WalletState wallet_state;
  wallet_state.ToJson(writer, properties.wallet);

  writer->String(kInlineTipsKey);
  writer->StartObject();
  for (const auto &inline_tip : properties.inline_tips) {
    writer->String(inline_tip.first.c_str());
    writer->Bool(inline_tip.second);
  }
  writer->EndObject();

  writer->EndObject();

  return true;
}

std::string ClientState::ToJson(
    const ClientProperties& properties) const {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  if (!ToJson(&writer, properties)) {
    NOTREACHED();
    return "";
  }

  return buffer.GetString();
}

}  // namespace ledger
