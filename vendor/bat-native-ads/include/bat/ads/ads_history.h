/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_ADS_HISTORY_H_
#define BAT_ADS_ADS_HISTORY_H_

#include <string>
#include <vector>

#include "bat/ads/export.h"
#include "bat/ads/result.h"

namespace ads {

struct AdHistoryDetail;

enum class AdsHistoryFilterType {
  kNone = 0,
  kConfirmationType
};

struct ADS_EXPORT AdsHistory {
  AdsHistory();
  explicit AdsHistory(const AdsHistory& history);
  ~AdsHistory();

  const std::string ToJson() const;
  Result FromJson(
      const std::string& json,
      std::string* error_description = nullptr);

  // Chronologically sorted vector of ad history detail entries
  std::vector<AdHistoryDetail> details;
};

}  // namespace ads

#endif  // BAT_ADS_ADS_HISTORY_H_
