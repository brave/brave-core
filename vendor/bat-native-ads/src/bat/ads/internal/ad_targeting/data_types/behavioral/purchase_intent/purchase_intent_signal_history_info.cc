/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/data_types/behavioral/purchase_intent/purchase_intent_signal_history_info.h"

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "bat/ads/internal/logging.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace ad_targeting {

namespace {

int64_t GetTimestamp(base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  const std::string* value = dictionary->FindStringKey("timestamp_in_seconds");
  if (!value) {
    return 0;
  }

  int64_t value_as_int64 = 0;
  if (!base::StringToInt64(*value, &value_as_int64)) {
    return 0;
  }

  return value_as_int64;
}

uint16_t GetWeight(base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  return static_cast<uint16_t>(dictionary->FindIntKey("weight").value_or(0));
}

}  // namespace

PurchaseIntentSignalHistoryInfo::PurchaseIntentSignalHistoryInfo() = default;

PurchaseIntentSignalHistoryInfo::PurchaseIntentSignalHistoryInfo(
    const int64_t timestamp_in_seconds,
    const uint16_t weight)
    : timestamp_in_seconds(timestamp_in_seconds), weight(weight) {}

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
  base::Value dictionary(base::Value::Type::DICTIONARY);

  dictionary.SetKey("timestamp_in_seconds",
                    base::Value(std::to_string(timestamp_in_seconds)));

  dictionary.SetKey("weight", base::Value(weight));

  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

bool PurchaseIntentSignalHistoryInfo::FromJson(const std::string& json) {
  absl::optional<base::Value> value = base::JSONReader::Read(json);
  if (!value) {
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return false;
  }

  timestamp_in_seconds = GetTimestamp(dictionary);
  weight = GetWeight(dictionary);

  return true;
}

}  // namespace ad_targeting
}  // namespace ads
