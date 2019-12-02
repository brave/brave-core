/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/unsigned_tx_state.h"
#include "base/json/json_reader.h"
#include "base/logging.h"

namespace ledger {

namespace {

// Do not change these values as they are required to transition legacy state
const char kAmountKey[] = "amount";
const char kCurrencyKey[] = "currency";
const char kDenominationKey[] = "denomination";
const char kDestinationKey[] = "destination";
const char kUnsignedTxKey[] = "unsignedTx";
const char kAmountPath[] = "denomination.amount";
const char kCurrencyPath[] = "denomination.currency";
const char kDestinationPath[] = "destination";

}  // namespace

UnsignedTxState::UnsignedTxState() = default;

UnsignedTxState::~UnsignedTxState() = default;

bool UnsignedTxState::FromJson(
    const std::string& json,
    UnsignedTxProperties* properties) const {
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

bool UnsignedTxState::FromJsonResponse(
    const std::string& json,
    UnsignedTxProperties* properties) const {
  DCHECK(properties);
  if (!properties) {
    NOTREACHED();
    return false;
  }

  auto json_value = base::JSONReader::Read(json);
  if (!json_value) {
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  json_value->GetAsDictionary(&dictionary);
  if (!dictionary) {
    NOTREACHED();
    return false;
  }

  // Unsigned Tx
  const auto* unsigned_tx_value = dictionary->FindKey(kUnsignedTxKey);
  if (!unsigned_tx_value || !unsigned_tx_value->is_dict()) {
    NOTREACHED();
    return false;
  }

  const base::DictionaryValue* unsigned_tx_dictionary = nullptr;
  unsigned_tx_value->GetAsDictionary(&unsigned_tx_dictionary);
  if (!unsigned_tx_dictionary) {
    NOTREACHED();
    return false;
  }

  return FromDict(unsigned_tx_dictionary, properties);
}

bool UnsignedTxState::FromDict(
    const base::DictionaryValue* dictionary,
    UnsignedTxProperties* properties) const {
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

  UnsignedTxProperties unsigned_tx_properties;

  // Amount
  const auto* amount = dictionary->FindStringPath(kAmountPath);
  if (!amount) {
    return false;
  }
  unsigned_tx_properties.amount = *amount;

  // Currency
  const auto* currency = dictionary->FindStringPath(kCurrencyPath);
  if (!currency) {
    return false;
  }
  unsigned_tx_properties.currency = *currency;

  // Destination
  const auto* destination = dictionary->FindStringPath(kDestinationPath);
  if (!destination) {
    return false;
  }
  unsigned_tx_properties.destination = *destination;

  *properties = unsigned_tx_properties;

  return true;
}

bool UnsignedTxState::ToJson(
    JsonWriter* writer,
    const UnsignedTxProperties& properties) const {
  DCHECK(writer);
  if (!writer) {
    NOTREACHED();
    return false;
  }

  writer->StartObject();

  writer->String(kDenominationKey);
  writer->StartObject();

  writer->String(kAmountKey);
  writer->String(properties.amount.c_str());

  writer->String(kCurrencyKey);
  writer->String(properties.currency.c_str());
  writer->EndObject();

  writer->String(kDestinationKey);
  writer->String(properties.destination.c_str());

  writer->EndObject();

  return true;
}

std::string UnsignedTxState::ToJson(
    const UnsignedTxProperties& properties) const {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  if (!ToJson(&writer, properties)) {
    NOTREACHED();
    return "";
  }

  return buffer.GetString();
}

}  // namespace ledger
