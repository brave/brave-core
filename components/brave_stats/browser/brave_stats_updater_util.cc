/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ctime>
#include <memory>
#include <string_view>

#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"

#include "base/environment.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_stats/browser/buildflags.h"
#include "build/build_config.h"

namespace brave_stats {

std::string GetDateAsYMD(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  return base::StringPrintf("%d-%02d-%02d", exploded.year, exploded.month,
                            exploded.day_of_month);
}

std::string GetPlatformIdentifier() {
#if BUILDFLAG(IS_WIN)
#if defined(ARCH_CPU_ARM64)
  return "winarm64-bc";
#else
  if (base::SysInfo::OperatingSystemArchitecture() == "x86")
    return "winia32-bc";
  else
    return "winx64-bc";
#endif
#elif BUILDFLAG(IS_MAC)
#if defined(ARCH_CPU_ARM64)
  return "osxarm64-bc";
#elif defined(ARCH_CPU_X86_64)
  return "osx-bc";
#endif
#elif BUILDFLAG(IS_ANDROID)
  return "android-bc";
#elif BUILDFLAG(IS_LINUX)
#if defined(ARCH_CPU_ARM64)
  return "linuxarm64-bc";
#else
  return "linux-bc";
#endif
#elif BUILDFLAG(IS_IOS)
  return "ios";
#else
  return std::string();
#endif
}

std::string GetGeneralPlatformIdentifier() {
#if BUILDFLAG(IS_WIN)
  return "windows";
#elif BUILDFLAG(IS_MAC)
  return "macos";
#elif BUILDFLAG(IS_LINUX)
  return "linux";
#elif BUILDFLAG(IS_IOS)
  return "ios";
#elif BUILDFLAG(IS_ANDROID)
  return "android";
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

base::Time GetLastMondayTime(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);

  int days_adjusted =
      (exploded.day_of_week == 0) ? 6 : exploded.day_of_week - 1;
  base::Time last_monday = base::Time::FromMillisecondsSinceUnixEpoch(
      time.InMillisecondsFSinceUnixEpoch() -
      (days_adjusted * base::Time::kMillisecondsPerDay));

  return last_monday;
}

base::Time GetYMDAsDate(std::string_view ymd) {
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
  std::string api_key = BUILDFLAG(BRAVE_STATS_API_KEY);
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
uint8_t UsageBitfieldFromTimestamp(const base::Time& last_usage_time,
                                   const base::Time& last_reported_usage_time) {
  uint8_t result = kIsInactiveUser;

  base::Time::Exploded usage_time_exp;
  last_usage_time.LocalExplode(&usage_time_exp);

  base::Time::Exploded report_time_exp;
  last_reported_usage_time.LocalExplode(&report_time_exp);

  bool is_year_diff = report_time_exp.year != usage_time_exp.year;
  bool is_month_diff = report_time_exp.month != usage_time_exp.month;

  if (is_year_diff || is_month_diff) {
    result |= kIsMonthlyUser;
  }
  if (is_year_diff || GetIsoWeekNumber(last_usage_time) !=
                          GetIsoWeekNumber(last_reported_usage_time)) {
    result |= kIsWeeklyUser;
  }
  if (is_year_diff || is_month_diff ||
      usage_time_exp.day_of_month != report_time_exp.day_of_month) {
    result |= kIsDailyUser;
  }

  return result;
}

}  // namespace brave_stats
