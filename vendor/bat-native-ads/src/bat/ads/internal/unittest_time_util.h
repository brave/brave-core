/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_UNITTEST_TIME_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_UNITTEST_TIME_UTIL_H_

#include <cstdint>
#include <string>

#include "base/time/time.h"

namespace ads {

int64_t TimestampFromDateString(const std::string& date);
base::Time TimeFromDateString(const std::string& date);

int64_t DistantPastAsTimestamp();
base::Time DistantPast();
std::string DistantPastAsISO8601();

int64_t NowAsTimestamp();
base::Time Now();
std::string NowAsISO8601();

int64_t DistantFutureAsTimestamp();
base::Time DistantFuture();
std::string DistantFutureAsISO8601();

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_UNITTEST_TIME_UTIL_H_
