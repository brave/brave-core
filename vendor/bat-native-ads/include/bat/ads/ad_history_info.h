/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_HISTORY_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_HISTORY_INFO_H_

#include <cstdint>
#include <string>

#include "bat/ads/ad_content_info.h"
#include "bat/ads/category_content_info.h"
#include "bat/ads/export.h"

namespace ads {

struct ADS_EXPORT AdHistoryInfo {
  AdHistoryInfo();
  AdHistoryInfo(const AdHistoryInfo& info);
  ~AdHistoryInfo();

  bool operator==(const AdHistoryInfo& rhs) const;
  bool operator!=(const AdHistoryInfo& rhs) const;

  std::string ToJson() const;
  bool FromJson(const std::string& json);

  uint64_t timestamp_in_seconds = 0;
  AdContentInfo ad_content;
  CategoryContentInfo category_content;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_HISTORY_INFO_H_
