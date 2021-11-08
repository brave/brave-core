/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_STATS_BRAVE_STATS_UPDATER_PARAMS_H_
#define BRAVE_BROWSER_BRAVE_STATS_BRAVE_STATS_UPDATER_PARAMS_H_

#include <cstdint>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/time/time.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"

class BraveStatsUpdaterTest;
class GURL;
class PrefService;

FORWARD_DECLARE_TEST(BraveStatsUpdaterTest, UsageBitstringDaily);
FORWARD_DECLARE_TEST(BraveStatsUpdaterTest, UsageBitstringWeekly);
FORWARD_DECLARE_TEST(BraveStatsUpdaterTest, UsageBitstringMonthly);
FORWARD_DECLARE_TEST(BraveStatsUpdaterTest, UsageBitstringInactive);

namespace brave_stats {

class BraveStatsUpdaterParams {
 public:
  explicit BraveStatsUpdaterParams(PrefService* stats_pref_service,
                                   PrefService* profile_pref_service,
                                   const ProcessArch arch);
  BraveStatsUpdaterParams(PrefService* stats_pref_service,
                          PrefService* profile_pref_service,
                          const ProcessArch arch,
                          const std::string& ymd,
                          int woy,
                          int month);
  BraveStatsUpdaterParams(const BraveStatsUpdaterParams&) = delete;
  BraveStatsUpdaterParams& operator=(const BraveStatsUpdaterParams&) = delete;
  ~BraveStatsUpdaterParams();

  std::string GetDailyParam() const;
  std::string GetWeeklyParam() const;
  std::string GetMonthlyParam() const;
  std::string GetFirstCheckMadeParam() const;
  std::string GetWeekOfInstallationParam() const;
  std::string GetDateOfInstallationParam() const;
  std::string GetReferralCodeParam() const;
  std::string GetAdsEnabledParam() const;
  std::string GetProcessArchParam() const;
  std::string GetWalletEnabledParam() const;
  GURL GetUpdateURL(const GURL& base_update_url,
                    const std::string platform_id,
                    const std::string channel_name,
                    const std::string full_brave_version) const;

  void SavePrefs();

 private:
  friend class ::BraveStatsUpdaterTest;
  FRIEND_TEST_ALL_PREFIXES(::BraveStatsUpdaterTest, UsageBitstringDaily);
  FRIEND_TEST_ALL_PREFIXES(::BraveStatsUpdaterTest, UsageBitstringWeekly);
  FRIEND_TEST_ALL_PREFIXES(::BraveStatsUpdaterTest, UsageBitstringMonthly);
  FRIEND_TEST_ALL_PREFIXES(::BraveStatsUpdaterTest, UsageBitstringInactive);

  PrefService* stats_pref_service_;
  PrefService* profile_pref_service_;
  ProcessArch arch_;
  std::string ymd_;
  int woy_;
  int month_;
  std::string last_check_ymd_;
  int last_check_woy_;
  int last_check_month_;
  bool first_check_made_;
  std::string week_of_installation_;
  base::Time date_of_installation_;
  std::string referral_promo_code_;
  static base::Time g_current_time;
  static bool g_force_first_run;

  void LoadPrefs();

  std::string BooleanToString(bool bool_value) const;

  std::string GetDateAsYMD(const base::Time& time) const;
  std::string GetCurrentDateAsYMD() const;
  std::string GetLastMondayAsYMD() const;
  int GetCurrentMonth() const;
  int GetCurrentISOWeekNumber() const;

  // Returns the current time, allows override from tests
  base::Time GetCurrentTimeNow() const;

  // Gets the previous day, since that is the most recent time another stats
  // ping could have fired.
  base::Time GetReferenceTime() const;

  bool ShouldForceFirstRun() const;

  static void SetCurrentTimeForTest(const base::Time& current_time);
  static void SetFirstRunForTest(bool first_run);
  // Returns the timestamp of the browsers first run
  static base::Time GetFirstRunTime(PrefService* pref_service);
};

}  // namespace brave_stats

#endif  // BRAVE_BROWSER_BRAVE_STATS_BRAVE_STATS_UPDATER_PARAMS_H_
