/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/analytics/p3a/brave_stats_helper.h"

#include "base/files/file_path.h"
#include "base/test/metrics/histogram_tester.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_test_util.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_ads {

class BraveStatsHelperBrowserTest : public PlatformBrowserTest {
 public:
  BraveStatsHelperBrowserTest() = default;

 protected:
  Profile& CreateProfile(base::FilePath& profile_path) {
    profile_path = profile_manager()->GenerateNextProfileDirectoryPath();
    return profiles::testing::CreateProfileSync(profile_manager(),
                                                profile_path);
  }

  ProfileManager* profile_manager() const {
    return g_browser_process->profile_manager();
  }

  PrefService* local_state() const { return g_browser_process->local_state(); }

  BraveStatsHelper* brave_stats_helper() const {
    return g_brave_browser_process->ads_brave_stats_helper();
  }

  base::HistogramTester histogram_tester_;
};

IN_PROC_BROWSER_TEST_F(BraveStatsHelperBrowserTest,
                       PrimaryProfileEnabledUpdate) {
  Profile* primary_profile = profile_manager()->GetLastUsedProfile();

  EXPECT_EQ(local_state()->GetBoolean(prefs::kEnabledForLastProfile), false);

  primary_profile->GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds,
                                          true);
  EXPECT_EQ(local_state()->GetBoolean(prefs::kEnabledForLastProfile), true);

  primary_profile->GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds,
                                          false);
  EXPECT_EQ(local_state()->GetBoolean(prefs::kEnabledForLastProfile), false);
}

#if !BUILDFLAG(IS_ANDROID)
IN_PROC_BROWSER_TEST_F(BraveStatsHelperBrowserTest, ProfileSwitch) {
  base::FilePath profile_one_path;
  Profile& profile_one = CreateProfile(profile_one_path);
  profile_one.GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds, true);

  profiles::testing::SwitchToProfileSync(profile_one_path);
  EXPECT_EQ(local_state()->GetBoolean(prefs::kEnabledForLastProfile), true);

  base::FilePath profile_two_path;
  CreateProfile(profile_two_path);
  profiles::testing::SwitchToProfileSync(profile_two_path);
  EXPECT_EQ(local_state()->GetBoolean(prefs::kEnabledForLastProfile), false);

  profiles::testing::SwitchToProfileSync(profile_one_path);
  EXPECT_EQ(local_state()->GetBoolean(prefs::kEnabledForLastProfile), true);
}

IN_PROC_BROWSER_TEST_F(BraveStatsHelperBrowserTest, MultiProfileEnabledUpdate) {
  base::FilePath profile_one_path;
  Profile& profile_one = CreateProfile(profile_one_path);
  profile_one.GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds, true);

  profiles::testing::SwitchToProfileSync(profile_one_path);
  EXPECT_EQ(local_state()->GetBoolean(prefs::kEnabledForLastProfile), true);

  base::FilePath profile_two_path;
  Profile& profile_two = CreateProfile(profile_two_path);
  profile_two.GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds, true);
  EXPECT_EQ(local_state()->GetBoolean(prefs::kEnabledForLastProfile), true);

  profile_one.GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds, false);
  EXPECT_EQ(local_state()->GetBoolean(prefs::kEnabledForLastProfile), false);

  profiles::testing::SwitchToProfileSync(profile_two_path);
  EXPECT_EQ(local_state()->GetBoolean(prefs::kEnabledForLastProfile), true);
}
#endif

IN_PROC_BROWSER_TEST_F(BraveStatsHelperBrowserTest,
                       AdsEnabledInstallationTime) {
  brave_stats_helper()->SetFirstRunTimeForTesting(base::Time::Now() -
                                                  base::Minutes(45));

  Profile* primary_profile = profile_manager()->GetLastUsedProfile();
  primary_profile->GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds,
                                          true);

  histogram_tester_.ExpectUniqueSample(kAdsEnabledInstallationTimeHistogramName,
                                       0, 1);

  primary_profile->GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds,
                                          false);
  primary_profile->GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds,
                                          true);

  histogram_tester_.ExpectUniqueSample(kAdsEnabledInstallationTimeHistogramName,
                                       0, 1);

  // Reset to test another bucket value
  primary_profile->GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds,
                                          false);
  local_state()->SetBoolean(prefs::kEverEnabledForAnyProfile, false);
  brave_stats_helper()->SetFirstRunTimeForTesting(base::Time::Now() -
                                                  base::Minutes(70));

  primary_profile->GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds,
                                          true);
  histogram_tester_.ExpectBucketCount(kAdsEnabledInstallationTimeHistogramName,
                                      1, 1);
}

}  // namespace brave_ads
