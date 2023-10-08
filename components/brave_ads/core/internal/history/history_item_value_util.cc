/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/history/history_item_value_util.h"

#include <utility>

#include "base/json/values_util.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_ads/core/public/history/ad_content_value_util.h"
#include "brave/components/brave_ads/core/public/history/category_content_value_util.h"

namespace brave_ads {

namespace {

constexpr char kCreatedAtKey[] = "created_at";
constexpr char kLegacyCreatedAtKey[] = "timestamp_in_seconds";
constexpr char kAdContentKey[] = "ad_content";
constexpr char kCategoryContentKey[] = "category_content";

constexpr char kUIUuidKey[] = "uuid";
constexpr char kUIJavaScriptTimestampKey[] = "timestampInMilliseconds";
constexpr char kUIDetailRowsKey[] = "adDetailRows";
constexpr char kUIAdContentKey[] = "adContent";
constexpr char kUICategoryContentKey[] = "categoryContent";

base::Value::Dict HistoryItemToValue(const HistoryItemInfo& history_item) {
  return base::Value::Dict()
      .Set(kCreatedAtKey, base::TimeToValue(history_item.created_at))
      .Set(kAdContentKey, AdContentToValue(history_item.ad_content))
      .Set(kCategoryContentKey,
           CategoryContentToValue(history_item.category_content));
}

base::Value::List HistoryItemToDetailRowsValue(
    const HistoryItemInfo& history_item) {
  base::Value::List list;

  auto dict =
      base::Value::Dict()
          .Set(kUIAdContentKey, AdContentToValue(history_item.ad_content))
          .Set(kUICategoryContentKey,
               CategoryContentToValue(history_item.category_content));

  list.Append(std::move(dict));

  return list;
}

HistoryItemInfo HistoryItemFromValue(const base::Value::Dict& dict) {
  HistoryItemInfo history_item;

  if (const auto* const value = dict.Find(kCreatedAtKey)) {
    history_item.created_at = base::ValueToTime(value).value_or(base::Time());
  } else if (const auto* const legacy_string_value =
                 dict.FindString(kLegacyCreatedAtKey)) {
    double value_as_double;
    if (base::StringToDouble(*legacy_string_value, &value_as_double)) {
      history_item.created_at = base::Time::FromDoubleT(value_as_double);
    }
  } else if (const auto legacy_double_value = dict.FindDouble(kCreatedAtKey)) {
    history_item.created_at = base::Time::FromDoubleT(*legacy_double_value);
  }

  if (const auto* const value = dict.FindDict(kAdContentKey)) {
    history_item.ad_content = AdContentFromValue(*value);
  }

  if (const auto* const value = dict.FindDict(kCategoryContentKey)) {
    history_item.category_content = CategoryContentFromValue(*value);
  }

  return history_item;
}

}  // namespace

base::Value::List HistoryItemsToValue(const HistoryItemList& history_items) {
  base::Value::List list;

  for (const auto& history_item : history_items) {
    list.Append(HistoryItemToValue(history_item));
  }

  return list;
}

base::Value::List HistoryItemsToUIValue(const HistoryItemList& history_items) {
  base::Value::List list;

  int uuid = 0;

  for (const auto& history_item : history_items) {
    auto dict =
        base::Value::Dict()
            .Set(kUIUuidKey, base::NumberToString(uuid++))
            .Set(kUIJavaScriptTimestampKey,
                 history_item.created_at.ToJsTimeIgnoringNull())
            .Set(kUIDetailRowsKey, HistoryItemToDetailRowsValue(history_item));

    list.Append(std::move(dict));
  }

  return list;
}

HistoryItemList HistoryItemsFromValue(const base::Value::List& list) {
  HistoryItemList history_items;

  for (const auto& item : list) {
    if (const auto* const item_dict = item.GetIfDict()) {
      history_items.push_back(HistoryItemFromValue(*item_dict));
    }
  }

  return history_items;
}

}  // namespace brave_ads
