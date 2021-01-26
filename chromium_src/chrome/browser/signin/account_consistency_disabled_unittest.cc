// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/account_consistency_mode_manager.h"

#include <memory>
#include <utility>

#include "base/test/scoped_feature_list.h"
#include "build/buildflag.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/supervised_user/supervised_user_constants.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_notifier_impl.h"
#include "components/prefs/testing_pref_store.h"
#include "components/signin/public/base/account_consistency_method.h"
#include "components/signin/public/base/signin_buildflags.h"
#include "components/signin/public/base/signin_pref_names.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
// Checks that new profiles are migrated at creation.
TEST(AccountConsistencyDisabledTest, NewProfile) {
  content::BrowserTaskEnvironment task_environment;
  // kSignInProcessIsolation used to be needed here but it has since been
  // turned on to 100% of the user base and is no longer needed.
  // See 36417aa39a5e8484b23f1ec927bfda23465f4f21
  TestingProfile::Builder profile_builder;
  {
    TestingPrefStore* user_prefs = new TestingPrefStore();

    // Set the read error so that Profile::IsNewProfile() returns true.
    user_prefs->set_read_error(PersistentPrefStore::PREF_READ_ERROR_NO_FILE);

    std::unique_ptr<sync_preferences::TestingPrefServiceSyncable> pref_service =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>(
            /*managed_prefs=*/new TestingPrefStore(),
            /*supervised_user_prefs=*/new TestingPrefStore(),
            /*extension_prefs=*/new TestingPrefStore(),
            user_prefs,
            /*recommended_prefs=*/new TestingPrefStore(),
            new user_prefs::PrefRegistrySyncable(),
            new PrefNotifierImpl());
    RegisterUserProfilePrefs(pref_service->registry());
    profile_builder.SetPrefService(std::move(pref_service));
  }
  std::unique_ptr<TestingProfile> profile = profile_builder.Build();
  ASSERT_TRUE(profile->IsNewProfile());
  EXPECT_TRUE(AccountConsistencyModeManager::IsDiceEnabledForProfile(
        profile.get()));
}

TEST(AccountConsistencyDisabledTest, DiceFixAuthErrorsForAllProfiles) {
  content::BrowserTaskEnvironment task_environment;

  {
    // Regular profile.
    TestingProfile profile;
    EXPECT_TRUE(
        AccountConsistencyModeManager::IsDiceEnabledForProfile(&profile));
    EXPECT_EQ(signin::AccountConsistencyMethod::kDice,
              AccountConsistencyModeManager::GetMethodForProfile(&profile));

    // Incognito profile.
    Profile* incognito_profile = profile.GetOffTheRecordProfile(
        Profile::OTRProfileID::PrimaryID());
    EXPECT_FALSE(AccountConsistencyModeManager::IsDiceEnabledForProfile(
        incognito_profile));
    EXPECT_FALSE(
        AccountConsistencyModeManager::GetForProfile(incognito_profile));
    EXPECT_EQ(
        signin::AccountConsistencyMethod::kDisabled,
        AccountConsistencyModeManager::GetMethodForProfile(incognito_profile));
  }

  {
    // Guest profile.
    TestingProfile::Builder profile_builder;
    profile_builder.SetGuestSession();
    std::unique_ptr<Profile> profile = profile_builder.Build();
    ASSERT_TRUE(profile->IsGuestSession());
    EXPECT_FALSE(
        AccountConsistencyModeManager::IsDiceEnabledForProfile(profile.get()));
    EXPECT_EQ(
        signin::AccountConsistencyMethod::kDisabled,
        AccountConsistencyModeManager::GetMethodForProfile(profile.get()));
  }
}
#endif  // BUILDFLAG(ENABLE_DICE_SUPPORT)
