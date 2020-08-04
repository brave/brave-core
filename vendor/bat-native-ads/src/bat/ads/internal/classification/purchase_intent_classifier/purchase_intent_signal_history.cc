/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/classification/purchase_intent_classifier/purchase_intent_signal_history.h"

#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/logging.h"

namespace ads {

PurchaseIntentSignalHistory::PurchaseIntentSignalHistory() = default;

PurchaseIntentSignalHistory::PurchaseIntentSignalHistory(
    const PurchaseIntentSignalHistory& properties) = default;

PurchaseIntentSignalHistory::~PurchaseIntentSignalHistory() = default;

bool PurchaseIntentSignalHistory::operator==(
    const PurchaseIntentSignalHistory& rhs) const {
  return timestamp_in_seconds == rhs.timestamp_in_seconds &&
      weight == rhs.weight;
}

bool PurchaseIntentSignalHistory::operator!=(
    const PurchaseIntentSignalHistory& rhs) const {
  return !(*this == rhs);
}

std::string PurchaseIntentSignalHistory::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result PurchaseIntentSignalHistory::FromJson(
    const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    BLOG(1, helper::JSON::GetLastError(&document));
    return FAILED;
  }

  if (document.HasMember("timestamp_in_seconds")) {
    timestamp_in_seconds = document["timestamp_in_seconds"].GetUint64();
  }

  if (document.HasMember("weight")) {
    weight = document["weight"].GetUint();
  }

  return SUCCESS;
}

void SaveToJson(JsonWriter* writer,
    const PurchaseIntentSignalHistory& history) {
  writer->StartObject();

  writer->String("timestamp_in_seconds");
  writer->Uint64(history.timestamp_in_seconds);

  writer->String("weight");
  writer->Uint(history.weight);

  writer->EndObject();
}

}  // namespace ads
