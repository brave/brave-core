/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ctime>

#include "brave/browser/brave_stats_updater_util.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"

namespace brave {

std::string GetDateAsYMD(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  return base::StringPrintf("%d-%02d-%02d", exploded.year, exploded.month,
                            exploded.day_of_month);
}

int GetIsoWeekNumber(const base::Time& time) {
  char buffer[24];
  time_t rawtime = time.ToTimeT();
  struct tm* timeinfo = std::localtime(&rawtime);
  strftime(buffer, 24, "%V", timeinfo);

  int week_number = 0;
  if (!base::StringToInt(buffer, &week_number))
    return 0;

  return week_number;
}

}  // namespace brave
