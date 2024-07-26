/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/profile_attributes_entry.h"

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/grit/brave_generated_resources.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/grit/theme_resources.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/resource/resource_bundle.h"

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

#endif  // !BUILDFLAG(IS_ANDROID)
