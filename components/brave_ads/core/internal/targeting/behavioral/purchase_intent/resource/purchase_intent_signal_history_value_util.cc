/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_value_util.h"

#include "base/json/values_util.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_info.h"

namespace brave_ads {

base::Value::Dict PurchaseIntentSignalHistoryToValue(
    const PurchaseIntentSignalHistoryInfo& purchase_intent_signal_history) {
  return base::Value::Dict()
      .Set("created_at",
           base::TimeToValue(purchase_intent_signal_history.created_at))
      .Set("weight", purchase_intent_signal_history.weight);
}

PurchaseIntentSignalHistoryInfo PurchaseIntentSignalHistoryFromValue(
    const base::Value::Dict& dict) {
  base::Time created_at;

  if (const auto* const value = dict.Find("created_at")) {
    created_at = base::ValueToTime(value).value_or(base::Time());
  } else if (const auto* const legacy_string_value =
                 dict.FindString("timestamp_in_seconds")) {
    double value_as_double;
    if (base::StringToDouble(*legacy_string_value, &value_as_double)) {
      created_at = base::Time::FromDoubleT(value_as_double);
    }
  }

  const int weight = dict.FindInt("weight").value_or(0);

  return {created_at, weight};
}

}  // namespace brave_ads
