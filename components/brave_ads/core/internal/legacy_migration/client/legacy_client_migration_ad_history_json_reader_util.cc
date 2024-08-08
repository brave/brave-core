/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration_ad_history_json_reader_util.h"

#include "brave/components/brave_ads/core/public/history/ad_history_item_value_util.h"

namespace brave_ads::json::reader {

namespace {
constexpr char kAdHistoryKey[] = "adsShownHistory";
}  // namespace

std::optional<AdHistoryList> ParseAdHistory(const base::Value::Dict& dict) {
  const auto* const list = dict.FindList(kAdHistoryKey);
  if (!list) {
    return std::nullopt;
  }

  AdHistoryList ad_history;
  ad_history.reserve(list->size());

  for (const auto& value : *list) {
    if (const auto* const ad_history_item = value.GetIfDict()) {
      ad_history.push_back(AdHistoryItemFromValue(*ad_history_item));
    }
  }

  return ad_history;
}

}  // namespace brave_ads::json::reader
