/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/history/ad_history_item_value_util.h"

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_value_util_internal.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

namespace brave_ads {

AdHistoryItemInfo AdHistoryItemFromDict(const base::DictValue& dict) {
  AdHistoryItemInfo ad_history_item;

  ParseCreatedAt(dict, ad_history_item);
  ParseAdContent(dict, ad_history_item);
  ParseSegmentContent(dict, ad_history_item);

  return ad_history_item;
}

}  // namespace brave_ads
