/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <vector>

#include "base/functional/callback_helpers.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/components/tor/tor_constants.h"
#include "brave/components/tor/tor_utils.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_test_util.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/test/base/ui_test_utils.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_ADS)
#include "brave/browser/brave_ads/ads_service_factory.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)

class BraveProfileManagerTest : public PlatformBrowserTest {
};


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

  EXPECT_EQ(brave_rewards::RewardsServiceFactory::GetForProfile(guest_profile),
            nullptr);
#if BUILDFLAG(ENABLE_BRAVE_ADS)
  EXPECT_EQ(brave_ads::AdsServiceFactory::GetForProfile(guest_profile),
            nullptr);
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)

  ASSERT_TRUE(otr_profile->IsOffTheRecord());

  EXPECT_NE(brave_rewards::RewardsServiceFactory::GetForProfile(profile),
            nullptr);
  EXPECT_EQ(brave_rewards::RewardsServiceFactory::GetForProfile(otr_profile),
            nullptr);

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
IN_PROC_BROWSER_TEST_F(BraveProfileManagerTest,
                       GetLastUsedProfileName) {
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
