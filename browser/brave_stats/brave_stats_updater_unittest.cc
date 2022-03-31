/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_split.h"
#include "base/system/sys_info.h"
#include "base/test/bind.h"
#include "base/time/time.h"
#include "bat/ads/pref_names.h"
#include "brave/browser/brave_stats/brave_stats_updater.h"
#include "brave/browser/brave_stats/brave_stats_updater_params.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
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
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}
  ~BraveStatsUpdaterTest() override {}

  void SetUp() override {
#if BUILDFLAG(IS_ANDROID)
    task_environment_.AdvanceClock(base::Days(2));
#else
    base::Time future_mock_time;
    if (base::Time::FromString("3000-01-04", &future_mock_time)) {
      task_environment_.AdvanceClock(future_mock_time - base::Time::Now());
    }
#endif
    task_environment_.AdvanceClock(base::Minutes(30));

    profile_ = CreateBraveAdsProfile();
    EXPECT_TRUE(profile_.get() != NULL);
    brave_stats::RegisterLocalStatePrefs(testing_local_state_.registry());
    brave::RegisterPrefsForBraveReferralsService(
        testing_local_state_.registry());
    brave_stats::BraveStatsUpdaterParams::SetFirstRunForTest(true);
  }

  PrefService* GetLocalState() { return &testing_local_state_; }
  PrefService* GetProfilePrefs() { return profile_->GetPrefs(); }
  std::unique_ptr<brave_stats::BraveStatsUpdaterParams> BuildUpdaterParams() {
    return std::make_unique<brave_stats::BraveStatsUpdaterParams>(
        GetLocalState(), GetProfilePrefs(),
        brave_stats::ProcessArch::kArchSkip);
  }
  void SetEnableAds(bool ads_enabled) {
    GetProfilePrefs()->SetBoolean(ads::prefs::kEnabled, ads_enabled);
  }

  void SetCurrentTimeForTest(const base::Time& current_time) {
    brave_stats::BraveStatsUpdaterParams::SetCurrentTimeForTest(current_time);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<Profile> profile_;

 private:
  std::unique_ptr<Profile> CreateBraveAdsProfile() {
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    brave_rewards::RewardsService::RegisterProfilePrefs(prefs->registry());
    brave_ads::AdsService::RegisterProfilePrefs(prefs->registry());
    RegisterUserProfilePrefs(prefs->registry());

    TestingProfile::Builder profile_builder;
    profile_builder.SetPrefService(std::move(prefs));
    return profile_builder.Build();
  }

  TestingPrefServiceSimple testing_local_state_;
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
  // Make first run date 15 days earlier (still within 30 day window)
  exploded.day_of_month = 20;
  EXPECT_TRUE(base::Time::FromLocalExploded(exploded, &current_time));

  SetCurrentTimeForTest(dtoi_time);
  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);
  SetCurrentTimeForTest(current_time);
  EXPECT_EQ(brave_stats_updater_params.GetDateOfInstallationParam(),
            "2018-11-04");
}

TEST_F(BraveStatsUpdaterTest, GetUpdateURLHasFirstAndDtoi) {
  base::Time current_time, install_time;

  // Set date to 2018-11-04
  EXPECT_TRUE(base::Time::FromString("2018-11-04", &install_time));

  // Make first run date 15 days earlier (still within 30 day window)
  current_time = install_time + base::Days(16);

  SetCurrentTimeForTest(install_time);
  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);
  SetCurrentTimeForTest(current_time);

  GURL response = brave_stats_updater_params.GetUpdateURL(
      GURL("https://demo.brave.com"), "platform id here", "channel name here",
      "full brave version here");

  base::StringPairs kv_pairs;
  // this will return `false` because at least one argument has no value
  // ex: `arch` will have an empty value (because of kArchSkip).
  base::SplitStringIntoKeyValuePairsUsingSubstr(response.query(), '=', "&",
                                                &kv_pairs);
  EXPECT_FALSE(kv_pairs.empty());

  bool first_is_true = false;
  bool has_dtoi = false;
  for (auto& kv : kv_pairs) {
    if (kv.first == "first") {
      EXPECT_EQ(kv.second, "true");
      first_is_true = true;
    } else if (kv.first == "dtoi") {
      EXPECT_EQ(kv.second, "2018-11-04");
      has_dtoi = true;
      // Audit passed-through parameters.
      // Should not be modified (other than url encode).
    } else if (kv.first == "platform") {
      EXPECT_EQ(kv.second, "platform+id+here");
    } else if (kv.first == "channel") {
      EXPECT_EQ(kv.second, "channel+name+here");
    } else if (kv.first == "version") {
      EXPECT_EQ(kv.second, "full+brave+version+here");
    }
  }

  EXPECT_EQ(true, first_is_true);
  EXPECT_EQ(true, has_dtoi);
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
  // Make first run date a month earlier (outside 30 day window)
  exploded.month = 12;
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

TEST_F(BraveStatsUpdaterTest, UsageBitstringDaily) {
  base::Time last_reported_use;
  base::Time last_use;

  EXPECT_TRUE(base::Time::FromString("2020-03-31", &last_use));
  EXPECT_TRUE(base::Time::FromString("2020-03-30", &last_reported_use));

  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);

  EXPECT_EQ(0b001, brave_stats::UsageBitfieldFromTimestamp(last_use,
                                                           last_reported_use));
}

TEST_F(BraveStatsUpdaterTest, UsageBitstringWeekly) {
  base::Time last_reported_use;
  base::Time last_use;

  EXPECT_TRUE(base::Time::FromString("2020-03-31", &last_use));
  EXPECT_TRUE(base::Time::FromString("2020-03-26", &last_reported_use));

  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);

  EXPECT_EQ(0b011, brave_stats::UsageBitfieldFromTimestamp(last_use,
                                                           last_reported_use));
}

TEST_F(BraveStatsUpdaterTest, UsageBitstringMonthlySameWeek) {
  base::Time last_reported_use;
  base::Time last_use;

  EXPECT_TRUE(base::Time::FromString("2020-07-01", &last_use));
  EXPECT_TRUE(base::Time::FromString("2020-06-30", &last_reported_use));

  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);
  EXPECT_EQ(0b101, brave_stats::UsageBitfieldFromTimestamp(last_use,
                                                           last_reported_use));
}

TEST_F(BraveStatsUpdaterTest, UsageBitstringMonthlyDiffWeek) {
  base::Time last_reported_use;
  base::Time last_use;

  EXPECT_TRUE(base::Time::FromString("2020-03-01", &last_use));
  EXPECT_TRUE(base::Time::FromString("2020-02-15", &last_reported_use));

  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);
  EXPECT_EQ(0b111, brave_stats::UsageBitfieldFromTimestamp(last_use,
                                                           last_reported_use));
}

TEST_F(BraveStatsUpdaterTest, UsageBitstringInactive) {
  base::Time last_reported_use;
  base::Time last_use;

  EXPECT_TRUE(base::Time::FromString("2020-03-31", &last_use));
  EXPECT_TRUE(base::Time::FromString("2020-03-31", &last_reported_use));

  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);
  EXPECT_EQ(0b000, brave_stats::UsageBitfieldFromTimestamp(last_use,
                                                           last_reported_use));
}

TEST_F(BraveStatsUpdaterTest, UsageBitstringNeverUsed) {
  base::Time last_reported_use;
  base::Time last_use;

  brave_stats::BraveStatsUpdaterParams brave_stats_updater_params(
      GetLocalState(), GetProfilePrefs(), brave_stats::ProcessArch::kArchSkip,
      kToday, kThisWeek, kThisMonth);
  EXPECT_EQ(0b000, brave_stats::UsageBitfieldFromTimestamp(last_use,
                                                           last_reported_use));
}

TEST_F(BraveStatsUpdaterTest, UsageURLFlags) {
  auto params = BuildUpdaterParams();

  GURL base_url("http://localhost:8080");
  GURL url;

  url = params->GetUpdateURL(base_url, "", "", "");
  EXPECT_TRUE(url.query().find("daily=true&weekly=true&monthly=true") !=
              std::string::npos);
  EXPECT_TRUE(url.query().find("wallet2=0") != std::string::npos);
  params->SavePrefs();

  task_environment_.AdvanceClock(base::Days(1));
  GetProfilePrefs()->SetTime(kBraveWalletLastUnlockTime, base::Time::Now());

  params = BuildUpdaterParams();
  url = params->GetUpdateURL(base_url, "", "", "");
  EXPECT_NE(url.query().find("daily=true&weekly=false&monthly=false"),
            std::string::npos);
  EXPECT_NE(url.query().find("wallet2=7"), std::string::npos);
  params->SavePrefs();

  task_environment_.AdvanceClock(base::Days(6));
  GetProfilePrefs()->SetTime(kBraveWalletLastUnlockTime, base::Time::Now());
  params = BuildUpdaterParams();
  url = params->GetUpdateURL(base_url, "", "", "");
  EXPECT_NE(url.query().find("daily=true&weekly=true&monthly=false"),
            std::string::npos);
  EXPECT_NE(url.query().find("wallet2=3"), std::string::npos);
  params->SavePrefs();

  task_environment_.AdvanceClock(base::Days(1));
  GetProfilePrefs()->SetTime(kBraveWalletLastUnlockTime, base::Time::Now());
  params = BuildUpdaterParams();
  url = params->GetUpdateURL(base_url, "", "", "");
  EXPECT_NE(url.query().find("daily=true&weekly=false&monthly=false"),
            std::string::npos);
  EXPECT_NE(url.query().find("wallet2=1"), std::string::npos);
  params->SavePrefs();
}

TEST_F(BraveStatsUpdaterTest, UsagePingRequest) {
  int ping_count = 0;
  GURL last_url;

  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(request.url.spec(), "{\"ok\":1}");
        EXPECT_EQ(request.url.spec().find("https://localhost:8443"), (size_t)0);
      }));

  brave_stats::BraveStatsUpdater updater(GetLocalState());
  updater.SetURLLoaderFactoryForTesting(shared_url_loader_factory_);
  brave_stats::BraveStatsUpdater::StatsUpdatedCallback cb = base::BindRepeating(
      [](int* ping_count, GURL* last_url, const GURL& url) {
        *last_url = url;
        (*ping_count)++;
      },
      &ping_count, &last_url);
  updater.SetStatsUpdatedCallbackForTesting(&cb);
  updater.SetUsageServerForTesting("https://localhost:8443");
  updater.SetProfilePrefsForTesting(GetProfilePrefs());

  updater.Start();

  // daily, monthly, weekly ping
  task_environment_.FastForwardBy(base::Hours(1));
  EXPECT_NE(last_url.query().find("daily=true&weekly=true&monthly=true"),
            std::string::npos);

  // daily ping
  task_environment_.AdvanceClock(base::Days(1));
  task_environment_.FastForwardBy(base::Seconds(1));
  EXPECT_NE(last_url.query().find("daily=true&weekly=false&monthly=false"),
            std::string::npos);

  // daily, weekly ping
  task_environment_.AdvanceClock(base::Days(7));
  task_environment_.FastForwardBy(base::Seconds(1));
  EXPECT_NE(last_url.query().find("daily=true&weekly=true&monthly=false"),
            std::string::npos);

  ASSERT_EQ(ping_count, 3);
}
