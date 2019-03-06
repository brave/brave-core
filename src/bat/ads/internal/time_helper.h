/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_TIME_HELPER_H_
#define BAT_ADS_INTERNAL_TIME_HELPER_H_

#include <stdint.h>
#include <string>

namespace helper {

class Time {
 public:
  static std::string TimeStamp();

  static uint64_t NowInSeconds();
};

}  // namespace helper

#endif  // BAT_ADS_INTERNAL_TIME_HELPER_H_
