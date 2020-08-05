/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_ADS_HISTORY_H_
#define BAT_ADS_ADS_HISTORY_H_

#include <string>
#include <vector>

#include "bat/ads/ad_history.h"
#include "bat/ads/export.h"
#include "bat/ads/result.h"

namespace ads {

struct ADS_EXPORT AdsHistory {
  AdsHistory();
  AdsHistory(
      const AdsHistory& history);
  ~AdsHistory();

  enum class FilterType {
    kNone = 0,
    kConfirmationType,
    kAdConversion
  };

  enum class SortType {
    kNone = 0,
    kAscendingOrder,
    kDescendingOrder
  };

  std::string ToJson() const;
  Result FromJson(
      const std::string& json);

  std::vector<AdHistory> entries;
};

}  // namespace ads

#endif  // BAT_ADS_ADS_HISTORY_H_
