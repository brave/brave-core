/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/common/time_util.h"

namespace braveledger_time_util {

ledger::ActivityMonth GetCurrentMonth() {
  base::Time now = base::Time::Now();
  return GetMonth(now);
}

ledger::ActivityMonth GetMonth(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  return (ledger::ActivityMonth)exploded.month;
}

uint32_t GetCurrentYear() {
  base::Time now = base::Time::Now();
  return GetYear(now);
}

uint32_t GetYear(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  return exploded.year;
}

}  // namespace braveledger_time_util
