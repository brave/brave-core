/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/wallet_state.h"
#include "base/json/json_reader.h"
#include "base/logging.h"

namespace ledger {

namespace {

// Do not change these values as they are required to transition legacy state
const char kFeeAmountKey[] = "fee_amount";
const char kParametersKey[] = "parameters";
const char kAdFreeKey[] = "adFree";
const char kFeeKey[] = "fee";
const char kBatKey[] = "BAT";
const char kChoicesKey[] = "choices";
const char kChoicesBatPath[] = "parameters.adFree.choices.BAT";
const char kFeeBatPath[] = "parameters.adFree.fee.BAT";
const char kUserFundsPresentPath[] = "userFundsPresent";

}  // namespace

WalletState::WalletState() = default;

WalletState::~WalletState() = default;

bool WalletState::FromJson(
    const std::string& json,
    WalletProperties* properties) const {
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

bool WalletState::FromDict(
    const base::DictionaryValue* dictionary,
    WalletProperties* properties) const {
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

  WalletProperties wallet_properties;

  // Fee Amount
  const auto fee_amount = dictionary->FindDoublePath(kFeeBatPath);
  if (!fee_amount) {
    NOTREACHED();
    return false;
  }
  wallet_properties.fee_amount = *fee_amount;

  // Choices BAT
  const auto* choices_bat_list = dictionary->FindListPath(kChoicesBatPath);
  if (!choices_bat_list) {
    NOTREACHED();
    return false;
  }

  for (const auto&choices_bat_value : choices_bat_list->GetList()) {
    const double bat = choices_bat_value.GetDouble();
    wallet_properties.parameters_choices.push_back(bat);
  }

  // User funds present
  const auto user_funds_present =
      dictionary->FindBoolPath(kUserFundsPresentPath);
  if (!user_funds_present) {
    wallet_properties.user_funds_present = false;
  } else {
    wallet_properties.user_funds_present = *user_funds_present;
  }

  *properties = wallet_properties;

  return true;
}

bool WalletState::ToJson(
    JsonWriter* writer,
    const WalletProperties& properties) const {
  DCHECK(writer);
  if (!writer) {
    NOTREACHED();
    return false;
  }

  writer->StartObject();

  writer->String(kFeeAmountKey);
  writer->Double(properties.fee_amount);

  writer->String(kParametersKey);
  writer->StartObject();
  writer->String(kAdFreeKey);
  writer->StartObject();

  writer->String(kFeeKey);
  writer->StartObject();
  writer->String(kBatKey);
  writer->Double(properties.fee_amount);
  writer->EndObject();

  writer->String(kChoicesKey);
  writer->StartObject();
  writer->String(kBatKey);

  writer->StartArray();
  for (const auto& parameters_choice : properties.parameters_choices) {
    writer->Double(parameters_choice);
  }
  writer->EndArray();
  writer->EndObject();

  writer->EndObject();
  writer->EndObject();
  writer->EndObject();

  return true;
}

std::string WalletState::ToJson(
    const WalletProperties& properties) const {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  if (!ToJson(&writer, properties)) {
    NOTREACHED();
    return "";
  }

  return buffer.GetString();
}

}  // namespace ledger
