/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_stats_updater.h"

#include "base/time/time.h"
#include "brave/browser/brave_stats_updater_params.h"
#include "brave/browser/brave_stats_updater_util.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "chrome/browser/browser_process.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

const char kYesterday[] = "2018-06-21";
const char kToday[] = "2018-06-22";
const char kTomorrow[] = "2018-06-23";

const int kLastWeek = 24;
const int kThisWeek = 25;
const int kNextWeek = 26;

const int kLastMonth = 5;
const int kThisMonth = 6;
const int kNextMonth = 7;

class BraveStatsUpdaterTest: public testing::Test {
 public:
  BraveStatsUpdaterTest() {
  }
  ~BraveStatsUpdaterTest() override {}

  void SetUp() override {
    brave::RegisterPrefsForBraveStatsUpdater(testing_local_state_.registry());
    brave::RegisterPrefsForBraveReferralsService(
        testing_local_state_.registry());
    brave::BraveStatsUpdaterParams::SetFirstRunForTest(true);
  }

  PrefService* GetLocalState() { return &testing_local_state_; }

  void SetCurrentTimeForTest(const base::Time& current_time) {
    brave::BraveStatsUpdaterParams::SetCurrentTimeForTest(current_time);
  }

 private:
  TestingPrefServiceSimple testing_local_state_;
};


TEST_F(BraveStatsUpdaterTest, IsDailyUpdateNeededLastCheckedYesterday) {
  GetLocalState()->SetString(kLastCheckYMD, kYesterday);

  brave::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), kToday, kThisWeek, kThisMonth);
  ASSERT_EQ(brave_stats_updater_params.GetDailyParam(), "true");
  brave_stats_updater_params.SavePrefs();

  ASSERT_EQ(GetLocalState()->GetString(kLastCheckYMD), kToday);
}

TEST_F(BraveStatsUpdaterTest, IsDailyUpdateNeededLastCheckedToday) {
  GetLocalState()->SetString(kLastCheckYMD, kToday);

  brave::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), kToday, kThisWeek, kThisMonth);
  ASSERT_EQ(brave_stats_updater_params.GetDailyParam(), "false");
  brave_stats_updater_params.SavePrefs();

  ASSERT_EQ(GetLocalState()->GetString(kLastCheckYMD), kToday);
}

TEST_F(BraveStatsUpdaterTest, IsDailyUpdateNeededLastCheckedTomorrow) {
  GetLocalState()->SetString(kLastCheckYMD, kTomorrow);

  brave::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), kToday, kThisWeek, kThisMonth);
  ASSERT_EQ(brave_stats_updater_params.GetDailyParam(), "false");
  brave_stats_updater_params.SavePrefs();

  ASSERT_EQ(GetLocalState()->GetString(kLastCheckYMD), kToday);
}

TEST_F(BraveStatsUpdaterTest, IsWeeklyUpdateNeededLastCheckedLastWeek) {
  GetLocalState()->SetInteger(kLastCheckWOY, kLastWeek);

  brave::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), kToday, kThisWeek, kThisMonth);
  ASSERT_EQ(brave_stats_updater_params.GetWeeklyParam(), "true");
  brave_stats_updater_params.SavePrefs();

  ASSERT_EQ(GetLocalState()->GetInteger(kLastCheckWOY), kThisWeek);
}

TEST_F(BraveStatsUpdaterTest, IsWeeklyUpdateNeededLastCheckedThisWeek) {
  GetLocalState()->SetInteger(kLastCheckWOY, kThisWeek);

  brave::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), kToday, kThisWeek, kThisMonth);
  ASSERT_EQ(brave_stats_updater_params.GetWeeklyParam(), "false");
  brave_stats_updater_params.SavePrefs();

  ASSERT_EQ(GetLocalState()->GetInteger(kLastCheckWOY), kThisWeek);
}

TEST_F(BraveStatsUpdaterTest, IsWeeklyUpdateNeededLastCheckedNextWeek) {
  GetLocalState()->SetInteger(kLastCheckWOY, kNextWeek);

  brave::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), kToday, kThisWeek, kThisMonth);
  ASSERT_EQ(brave_stats_updater_params.GetWeeklyParam(), "true");
  brave_stats_updater_params.SavePrefs();

  ASSERT_EQ(GetLocalState()->GetInteger(kLastCheckWOY), kThisWeek);
}

TEST_F(BraveStatsUpdaterTest, IsMonthlyUpdateNeededLastCheckedLastMonth) {
  GetLocalState()->SetInteger(kLastCheckMonth, kLastMonth);

  brave::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), kToday, kThisWeek, kThisMonth);
  ASSERT_EQ(brave_stats_updater_params.GetMonthlyParam(), "true");
  brave_stats_updater_params.SavePrefs();

  ASSERT_EQ(GetLocalState()->GetInteger(kLastCheckMonth), kThisMonth);
}

TEST_F(BraveStatsUpdaterTest, IsMonthlyUpdateNeededLastCheckedThisMonth) {
  GetLocalState()->SetInteger(kLastCheckMonth, kThisMonth);

  brave::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), kToday, kThisWeek, kThisMonth);
  ASSERT_EQ(brave_stats_updater_params.GetMonthlyParam(), "false");
  brave_stats_updater_params.SavePrefs();

  ASSERT_EQ(GetLocalState()->GetInteger(kLastCheckMonth), kThisMonth);
}

TEST_F(BraveStatsUpdaterTest, IsMonthlyUpdateNeededLastCheckedNextMonth) {
  GetLocalState()->SetInteger(kLastCheckMonth, kNextMonth);

  brave::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), kToday, kThisWeek, kThisMonth);
  ASSERT_EQ(brave_stats_updater_params.GetMonthlyParam(), "true");
  brave_stats_updater_params.SavePrefs();

  ASSERT_EQ(GetLocalState()->GetInteger(kLastCheckMonth), kThisMonth);
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

  ASSERT_TRUE(base::Time::FromLocalExploded(exploded, &current_time));
  SetCurrentTimeForTest(current_time);

  brave::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), kToday, kThisWeek, kThisMonth);
  ASSERT_EQ(brave_stats_updater_params.GetDateOfInstallationParam(),
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

  ASSERT_TRUE(base::Time::FromLocalExploded(exploded, &dtoi_time));
  // Make first run date 6 days earlier (still within 14 day window)
  exploded.day_of_month = 10;
  ASSERT_TRUE(base::Time::FromLocalExploded(exploded, &current_time));

  SetCurrentTimeForTest(dtoi_time);
  brave::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), kToday, kThisWeek, kThisMonth);
  SetCurrentTimeForTest(current_time);
  ASSERT_EQ(brave_stats_updater_params.GetDateOfInstallationParam(),
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

  ASSERT_TRUE(base::Time::FromLocalExploded(exploded, &dtoi_time));
  // Make first run date 14 days earlier (outside 14 day window)
  exploded.day_of_month = 18;
  ASSERT_TRUE(base::Time::FromLocalExploded(exploded, &current_time));

  SetCurrentTimeForTest(dtoi_time);
  brave::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), kToday, kThisWeek, kThisMonth);
  SetCurrentTimeForTest(current_time);
  ASSERT_EQ(brave_stats_updater_params.GetDateOfInstallationParam(), "null");
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

    ASSERT_TRUE(base::Time::FromLocalExploded(exploded, &current_time));

    SetCurrentTimeForTest(current_time);
    brave::BraveStatsUpdaterParams brave_stats_updater_params(GetLocalState());

    // Make sure that the weekly param was set to true, since this is
    // a new ISO week (#44)
    ASSERT_EQ(brave_stats_updater_params.GetWeeklyParam(), "true");
    brave_stats_updater_params.SavePrefs();

    // Make sure that local state was updated to reflect this as well
    ASSERT_EQ(GetLocalState()->GetInteger(kLastCheckWOY), 44);
  }

  {
    // Now it's the next day (Monday)
    exploded.day_of_week = 1;
    exploded.day_of_month = 5;

    ASSERT_TRUE(base::Time::FromLocalExploded(exploded, &current_time));

    SetCurrentTimeForTest(current_time);
    brave::BraveStatsUpdaterParams brave_stats_updater_params(GetLocalState());

    // Make sure that the weekly param was set to true, since this is
    // a new ISO week (#45)
    ASSERT_EQ(brave_stats_updater_params.GetWeeklyParam(), "true");
    brave_stats_updater_params.SavePrefs();

    // Make sure that local state was updated to reflect this as well
    ASSERT_EQ(GetLocalState()->GetInteger(kLastCheckWOY), 45);
  }

  {
    // Now it's the next day (Tuesday)
    exploded.day_of_week = 2;
    exploded.day_of_month = 6;

    ASSERT_TRUE(base::Time::FromLocalExploded(exploded, &current_time));

    SetCurrentTimeForTest(current_time);
    brave::BraveStatsUpdaterParams brave_stats_updater_params(GetLocalState());

    // Make sure that the weekly param was set to false, since this is
    // still the same ISO week (#45)
    ASSERT_EQ(brave_stats_updater_params.GetWeeklyParam(), "false");
    brave_stats_updater_params.SavePrefs();

    // Make sure that local state also didn't change
    ASSERT_EQ(GetLocalState()->GetInteger(kLastCheckWOY), 45);
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

    ASSERT_TRUE(base::Time::FromLocalExploded(exploded, &current_time));
    SetCurrentTimeForTest(current_time);

    // Make sure that week of installation is previous Monday
    brave::BraveStatsUpdaterParams brave_stats_updater_params(GetLocalState());
    ASSERT_EQ(brave_stats_updater_params.GetWeekOfInstallationParam(),
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

    ASSERT_TRUE(base::Time::FromLocalExploded(exploded, &current_time));
    SetCurrentTimeForTest(current_time);

    // Make sure that week of installation is today, since today is a
    // Monday
    brave::BraveStatsUpdaterParams brave_stats_updater_params(GetLocalState());
    ASSERT_EQ(brave_stats_updater_params.GetWeekOfInstallationParam(),
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

    ASSERT_TRUE(base::Time::FromLocalExploded(exploded, &current_time));
    SetCurrentTimeForTest(current_time);

    // Make sure that week of installation is previous Monday
    brave::BraveStatsUpdaterParams brave_stats_updater_params(GetLocalState());
    ASSERT_EQ(brave_stats_updater_params.GetWeekOfInstallationParam(),
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
  ASSERT_TRUE(base::Time::FromLocalExploded(exploded, &time));
  EXPECT_EQ(brave::GetIsoWeekNumber(time), 31);

  exploded.day_of_month = 30;
  exploded.month = 9;

  ASSERT_TRUE(base::Time::FromLocalExploded(exploded, &time));
  EXPECT_EQ(brave::GetIsoWeekNumber(time), 40);

  exploded.day_of_month = 1;
  exploded.month = 9;
  exploded.day_of_week = 0;

  ASSERT_TRUE(base::Time::FromLocalExploded(exploded, &time));
  EXPECT_EQ(brave::GetIsoWeekNumber(time), 35);
}
