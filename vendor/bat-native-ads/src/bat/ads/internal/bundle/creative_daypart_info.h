/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_DAYPART_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_DAYPART_INFO_H_

#include <string>

#include "base/time/time.h"

namespace ads {

struct CreativeDaypartInfo final {
  CreativeDaypartInfo();
  ~CreativeDaypartInfo();

  bool operator==(const CreativeDaypartInfo& rhs) const;
  bool operator!=(const CreativeDaypartInfo& rhs) const;

  std::string dow = "0123456";
  int start_minute = 0;
  int end_minute = (base::Time::kMinutesPerHour * base::Time::kHoursPerDay) - 1;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_DAYPART_INFO_H_
