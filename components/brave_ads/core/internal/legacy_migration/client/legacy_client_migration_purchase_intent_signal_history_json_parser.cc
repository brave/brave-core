/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration_purchase_intent_signal_history_json_parser.h"

#include <optional>
#include <string_view>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/values_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"

namespace brave_ads::json::reader {

namespace {

constexpr char kPurchaseIntentSignalHistoryKey[] =
    "purchaseIntentSignalHistory";
constexpr char kSignaledAtKey[] = "created_at";
constexpr char kWeightKey[] = "weight";

}  // namespace

std::optional<PurchaseIntentSignalHistoryMap> ParsePurchaseIntentSignalHistory(
    std::string_view json) {
  std::optional<base::DictValue> dict =
      base::JSONReader::ReadDict(json, base::JSON_PARSE_RFC);
  if (!dict) {
    return std::nullopt;
  }

  PurchaseIntentSignalHistoryMap purchase_intent_signal_history;

  const auto* const segments = dict->FindDict(kPurchaseIntentSignalHistoryKey);
  if (!segments) {
    // No purchase intent signal history to migrate.
    return purchase_intent_signal_history;
  }

  for (const auto [segment, history] : *segments) {
    const auto* const items = history.GetIfList();
    if (!items) {
      BLOG(0, "Purchase intent signal history for " << segment
                                                    << " should be a list");
      continue;
    }

    PurchaseIntentSignalHistoryList entries;
    entries.reserve(items->size());

    for (const auto& item : *items) {
      const auto* const item_dict = item.GetIfDict();
      if (!item_dict) {
        BLOG(0, "Purchase intent signal history entry should be a dictionary");
        continue;
      }

      base::Time at;
      if (const auto* const value = item_dict->Find(kSignaledAtKey)) {
        at = base::ValueToTime(value).value_or(base::Time());
      }

      const int weight = item_dict->FindInt(kWeightKey).value_or(0);

      entries.emplace_back(at, weight);
    }

    if (!entries.empty()) {
      purchase_intent_signal_history.emplace(segment, std::move(entries));
    }
  }

  return purchase_intent_signal_history;
}

}  // namespace brave_ads::json::reader
