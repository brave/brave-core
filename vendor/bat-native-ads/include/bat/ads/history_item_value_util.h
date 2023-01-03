/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_HISTORY_ITEM_VALUE_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_HISTORY_ITEM_VALUE_UTIL_H_

#include "base/values.h"

#include "bat/ads/history_item_info.h"

namespace ads {

base::Value::List HistoryItemsToValue(const HistoryItemList& history_items);
base::Value::List HistoryItemsToUIValue(const HistoryItemList& history_items);
HistoryItemList HistoryItemsFromValue(const base::Value::List& list);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_HISTORY_ITEM_VALUE_UTIL_H_
