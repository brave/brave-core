/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_stats_updater.h"

#include "brave/browser/brave_stats_updater_params.h"
#include "brave/browser/referrals/brave_referrals_service.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

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
    brave::RegisterPrefsForBraveReferralsService(testing_local_state_.registry());
  }

  PrefService* GetLocalState() { return &testing_local_state_; }

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

}  // namespace
