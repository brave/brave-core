/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/bundle/creative_daypart_info.h"

namespace ads {

CreativeDaypartInfo::CreativeDaypartInfo() = default;

CreativeDaypartInfo::~CreativeDaypartInfo() = default;

bool CreativeDaypartInfo::operator==(const CreativeDaypartInfo& rhs) const {
  return dow == rhs.dow && start_minute == rhs.start_minute &&
         end_minute == rhs.end_minute;
}

bool CreativeDaypartInfo::operator!=(const CreativeDaypartInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
