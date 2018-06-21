/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cmath>

#include "brave/browser/brave_stats_updater_params.h"

#include "base/strings/stringprintf.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"

namespace brave {

BraveStatsUpdaterParams::BraveStatsUpdaterParams()
    : today_ymd_(GetCurrentDateAsYMD()),
      today_woy_(GetCurrentISOWeekNumber()),
      today_month_(GetCurrentMonth()) {
  LoadPrefs();
}

BraveStatsUpdaterParams::~BraveStatsUpdaterParams() {
  SavePrefs();
}

std::string BraveStatsUpdaterParams::GetDailyParam() const {
  return BooleanToString(
      base::CompareCaseInsensitiveASCII(today_ymd_, last_check_ymd_) == 1);
}

std::string BraveStatsUpdaterParams::GetWeeklyParam() const {
  return BooleanToString(last_check_woy_ == 0 || today_woy_ != last_check_woy_);
}

std::string BraveStatsUpdaterParams::GetMonthlyParam() const {
  return BooleanToString(last_check_month_ == 0 ||
                         today_month_ != last_check_month_);
}

std::string BraveStatsUpdaterParams::GetFirstCheckMadeParam() const {
  return BooleanToString(!first_check_made_);
}

std::string BraveStatsUpdaterParams::GetWeekOfInstallationParam() const {
  return week_of_installation_;
}

void BraveStatsUpdaterParams::LoadPrefs() {
  PrefService* prefs = ProfileManager::GetActiveUserProfile()->GetPrefs();
  last_check_ymd_ = prefs->GetString(kLastCheckYMD);
  last_check_woy_ = prefs->GetInteger(kLastCheckWOY);
  last_check_month_ = prefs->GetInteger(kLastCheckMonth);
  first_check_made_ = prefs->GetBoolean(kFirstCheckMade);
  week_of_installation_ = prefs->GetString(kWeekOfInstallation);
  if (week_of_installation_.empty())
    week_of_installation_ = GetLastMondayAsYMD();
}

void BraveStatsUpdaterParams::SavePrefs() {
  PrefService* prefs = ProfileManager::GetActiveUserProfile()->GetPrefs();
  prefs->SetString(kLastCheckYMD, today_ymd_);
  prefs->SetInteger(kLastCheckWOY, today_woy_);
  prefs->SetInteger(kLastCheckMonth, today_month_);
  prefs->SetBoolean(kFirstCheckMade, first_check_made_);
  prefs->SetString(kWeekOfInstallation, week_of_installation_);
}

std::string BraveStatsUpdaterParams::BooleanToString(bool bool_value) const {
  return bool_value ? "true" : "false";
}

std::string BraveStatsUpdaterParams::GetDateAsYMD(const base::Time& time) const {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  return base::StringPrintf("%d-%02d-%02d", exploded.year, exploded.month,
                            exploded.day_of_month);
}

std::string BraveStatsUpdaterParams::GetCurrentDateAsYMD() const {
  return GetDateAsYMD(base::Time::Now());
}

std::string BraveStatsUpdaterParams::GetLastMondayAsYMD() const {
  base::Time now = base::Time::Now();
  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);

  base::Time last_monday;
  last_monday = base::Time::FromJsTime(
      now.ToJsTime() -
      ((exploded.day_of_week - 1) * base::Time::kMillisecondsPerDay));

  return GetDateAsYMD(last_monday);
}

int BraveStatsUpdaterParams::GetCurrentMonth() const {
  base::Time now = base::Time::Now();
  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);
  return exploded.month;
}

int BraveStatsUpdaterParams::GetCurrentISOWeekNumber() const {
  base::Time now = base::Time::Now();
  base::Time::Exploded now_exploded;
  now.LocalExplode(&now_exploded);
  now_exploded.hour = 0;
  now_exploded.minute = 0;
  now_exploded.second = 0;
  now_exploded.millisecond = 0;
  now_exploded.day_of_month =
      now_exploded.day_of_month + 3 - ((now_exploded.day_of_week + 6) % 7);

  base::Time now_adjusted;
  if (!base::Time::FromLocalExploded(now_exploded, &now_adjusted))
    return 0;

  base::Time::Exploded jan4_exploded = {0};
  jan4_exploded.year = now_exploded.year;
  jan4_exploded.month = 1;
  jan4_exploded.day_of_week = 0;
  jan4_exploded.day_of_month = 4;
  jan4_exploded.hour = 0;
  jan4_exploded.minute = 0;
  jan4_exploded.second = 0;
  jan4_exploded.millisecond = 0;

  base::Time jan4_time;
  if (!base::Time::FromLocalExploded(jan4_exploded, &jan4_time))
    return 0;

  return 1 + std::round(
                 ((now_adjusted.ToJsTime() - jan4_time.ToJsTime()) / 86400000 -
                  3 + (jan4_exploded.day_of_month + 6) % 7) /
                 7);
}

}  // namespace brave
