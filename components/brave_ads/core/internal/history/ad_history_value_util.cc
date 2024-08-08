/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/ad_history_value_util.h"

#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_value_util_internal.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_value_util.h"

namespace brave_ads {

namespace {

constexpr char kUuidKey[] = "uuid";
constexpr char kCreatedAtKey[] = "timestampInMilliseconds";
constexpr char kRowKey[] = "adDetailRows";

}  // namespace

base::Value::List AdHistoryToValue(const AdHistoryList& ad_history) {
  base::Value::List list;
  list.reserve(ad_history.size());

  int row = 0;

  for (const auto& ad_history_item : ad_history) {
    list.Append(base::Value::Dict()
                    .Set(kUuidKey, base::NumberToString(row++))
                    .Set(kCreatedAtKey,
                         ad_history_item.created_at
                             .InMillisecondsFSinceUnixEpochIgnoringNull())
                    .Set(kRowKey, base::Value::List().Append(
                                      AdHistoryItemToValue(ad_history_item))));
  }

  return list;
}

}  // namespace brave_ads
