// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <string>
#include <utility>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/strings/string_util.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_path_override.h"
#include "base/test/test_future.h"
#include "brave/browser/profiles/custom_profile_image/profile_image.h"
#include "brave/browser/ui/webui/custom_profile_image/features.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_init_params.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_shortcut_manager.h"
#include "chrome/browser/profiles/profile_shortcut_manager_win.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace custom_profile_image {
namespace {

// Windows writes the avatar into a badged icon in the profile directory, a
// copy of the image that ImageFile does not manage.
class CustomProfileImageWinIconTest : public testing::Test {
 protected:
  CustomProfileImageWinIconTest()
      : manager_(TestingBrowserProcess::GetGlobal()),
        fake_user_desktop_(base::DIR_USER_DESKTOP),
        fake_system_desktop_(base::DIR_COMMON_DESKTOP) {
    features_.InitAndEnableFeature(features::kBraveCustomProfileImage);
  }

  void SetUp() override {
    ProfileShortcutManagerWin::DisableUnpinningForTests();
    ProfileShortcutManagerWin::DisableOutOfProcessShortcutOpsForTests();
    ASSERT_TRUE(manager_.SetUp());
    // The icon is badged with the avatar only when there are two profiles.
    path_ = AddProfile(u"Custom");
    AddProfile(u"Other");
    // Created after the profiles so this is the only icon write in flight;
    // writes are not ordered with each other.
    shortcuts_ = ProfileShortcutManager::Create(manager_.profile_manager());
    shortcuts_->CreateOrUpdateProfileIcon(path_);
    ASSERT_TRUE(base::test::RunUntil([&] { return !ReadIcon().empty(); }));
  }

  base::FilePath AddProfile(const std::u16string& name) {
    const base::FilePath path =
        manager_.profiles_dir().Append(base::AsWString(name));
    EXPECT_TRUE(base::CreateDirectory(path));
    ProfileAttributesInitParams params;
    params.profile_path = path;
    params.profile_name = name;
    storage().AddProfile(std::move(params));
    return path;
  }

  ProfileAttributesStorage& storage() {
    return *manager_.profile_attributes_storage();
  }
  ProfileImage& image() {
    return ProfileImage::Get(*storage().GetProfileAttributesWithPath(path_));
  }

  // Icon files are written atomically, so a read sees a whole icon or none.
  std::string ReadIcon() {
    std::string icon;
    base::ReadFileToString(profiles::internal::GetProfileIconPath(path_),
                           &icon);
    return icon;
  }

  base::test::ScopedFeatureList features_;
  content::BrowserTaskEnvironment task_environment_;
  TestingProfileManager manager_;
  base::ScopedPathOverride fake_user_desktop_;
  base::ScopedPathOverride fake_system_desktop_;
  std::unique_ptr<ProfileShortcutManager> shortcuts_;
  base::FilePath path_;
};

TEST_F(CustomProfileImageWinIconTest, IconFollowsSaveAndRemove) {
  const std::string preset_icon = ReadIcon();

  SkBitmap bitmap;
  bitmap.allocN32Pixels(64, 64);
  bitmap.eraseColor(SK_ColorRED);
  base::test::TestFuture<Result> saved;
  image().Replace(bitmap, saved.GetCallback());
  ASSERT_EQ(saved.Get(), Result::kSuccess);
  EXPECT_TRUE(base::test::RunUntil([&] { return ReadIcon() != preset_icon; }));

  base::test::TestFuture<Result> removed;
  image().Remove(removed.GetCallback());
  ASSERT_EQ(removed.Get(), Result::kSuccess);
  EXPECT_TRUE(base::test::RunUntil([&] { return ReadIcon() == preset_icon; }));
}

}  // namespace
}  // namespace custom_profile_image
