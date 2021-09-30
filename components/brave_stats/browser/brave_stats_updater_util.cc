/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ctime>
#include <memory>

#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"

#include "base/environment.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"

namespace brave_stats {

std::string GetDateAsYMD(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  return base::StringPrintf("%d-%02d-%02d", exploded.year, exploded.month,
                            exploded.day_of_month);
}

std::string GetPlatformIdentifier() {
#if defined(OS_WIN)
  if (base::SysInfo::OperatingSystemArchitecture() == "x86")
    return "winia32-bc";
  else
    return "winx64-bc";
#elif defined(OS_MAC)
#if defined(ARCH_CPU_X86_64)
  return "osx-bc";
#elif defined(ARCH_CPU_ARM64)
  return "osxarm64-bc";
#endif
#elif defined(OS_ANDROID)
  return "android-bc";
#elif defined(OS_LINUX)
  return "linux-bc";
#else
  return std::string();
#endif
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

base::Time GetYMDAsDate(const base::StringPiece& ymd) {
  const auto pieces = base::SplitStringPiece(ymd, "-", base::TRIM_WHITESPACE,
                                             base::SPLIT_WANT_NONEMPTY);
  DCHECK_EQ(pieces.size(), 3ull);
  base::Time::Exploded time = {};

  bool ok = base::StringToInt(pieces[0], &time.year);
  DCHECK(ok);
  ok = base::StringToInt(pieces[1], &time.month);
  DCHECK(ok);
  ok = base::StringToInt(pieces[2], &time.day_of_month);
  DCHECK(ok);

  DCHECK(time.HasValidValues());

  base::Time result;
  ok = base::Time::FromLocalExploded(time, &result);
  DCHECK(ok);

  return result;
}

std::string GetAPIKey() {
  std::string api_key = BRAVE_STATS_API_KEY;
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  if (env->HasVar("BRAVE_STATS_API_KEY"))
    env->GetVar("BRAVE_STATS_API_KEY", &api_key);

  return api_key;
}

// This is a helper method for dealing with timestamps set by other services in
// the browser. This method makes the assumption that enabling the service
// required a user interaction, and thus the uasge ping for the current day has
// already fired. All calculations for daily, weekly, and monthly can use a
// caller-specified timestamp as a reference, to accomodate non-reactive
// services (stats_updater).
//
// The method returns a bitstring with the following values according to the
// timestamp. All unannotated fields are unused.
//
// 0b00000000
//        |||
//        |||_____ Daily
//        ||______ Weekly
//        |_______ Monthly
uint8_t UsageBitstringFromTimestamp(const base::Time& time,
                                    const base::Time& reference) {
  base::Time::Exploded target_exploded;
  time.LocalExplode(&target_exploded);
  uint8_t enabled_bitset = kIsInactiveUser;

  base::Time::Exploded reference_exploded;
  reference.LocalExplode(&reference_exploded);
  if (reference_exploded.year == target_exploded.year) {
    if (reference_exploded.month == target_exploded.month) {
      enabled_bitset |= kIsMonthlyUser;
      if (GetIsoWeekNumber(time) == GetIsoWeekNumber(reference)) {
        enabled_bitset |= kIsWeeklyUser;
        if (reference_exploded.day_of_month == target_exploded.day_of_month) {
          enabled_bitset |= kIsDailyUser;
        }
      }
    }
  }
  return enabled_bitset;
}

}  // namespace brave_stats
