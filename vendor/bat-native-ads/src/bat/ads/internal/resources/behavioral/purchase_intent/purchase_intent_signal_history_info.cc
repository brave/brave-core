/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_signal_history_info.h"

#include "base/check.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ads/internal/base/numbers/number_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace targeting {

PurchaseIntentSignalHistoryInfo::PurchaseIntentSignalHistoryInfo() = default;

PurchaseIntentSignalHistoryInfo::PurchaseIntentSignalHistoryInfo(
    const base::Time created_at,
    const uint16_t weight)
    : created_at(created_at), weight(weight) {}

PurchaseIntentSignalHistoryInfo::PurchaseIntentSignalHistoryInfo(
    const PurchaseIntentSignalHistoryInfo& info) = default;

PurchaseIntentSignalHistoryInfo& PurchaseIntentSignalHistoryInfo::operator=(
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

base::Value::Dict PurchaseIntentSignalHistoryInfo::ToValue() const {
  base::Value::Dict dict;
  dict.Set("timestamp_in_seconds",
           base::NumberToString(created_at.ToDoubleT()));

  dict.Set("weight", static_cast<int>(weight));
  return dict;
}

bool PurchaseIntentSignalHistoryInfo::FromValue(const base::Value::Dict& root) {
  weight = static_cast<uint16_t>(root.FindInt("weight").value_or(0));

  if (const auto* value = root.FindString("timestamp_in_seconds")) {
    double value_as_double = 0;
    if (base::StringToDouble(*value, &value_as_double)) {
      created_at = base::Time::FromDoubleT(value_as_double);
    } else {
      created_at = base::Time();
    }
  } else {
    created_at = base::Time();
  }

  return true;
}

}  // namespace targeting
}  // namespace ads
