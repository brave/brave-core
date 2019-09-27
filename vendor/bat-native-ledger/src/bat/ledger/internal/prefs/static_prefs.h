/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_STATIC_PREFS_H_
#define BRAVELEDGER_STATIC_PREFS_H_

#include <cstdint>
#include "base/time/time.h"

namespace braveledger_ledger {
  
// 24 hours in seconds
static const uint64_t _publishers_list_load_interval =
  base::Time::kHoursPerDay * base::Time::kSecondsPerHour;
  
}  // namespace braveledger_ledger

#endif  // BRAVELEDGER_STATIC_PREFS_H_
