/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/browser/brave_stats/brave_stats_updater.h"

#include "base/files/scoped_temp_dir.h"
#include "base/system/sys_info.h"
#include "base/time/time.h"
#include "bat/ads/pref_names.h"
#include "brave/browser/brave_stats/brave_stats_updater_params.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_ads/browser/test_util.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveStatsUpdaterTest.*

const char kYesterday[] = "2018-06-21";
const char kToday[] = "2018-06-22";
const char kTomorrow[] = "2018-06-23";

const int kLastWeek = 24;
const int kThisWeek = 25;
const int kNextWeek = 26;

const int kLastMonth = 5;
const int kThisMonth = 6;
const int kNextMonth = 7;

class BraveStatsUpdaterTest : public testing::Test {
 public:
  BraveStatsUpdaterTest()
      : profile_manager_(TestingBrowserProcess::GetGlobal()) {}
  ~BraveStatsUpdaterTest() override {}

  void SetUp() override {
    EXPECT_TRUE(temp_dir_.CreateUniqueTempDir());
    EXPECT_TRUE(profile_manager_.SetUp());
    profile_ = brave_ads::CreateBraveAdsProfile(temp_dir_.GetPath());
    EXPECT_TRUE(profile_.get() != NULL);
    brave_stats::RegisterLocalStatePrefs(testing_local_state_.registry());
    brave::RegisterPrefsForBraveReferralsService(
        testing_local_state_.registry());
    brave_stats::BraveStatsUpdaterParams::SetFirstRunForTest(true);
  }

  void TearDown() override { profile_.reset(); }

  PrefService* GetLocalState() { return &testing_local_state_; }
  PrefService* GetProfilePrefs() { return profile_->GetPrefs(); }
  void SetEnableAds(bool ads_enabled) {
    GetProfilePrefs()->SetBoolean(ads::prefs::kEnabled, ads_enabled);
  }

  void SetCurrentTimeForTest(const base::Time& current_time) {
    brave_stats::BraveStatsUpdaterParams::SetCurrentTimeForTest(current_time);
  }

 private:
  TestingPrefServiceSimple testing_local_state_;
  TestingProfileManager profile_manager_;
  std::unique_ptr<Profile> profile_;
  base::ScopedTempDir temp_dir_;
  content::BrowserTaskEnvironment task_environment_;
};

TEST_F(BraveStatsUpdaterTest, IsDailyUpdateNeededLastCheckedYesterday) {
  GetLocalState()->SetString(kLastCheckYMD, kYesterday);

  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);
  EXPECT_EQ(brave_stats_updater_params.GetDailyParam(), "true");
  brave_stats_updater_params.SavePrefs();

  EXPECT_EQ(GetLocalState()->GetString(kLastCheckYMD), kToday);
}

TEST_F(BraveStatsUpdaterTest, IsDailyUpdateNeededLastCheckedToday) {
  GetLocalState()->SetString(kLastCheckYMD, kToday);

  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);
  EXPECT_EQ(brave_stats_updater_params.GetDailyParam(), "false");
  brave_stats_updater_params.SavePrefs();

  EXPECT_EQ(GetLocalState()->GetString(kLastCheckYMD), kToday);
}

TEST_F(BraveStatsUpdaterTest, IsDailyUpdateNeededLastCheckedTomorrow) {
  GetLocalState()->SetString(kLastCheckYMD, kTomorrow);

  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);
  EXPECT_EQ(brave_stats_updater_params.GetDailyParam(), "false");
  brave_stats_updater_params.SavePrefs();

  EXPECT_EQ(GetLocalState()->GetString(kLastCheckYMD), kToday);
}

TEST_F(BraveStatsUpdaterTest, IsWeeklyUpdateNeededLastCheckedLastWeek) {
  GetLocalState()->SetInteger(kLastCheckWOY, kLastWeek);

  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);
  EXPECT_EQ(brave_stats_updater_params.GetWeeklyParam(), "true");
  brave_stats_updater_params.SavePrefs();

  EXPECT_EQ(GetLocalState()->GetInteger(kLastCheckWOY), kThisWeek);
}

TEST_F(BraveStatsUpdaterTest, IsWeeklyUpdateNeededLastCheckedThisWeek) {
  GetLocalState()->SetInteger(kLastCheckWOY, kThisWeek);

  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);
  EXPECT_EQ(brave_stats_updater_params.GetWeeklyParam(), "false");
  brave_stats_updater_params.SavePrefs();

  EXPECT_EQ(GetLocalState()->GetInteger(kLastCheckWOY), kThisWeek);
}

TEST_F(BraveStatsUpdaterTest, IsWeeklyUpdateNeededLastCheckedNextWeek) {
  GetLocalState()->SetInteger(kLastCheckWOY, kNextWeek);

  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);
  EXPECT_EQ(brave_stats_updater_params.GetWeeklyParam(), "true");
  brave_stats_updater_params.SavePrefs();

  EXPECT_EQ(GetLocalState()->GetInteger(kLastCheckWOY), kThisWeek);
}

TEST_F(BraveStatsUpdaterTest, IsMonthlyUpdateNeededLastCheckedLastMonth) {
  GetLocalState()->SetInteger(kLastCheckMonth, kLastMonth);

  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);
  EXPECT_EQ(brave_stats_updater_params.GetMonthlyParam(), "true");
  brave_stats_updater_params.SavePrefs();

  EXPECT_EQ(GetLocalState()->GetInteger(kLastCheckMonth), kThisMonth);
}

TEST_F(BraveStatsUpdaterTest, IsMonthlyUpdateNeededLastCheckedThisMonth) {
  GetLocalState()->SetInteger(kLastCheckMonth, kThisMonth);

  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);
  EXPECT_EQ(brave_stats_updater_params.GetMonthlyParam(), "false");
  brave_stats_updater_params.SavePrefs();

  EXPECT_EQ(GetLocalState()->GetInteger(kLastCheckMonth), kThisMonth);
}

TEST_F(BraveStatsUpdaterTest, IsMonthlyUpdateNeededLastCheckedNextMonth) {
  GetLocalState()->SetInteger(kLastCheckMonth, kNextMonth);

  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);
  EXPECT_EQ(brave_stats_updater_params.GetMonthlyParam(), "true");
  brave_stats_updater_params.SavePrefs();

  EXPECT_EQ(GetLocalState()->GetInteger(kLastCheckMonth), kThisMonth);
}

TEST_F(BraveStatsUpdaterTest, HasAdsDisabled) {
  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);
  SetEnableAds(false);
  EXPECT_EQ(brave_stats_updater_params.GetAdsEnabledParam(), "false");
}

TEST_F(BraveStatsUpdaterTest, HasAdsEnabled) {
  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);
  SetEnableAds(true);
  EXPECT_EQ(brave_stats_updater_params.GetAdsEnabledParam(), "true");
}

TEST_F(BraveStatsUpdaterTest, HasArchSkip) {
  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);
  EXPECT_EQ(brave_stats_updater_params.GetProcessArchParam(), "");
}

TEST_F(BraveStatsUpdaterTest, HasArchVirt) {
  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchVirt,
      kToday, kThisWeek, kThisMonth);
  EXPECT_EQ(brave_stats_updater_params.GetProcessArchParam(), "virt");
}

TEST_F(BraveStatsUpdaterTest, HasArchMetal) {
  auto arch = base::SysInfo::OperatingSystemArchitecture();
  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchMetal,
      kToday, kThisWeek, kThisMonth);
  EXPECT_EQ(brave_stats_updater_params.GetProcessArchParam(), arch);
}

TEST_F(BraveStatsUpdaterTest, HasDateOfInstallationFirstRun) {
  base::Time::Exploded exploded;
  base::Time current_time;

  // Set date to 2018-11-04 (ISO week #44)
  exploded.hour = 0;
  exploded.minute = 0;
  exploded.second = 0;
  exploded.millisecond = 0;
  exploded.day_of_week = 0;
  exploded.year = 2018;
  exploded.month = 11;
  exploded.day_of_month = 4;

  EXPECT_TRUE(base::Time::FromLocalExploded(exploded, &current_time));
  SetCurrentTimeForTest(current_time);

  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);
  EXPECT_EQ(brave_stats_updater_params.GetDateOfInstallationParam(),
            "2018-11-04");
}

TEST_F(BraveStatsUpdaterTest, HasDailyRetention) {
  base::Time::Exploded exploded;
  base::Time current_time, dtoi_time;

  // Set date to 2018-11-04
  exploded.hour = 0;
  exploded.minute = 0;
  exploded.second = 0;
  exploded.millisecond = 0;
  exploded.day_of_week = 0;
  exploded.year = 2018;
  exploded.month = 11;
  exploded.day_of_month = 4;

  EXPECT_TRUE(base::Time::FromLocalExploded(exploded, &dtoi_time));
  // Make first run date 6 days earlier (still within 14 day window)
  exploded.day_of_month = 10;
  EXPECT_TRUE(base::Time::FromLocalExploded(exploded, &current_time));

  SetCurrentTimeForTest(dtoi_time);
  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);
  SetCurrentTimeForTest(current_time);
  EXPECT_EQ(brave_stats_updater_params.GetDateOfInstallationParam(),
            "2018-11-04");
}

TEST_F(BraveStatsUpdaterTest, HasDailyRetentionExpiration) {
  base::Time::Exploded exploded;
  base::Time current_time, dtoi_time;

  // Set date to 2018-11-04
  exploded.hour = 0;
  exploded.minute = 0;
  exploded.second = 0;
  exploded.millisecond = 0;
  exploded.day_of_week = 0;
  exploded.year = 2018;
  exploded.month = 11;
  exploded.day_of_month = 4;

  EXPECT_TRUE(base::Time::FromLocalExploded(exploded, &dtoi_time));
  // Make first run date 14 days earlier (outside 14 day window)
  exploded.day_of_month = 18;
  EXPECT_TRUE(base::Time::FromLocalExploded(exploded, &current_time));

  SetCurrentTimeForTest(dtoi_time);
  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);
  SetCurrentTimeForTest(current_time);
  EXPECT_EQ(brave_stats_updater_params.GetDateOfInstallationParam(), "null");
}

// This test ensures that our weekly stats cut over on Monday
TEST_F(BraveStatsUpdaterTest, IsWeeklyUpdateNeededOnMondayLastCheckedOnSunday) {
  base::Time::Exploded exploded;
  base::Time current_time;

  {
    // Set our local state to indicate that the last weekly check was
    // performed during ISO week #43
    GetLocalState()->SetInteger(kLastCheckWOY, 43);

    // Set date to 2018-11-04 (ISO week #44)
    exploded.hour = 0;
    exploded.minute = 0;
    exploded.second = 0;
    exploded.millisecond = 0;
    exploded.day_of_week = 0;
    exploded.year = 2018;
    exploded.month = 11;
    exploded.day_of_month = 4;

    EXPECT_TRUE(base::Time::FromLocalExploded(exploded, &current_time));

    SetCurrentTimeForTest(current_time);
    brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
        GetLocalState(), GetProfilePrefs(),
        brave_stats::ProcessArch::kArchSkip);

    // Make sure that the weekly param was set to true, since this is
    // a new ISO week (#44)
    EXPECT_EQ(brave_stats_updater_params.GetWeeklyParam(), "true");
    brave_stats_updater_params.SavePrefs();

    // Make sure that local state was updated to reflect this as well
    EXPECT_EQ(GetLocalState()->GetInteger(kLastCheckWOY), 44);
  }

  {
    // Now it's the next day (Monday)
    exploded.day_of_week = 1;
    exploded.day_of_month = 5;

    EXPECT_TRUE(base::Time::FromLocalExploded(exploded, &current_time));

    SetCurrentTimeForTest(current_time);
    brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
        GetLocalState(), GetProfilePrefs(),
        brave_stats::ProcessArch::kArchSkip);

    // Make sure that the weekly param was set to true, since this is
    // a new ISO week (#45)
    EXPECT_EQ(brave_stats_updater_params.GetWeeklyParam(), "true");
    brave_stats_updater_params.SavePrefs();

    // Make sure that local state was updated to reflect this as well
    EXPECT_EQ(GetLocalState()->GetInteger(kLastCheckWOY), 45);
  }

  {
    // Now it's the next day (Tuesday)
    exploded.day_of_week = 2;
    exploded.day_of_month = 6;

    EXPECT_TRUE(base::Time::FromLocalExploded(exploded, &current_time));

    SetCurrentTimeForTest(current_time);
    brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
        GetLocalState(), GetProfilePrefs(),
        brave_stats::ProcessArch::kArchSkip);

    // Make sure that the weekly param was set to false, since this is
    // still the same ISO week (#45)
    EXPECT_EQ(brave_stats_updater_params.GetWeeklyParam(), "false");
    brave_stats_updater_params.SavePrefs();

    // Make sure that local state also didn't change
    EXPECT_EQ(GetLocalState()->GetInteger(kLastCheckWOY), 45);
  }
}

TEST_F(BraveStatsUpdaterTest, HasCorrectWeekOfInstallation) {
  base::Time::Exploded exploded;
  base::Time current_time;

  {
    // Set date to 2019-03-24 (Sunday)
    exploded.hour = 0;
    exploded.minute = 0;
    exploded.second = 0;
    exploded.millisecond = 0;
    exploded.day_of_week = 0;
    exploded.year = 2019;
    exploded.month = 3;
    exploded.day_of_month = 24;

    EXPECT_TRUE(base::Time::FromLocalExploded(exploded, &current_time));
    SetCurrentTimeForTest(current_time);

    // Make sure that week of installation is previous Monday
    brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
        GetLocalState(), GetProfilePrefs(),
        brave_stats::ProcessArch::kArchSkip);
    EXPECT_EQ(brave_stats_updater_params.GetWeekOfInstallationParam(),
              "2019-03-18");
  }

  {
    // Set date to 2019-03-25 (Monday)
    exploded.hour = 0;
    exploded.minute = 0;
    exploded.second = 0;
    exploded.millisecond = 0;
    exploded.day_of_week = 0;
    exploded.year = 2019;
    exploded.month = 3;
    exploded.day_of_month = 25;

    EXPECT_TRUE(base::Time::FromLocalExploded(exploded, &current_time));
    SetCurrentTimeForTest(current_time);

    // Make sure that week of installation is today, since today is a
    // Monday
    brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
        GetLocalState(), GetProfilePrefs(),
        brave_stats::ProcessArch::kArchSkip);
    EXPECT_EQ(brave_stats_updater_params.GetWeekOfInstallationParam(),
              "2019-03-25");
  }

  {
    // Set date to 2019-03-30 (Saturday)
    exploded.hour = 0;
    exploded.minute = 0;
    exploded.second = 0;
    exploded.millisecond = 0;
    exploded.day_of_week = 0;
    exploded.year = 2019;
    exploded.month = 3;
    exploded.day_of_month = 30;

    EXPECT_TRUE(base::Time::FromLocalExploded(exploded, &current_time));
    SetCurrentTimeForTest(current_time);

    // Make sure that week of installation is previous Monday
    brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
        GetLocalState(), GetProfilePrefs(),
        brave_stats::ProcessArch::kArchSkip);
    EXPECT_EQ(brave_stats_updater_params.GetWeekOfInstallationParam(),
              "2019-03-25");
  }
}

TEST_F(BraveStatsUpdaterTest, GetIsoWeekNumber) {
  base::Time::Exploded exploded;
  exploded.hour = 0;
  exploded.minute = 0;
  exploded.second = 0;
  exploded.millisecond = 0;
  exploded.day_of_week = 1;
  exploded.day_of_month = 29;
  exploded.month = 7;
  exploded.year = 2019;

  base::Time time;
  EXPECT_TRUE(base::Time::FromLocalExploded(exploded, &time));
  EXPECT_EQ(brave_stats::GetIsoWeekNumber(time), 31);

  exploded.day_of_month = 30;
  exploded.month = 9;

  EXPECT_TRUE(base::Time::FromLocalExploded(exploded, &time));
  EXPECT_EQ(brave_stats::GetIsoWeekNumber(time), 40);

  exploded.day_of_month = 1;
  exploded.month = 9;
  exploded.day_of_week = 0;

  EXPECT_TRUE(base::Time::FromLocalExploded(exploded, &time));
  EXPECT_EQ(brave_stats::GetIsoWeekNumber(time), 35);
}
