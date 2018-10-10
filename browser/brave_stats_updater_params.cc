/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cmath>

#include "brave/browser/brave_stats_updater_params.h"

#include "base/strings/stringprintf.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave {

BraveStatsUpdaterParams::BraveStatsUpdaterParams(PrefService* pref_service)
    : BraveStatsUpdaterParams(pref_service,
                              GetCurrentDateAsYMD(),
                              GetCurrentISOWeekNumber(),
                              GetCurrentMonth()) {
}

BraveStatsUpdaterParams::BraveStatsUpdaterParams(PrefService* pref_service,
                                                 const std::string& ymd,
                                                 int woy,
                                                 int month)
    : pref_service_(pref_service), ymd_(ymd), woy_(woy), month_(month) {
  LoadPrefs();
}

BraveStatsUpdaterParams::~BraveStatsUpdaterParams() {
}

std::string BraveStatsUpdaterParams::GetDailyParam() const {
  return BooleanToString(
      base::CompareCaseInsensitiveASCII(ymd_, last_check_ymd_) == 1);
}

std::string BraveStatsUpdaterParams::GetWeeklyParam() const {
  return BooleanToString(last_check_woy_ == 0 || woy_ != last_check_woy_);
}

std::string BraveStatsUpdaterParams::GetMonthlyParam() const {
  return BooleanToString(last_check_month_ == 0 ||
                         month_ != last_check_month_);
}

std::string BraveStatsUpdaterParams::GetFirstCheckMadeParam() const {
  return BooleanToString(!first_check_made_);
}

std::string BraveStatsUpdaterParams::GetWeekOfInstallationParam() const {
  return week_of_installation_;
}

std::string BraveStatsUpdaterParams::GetReferralCodeParam() const {
  return referral_promo_code_.empty() ? "none" : referral_promo_code_;
}

void BraveStatsUpdaterParams::LoadPrefs() {
  last_check_ymd_ = pref_service_->GetString(kLastCheckYMD);
  last_check_woy_ = pref_service_->GetInteger(kLastCheckWOY);
  last_check_month_ = pref_service_->GetInteger(kLastCheckMonth);
  first_check_made_ = pref_service_->GetBoolean(kFirstCheckMade);
  week_of_installation_ = pref_service_->GetString(kWeekOfInstallation);
  if (week_of_installation_.empty())
    week_of_installation_ = GetLastMondayAsYMD();
  referral_promo_code_ = pref_service_->GetString(kReferralPromoCode);
}

void BraveStatsUpdaterParams::SavePrefs() {
  pref_service_->SetString(kLastCheckYMD, ymd_);
  pref_service_->SetInteger(kLastCheckWOY, woy_);
  pref_service_->SetInteger(kLastCheckMonth, month_);
  pref_service_->SetBoolean(kFirstCheckMade, true);
  pref_service_->SetString(kWeekOfInstallation, week_of_installation_);
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
