// Copyright 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/tor/tor_constants.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"

#if BUILDFLAG(IPFS_ENABLED)
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/ipfs/features.h"
#endif

namespace {

// An observer that returns back to test code after a new profile is
// initialized.
void OnUnblockOnProfileCreation(base::RunLoop* run_loop,
                                Profile* profile,
                                Profile::CreateStatus status) {
  if (status == Profile::CREATE_STATUS_INITIALIZED)
    run_loop->Quit();
}

struct TestProfileData {
  base::string16 profile_name;
  base::string16 profile_name_expected_after_migration;
  bool force_default_name;
  base::FilePath profile_path;
};

std::vector<TestProfileData> GetTestProfileData(
    ProfileManager* profile_manager) {
  const std::vector<TestProfileData> profile_data = {
    {
      base::ASCIIToUTF16("Person 1"),
      base::ASCIIToUTF16("Profile 1"), true,
      profile_manager->user_data_dir().Append(
          profile_manager->GetInitialProfileDir())},
    {
      base::ASCIIToUTF16("Person 2"),
      base::ASCIIToUTF16("Profile 2"), true,
      profile_manager->user_data_dir().Append(
          FILE_PATH_LITERAL("testprofile2"))},
    {
      base::ASCIIToUTF16("ZZCustom 3"),
      base::ASCIIToUTF16("ZZCustom 3"), false,
      profile_manager->user_data_dir().Append(
          FILE_PATH_LITERAL("testprofile3"))},
  };
  return profile_data;
}

}  // namespace

class BraveProfileManagerTest : public InProcessBrowserTest {
 public:
  BraveProfileManagerTest() {
#if BUILDFLAG(IPFS_ENABLED)
    feature_list_.InitAndEnableFeature(ipfs::features::kIpfsFeature);
#endif
  }

 private:
#if BUILDFLAG(IPFS_ENABLED)
  base::test::ScopedFeatureList feature_list_;
#endif
};

// Test that legacy profile names (Person X) that have
// not been user-modified are automatically renamed
// to brave profile names (Profile X).
IN_PROC_BROWSER_TEST_F(BraveProfileManagerTest,
                       DISABLED_PRE_MigrateProfileNames) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ProfileAttributesStorage& storage =
      profile_manager->GetProfileAttributesStorage();
  auto profile_data = GetTestProfileData(profile_manager);
  // Create profiles with old default name
  // Two profiles with legacy default names, to check rename happens
  // in correct order.
  // One profile with a custom name to check that it is not renamed.
  // First is the existing default profile.
  ProfileAttributesEntry* entry1 =
      storage.GetProfileAttributesWithPath(profile_data[0].profile_path);
  ASSERT_NE(entry1, nullptr);
  entry1->SetLocalProfileName(profile_data[0].profile_name,
                              profile_data[0].force_default_name);
  // Rest are generated
  for (size_t i = 0; i != profile_data.size(); i++) {
    base::RunLoop run_loop;
    profile_manager->CreateProfileAsync(
        profile_data[i].profile_path,
        base::Bind(&OnUnblockOnProfileCreation, &run_loop), base::string16(),
        std::string());
    run_loop.Run();
    ProfileAttributesEntry* entry =
        storage.GetProfileAttributesWithPath(profile_data[i].profile_path);
    ASSERT_NE(entry, nullptr);
    entry->SetLocalProfileName(profile_data[i].profile_name,
                               profile_data[i].force_default_name);
  }
}

IN_PROC_BROWSER_TEST_F(BraveProfileManagerTest,
                       DISABLED_MigrateProfileNames) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ProfileAttributesStorage& storage =
      profile_manager->GetProfileAttributesStorage();
  auto profile_data = GetTestProfileData(profile_manager);
  auto entries =
      storage.GetAllProfilesAttributesSortedByName();
  // Verify we still have the expected number of profiles.
  ASSERT_EQ(entries.size(), profile_data.size());
  // Order of items in entries and profile_data should be the same
  // since we manually ensure profile_data is alphabetical.
  for (size_t i = 0; i != entries.size(); i++) {
    // Verify the names changed
    ASSERT_EQ(entries[i]->GetName(),
        profile_data[i].profile_name_expected_after_migration);
    // Verify the path matches, i.e. it is the same profile that got the number
    // that the profile had before migration, so we're sure that profile numbers
    // aren't re-assigned.
    ASSERT_EQ(entries[i]->GetPath(), profile_data[i].profile_path);
  }
}

IN_PROC_BROWSER_TEST_F(BraveProfileManagerTest,
                       ExcludeServicesInOTRAndGuestProfiles) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);
  Profile* profile = ProfileManager::GetActiveUserProfile();
  Profile* otr_profile = profile->GetPrimaryOTRProfile();

  profiles::SwitchToGuestProfile(ProfileManager::CreateCallback());
  ui_test_utils::WaitForBrowserToOpen();

  Profile* guest_profile =
      profile_manager->GetProfileByPath(ProfileManager::GetGuestProfilePath());
  ASSERT_TRUE(otr_profile->IsOffTheRecord());
  ASSERT_TRUE(guest_profile->IsGuestSession());

  EXPECT_NE(
      brave_rewards::RewardsServiceFactory::GetForProfile(profile), nullptr);
  EXPECT_EQ(
      brave_rewards::RewardsServiceFactory::GetForProfile(otr_profile),
      nullptr);
  EXPECT_EQ(
      brave_rewards::RewardsServiceFactory::GetForProfile(guest_profile),
      nullptr);

  EXPECT_NE(brave_ads::AdsServiceFactory::GetForProfile(profile), nullptr);
  EXPECT_EQ(brave_ads::AdsServiceFactory::GetForProfile(otr_profile),
            nullptr);
  EXPECT_EQ(brave_ads::AdsServiceFactory::GetForProfile(guest_profile),
            nullptr);

#if BUILDFLAG(IPFS_ENABLED)
  EXPECT_NE(ipfs::IpfsServiceFactory::GetForContext(profile), nullptr);
  EXPECT_EQ(ipfs::IpfsServiceFactory::GetForContext(otr_profile), nullptr);
  EXPECT_EQ(ipfs::IpfsServiceFactory::GetForContext(guest_profile), nullptr);
#endif
}

IN_PROC_BROWSER_TEST_F(BraveProfileManagerTest,
                       GetLastUsedProfileName) {
  g_browser_process->local_state()->SetString(
      prefs::kProfileLastUsed,
      base::FilePath(tor::kTorProfileDir).AsUTF8Unsafe());
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  base::FilePath last_used_path =
      g_browser_process->profile_manager()->GetLastUsedProfileDir(
          profile_manager->user_data_dir());
  EXPECT_EQ(last_used_path.BaseName().AsUTF8Unsafe(), chrome::kInitialProfile);
}
