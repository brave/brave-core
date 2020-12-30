/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CONVERSIONS_AD_CONVERSION_INFO_H_
#define BAT_ADS_INTERNAL_CONVERSIONS_AD_CONVERSION_INFO_H_

#include <stdint.h>

#include <string>
#include <vector>

namespace ads {

struct ConversionInfo {
  ConversionInfo();
  ConversionInfo(
      const ConversionInfo& info);
  ~ConversionInfo();

  bool operator==(
      const ConversionInfo& rhs) const;
  bool operator!=(
      const ConversionInfo& rhs) const;

  enum class SortType {
    kNone = 0,
    kAscendingOrder,
    kDescendingOrder
  };

  std::string creative_set_id;
  std::string type;
  std::string url_pattern;
  int observation_window = 0;
  int64_t expiry_timestamp = 0;
};

using ConversionList = std::vector<ConversionInfo>;

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CONVERSIONS_AD_CONVERSION_INFO_H_
