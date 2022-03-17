// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/brave_p3a_version_util.h"

#include <string>

#include "brave/browser/brave_stats/brave_stats_updater.h"
#include "brave/common/pref_names.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/version_info/version_info.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave {

class BraveP3AVersionUtilTest : public testing::Test {
 public:
  BraveP3AVersionUtilTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

 protected:
  void SetUp() override {
    auto* registry = pref_service_.registry();
    RegisterP3AVersionUtilPrefs(registry);
    brave_stats::RegisterLocalStatePrefs(registry);
    task_environment_.AdvanceClock(base::Days(60));
  }

  PrefService* GetPrefs() { return &pref_service_; }

  content::BrowserTaskEnvironment task_environment_;

 private:
  TestingPrefServiceSimple pref_service_;
};

TEST_F(BraveP3AVersionUtilTest, TestFirstInstallDetected) {
  PrefService* prefs = GetPrefs();
  std::string curr_version =
      version_info::GetBraveVersionWithoutChromiumMajorVersion();

  IsBrowserAtLatestVersion(prefs);

  ASSERT_EQ(prefs->GetTime(kP3ACurrentVersionInstallTime), base::Time::Now());
  ASSERT_EQ(prefs->GetString(kP3ALastKnownInstalledVersion), curr_version);

  task_environment_.AdvanceClock(base::Days(3));

  IsBrowserAtLatestVersion(prefs);

  ASSERT_TRUE((base::Time::Now() -
               prefs->GetTime(kP3ACurrentVersionInstallTime)) >= base::Days(3));
  ASSERT_EQ(prefs->GetString(kP3ALastKnownInstalledVersion), curr_version);
}

TEST_F(BraveP3AVersionUtilTest, TestNewVersionDetected) {
  PrefService* prefs = GetPrefs();
  std::string curr_version =
      version_info::GetBraveVersionWithoutChromiumMajorVersion();

  prefs->SetString(kP3ALastKnownInstalledVersion, "1.0");
  prefs->SetTime(kP3ACurrentVersionInstallTime,
                 base::Time::Now() - base::Days(5));

  IsBrowserAtLatestVersion(prefs);

  ASSERT_EQ(prefs->GetTime(kP3ACurrentVersionInstallTime), base::Time::Now());
  ASSERT_EQ(prefs->GetString(kP3ALastKnownInstalledVersion), curr_version);
}

TEST_F(BraveP3AVersionUtilTest, TestWithUsagePingInfo) {
  PrefService* prefs = GetPrefs();
  std::string curr_version =
      version_info::GetBraveVersionWithoutChromiumMajorVersion();

  prefs->SetBoolean(kStatsReportingEnabled, true);
  prefs->SetString(kP3ALastKnownInstalledVersion, curr_version);
  prefs->SetTime(kP3ACurrentVersionInstallTime,
                 base::Time::Now() - base::Days(31));

  // if stats updater did not save the latest version yet, use fallback method
  ASSERT_FALSE(IsBrowserAtLatestVersion(prefs));

  prefs->SetTime(kP3ACurrentVersionInstallTime,
                 base::Time::Now() - base::Days(25));
  ASSERT_TRUE(IsBrowserAtLatestVersion(prefs));

  // stats updater has now set the latest version, do not use fallback method
  prefs->SetString(kLatestBrowserVersion,
                   version_info::GetBraveVersionWithoutChromiumMajorVersion());
  ASSERT_TRUE(IsBrowserAtLatestVersion(prefs));

  prefs->SetString(kLatestBrowserVersion, "9999.123.45");
  ASSERT_FALSE(IsBrowserAtLatestVersion(prefs));
}

TEST_F(BraveP3AVersionUtilTest, TestWithFallbackMethod) {
  PrefService* prefs = GetPrefs();
  std::string curr_version =
      version_info::GetBraveVersionWithoutChromiumMajorVersion();

  prefs->SetBoolean(kStatsReportingEnabled, false);
  prefs->SetString(kP3ALastKnownInstalledVersion, curr_version);
  prefs->SetTime(kP3ACurrentVersionInstallTime,
                 base::Time::Now() - base::Days(35));

  IsBrowserAtLatestVersion(prefs);

  ASSERT_FALSE(IsBrowserAtLatestVersion(prefs));

  prefs->SetTime(kP3ACurrentVersionInstallTime,
                 base::Time::Now() - base::Days(24));
  ASSERT_TRUE(IsBrowserAtLatestVersion(prefs));
}

}  // namespace brave
