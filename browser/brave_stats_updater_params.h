/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_STATS_UPDATER_PARAMS_H_
#define BRAVE_BROWSER_BRAVE_STATS_UPDATER_PARAMS_H_

#include <string>

#include "base/macros.h"

class PrefService;

namespace base {
class Time;
}

namespace brave {

class BraveStatsUpdaterParams {
 public:
  BraveStatsUpdaterParams(PrefService* pref_service);
  BraveStatsUpdaterParams(PrefService* pref_service,
                          const std::string& ymd,
                          int woy,
                          int month);
  ~BraveStatsUpdaterParams();

  std::string GetDailyParam() const;
  std::string GetWeeklyParam() const;
  std::string GetMonthlyParam() const;
  std::string GetFirstCheckMadeParam() const;
  std::string GetWeekOfInstallationParam() const;
  std::string GetReferralCodeParam() const;

  void SavePrefs();

private:
  PrefService* pref_service_;
  std::string ymd_;
  int woy_;
  int month_;
  std::string last_check_ymd_;
  int last_check_woy_;
  int last_check_month_;
  bool first_check_made_;
  std::string week_of_installation_;
  std::string referral_promo_code_;

  void LoadPrefs();

  std::string BooleanToString(bool bool_value) const;

  std::string GetDateAsYMD(const base::Time& time) const;
  std::string GetCurrentDateAsYMD() const;
  std::string GetLastMondayAsYMD() const;
  int GetCurrentMonth() const;
  int GetCurrentISOWeekNumber() const;

  DISALLOW_COPY_AND_ASSIGN(BraveStatsUpdaterParams);
};

}  // namespace brave

#endif  // BRAVE_BROWSER_BRAVE_STATS_UPDATER_PARAMS_H_
