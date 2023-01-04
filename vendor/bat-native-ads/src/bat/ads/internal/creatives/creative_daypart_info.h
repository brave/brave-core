/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_CREATIVE_DAYPART_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_CREATIVE_DAYPART_INFO_H_

#include <string>
#include <vector>

#include "base/time/time.h"

namespace ads {

struct CreativeDaypartInfo final {
  std::string dow = "0123456";
  int start_minute = 0;
  int end_minute = (base::Time::kMinutesPerHour * base::Time::kHoursPerDay) - 1;
};

bool operator==(const CreativeDaypartInfo& lhs, const CreativeDaypartInfo& rhs);
bool operator!=(const CreativeDaypartInfo& lhs, const CreativeDaypartInfo& rhs);

using CreativeDaypartList = std::vector<CreativeDaypartInfo>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_CREATIVE_DAYPART_INFO_H_
