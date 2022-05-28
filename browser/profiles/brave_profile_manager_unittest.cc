// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <memory>

#include "base/files/scoped_temp_dir.h"
#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

class BraveProfileManagerTest : public testing::Test {
 public:
  BraveProfileManagerTest()
      : local_state_(TestingBrowserProcess::GetGlobal()) {}

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    TestingBrowserProcess::GetGlobal()->SetProfileManager(
        CreateProfileManagerForTest());
  }

  void TearDown() override {
    TestingBrowserProcess::GetGlobal()->SetProfileManager(nullptr);
    content::RunAllTasksUntilIdle();
  }

 protected:
  std::unique_ptr<BraveProfileManager> CreateProfileManagerForTest() {
    return std::make_unique<BraveProfileManagerWithoutInit>(
        temp_dir_.GetPath());
  }

  base::FilePath GetTempPath() { return temp_dir_.GetPath(); }

 private:
  base::ScopedTempDir temp_dir_;
  content::BrowserTaskEnvironment task_environment_;
  ScopedTestingLocalState local_state_;
};

TEST_F(BraveProfileManagerTest, EnableMediaRouterOnRestartDefaultValue) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);

  base::FilePath path = GetTempPath().AppendASCII("profile");
  TestingProfile::Builder builder;
  builder.SetPath(path);
  builder.SetIsNewProfile(true);
  std::unique_ptr<TestingProfile> profile = builder.Build();

  auto* pref_service = profile->GetTestingPrefService();

  pref_service->RemoveUserPref(kEnableMediaRouterOnRestart);
  EXPECT_TRUE(pref_service->FindPreference(kEnableMediaRouterOnRestart)
                  ->IsDefaultValue());
  pref_service->SetBoolean(::prefs::kEnableMediaRouter, true);
  profile_manager->InitProfileUserPrefs(profile.get());
  EXPECT_TRUE(pref_service->GetBoolean(kEnableMediaRouterOnRestart));

  pref_service->RemoveUserPref(kEnableMediaRouterOnRestart);
  EXPECT_TRUE(pref_service->FindPreference(kEnableMediaRouterOnRestart)
                  ->IsDefaultValue());
  pref_service->SetBoolean(::prefs::kEnableMediaRouter, false);
  profile_manager->InitProfileUserPrefs(profile.get());
  EXPECT_FALSE(pref_service->GetBoolean(kEnableMediaRouterOnRestart));
}

TEST_F(BraveProfileManagerTest, EnableMediaRouterOnRestartNonDefaultValue) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);

  base::FilePath path = GetTempPath().AppendASCII("profile");
  TestingProfile::Builder builder;
  builder.SetPath(path);
  builder.SetIsNewProfile(true);
  std::unique_ptr<TestingProfile> profile = builder.Build();

  auto* pref_service = profile->GetTestingPrefService();

  EXPECT_FALSE(pref_service->FindPreference(kEnableMediaRouterOnRestart)
                   ->IsDefaultValue());
  pref_service->SetBoolean(kEnableMediaRouterOnRestart, true);
  pref_service->SetBoolean(::prefs::kEnableMediaRouter, false);
  profile_manager->InitProfileUserPrefs(profile.get());
  EXPECT_TRUE(pref_service->GetBoolean(kEnableMediaRouterOnRestart));

  EXPECT_FALSE(pref_service->FindPreference(kEnableMediaRouterOnRestart)
                   ->IsDefaultValue());
  pref_service->SetBoolean(kEnableMediaRouterOnRestart, false);
  pref_service->SetBoolean(::prefs::kEnableMediaRouter, true);
  profile_manager->InitProfileUserPrefs(profile.get());
  EXPECT_FALSE(pref_service->GetBoolean(kEnableMediaRouterOnRestart));
}
