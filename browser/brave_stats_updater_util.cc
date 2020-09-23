/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ctime>
#include <memory>

#include "brave/browser/brave_stats_updater_util.h"

#include "base/environment.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/first_run/first_run.h"

namespace brave {

std::string GetDateAsYMD(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  return base::StringPrintf("%d-%02d-%02d", exploded.year, exploded.month,
                            exploded.day_of_month);
}

base::Time GetFirstRunTime(PrefService *pref_service) {
#if defined(OS_ANDROID)
  // Android doesn't use a sentinel to track first run, so we use a
  // preference instead. kReferralAndroidFirstRunTimestamp is used because
  // previously only referrals needed to know the first run value.
  base::Time first_run_timestamp =
      pref_service->GetTime(kReferralAndroidFirstRunTimestamp);
  if (first_run_timestamp.is_null()) {
    first_run_timestamp = base::Time::Now();
    pref_service->SetTime(kReferralAndroidFirstRunTimestamp,
                           first_run_timestamp);
  }
  return first_run_timestamp;
#else
  (void)pref_service;  // suppress unused warning

  // Note that CreateSentinelIfNeeded() is called in chrome_browser_main.cc,
  // so this will be a non-blocking read of the cached sentinel value.
  return first_run::GetFirstRunSentinelCreationTime();
#endif  // #defined(OS_ANDROID)
}

std::string GetPlatformIdentifier() {
#if defined(OS_WIN)
  if (base::SysInfo::OperatingSystemArchitecture() == "x86")
    return "winia32-bc";
  else
    return "winx64-bc";
#elif defined(OS_MAC)
  return "osx-bc";
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

}  // namespace brave
