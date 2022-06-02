/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_HISTORY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_HISTORY_H_

#include "bat/ads/history_filter_types.h"
#include "bat/ads/history_sort_types.h"

namespace base {
class Time;
}  // namespace base

namespace ads {

class ConfirmationType;
struct NotificationAdInfo;
struct HistoryInfo;
struct InlineContentAdInfo;
struct NewTabPageAdInfo;
struct PromotedContentAdInfo;
struct SearchResultAdInfo;

namespace history {

HistoryInfo Get(const HistoryFilterType filter_type,
                const HistorySortType sort_type,
                const base::Time from_time,
                const base::Time to_time);

void AddNotificationAd(const NotificationAdInfo& ad,
                       const ConfirmationType& confirmation_type);

void AddNewTabPageAd(const NewTabPageAdInfo& ad,
                     const ConfirmationType& confirmation_type);

void AddPromotedContentAd(const PromotedContentAdInfo& ad,
                          const ConfirmationType& confirmation_type);

void AddInlineContentAd(const InlineContentAdInfo& ad,
                        const ConfirmationType& confirmation_type);

void AddSearchResultAd(const SearchResultAdInfo& ad,
                       const ConfirmationType& confirmation_type);

}  // namespace history
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_HISTORY_H_
