/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/transaction_ballot_state.h"
#include "base/json/json_reader.h"
#include "base/logging.h"

namespace ledger {

namespace {

// Do not change these values as they are required to transition legacy state
const char kOffsetKey[] = "offset";
const char kPublisherKey[] = "publisher";

}  // namespace

TransactionBallotState::TransactionBallotState() = default;

TransactionBallotState::~TransactionBallotState() = default;

bool TransactionBallotState::FromJson(
    const std::string& json,
    TransactionBallotProperties* properties) const {
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

bool TransactionBallotState::FromDict(
    const base::DictionaryValue* dictionary,
    TransactionBallotProperties* properties) const {
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

  TransactionBallotProperties transaction_ballot_properties;

  // Publisher
  const auto* publisher = dictionary->FindStringKey(kPublisherKey);
  if (!publisher) {
    NOTREACHED();
    return false;
  }
  transaction_ballot_properties.publisher = *publisher;

  // Offset (There is no support for unsigned int. Writing JSON with such types
  // violates the spec. As we need an unsigned int, we need to a double and cast
  // to an unsigned int)
  const auto count = dictionary->FindDoubleKey(kOffsetKey);
  if (!count) {
    NOTREACHED();
    return false;
  }
  transaction_ballot_properties.count = static_cast<unsigned int>(*count);

  *properties = transaction_ballot_properties;

  return true;
}

bool TransactionBallotState::ToJson(
    JsonWriter* writer,
    const TransactionBallotProperties& properties) const {
  DCHECK(writer);
  if (!writer) {
    NOTREACHED();
    return false;
  }

  writer->StartObject();

  writer->String(kPublisherKey);
  writer->String(properties.publisher.c_str());

  writer->String(kOffsetKey);
  writer->Uint(properties.count);

  writer->EndObject();

  return true;
}

std::string TransactionBallotState::ToJson(
    const TransactionBallotProperties& properties) const {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  if (!ToJson(&writer, properties)) {
    NOTREACHED();
    return "";
  }

  return buffer.GetString();
}

}  // namespace ledger
