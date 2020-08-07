/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <limits>
#include <map>

#include "bat/ledger/internal/logging/logging.h"
#include "bat/ledger/internal/legacy/client_state.h"
#include "bat/ledger/internal/legacy/wallet_info_state.h"
#include "base/json/json_reader.h"

namespace ledger {

namespace {

// Do not change these values as they are required to transition legacy state
const char kAutoContributeKey[] = "auto_contribute";
const char kBootTimestampKey[] = "bootStamp";
const char kFeeAmountKey[] = "fee_amount";
const char kInlineTipsKey[] = "inlineTip";
const char kReconcileTimestampKey[] = "reconcileStamp";
const char kRewardsEnabledKey[] = "rewards_enabled";
const char kUserChangedFeeKey[] = "user_changed_fee";
const char kWalletInfoKey[] = "walletInfo";

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

  writer->String(kFeeAmountKey);
  writer->Double(properties.fee_amount);

  writer->String(kUserChangedFeeKey);
  writer->Bool(properties.user_changed_fee);

  writer->String(kRewardsEnabledKey);
  writer->Bool(properties.rewards_enabled);

  writer->String(kAutoContributeKey);
  writer->Bool(properties.auto_contribute);

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
