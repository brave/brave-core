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
#include "bat/ads/internal/number_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace ad_targeting {

namespace {

base::Time GetCreatedAtTime(base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  const std::string* value = dictionary->FindStringKey("timestamp_in_seconds");
  if (!value) {
    return base::Time();
  }

  double value_as_double = 0;
  if (!base::StringToDouble(*value, &value_as_double)) {
    return base::Time();
  }

  return base::Time::FromDoubleT(value_as_double);
}

uint16_t GetWeight(base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  return static_cast<uint16_t>(dictionary->FindIntKey("weight").value_or(0));
}

}  // namespace

PurchaseIntentSignalHistoryInfo::PurchaseIntentSignalHistoryInfo() = default;

PurchaseIntentSignalHistoryInfo::PurchaseIntentSignalHistoryInfo(
    const base::Time& created_at,
    const uint16_t weight)
    : created_at(created_at), weight(weight) {}

PurchaseIntentSignalHistoryInfo::PurchaseIntentSignalHistoryInfo(
    const PurchaseIntentSignalHistoryInfo& info) = default;

PurchaseIntentSignalHistoryInfo::~PurchaseIntentSignalHistoryInfo() = default;

bool PurchaseIntentSignalHistoryInfo::operator==(
    const PurchaseIntentSignalHistoryInfo& rhs) const {
  return DoubleEquals(created_at.ToDoubleT(), rhs.created_at.ToDoubleT()) &&
         weight == rhs.weight;
}

bool PurchaseIntentSignalHistoryInfo::operator!=(
    const PurchaseIntentSignalHistoryInfo& rhs) const {
  return !(*this == rhs);
}

std::string PurchaseIntentSignalHistoryInfo::ToJson() const {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  dictionary.SetKey("timestamp_in_seconds",
                    base::Value(base::NumberToString(created_at.ToDoubleT())));

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

  created_at = GetCreatedAtTime(dictionary);

  weight = GetWeight(dictionary);

  return true;
}

}  // namespace ad_targeting
}  // namespace ads
