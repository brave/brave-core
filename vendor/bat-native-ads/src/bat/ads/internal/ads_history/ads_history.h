/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_ADS_HISTORY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_ADS_HISTORY_H_

#include "bat/ads/ads_history_filter_types.h"
#include "bat/ads/ads_history_sort_types.h"

namespace base {
class Time;
}  // namespace base

namespace ads {

class ConfirmationType;
struct AdNotificationInfo;
struct AdsHistoryInfo;
struct InlineContentAdInfo;
struct NewTabPageAdInfo;
struct PromotedContentAdInfo;

namespace history {

const int kForDays = 30;

AdsHistoryInfo Get(const AdsHistoryFilterType filter_type,
                   const AdsHistorySortType sort_type,
                   const base::Time& from,
                   const base::Time& to);

void AddAdNotification(const AdNotificationInfo& ad,
                       const ConfirmationType& confirmation_type);

void AddNewTabPageAd(const NewTabPageAdInfo& ad,
                     const ConfirmationType& confirmation_type);

void AddPromotedContentAd(const PromotedContentAdInfo& ad,
                          const ConfirmationType& confirmation_type);

void AddInlineContentAd(const InlineContentAdInfo& ad,
                        const ConfirmationType& confirmation_type);

}  // namespace history
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_ADS_HISTORY_H_
