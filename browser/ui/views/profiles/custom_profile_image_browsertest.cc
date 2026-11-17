// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>

#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/profiles/custom_profile_image/image_file.h"
#include "brave/browser/profiles/custom_profile_image/profile_image.h"
#include "brave/browser/ui/webui/custom_profile_image/features.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/delete_profile_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_metrics.h"
#include "chrome/browser/profiles/profile_test_util.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/profiles/avatar_toolbar_button.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/image/image_skia_rep.h"

namespace custom_profile_image {

class CustomProfileImageBrowserTest : public InProcessBrowserTest {
 protected:
  CustomProfileImageBrowserTest() {
    features_.InitAndEnableFeature(features::kBraveCustomProfileImage);
  }

  void SetUpOnMainThread() override {
    auto* manager = g_browser_process->profile_manager();
    profiles::testing::CreateProfileSync(
        manager, manager->GenerateNextProfileDirectoryPath());
  }

  ProfileAttributesEntry& entry() {
    return *g_browser_process->profile_manager()
                ->GetProfileAttributesStorage()
                .GetProfileAttributesWithPath(
                    browser()->GetProfile()->GetPath());
  }

  ProfileAttributesEntry* OtherEntry() {
    auto entries = g_browser_process->profile_manager()
                       ->GetProfileAttributesStorage()
                       .GetAllProfilesAttributes();
    const auto it = std::ranges::find_if(entries, [&](auto* candidate) {
      return candidate->GetPath() != entry().GetPath();
    });
    return it == entries.end() ? nullptr : *it;
  }

  AvatarToolbarButton* button() {
    return static_cast<AvatarToolbarButton*>(
        BrowserView::GetBrowserViewForBrowser(browser())
            ->toolbar_button_provider()
            ->GetAvatarToolbarButtonInterface());
  }

  SkColor ToolbarColor() {
    const auto image = button()->GetImage(views::Button::STATE_NORMAL);
    const auto bitmap = image.GetRepresentation(1.0f).GetBitmap();
    return bitmap.drawsNothing()
               ? SK_ColorTRANSPARENT
               : bitmap.getColor(bitmap.width() / 2, bitmap.height() / 2);
  }

  void Save(SkColor color,
            ProfileAttributesEntry* target = nullptr,
            const base::Location& location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    SkBitmap bitmap;
    bitmap.allocN32Pixels(64, 64);
    bitmap.eraseColor(color);
    base::test::TestFuture<Result> saved;
    ProfileImage::Get(target ? *target : entry())
        .Replace(bitmap, saved.GetCallback());
    ASSERT_EQ(saved.Get(), Result::kSuccess);
  }

  base::test::ScopedFeatureList features_;
};

IN_PROC_BROWSER_TEST_F(CustomProfileImageBrowserTest, ToolbarUpdatesOnReplace) {
  ASSERT_TRUE(button());
  ASSERT_NO_FATAL_FAILURE(Save(SK_ColorRED));
  ASSERT_TRUE(
      base::test::RunUntil([&] { return ToolbarColor() == SK_ColorRED; }));
  ASSERT_NO_FATAL_FAILURE(Save(SK_ColorBLUE));
  ASSERT_TRUE(
      base::test::RunUntil([&] { return ToolbarColor() == SK_ColorBLUE; }));
  profiles::SetDefaultProfileAvatarIndex(browser()->GetProfile(),
                                         entry().GetAvatarIconIndex());
  ASSERT_TRUE(
      base::test::RunUntil([&] { return ToolbarColor() != SK_ColorBLUE; }));
}

// The image goes with the profile directory, even with a write pending. A
// loaded profile stays loaded after deletion, so its directory is removed at
// shutdown, after pending writes finish.
IN_PROC_BROWSER_TEST_F(CustomProfileImageBrowserTest,
                       PRE_ProfileDeletionLeavesNoImage) {
  auto* manager = g_browser_process->profile_manager();
  ProfileAttributesEntry* other = OtherEntry();
  ASSERT_TRUE(other);
  const auto path = other->GetPath();
  ASSERT_NO_FATAL_FAILURE(Save(SK_ColorRED, other));
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(base::PathExists(path.AppendASCII(kImageFileName)));
  }
  SkBitmap bitmap;
  bitmap.allocN32Pixels(64, 64);
  bitmap.eraseColor(SK_ColorBLUE);
  ProfileImage::Get(*other).Replace(bitmap, base::DoNothing());
  manager->GetDeleteProfileHelper().MaybeScheduleProfileForDeletion(
      path, base::DoNothing(), ProfileMetrics::DELETE_PROFILE_SETTINGS);
  ASSERT_TRUE(base::test::RunUntil([&] {
    return !manager->GetProfileAttributesStorage().GetProfileAttributesWithPath(
        path);
  }));
}

IN_PROC_BROWSER_TEST_F(CustomProfileImageBrowserTest,
                       ProfileDeletionLeavesNoImage) {
  const base::FilePath user_data_dir =
      g_browser_process->profile_manager()->user_data_dir();
  ASSERT_TRUE(base::test::RunUntil([&] {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::FileEnumerator profiles(user_data_dir, /*recursive=*/false,
                                  base::FileEnumerator::DIRECTORIES);
    for (auto dir = profiles.Next(); !dir.empty(); dir = profiles.Next()) {
      if (base::PathExists(dir.AppendASCII(kImageFileName))) {
        return false;
      }
    }
    return true;
  }));
}

}  // namespace custom_profile_image
