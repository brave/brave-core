/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/profile_attributes_entry.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/values.h"
#include "brave/grit/brave_generated_resources.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/grit/theme_resources.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_unittest_util.h"

#if !BUILDFLAG(IS_ANDROID)

class ProfileAttributeMigrationTest : public testing::Test {
 public:
  ProfileAttributeMigrationTest() = default;
  ~ProfileAttributeMigrationTest() override = default;

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

  base::FilePath GetProfilePath(const std::string& base_name) {
    return profile_manager_->profile_manager()->user_data_dir().AppendASCII(
        base_name);
  }

  ProfileAttributesStorage& storage() {
    return g_browser_process->profile_manager()->GetProfileAttributesStorage();
  }

  ProfileAttributesEntry* entry() {
    return storage().GetProfileAttributesWithPath(
        GetProfilePath("TestProfile"));
  }

  void RunMigration(ProfileAttributesEntry* entry) {
    entry->MigrateObsoleteProfileAttributes();
  }

  Profile* profile() { return profile_; }

 private:
  content::BrowserTaskEnvironment task_environment_;
  raw_ptr<Profile> profile_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
};

TEST_F(ProfileAttributeMigrationTest,
       MigrateObsoleteProfileAttributes_LegacyAvatarIcon) {
  // get the entry and set an invalid value (legacy icon id).
  auto* profile_attribute_entry = entry();
  profile_attribute_entry->SetAvatarIconIndex(28);

  // migration should change this to default icon.
  RunMigration(profile_attribute_entry);
  EXPECT_EQ(profile_attribute_entry->GetAvatarIconIndex(), 26u);
}

TEST_F(ProfileAttributeMigrationTest,
       MigrateObsoleteProfileAttributes_DefaultAvatarIcon) {
  // get the entry and set it to default
  auto* profile_attribute_entry = entry();
  profile_attribute_entry->SetAvatarIconIndex(26);

  // migration should not change this one.
  RunMigration(profile_attribute_entry);
  EXPECT_EQ(profile_attribute_entry->GetAvatarIconIndex(), 26u);
}

TEST_F(ProfileAttributeMigrationTest,
       MigrateObsoleteProfileAttributes_BraveAvatarIcon) {
  // get the entry and set it to default
  auto* profile_attribute_entry = entry();
  profile_attribute_entry->SetAvatarIconIndex(56);

  // migration should not change this one.
  RunMigration(profile_attribute_entry);
  EXPECT_EQ(profile_attribute_entry->GetAvatarIconIndex(), 56u);
}

// Tests for the Brave custom-uploaded profile avatar API.
class BraveCustomAvatarTest : public ProfileAttributeMigrationTest {};

// Initially no custom avatar is set.
TEST_F(BraveCustomAvatarTest, NoCustomAvatarByDefault) {
  ProfileAttributesEntry* e = entry();
  EXPECT_FALSE(e->HasBraveCustomAvatar());
  EXPECT_FALSE(e->IsUsingBraveCustomAvatar());
  EXPECT_EQ(nullptr, e->GetBraveCustomAvatar());
}

// Setting a non-empty image flips the flag and makes the image available
// via `GetBraveCustomAvatar()` (synchronously, thanks to the in-memory
// cache populated by `SaveGAIAImageAtPath`).
TEST_F(BraveCustomAvatarTest, SetMakesCustomAvatarAvailable) {
  ProfileAttributesEntry* e = entry();
  gfx::Image image = gfx::test::CreateImage(64, 64);

  bool save_callback_called = false;
  bool save_callback_success = false;
  e->SetBraveCustomAvatar(image, base::BindLambdaForTesting([&](bool ok) {
                            save_callback_called = true;
                            save_callback_success = ok;
                          }));

  EXPECT_TRUE(save_callback_called);
  EXPECT_TRUE(save_callback_success);
  EXPECT_TRUE(e->HasBraveCustomAvatar());
  EXPECT_TRUE(e->IsUsingBraveCustomAvatar());
  const gfx::Image* fetched = e->GetBraveCustomAvatar();
  ASSERT_NE(nullptr, fetched);
  EXPECT_TRUE(gfx::test::AreImagesEqual(image, *fetched));
}

// When a custom avatar is set, `GetAvatarIconWithType` short-circuits to
// it (returning a non-placeholder result) regardless of whether a GAIA or
// default avatar would otherwise win.
TEST_F(BraveCustomAvatarTest, GetAvatarIconWithTypeReturnsCustom) {
  ProfileAttributesEntry* e = entry();

  // Make sure a regular default avatar is configured first - otherwise the
  // upstream code path is exercised correctly.
  e->SetAvatarIconIndex(profiles::GetPlaceholderAvatarIndex());

  gfx::Image image = gfx::test::CreateImage(72, 72);
  e->SetBraveCustomAvatar(image, base::OnceCallback<void(bool)>());

  auto [resolved_image, icon_type] = e->GetAvatarIconWithType(
      /*size_for_placeholder_avatar=*/72,
      /*use_high_res_file=*/true, profiles::PlaceholderAvatarIconParams{});
  EXPECT_EQ(AvatarIconType::kNonPlaceholder, icon_type);
  EXPECT_TRUE(gfx::test::AreImagesEqual(image, resolved_image));
}

// Setting an empty image is equivalent to clearing the custom avatar and
// surfaces a failure to the caller's save callback.
TEST_F(BraveCustomAvatarTest, SetWithEmptyImageClears) {
  ProfileAttributesEntry* e = entry();
  e->SetBraveCustomAvatar(gfx::test::CreateImage(8, 8),
                          base::OnceCallback<void(bool)>());
  ASSERT_TRUE(e->HasBraveCustomAvatar());
  ASSERT_TRUE(e->IsUsingBraveCustomAvatar());

  bool save_callback_called = false;
  bool save_callback_success = true;
  e->SetBraveCustomAvatar(gfx::Image(),
                          base::BindLambdaForTesting([&](bool ok) {
                            save_callback_called = true;
                            save_callback_success = ok;
                          }));
  EXPECT_TRUE(save_callback_called);
  EXPECT_FALSE(save_callback_success);
  EXPECT_FALSE(e->HasBraveCustomAvatar());
  EXPECT_FALSE(e->IsUsingBraveCustomAvatar());
}

// Clearing wipes the flag, removes the file from disk on a background
// thread, and is a no-op when nothing is currently set.
TEST_F(BraveCustomAvatarTest, ClearRemovesFlagAndFile) {
  ProfileAttributesEntry* e = entry();

  // No-op when nothing is set.
  e->ClearBraveCustomAvatar();
  EXPECT_FALSE(e->HasBraveCustomAvatar());
  EXPECT_FALSE(e->IsUsingBraveCustomAvatar());

  e->SetBraveCustomAvatar(gfx::test::CreateImage(16, 16),
                          base::OnceCallback<void(bool)>());
  ASSERT_TRUE(e->HasBraveCustomAvatar());
  ASSERT_TRUE(e->IsUsingBraveCustomAvatar());

  // Wait for the background PNG write so we can observe the file getting
  // deleted afterwards.
  content::RunAllTasksUntilIdle();
  const base::FilePath image_path =
      GetProfilePath("TestProfile").AppendASCII("Brave Custom Avatar.png");
  ASSERT_TRUE(base::PathExists(image_path));

  e->ClearBraveCustomAvatar();
  EXPECT_FALSE(e->HasBraveCustomAvatar());
  EXPECT_FALSE(e->IsUsingBraveCustomAvatar());

  content::RunAllTasksUntilIdle();
  EXPECT_FALSE(base::PathExists(image_path));
}

// Deactivation keeps the file and bitmap available but stops using custom
// for `IsUsingBraveCustomAvatar` / avatar resolution.
TEST_F(BraveCustomAvatarTest, DeactivateKeepsFileActivateRestores) {
  ProfileAttributesEntry* e = entry();
  gfx::Image image = gfx::test::CreateImage(32, 32);
  e->SetBraveCustomAvatar(image, base::OnceCallback<void(bool)>());
  content::RunAllTasksUntilIdle();
  ASSERT_TRUE(e->HasBraveCustomAvatar());
  ASSERT_TRUE(e->IsUsingBraveCustomAvatar());

  e->DeactivateBraveCustomAvatar();
  EXPECT_TRUE(e->HasBraveCustomAvatar());
  EXPECT_FALSE(e->IsUsingBraveCustomAvatar());
  const gfx::Image* after_deactivate = e->GetBraveCustomAvatar();
  ASSERT_NE(nullptr, after_deactivate);

  e->ActivateBraveCustomAvatar();
  EXPECT_TRUE(e->HasBraveCustomAvatar());
  EXPECT_TRUE(e->IsUsingBraveCustomAvatar());
}

#endif  // !BUILDFLAG(IS_ANDROID)
