/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/history_item_info.h"

#include "bat/ads/internal/base/numbers/number_util.h"

namespace ads {

HistoryItemInfo::HistoryItemInfo() = default;

HistoryItemInfo::HistoryItemInfo(const HistoryItemInfo& info) = default;

HistoryItemInfo& HistoryItemInfo::operator=(const HistoryItemInfo& info) =
    default;

HistoryItemInfo::~HistoryItemInfo() = default;

bool HistoryItemInfo::operator==(const HistoryItemInfo& rhs) const {
  return DoubleEquals(created_at.ToDoubleT(), rhs.created_at.ToDoubleT()) &&
         ad_content == rhs.ad_content &&
         category_content == rhs.category_content;
}

bool HistoryItemInfo::operator!=(const HistoryItemInfo& rhs) const {
  return !(*this == rhs);
}

base::Value::Dict HistoryItemInfo::ToValue() const {
  base::Value::Dict dictionary;

  dictionary.Set("timestamp_in_seconds",
                 base::NumberToString(created_at.ToDoubleT()));
  dictionary.Set("ad_content", ad_content.ToValue());
  dictionary.Set("category_content", category_content.ToValue());
  return dictionary;
}

bool HistoryItemInfo::FromValue(const base::Value::Dict& root) {
  if (const auto value = root.FindDouble("timestamp_in_seconds")) {
    // Migrate legacy timestamp
    created_at = base::Time::FromDoubleT(*value);
  } else if (const auto* value = root.FindString("timestamp_in_seconds")) {
    double timestamp = 0.0;
    if (base::StringToDouble(*value, &timestamp)) {
      created_at = base::Time::FromDoubleT(timestamp);
    }
  }

  if (const auto* value = root.FindDict("ad_content")) {
    if (!ad_content.FromValue(*value)) {
      return false;
    }
  }

  if (const auto* value = root.FindDict("category_content")) {
    category_content.FromValue(*value);
  }

  return true;
}

}  // namespace ads
