/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_HISTORY_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_HISTORY_INFO_H_

#include <string>
#include <vector>

#include "bat/ads/export.h"

namespace ads {

struct AdHistoryInfo;

struct ADS_EXPORT AdsHistoryInfo final {
  AdsHistoryInfo();
  AdsHistoryInfo(const AdsHistoryInfo& info);
  ~AdsHistoryInfo();

  std::string ToJson() const;
  bool FromJson(const std::string& json);

  std::vector<AdHistoryInfo> items;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_HISTORY_INFO_H_
