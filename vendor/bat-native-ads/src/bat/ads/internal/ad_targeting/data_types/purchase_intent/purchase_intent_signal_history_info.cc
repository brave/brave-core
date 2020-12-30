/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/data_types/purchase_intent/purchase_intent_signal_history_info.h"

#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/json_helper.h"

namespace ads {

PurchaseIntentSignalHistoryInfo::PurchaseIntentSignalHistoryInfo() = default;

PurchaseIntentSignalHistoryInfo::PurchaseIntentSignalHistoryInfo(
    const int64_t timestamp_in_seconds,
    const uint16_t weight)
    : timestamp_in_seconds(timestamp_in_seconds),
      weight(weight) {}

PurchaseIntentSignalHistoryInfo::PurchaseIntentSignalHistoryInfo(
    const PurchaseIntentSignalHistoryInfo& info) = default;

PurchaseIntentSignalHistoryInfo::~PurchaseIntentSignalHistoryInfo() = default;

bool PurchaseIntentSignalHistoryInfo::operator==(
    const PurchaseIntentSignalHistoryInfo& rhs) const {
  return timestamp_in_seconds == rhs.timestamp_in_seconds &&
      weight == rhs.weight;
}

bool PurchaseIntentSignalHistoryInfo::operator!=(
    const PurchaseIntentSignalHistoryInfo& rhs) const {
  return !(*this == rhs);
}

std::string PurchaseIntentSignalHistoryInfo::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result PurchaseIntentSignalHistoryInfo::FromJson(
    const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    BLOG(1, helper::JSON::GetLastError(&document));
    return FAILED;
  }

  if (document.HasMember("timestamp_in_seconds")) {
    timestamp_in_seconds = document["timestamp_in_seconds"].GetInt64();
  }

  if (document.HasMember("weight")) {
    weight = document["weight"].GetUint();
  }

  return SUCCESS;
}

void SaveToJson(JsonWriter* writer,
    const PurchaseIntentSignalHistoryInfo& history) {
  writer->StartObject();

  writer->String("timestamp_in_seconds");
  writer->Int64(history.timestamp_in_seconds);

  writer->String("weight");
  writer->Uint(history.weight);

  writer->EndObject();
}

}  // namespace ads
