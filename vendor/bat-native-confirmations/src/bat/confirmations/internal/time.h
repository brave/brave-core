/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_TIME_H_
#define BAT_CONFIRMATIONS_INTERNAL_TIME_H_

#include <stdint.h>

namespace confirmations {

class Time {
 public:
  static uint64_t NowInSeconds();
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_TIME_H_
