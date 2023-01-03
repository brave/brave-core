/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/history_item_value_util.h"

#include "base/strings/string_number_conversions.h"
#include "bat/ads/ad_content_value_util.h"
#include "bat/ads/internal/history/category_content_value_util.h"

namespace ads {

namespace {

constexpr char kCreatedAtKey[] = "timestamp_in_seconds";
constexpr char kAdContentKey[] = "ad_content";
constexpr char kCategoryContentKey[] = "category_content";

constexpr char kUIUuidKey[] = "uuid";
constexpr char kUIJavaScriptTimestampKey[] = "timestampInMilliseconds";
constexpr char kUIDetailRowsKey[] = "adDetailRows";
constexpr char kUIAdContentKey[] = "adContent";
constexpr char kUICategoryContentKey[] = "categoryContent";

base::Value::Dict HistoryItemToValue(const HistoryItemInfo& history_item) {
  base::Value::Dict dict;

  dict.Set(kCreatedAtKey,
           base::NumberToString(history_item.created_at.ToDoubleT()));

  dict.Set(kAdContentKey, AdContentToValue(history_item.ad_content));

  dict.Set(kCategoryContentKey,
           CategoryContentToValue(history_item.category_content));

  return dict;
}

base::Value::List HistoryItemToDetailRowsValue(
    const HistoryItemInfo& history_item) {
  base::Value::List list;

  base::Value::Dict dict;
  dict.Set(kUIAdContentKey, AdContentToValue(history_item.ad_content));
  dict.Set(kUICategoryContentKey,
           ads::CategoryContentToValue(history_item.category_content));

  list.Append(std::move(dict));

  return list;
}

HistoryItemInfo HistoryItemFromValue(const base::Value::Dict& root) {
  HistoryItemInfo history_item;

  if (const auto* created_at_value_string = root.FindString(kCreatedAtKey)) {
    double created_at_as_double = 0.0;
    if (base::StringToDouble(*created_at_value_string, &created_at_as_double)) {
      history_item.created_at = base::Time::FromDoubleT(created_at_as_double);
    }
  } else if (const auto created_at_value_double =
                 root.FindDouble(kCreatedAtKey)) {
    history_item.created_at = base::Time::FromDoubleT(*created_at_value_double);
  }

  if (const auto* value = root.FindDict(kAdContentKey)) {
    history_item.ad_content = AdContentFromValue(*value);
  }

  if (const auto* value = root.FindDict(kCategoryContentKey)) {
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
    base::Value::Dict dict;
    dict.Set(kUIUuidKey, base::NumberToString(uuid++));
    dict.Set(kUIJavaScriptTimestampKey,
             history_item.created_at.ToJsTimeIgnoringNull());
    dict.Set(kUIDetailRowsKey, HistoryItemToDetailRowsValue(history_item));

    list.Append(std::move(dict));
  }

  return list;
}

HistoryItemList HistoryItemsFromValue(const base::Value::List& list) {
  HistoryItemList history_items;

  for (const auto& history_item : list) {
    const base::Value::Dict* const dict = history_item.GetIfDict();
    if (!dict) {
      continue;
    }

    history_items.push_back(HistoryItemFromValue(*dict));
  }

  return history_items;
}

}  // namespace ads
