/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/reconcile_direction_state.h"
#include "base/json/json_reader.h"
#include "base/logging.h"

namespace ledger {

namespace {

// Do not change these values as they are required to transition legacy state
const char kAmountKey[] = "amount";
const char kAmountPercentKey[] = "amount_percent";
const char kPublisherKeyKey[] = "publisher_key";

}  // namespace

ReconcileDirectionState::ReconcileDirectionState() = default;

ReconcileDirectionState::~ReconcileDirectionState() = default;

bool ReconcileDirectionState::FromJson(
    const std::string& json,
    ReconcileDirectionProperties* properties) const {
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

bool ReconcileDirectionState::FromDict(
    const base::DictionaryValue* dictionary,
    ReconcileDirectionProperties* properties) const {
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

  ReconcileDirectionProperties reconcile_direction_properties;

  // Publisher Key
  const auto* publisher_key = dictionary->FindStringKey(kPublisherKeyKey);
  if (!publisher_key) {
    NOTREACHED();
    return false;
  }
  reconcile_direction_properties.publisher_key = *publisher_key;

  // Amount Percent
  const auto amount_percent = dictionary->FindDoubleKey(kAmountPercentKey);
  if (amount_percent) {
    reconcile_direction_properties.amount_percent = *amount_percent;
  } else {
    const auto amount_percent = dictionary->FindDoubleKey(kAmountKey);
    if (!amount_percent) {
      NOTREACHED();
      return false;
    }
  }

  *properties = reconcile_direction_properties;

  return true;
}

bool ReconcileDirectionState::ToJson(
    JsonWriter* writer,
    const ReconcileDirectionProperties& properties) const {
  DCHECK(writer);
  if (!writer) {
    NOTREACHED();
    return false;
  }

  writer->StartObject();

  writer->String(kAmountPercentKey);
  writer->Double(properties.amount_percent);

  writer->String(kPublisherKeyKey);
  writer->String(properties.publisher_key.c_str());

  writer->EndObject();

  return true;
}

std::string ReconcileDirectionState::ToJson(
    const ReconcileDirectionProperties& properties) const {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  if (!ToJson(&writer, properties)) {
    NOTREACHED();
    return "";
  }

  return buffer.GetString();
}

}  // namespace ledger
