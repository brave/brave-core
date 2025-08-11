/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// This is a modified version of
// chrome/browser/ui/startup/first_run_service_unittest.cc

#include "chrome/browser/ui/startup/first_run_service.h"

#include "base/files/file_path.h"
#include "base/test/task_environment.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/ui/startup/first_run_test_util.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/signin/public/identity_manager/identity_test_environment.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !BUILDFLAG(ENABLE_DICE_SUPPORT)
#error "Unsupported platform"
#endif

class FirstRunServiceTest : public testing::Test {
 private:
  content::BrowserTaskEnvironment task_environment_;
};

// Test that even if primary account is populated, the profile name isn't
// changed (because we override FinalizeNewProfileSetup).
TEST_F(FirstRunServiceTest, FinishProfileSetUpShouldNotChangeName) {
  TestingProfileManager testing_profile_manager{
      TestingBrowserProcess::GetGlobal()};
  ASSERT_TRUE(testing_profile_manager.SetUp());
  Profile* profile =
      testing_profile_manager.CreateTestingProfile("Test Profile");

  ProfileAttributesEntry* entry =
      testing_profile_manager.profile_attributes_storage()
          ->GetProfileAttributesWithPath(profile->GetPath());
  EXPECT_EQ(u"Test Profile", entry->GetLocalProfileName());

  // Note: the identity manager is not connected to the profile, but for this
  // test, it's not necessary.
  signin::IdentityTestEnvironment identity_test_env;
  auto first_run_service =
      FirstRunService(*profile, *identity_test_env.identity_manager());

  g_browser_process->local_state()->SetBoolean(prefs::kFirstRunFinished, true);
  first_run_service.FinishProfileSetUp(u"New Profile Name");

  // The profile name should still be unchanged.
  EXPECT_EQ(u"Test Profile", entry->GetLocalProfileName());
}
