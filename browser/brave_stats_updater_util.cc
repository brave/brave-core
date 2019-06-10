/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_stats_updater_util.h"

#include "base/strings/stringprintf.h"

namespace brave {

std::string GetDateAsYMD(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  return base::StringPrintf(
      "%d-%02d-%02d", exploded.year, exploded.month, exploded.day_of_month);
}

}  // namespace brave
