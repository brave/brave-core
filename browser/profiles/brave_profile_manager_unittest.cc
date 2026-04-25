// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/profiles/brave_profile_manager.h"

#include <memory>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/constants/brave_constants.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/test_browser_window.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/features.h"
#endif

namespace {

// ProfileManager that provides TestingProfile so that
// creation doesn't crash due to prefs, etc.
// (ref. chrome/test/base/fake_profile_manager.cc)
class TestingBraveProfileManager : public BraveProfileManagerWithoutInit {
 public:
  explicit TestingBraveProfileManager(const base::FilePath& user_data_dir)
      : BraveProfileManagerWithoutInit(user_data_dir) {}

  ~TestingBraveProfileManager() override = default;

  std::unique_ptr<TestingProfile> BuildTestingProfile(
      const base::FilePath& path,
      Delegate* delegate,
      Profile::CreateMode create_mode) {
    return std::make_unique<TestingProfile>(path, delegate, create_mode);
  }

  std::unique_ptr<Profile> CreateProfileHelper(
      const base::FilePath& path) final {
    if (!base::PathExists(path) && !base::CreateDirectory(path)) {
      return nullptr;
    }
    return BuildTestingProfile(path, this, Profile::CreateMode::kSynchronous);
  }

  std::unique_ptr<Profile> CreateProfileAsyncHelper(
      const base::FilePath& path) final {
    // SingleThreadTaskRunner::GetCurrentDefault() is TestingProfile's "async"
    // IOTaskRunner (ref. TestingProfile::GetIOTaskRunner()).
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(base::IgnoreResult(&base::CreateDirectory), path));

    return BuildTestingProfile(path, this, Profile::CreateMode::kAsynchronous);
  }
};

}  // namespace

class BraveProfileManagerTest : public testing::Test {
 public:
  BraveProfileManagerTest() = default;

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
    return std::make_unique<TestingBraveProfileManager>(temp_dir_.GetPath());
  }

  base::FilePath GetTempPath() { return temp_dir_.GetPath(); }

 private:
  base::ScopedTempDir temp_dir_;
  content::BrowserTaskEnvironment task_environment_;
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

#if BUILDFLAG(ENABLE_BRAVE_AI_CHAT_AGENT_PROFILE)

// Parameterized test class for AI Chat Agent profile functionality
class BraveProfileManagerAIAgentProfileTest
    : public BraveProfileManagerTest,
      public testing::WithParamInterface<bool> {
 protected:
  void SetUp() override {
    BraveProfileManagerTest::SetUp();

    // Set up feature flag based on parameter
    if (IsAIChatAgentProfileFeatureEnabled()) {
      scoped_feature_list_.InitAndEnableFeature(
          ai_chat::features::kAIChatAgentProfile);
    } else {
      scoped_feature_list_.InitAndDisableFeature(
          ai_chat::features::kAIChatAgentProfile);
    }
  }

  void TearDown() override {
    BraveProfileManagerTest::TearDown();
    scoped_feature_list_.Reset();
  }

  bool IsAIChatAgentProfileFeatureEnabled() const { return GetParam(); }

  Profile* CreateAIChatAgentProfile() {
    ProfileAttributesStorage& storage =
        g_browser_process->profile_manager()->GetProfileAttributesStorage();
    size_t num_profiles = storage.GetNumberOfProfiles();
    base::FilePath path = GetTempPath().Append(brave::kAIChatAgentProfileDir);
    ProfileAttributesInitParams params;
    params.profile_path = path;
    params.profile_name = u"Testing AI Chat Agent Profile";
    storage.AddProfile(std::move(params));
    EXPECT_EQ(num_profiles + 1u, storage.GetNumberOfProfiles());
    auto* profile = g_browser_process->profile_manager()->GetProfile(path);
    EXPECT_TRUE(profile);
    return profile;
  }

  Profile* CreateRegularProfile(const std::string& name) {
    ProfileAttributesStorage& storage =
        g_browser_process->profile_manager()->GetProfileAttributesStorage();
    size_t num_profiles = storage.GetNumberOfProfiles();
    base::FilePath path = GetTempPath().AppendASCII(name);
    ProfileAttributesInitParams params;
    params.profile_path = path;
    params.profile_name = base::UTF8ToUTF16(name);
    storage.AddProfile(std::move(params));
    EXPECT_EQ(num_profiles + 1u, storage.GetNumberOfProfiles());
    return g_browser_process->profile_manager()->GetProfile(path);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_P(BraveProfileManagerAIAgentProfileTest, SetProfileAsLastUsed) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);

  // Create a regular profile
  auto* regular_profile = CreateRegularProfile("regular_profile");
  base::FilePath regular_path = regular_profile->GetPath();
  ASSERT_FALSE(regular_profile->IsAIChatAgent());

  // Create an AI Chat Agent profile. We create it for both when the feature is
  // enabled and when it is disabled because if the feature flag is disabled,
  // the profile will still exist and be useable as a regular profile.
  auto* ai_chat_profile = CreateAIChatAgentProfile();
  base::FilePath ai_chat_path = ai_chat_profile->GetPath();

  // When feature is enabled, this should be an AI Chat Agent
  // When feature is disabled, this should NOT be an AI Chat Agent
  if (IsAIChatAgentProfileFeatureEnabled()) {
    ASSERT_TRUE(ai_chat_profile->IsAIChatAgent());
  } else {
    ASSERT_FALSE(ai_chat_profile->IsAIChatAgent());
  }

  ProfileAttributesStorage* storage =
      &profile_manager->GetProfileAttributesStorage();

  // Test regular profile - should always update last used time
  base::Time before_regular = base::Time::Now();
  profile_manager->SetProfileAsLastUsed(regular_profile);

  ProfileAttributesEntry* regular_entry =
      storage->GetProfileAttributesWithPath(regular_path);
  ASSERT_TRUE(regular_entry);
  EXPECT_GE(regular_entry->GetActiveTime(), before_regular);

  // Test AI Chat Agent profile behavior depends on feature flag
  ProfileAttributesEntry* ai_chat_entry =
      storage->GetProfileAttributesWithPath(ai_chat_path);
  ASSERT_TRUE(ai_chat_entry);
  base::Time ai_chat_time_before = ai_chat_entry->GetActiveTime();

  profile_manager->SetProfileAsLastUsed(ai_chat_profile);

  if (IsAIChatAgentProfileFeatureEnabled()) {
    // When feature is enabled, AI Chat Agent profile should NOT update last
    // used time
    EXPECT_EQ(ai_chat_entry->GetActiveTime(), ai_chat_time_before);
  } else {
    // When feature is disabled, it should behave like a regular profile
    EXPECT_GE(ai_chat_entry->GetActiveTime(), ai_chat_time_before);
  }
}

TEST_P(BraveProfileManagerAIAgentProfileTest, GetNumberOfProfiles) {
  BraveProfileManager* profile_manager =
      static_cast<BraveProfileManager*>(g_browser_process->profile_manager());
  ASSERT_TRUE(profile_manager);

  // Initially should have 0 profiles
  EXPECT_EQ(0u, profile_manager->GetNumberOfProfiles());

  // Add a regular profile
  CreateRegularProfile("test_profile");
  EXPECT_EQ(1u, profile_manager->GetNumberOfProfiles());

  // Create an AI Chat Agent profile. We create it for both when the feature is
  // enabled and when it is disabled because if the feature flag is disabled,
  // the profile will still exist and be useable as a regular profile.
  CreateAIChatAgentProfile();

  if (IsAIChatAgentProfileFeatureEnabled()) {
    // When feature is enabled, AI Chat Agent profile should NOT be counted
    EXPECT_EQ(1u, profile_manager->GetNumberOfProfiles());
  } else {
    // When feature is disabled, it should be counted as a regular profile
    EXPECT_EQ(2u, profile_manager->GetNumberOfProfiles());
  }

  // Add another regular profile
  CreateRegularProfile("test_profile2");

  if (IsAIChatAgentProfileFeatureEnabled()) {
    EXPECT_EQ(2u, profile_manager->GetNumberOfProfiles());
  } else {
    EXPECT_EQ(3u, profile_manager->GetNumberOfProfiles());
  }
}

TEST_P(BraveProfileManagerAIAgentProfileTest, GetLastOpenedProfiles) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);

  // Create profiles
  auto* regular_profile1 = CreateRegularProfile("regular1");
  auto* regular_profile2 = CreateRegularProfile("regular2");
  auto* ai_chat_profile = CreateAIChatAgentProfile();

  base::FilePath regular1_path = regular_profile1->GetPath();
  base::FilePath regular2_path = regular_profile2->GetPath();
  base::FilePath ai_chat_path = ai_chat_profile->GetPath();

  std::vector<Profile*> last_opened_profiles =
      profile_manager->GetLastOpenedProfiles();
  ASSERT_EQ(0U, last_opened_profiles.size());

  // Simulate opening profiles by setting them as active in the same way
  // as profile_manager_unittest.cc
  Browser::CreateParams profile1_params(regular_profile1, true);
  std::unique_ptr<Browser> browser1(
      CreateBrowserWithTestWindowForParams(profile1_params));

  last_opened_profiles = profile_manager->GetLastOpenedProfiles();
  ASSERT_EQ(1U, last_opened_profiles.size());
  EXPECT_TRUE(profile_manager->has_updated_last_opened_profiles());
  EXPECT_EQ(regular_profile1, last_opened_profiles[0]);

  // And for ai chat profile
  Browser::CreateParams ai_chat_params(ai_chat_profile, true);
  std::unique_ptr<Browser> browser_ai_chat(
      CreateBrowserWithTestWindowForParams(ai_chat_params));

  last_opened_profiles = profile_manager->GetLastOpenedProfiles();
  if (IsAIChatAgentProfileFeatureEnabled()) {
    ASSERT_EQ(1U, last_opened_profiles.size());
    EXPECT_EQ(regular_profile1, last_opened_profiles[0]);
  } else {
    ASSERT_EQ(2U, last_opened_profiles.size());
    EXPECT_EQ(regular_profile1, last_opened_profiles[0]);
    EXPECT_EQ(ai_chat_profile, last_opened_profiles[1]);
  }

  // And for profile2
  Browser::CreateParams profile2_params(regular_profile2, true);
  std::unique_ptr<Browser> browser2(
      CreateBrowserWithTestWindowForParams(profile2_params));

  last_opened_profiles = profile_manager->GetLastOpenedProfiles();
  if (IsAIChatAgentProfileFeatureEnabled()) {
    ASSERT_EQ(2U, last_opened_profiles.size());
    EXPECT_EQ(regular_profile1, last_opened_profiles[0]);
    EXPECT_EQ(regular_profile2, last_opened_profiles[1]);
  } else {
    ASSERT_EQ(3U, last_opened_profiles.size());
    EXPECT_EQ(regular_profile1, last_opened_profiles[0]);
    EXPECT_EQ(ai_chat_profile, last_opened_profiles[1]);
    EXPECT_EQ(regular_profile2, last_opened_profiles[2]);
  }

  // Test what happens when only the AI Chat Agent profile is left
  browser1.reset();
  EXPECT_EQ(IsAIChatAgentProfileFeatureEnabled() ? 1U : 2U,
            profile_manager->GetLastOpenedProfiles().size());
  browser2.reset();
  EXPECT_EQ(IsAIChatAgentProfileFeatureEnabled() ? 0U : 1U,
            profile_manager->GetLastOpenedProfiles().size());
  browser_ai_chat.reset();
  EXPECT_EQ(0U, profile_manager->GetLastOpenedProfiles().size());
}

INSTANTIATE_TEST_SUITE_P(FeatureEnabledAndDisabled,
                         BraveProfileManagerAIAgentProfileTest,
                         testing::Bool(),
                         [](const testing::TestParamInfo<bool>& info) {
                           return info.param ? "AIChatAgentProfileEnabled"
                                             : "AIChatAgentProfileDisabled";
                         });

#endif  // BUILDFLAG(ENABLE_BRAVE_AI_CHAT_AGENT_PROFILE)
