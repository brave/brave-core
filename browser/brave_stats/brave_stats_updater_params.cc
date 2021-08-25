/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cmath>

#include "brave/browser/brave_stats/brave_stats_updater_params.h"
#include "brave/components/brave_referrals/buildflags/buildflags.h"

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/system/sys_info.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "bat/ads/pref_names.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_referrals/common/pref_names.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "chrome/browser/first_run/first_run.h"
#include "components/prefs/pref_service.h"

namespace brave_stats {

base::Time BraveStatsUpdaterParams::g_current_time;
bool BraveStatsUpdaterParams::g_force_first_run = false;
static constexpr base::TimeDelta g_dtoi_delete_delta =
    base::TimeDelta::FromSeconds(30 * 24 * 60 * 60);

BraveStatsUpdaterParams::BraveStatsUpdaterParams(
    PrefService* stats_pref_service,
    PrefService* profile_pref_service,
    const ProcessArch arch)
    : BraveStatsUpdaterParams(stats_pref_service,
                              profile_pref_service,
                              arch,
                              GetCurrentDateAsYMD(),
                              GetCurrentISOWeekNumber(),
                              GetCurrentMonth()) {}

BraveStatsUpdaterParams::BraveStatsUpdaterParams(
    PrefService* stats_pref_service,
    PrefService* profile_pref_service,
    const ProcessArch arch,
    const std::string& ymd,
    int woy,
    int month)
    : stats_pref_service_(stats_pref_service),
      profile_pref_service_(profile_pref_service),
      arch_(arch),
      ymd_(ymd),
      woy_(woy),
      month_(month) {
  LoadPrefs();
}

BraveStatsUpdaterParams::~BraveStatsUpdaterParams() {}

std::string BraveStatsUpdaterParams::GetDailyParam() const {
  return BooleanToString(
      base::CompareCaseInsensitiveASCII(ymd_, last_check_ymd_) == 1);
}

std::string BraveStatsUpdaterParams::GetWeeklyParam() const {
  return BooleanToString(last_check_woy_ == 0 || woy_ != last_check_woy_);
}

std::string BraveStatsUpdaterParams::GetMonthlyParam() const {
  return BooleanToString(last_check_month_ == 0 || month_ != last_check_month_);
}

std::string BraveStatsUpdaterParams::GetFirstCheckMadeParam() const {
  return BooleanToString(!first_check_made_);
}

std::string BraveStatsUpdaterParams::GetWeekOfInstallationParam() const {
  return week_of_installation_;
}

std::string BraveStatsUpdaterParams::GetDateOfInstallationParam() const {
  return (GetCurrentTimeNow() - date_of_installation_ >= g_dtoi_delete_delta)
             ? "null"
             : brave_stats::GetDateAsYMD(date_of_installation_);
}

std::string BraveStatsUpdaterParams::GetReferralCodeParam() const {
  return referral_promo_code_.empty() ? "none" : referral_promo_code_;
}

std::string BraveStatsUpdaterParams::GetAdsEnabledParam() const {
  return BooleanToString(
      profile_pref_service_->GetBoolean(ads::prefs::kEnabled));
}

std::string BraveStatsUpdaterParams::GetProcessArchParam() const {
  if (arch_ == ProcessArch::kArchSkip) {
    return "";
  } else if (arch_ == ProcessArch::kArchMetal) {
    return base::SysInfo::OperatingSystemArchitecture();
  } else {
    return "virt";
  }
}

std::string BraveStatsUpdaterParams::GetWalletEnabledParam() const {
  base::Time wallet_last_unlocked =
      profile_pref_service_->GetTime(kBraveWalletLastUnlockTime);
  uint8_t usage_bitset = UsageBitstringFromTimestamp(wallet_last_unlocked);
  return std::to_string(usage_bitset);
}

// This is a helper method for dealing with timestamps set by other services in
// the browser. This method makes the assumption that enabling the service
// required a user interaction, and thus the uasge ping for the current day has
// already fired. All calculations for daily, weekly, and monthly are made using
// yesterday's timestamp as reference.
//
// The method returns a bitstring with the following values according to the
// timestamp. All unannotated fields are unused.
//
// 0b00000000
//        |||
//        |||_____ Daily
//        ||______ Weekly
//        |_______ Monthly

enum : uint8_t {
  IsInactiveUser = 0,
  IsDailyUser = (1 << 0),
  IsWeeklyUser = (1 << 1),
  IsMonthlyUser = (1 << 2),
};
uint8_t BraveStatsUpdaterParams::UsageBitstringFromTimestamp(
    const base::Time& time) const {
  base::Time::Exploded target_exploded;
  time.LocalExplode(&target_exploded);
  uint8_t enabled_bitset = IsInactiveUser;

  base::Time yesterday = GetCurrentTimeNow() - base::TimeDelta::FromDays(1);
  base::Time::Exploded yesterday_exploded;
  yesterday.LocalExplode(&yesterday_exploded);
  if (yesterday_exploded.year == target_exploded.year) {
    if (yesterday_exploded.month == target_exploded.month) {
      enabled_bitset |= IsMonthlyUser;
      if (GetIsoWeekNumber(time) == GetIsoWeekNumber(yesterday)) {
        enabled_bitset |= IsWeeklyUser;
        if (yesterday_exploded.day_of_month == target_exploded.day_of_month) {
          enabled_bitset |= IsDailyUser;
        }
      }
    }
  }
  return enabled_bitset;
}

void BraveStatsUpdaterParams::LoadPrefs() {
  last_check_ymd_ = stats_pref_service_->GetString(kLastCheckYMD);
  last_check_woy_ = stats_pref_service_->GetInteger(kLastCheckWOY);
  last_check_month_ = stats_pref_service_->GetInteger(kLastCheckMonth);
  first_check_made_ = stats_pref_service_->GetBoolean(kFirstCheckMade);
  week_of_installation_ = stats_pref_service_->GetString(kWeekOfInstallation);
  if (week_of_installation_.empty())
    week_of_installation_ = GetLastMondayAsYMD();

  if (ShouldForceFirstRun()) {
    date_of_installation_ = GetCurrentTimeNow();
  } else {
    date_of_installation_ = GetFirstRunTime(stats_pref_service_);
    if (date_of_installation_.is_null()) {
      LOG(WARNING)
          << "Couldn't find the time of first run. This should only happen "
             "when running tests, but never in production code.";
    }
  }

#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
  referral_promo_code_ = stats_pref_service_->GetString(kReferralPromoCode);
#endif
}

void BraveStatsUpdaterParams::SavePrefs() {
  stats_pref_service_->SetString(kLastCheckYMD, ymd_);
  stats_pref_service_->SetInteger(kLastCheckWOY, woy_);
  stats_pref_service_->SetInteger(kLastCheckMonth, month_);
  stats_pref_service_->SetBoolean(kFirstCheckMade, true);
  stats_pref_service_->SetString(kWeekOfInstallation, week_of_installation_);
}

std::string BraveStatsUpdaterParams::BooleanToString(bool bool_value) const {
  return bool_value ? "true" : "false";
}

std::string BraveStatsUpdaterParams::GetCurrentDateAsYMD() const {
  return brave_stats::GetDateAsYMD(GetCurrentTimeNow());
}

std::string BraveStatsUpdaterParams::GetLastMondayAsYMD() const {
  base::Time now = GetCurrentTimeNow();
  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);

  int days_adjusted =
      (exploded.day_of_week == 0) ? 6 : exploded.day_of_week - 1;
  base::Time last_monday = base::Time::FromJsTime(
      now.ToJsTime() - (days_adjusted * base::Time::kMillisecondsPerDay));

  return brave_stats::GetDateAsYMD(last_monday);
}

int BraveStatsUpdaterParams::GetCurrentMonth() const {
  base::Time now = GetCurrentTimeNow();
  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);
  return exploded.month;
}

int BraveStatsUpdaterParams::GetCurrentISOWeekNumber() const {
  return GetIsoWeekNumber(GetCurrentTimeNow());
}

base::Time BraveStatsUpdaterParams::GetCurrentTimeNow() const {
  return g_current_time.is_null() ? base::Time::Now() : g_current_time;
}

// static
bool BraveStatsUpdaterParams::ShouldForceFirstRun() const {
  return g_force_first_run;
}

// static
void BraveStatsUpdaterParams::SetCurrentTimeForTest(
    const base::Time& current_time) {
  g_current_time = current_time;
}

// static
void BraveStatsUpdaterParams::SetFirstRunForTest(bool first_run) {
  g_force_first_run = first_run;
}

// static
base::Time BraveStatsUpdaterParams::GetFirstRunTime(PrefService* pref_service) {
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

  // CreateSentinelIfNeeded() is called in chrome_browser_main.cc, making this a
  // non-blocking read of the cached sentinel value when running from production
  // code. However tests will never create the sentinel file due to being run
  // with the switches:kNoFirstRun flag, so we need to allow blocking for that.
  base::ScopedAllowBlockingForTesting allow_blocking;
  return first_run::GetFirstRunSentinelCreationTime();
#endif  // #defined(OS_ANDROID)
}

}  // namespace brave_stats
