/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_value_util.h"

#include "base/json/values_util.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_info.h"

namespace brave_ads {

namespace {

constexpr char kSignaledAtKey[] = "created_at";
constexpr char kWeightKey[] = "weight";
constexpr int kDefaultWeight = 0;

}  // namespace

base::Value::Dict PurchaseIntentSignalHistoryToValue(
    const PurchaseIntentSignalHistoryInfo& purchase_intent_signal_history) {
  return base::Value::Dict()
      .Set(kSignaledAtKey, base::TimeToValue(purchase_intent_signal_history.at))
      .Set(kWeightKey, purchase_intent_signal_history.weight);
}

PurchaseIntentSignalHistoryInfo PurchaseIntentSignalHistoryFromValue(
    const base::Value::Dict& dict) {
  base::Time signaled_at = base::Time();

  if (const auto* const value = dict.Find(kSignaledAtKey)) {
    signaled_at = base::ValueToTime(value).value_or(base::Time());
  }

  const int weight = dict.FindInt(kWeightKey).value_or(kDefaultWeight);

  return {signaled_at, weight};
}

}  // namespace brave_ads
