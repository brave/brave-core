/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_ADS_HISTORY_ADS_HISTORY_H_
#define BAT_ADS_INTERNAL_ADS_HISTORY_ADS_HISTORY_H_

#include <stdint.h>

#include "bat/ads/ads_history_info.h"

namespace ads {

class AdsImpl;
class ConfirmationType;
struct AdNotificationInfo;
struct NewTabPageAdInfo;

class AdsHistory {
 public:
  AdsHistory(
      AdsImpl* ads);

  ~AdsHistory();

  AdsHistoryInfo Get(
      const AdsHistoryInfo::FilterType filter_type,
      const AdsHistoryInfo::SortType sort_type,
      const uint64_t from_timestamp,
      const uint64_t to_timestamp) const;

  void AddAdNotification(
      const AdNotificationInfo& ad,
      const ConfirmationType& confirmation_type);

  void AddNewTabPageAd(
      const NewTabPageAdInfo& ad,
      const ConfirmationType& confirmation_type);

 private:
  AdsImpl* ads_;  // NOT OWNED
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_ADS_HISTORY_ADS_HISTORY_H_
