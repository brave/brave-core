/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/profile_avatar_icon_util.h"

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/grit/brave_generated_resources.h"
#include "build/build_config.h"
#include "chrome/grit/theme_resources.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

TEST(ProfileUtilTest, KeepChromiumChoice) {
  // Test that a legacy chromium avatar choice will
  // still display the chromium avatar
  size_t chromium_avatar_index = 42;
  int expected = IDR_PROFILE_AVATAR_42;
  int actual = profiles::GetDefaultAvatarIconResourceIDAtIndex(
      chromium_avatar_index);
  EXPECT_EQ(actual, expected);
}

TEST(ProfileUtilTest, BraveAvatarIconChoices) {
  // Test that the avatar icon choices presented to the user are brave's.
  base::Value::List avatars = profiles::GetCustomProfileAvatarIconsAndLabels(0);

  const size_t expected_selectable_avatar_count =
      profiles::kBraveDefaultAvatarIconsCount;
  const size_t actual_selectable_avatar_count = avatars.size();

  // Avatars are Brave's, not Chromium's
  EXPECT_EQ(actual_selectable_avatar_count,
      expected_selectable_avatar_count);
}

TEST(ProfileUtilTest, RandomIconNeverFirstIcon) {
  // Test that for Brave, a call to get a random
  // avatar icon will *never* get the placeholder icon.

  // Unfortunately this uses an implementation detail that may change,
  // and if so this test will need updating.
  // It checks that the 'ModernAvatarIconStartIndex' is greater than
  // the placeholder index (which for brave is always the first item).
  size_t placeholder_index = profiles::GetPlaceholderAvatarIndex();
  size_t random_start_index = profiles::GetModernAvatarIconStartIndex();
  EXPECT_GT(random_start_index, placeholder_index);
}

#if !BUILDFLAG(IS_ANDROID)
class ProfileAvatarSelectorTest : public testing::Test {
 public:
  ProfileAvatarSelectorTest() = default;
  ~ProfileAvatarSelectorTest() override = default;

  void SetUp() override {
    TestingBrowserProcess* browser_process = TestingBrowserProcess::GetGlobal();
    profile_manager_ = std::make_unique<TestingProfileManager>(browser_process);
    ASSERT_TRUE(profile_manager_->SetUp());
    profile_ = profile_manager_->CreateTestingProfile("TestProfile");
  }

  void TearDown() override {
    profile_ = nullptr;
    profile_manager_->DeleteTestingProfile("TestProfile");
  }

  Profile* profile() { return profile_; }

 private:
  content::BrowserTaskEnvironment task_environment_;
  raw_ptr<Profile> profile_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
};

TEST_F(ProfileAvatarSelectorTest, ProfileAvatarSelectorPlaceholder) {
  // Test that the default avatar presented to the user in the profile
  // customiztion UI is Brave's.
  base::Value::List avatars =
      profiles::GetIconsAndLabelsForProfileAvatarSelector(profile()->GetPath());
  const base::Value::Dict* default_avatar = avatars[0].GetIfDict();
  EXPECT_NE(nullptr, default_avatar);

  const std::string* label = default_avatar->FindString("label");
  EXPECT_NE(nullptr, label);

  const std::string expected_label =
      ui::ResourceBundle::GetSharedInstance().LoadLocalizedResourceString(
          IDS_BRAVE_AVATAR_LABEL_PLACEHOLDER);

  EXPECT_EQ(expected_label, *label);
}
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace
