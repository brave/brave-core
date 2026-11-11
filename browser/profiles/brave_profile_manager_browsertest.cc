/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <vector>

#include "base/functional/callback_helpers.h"
#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/components/tor/tor_constants.h"
#include "brave/components/tor/tor_utils.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_test_util.h"
#include "chrome/browser/ssl/https_first_mode_settings_tracker.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "net/base/features.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/test/base/ui_test_utils.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_ADS)
#include "brave/browser/brave_ads/ads_service_factory.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#endif

class BraveProfileManagerTest : public PlatformBrowserTest {};

// We use x86 builds on Android to run tests and rewards with ads
// are off on x86 builds
#if !BUILDFLAG(IS_ANDROID)
IN_PROC_BROWSER_TEST_F(BraveProfileManagerTest,
                       ExcludeServicesInOTRAndGuestProfiles) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);
  Profile* profile = ProfileManager::GetLastUsedProfile();
  Profile* otr_profile =
      profile->GetPrimaryOTRProfile(/*create_if_needed=*/true);

  profiles::SwitchToGuestProfile(base::DoNothing());
  ui_test_utils::WaitForBrowserToOpen();

  Profile* guest_profile =
      profile_manager->GetProfileByPath(ProfileManager::GetGuestProfilePath());

  ASSERT_TRUE(guest_profile->IsGuestSession());

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  EXPECT_EQ(brave_rewards::RewardsServiceFactory::GetForProfile(guest_profile),
            nullptr);
#endif

#if BUILDFLAG(ENABLE_BRAVE_ADS)
  EXPECT_EQ(brave_ads::AdsServiceFactory::GetForProfile(guest_profile),
            nullptr);
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)

  ASSERT_TRUE(otr_profile->IsOffTheRecord());

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  EXPECT_NE(brave_rewards::RewardsServiceFactory::GetForProfile(profile),
            nullptr);
  EXPECT_EQ(brave_rewards::RewardsServiceFactory::GetForProfile(otr_profile),
            nullptr);
#endif

#if BUILDFLAG(ENABLE_BRAVE_ADS)
  EXPECT_NE(brave_ads::AdsServiceFactory::GetForProfile(profile), nullptr);
  EXPECT_EQ(brave_ads::AdsServiceFactory::GetForProfile(otr_profile), nullptr);
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)
}
#endif

#if !BUILDFLAG(IS_ANDROID)
IN_PROC_BROWSER_TEST_F(BraveProfileManagerTest,
                       PRE_MediaRouterDisabledRestartTest) {
  Profile* profile = g_browser_process->profile_manager()->GetLastUsedProfile();
  {
    profile->GetPrefs()->SetBoolean(::prefs::kEnableMediaRouter, true);
    profile->GetPrefs()->SetBoolean(kEnableMediaRouterOnRestart, false);
    EXPECT_TRUE(profile->GetPrefs()->GetBoolean(::prefs::kEnableMediaRouter));
    EXPECT_FALSE(profile->GetPrefs()->GetBoolean(kEnableMediaRouterOnRestart));
  }
}

IN_PROC_BROWSER_TEST_F(BraveProfileManagerTest,
                       MediaRouterDisabledRestartTest) {
  Profile* profile = g_browser_process->profile_manager()->GetLastUsedProfile();
  {
    EXPECT_FALSE(profile->GetPrefs()->GetBoolean(::prefs::kEnableMediaRouter));
    EXPECT_FALSE(profile->GetPrefs()->GetBoolean(kEnableMediaRouterOnRestart));
  }
}

IN_PROC_BROWSER_TEST_F(BraveProfileManagerTest,
                       PRE_MediaRouterEnabledRestartTest) {
  Profile* profile = g_browser_process->profile_manager()->GetLastUsedProfile();
  {
    profile->GetPrefs()->SetBoolean(::prefs::kEnableMediaRouter, false);
    profile->GetPrefs()->SetBoolean(kEnableMediaRouterOnRestart, true);
    EXPECT_FALSE(profile->GetPrefs()->GetBoolean(::prefs::kEnableMediaRouter));
    EXPECT_TRUE(profile->GetPrefs()->GetBoolean(kEnableMediaRouterOnRestart));
  }
}

IN_PROC_BROWSER_TEST_F(BraveProfileManagerTest, MediaRouterEnabledRestartTest) {
  Profile* profile = g_browser_process->profile_manager()->GetLastUsedProfile();
  {
    EXPECT_TRUE(profile->GetPrefs()->GetBoolean(::prefs::kEnableMediaRouter));
    EXPECT_TRUE(profile->GetPrefs()->GetBoolean(kEnableMediaRouterOnRestart));
  }
}
#endif

#if BUILDFLAG(ENABLE_TOR)
IN_PROC_BROWSER_TEST_F(BraveProfileManagerTest, GetLastUsedProfileName) {
  g_browser_process->local_state()->SetString(
      prefs::kProfileLastUsed,
      base::FilePath(tor::kTorProfileDir).AsUTF8Unsafe());

  // The migration happens during the initialization of the browser process, so
  // we need to explicitly call the method here to test it actually works.
  tor::MigrateLastUsedProfileFromLocalStatePrefs(
      g_browser_process->local_state());

  ProfileManager* profile_manager = g_browser_process->profile_manager();
  base::FilePath last_used_path = profile_manager->GetLastUsedProfileDir();
  EXPECT_EQ(last_used_path.BaseName().AsUTF8Unsafe(), chrome::kInitialProfile);
}
#endif

struct HttpsUpgradesCase {
  std::vector<base::test::FeatureRef> enabled_features;
  std::vector<base::test::FeatureRef> disabled_features;

  HttpsFirstModeSetting upstream_https_upgrade;
  brave_shields::ControlType brave_https_upgrade;

  HttpsFirstModeSetting expected_upstream_https_upgrade;
  brave_shields::ControlType expected_brave_https_upgrade;
};

class BraveProfileManagerHttpsUpgradesTest
    : public PlatformBrowserTest,
      public ::testing::WithParamInterface<HttpsUpgradesCase> {
 public:
  BraveProfileManagerHttpsUpgradesTest() {
    if (IsSetup()) {
      scoped_feature_list_.InitAndEnableFeature(
          net::features::kBraveHttpsByDefault);
    } else {
      scoped_feature_list_.InitWithFeatures(GetParam().enabled_features,
                                            GetParam().disabled_features);
    }
  }

  static bool IsSetup() {
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    return std::string_view(test_info->name()).starts_with("PRE_PRE_");
  }

  static bool IsMainTest() {
    if (IsSetup()) {
      return false;
    }
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    return std::string_view(test_info->name()).starts_with("PRE_");
  }

  static bool IsNextLauchTest() {
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    return !std::string_view(test_info->name()).starts_with("PRE_");
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

INSTANTIATE_TEST_SUITE_P(
    ,
    BraveProfileManagerHttpsUpgradesTest,
    ::testing::Values(
        // Balance mode is unavailable.
        HttpsUpgradesCase{
            .enabled_features =
                {brave_shields::features::kTransitionToUpstreamHttpsUpgrades},
            .disabled_features = {net::features::kBraveHttpsByDefault},
            .brave_https_upgrade = brave_shields::ControlType::BLOCK,
            .expected_upstream_https_upgrade =
                HttpsFirstModeSetting::kEnabledFull,
            .expected_brave_https_upgrade = brave_shields::ControlType::BLOCK,
        },
        HttpsUpgradesCase{
            .enabled_features =
                {brave_shields::features::kTransitionToUpstreamHttpsUpgrades},
            .disabled_features = {net::features::kBraveHttpsByDefault},
            .brave_https_upgrade =
                brave_shields::ControlType::BLOCK_THIRD_PARTY,
            .expected_upstream_https_upgrade = HttpsFirstModeSetting::kDisabled,
            .expected_brave_https_upgrade =
                brave_shields::ControlType::BLOCK_THIRD_PARTY,
        },
        HttpsUpgradesCase{
            .enabled_features =
                {brave_shields::features::kTransitionToUpstreamHttpsUpgrades},
            .disabled_features = {net::features::kBraveHttpsByDefault},
            .brave_https_upgrade = brave_shields::ControlType::ALLOW,
            .expected_upstream_https_upgrade = HttpsFirstModeSetting::kDisabled,
            .expected_brave_https_upgrade = brave_shields::ControlType::ALLOW,
        },
        // Balance mode is available.
        HttpsUpgradesCase{
            .enabled_features =
                {brave_shields::features::kTransitionToUpstreamHttpsUpgrades,
                 features::kHttpsFirstBalancedMode},
            .disabled_features = {net::features::kBraveHttpsByDefault},
            .brave_https_upgrade = brave_shields::ControlType::BLOCK,
            .expected_upstream_https_upgrade =
                HttpsFirstModeSetting::kEnabledFull,
            .expected_brave_https_upgrade = brave_shields::ControlType::BLOCK,
        },
        HttpsUpgradesCase{
            .enabled_features =
                {brave_shields::features::kTransitionToUpstreamHttpsUpgrades,
                 features::kHttpsFirstBalancedMode},
            .disabled_features = {net::features::kBraveHttpsByDefault},
            .brave_https_upgrade =
                brave_shields::ControlType::BLOCK_THIRD_PARTY,
            .expected_upstream_https_upgrade =
                HttpsFirstModeSetting::kEnabledBalanced,
            .expected_brave_https_upgrade =
                brave_shields::ControlType::BLOCK_THIRD_PARTY,
        },
        HttpsUpgradesCase{
            .enabled_features =
                {brave_shields::features::kTransitionToUpstreamHttpsUpgrades,
                 features::kHttpsFirstBalancedMode},
            .disabled_features = {net::features::kBraveHttpsByDefault},
            .upstream_https_upgrade = HttpsFirstModeSetting::kDisabled,
            .brave_https_upgrade = brave_shields::ControlType::ALLOW,
            .expected_upstream_https_upgrade = HttpsFirstModeSetting::kDisabled,
            .expected_brave_https_upgrade = brave_shields::ControlType::ALLOW,
        },
        // Old migration.
        HttpsUpgradesCase{
            .enabled_features = {features::kHttpsFirstBalancedMode},
            .disabled_features = {net::features::kBraveHttpsByDefault},
            .brave_https_upgrade = brave_shields::ControlType::BLOCK,
            .expected_upstream_https_upgrade =
                HttpsFirstModeSetting::kEnabledFull,
            .expected_brave_https_upgrade =
                brave_shields::ControlType::BLOCK_THIRD_PARTY,
        },
        HttpsUpgradesCase{
            .enabled_features = {features::kHttpsFirstBalancedMode},
            .disabled_features = {net::features::kBraveHttpsByDefault},
            .brave_https_upgrade =
                brave_shields::ControlType::BLOCK_THIRD_PARTY,
            .expected_upstream_https_upgrade = HttpsFirstModeSetting::kDisabled,
            .expected_brave_https_upgrade =
                brave_shields::ControlType::BLOCK_THIRD_PARTY,
        },
        HttpsUpgradesCase{
            .enabled_features = {features::kHttpsFirstBalancedMode},
            .disabled_features = {net::features::kBraveHttpsByDefault},
            .brave_https_upgrade = brave_shields::ControlType::ALLOW,
            .expected_upstream_https_upgrade = HttpsFirstModeSetting::kDisabled,
            .expected_brave_https_upgrade = brave_shields::ControlType::ALLOW,
        }));

IN_PROC_BROWSER_TEST_P(BraveProfileManagerHttpsUpgradesTest,
                       PRE_PRE_Migration) {
  brave_shields::SetHttpsUpgradeControlType(
      HostContentSettingsMapFactory::GetForProfile(GetProfile()),
      GetParam().brave_https_upgrade, GURL());

  HttpsFirstModeService* hfm_service =
      HttpsFirstModeServiceFactory::GetForProfile(GetProfile());
  hfm_service->UpdatePrefs(HttpsFirstModeSetting::kDisabled);
}

IN_PROC_BROWSER_TEST_P(BraveProfileManagerHttpsUpgradesTest, PRE_Migration) {
  EXPECT_EQ(
      GetParam().expected_brave_https_upgrade,
      brave_shields::GetHttpsUpgradeControlType(
          HostContentSettingsMapFactory::GetForProfile(GetProfile()), GURL()));

  HttpsFirstModeService* hfm_service =
      HttpsFirstModeServiceFactory::GetForProfile(GetProfile());
  EXPECT_EQ(GetParam().expected_upstream_https_upgrade,
            hfm_service->GetCurrentSetting());

  hfm_service->UpdatePrefs(HttpsFirstModeSetting::kDisabled);
}

IN_PROC_BROWSER_TEST_P(BraveProfileManagerHttpsUpgradesTest, Migration) {
  EXPECT_EQ(
      GetParam().expected_brave_https_upgrade,
      brave_shields::GetHttpsUpgradeControlType(
          HostContentSettingsMapFactory::GetForProfile(GetProfile()), GURL()));

  HttpsFirstModeService* hfm_service =
      HttpsFirstModeServiceFactory::GetForProfile(GetProfile());
  EXPECT_EQ(HttpsFirstModeSetting::kDisabled, hfm_service->GetCurrentSetting());
}
