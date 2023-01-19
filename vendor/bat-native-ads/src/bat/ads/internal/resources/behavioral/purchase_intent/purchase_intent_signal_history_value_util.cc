/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_signal_history_value_util.h"

#include "base/strings/string_number_conversions.h"
#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_signal_history_info.h"

namespace ads::targeting {

base::Value::Dict PurchaseIntentSignalHistoryToValue(
    const PurchaseIntentSignalHistoryInfo& purchase_intent_signal_history) {
  base::Value::Dict dict;

  dict.Set("timestamp_in_seconds",
           base::NumberToString(
               purchase_intent_signal_history.created_at.ToDoubleT()));

  dict.Set("weight", int{purchase_intent_signal_history.weight});

  return dict;
}

PurchaseIntentSignalHistoryInfo PurchaseIntentSignalHistoryFromValue(
    const base::Value::Dict& dict) {
  base::Time created_at;
  if (const auto* value = dict.FindString("timestamp_in_seconds")) {
    double value_as_double = 0;
    if (base::StringToDouble(*value, &value_as_double)) {
      created_at = base::Time::FromDoubleT(value_as_double);
    }
  }

  const uint16_t weight =
      static_cast<uint16_t>(dict.FindInt("weight").value_or(0));

  return {created_at, weight};
}

}  // namespace ads::targeting
