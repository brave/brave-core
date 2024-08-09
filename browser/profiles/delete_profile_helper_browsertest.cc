/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/delete_profile_helper.h"

#include "base/files/file_path.h"
#include "base/functional/callback_helpers.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "brave/components/sync/service/brave_sync_service_impl.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/keep_alive/profile_keep_alive_waiter.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_test_util.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

// Profiles are not supported on Android and iOS, so can't do the test
static_assert(!BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS));

namespace {
const char kValidSyncCode[] =
    "fringe digital begin feed equal output proof cheap "
    "exotic ill sure question trial squirrel glove celery "
    "awkward push jelly logic broccoli almost grocery drift";
}

// Inspired by src/chrome/browser/profiles/delete_profile_helper_browsertest.cc
class DeleteProfileHelperBrowserTest : public InProcessBrowserTest {
 public:
  DeleteProfileHelperBrowserTest() = default;
  ~DeleteProfileHelperBrowserTest() override = default;
};

IN_PROC_BROWSER_TEST_F(DeleteProfileHelperBrowserTest,
                       SyncStoppedForDeletedProfile) {
  // Create an additional profile.
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  base::FilePath profile_path_to_delete =
      profile_manager->GenerateNextProfileDirectoryPath();
  Profile& profile_to_delete = profiles::testing::CreateProfileSync(
      profile_manager, profile_path_to_delete);
  ASSERT_TRUE(profile_manager->GetProfileAttributesStorage()
                  .GetProfileAttributesWithPath(profile_path_to_delete));
  // Set the profile as last-used, so that the callback of
  // `MaybeScheduleProfileForDeletion()` is called.
  Browser::Create(Browser::CreateParams(&profile_to_delete, true));
  profiles::SetLastUsedProfile(profile_path_to_delete.BaseName());
  // Schedule profile deletion.
  ProfileKeepAliveAddedWaiter keep_alive_added_waiter(
      &profile_to_delete, ProfileKeepAliveOrigin::kProfileDeletionProcess);

  syncer::BraveSyncServiceImpl* brave_sync_service =
      static_cast<syncer::BraveSyncServiceImpl*>(
          SyncServiceFactory::GetAsSyncServiceImplForProfileForTesting(
              &profile_to_delete));

  // Set the sync code to ensure it will be cleared on profile deletion
  EXPECT_TRUE(brave_sync_service->SetSyncCode(kValidSyncCode));

  brave_sync::Prefs brave_sync_prefs(profile_to_delete.GetPrefs());
  bool failed_to_decrypt;
  std::string seed = brave_sync_prefs.GetSeed(&failed_to_decrypt);
  ASSERT_FALSE(failed_to_decrypt);
  EXPECT_FALSE(seed.empty());

  base::RunLoop loop;
  profile_manager->GetDeleteProfileHelper().MaybeScheduleProfileForDeletion(
      profile_path_to_delete,
      base::BindLambdaForTesting([&loop](Profile* profile) { loop.Quit(); }),
      ProfileMetrics::DELETE_PROFILE_PRIMARY_ACCOUNT_NOT_ALLOWED);

  keep_alive_added_waiter.Wait();
  loop.Run();

  // Ensure that we've invoked BraveSyncServiceImpl::StopAndClear() from
  // DisableSyncForProfileDeletion at delete_profile_helper.cc, so
  // now seed should be empty
  seed = brave_sync_prefs.GetSeed(&failed_to_decrypt);
  ASSERT_FALSE(failed_to_decrypt);
  EXPECT_TRUE(seed.empty());
}
