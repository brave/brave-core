/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stddef.h>

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/bookmarks/bookmark_context_menu_controller.h"
#include "chrome/browser/ui/bookmarks/bookmark_stats.h"
#include "chrome/browser/ui/bookmarks/bookmark_utils.h"
#include "chrome/browser/ui/bookmarks/bookmark_utils_desktop.h"
#include "chrome/test/base/testing_profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;
using content::BrowserThread;

class BraveBookmarkContextMenuControllerTest : public testing::Test {
 public:
  BraveBookmarkContextMenuControllerTest() : model_(nullptr) {}

  void SetUp() override {
    TestingProfile::Builder builder;
    builder.AddTestingFactory(BookmarkModelFactory::GetInstance(),
                              BookmarkModelFactory::GetDefaultFactory());
    profile_ = builder.Build();
    model_ = BookmarkModelFactory::GetForBrowserContext(profile_.get());
    bookmarks::test::WaitForBookmarkModelToLoad(model_);
  }

  static base::RepeatingCallback<content::PageNavigator*()>
  NullNavigatorGetter() {
    return base::BindRepeating(
        []() -> content::PageNavigator* { return nullptr; });
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  raw_ptr<BookmarkModel> model_ = nullptr;
};

TEST_F(BraveBookmarkContextMenuControllerTest,
       DontShowAppsShortcutContextMenuInBookmarksBar) {
  BookmarkContextMenuController controller(
      NULL, NULL, NULL, profile_.get(), NullNavigatorGetter(),
      BOOKMARK_LAUNCH_LOCATION_CONTEXT_MENU, model_->bookmark_bar_node(),
      std::vector<const BookmarkNode*>());

  // Show apps command is not present by default.
  sync_preferences::TestingPrefServiceSyncable* prefs =
      profile_->GetTestingPrefService();
  EXPECT_FALSE(prefs->IsManagedPreference(
      bookmarks::prefs::kShowAppsShortcutInBookmarkBar));
  EXPECT_EQ(controller.menu_model()->GetIndexOfCommandId(
                IDC_BOOKMARK_BAR_SHOW_APPS_SHORTCUT),
            -1);

  // Disabling the shorcut by policy doesn't cause the command to be added.
  prefs->SetManagedPref(bookmarks::prefs::kShowAppsShortcutInBookmarkBar,
                        std::make_unique<base::Value>(false));
  EXPECT_EQ(controller.menu_model()->GetIndexOfCommandId(
                IDC_BOOKMARK_BAR_SHOW_APPS_SHORTCUT),
            -1);

  // And enabling the shortcut by policy doesn't cause the command to be added.
  prefs->SetManagedPref(bookmarks::prefs::kShowAppsShortcutInBookmarkBar,
                        std::make_unique<base::Value>(true));
  EXPECT_EQ(controller.menu_model()->GetIndexOfCommandId(
                IDC_BOOKMARK_BAR_SHOW_APPS_SHORTCUT),
            -1);

  // And enabling the shortcut by user doesn't cause the command to be added.
  prefs->RemoveManagedPref(bookmarks::prefs::kShowAppsShortcutInBookmarkBar);
  prefs->SetUserPref(bookmarks::prefs::kShowAppsShortcutInBookmarkBar,
                 std::make_unique<base::Value>(true));
  EXPECT_EQ(controller.menu_model()->GetIndexOfCommandId(
                IDC_BOOKMARK_BAR_SHOW_APPS_SHORTCUT),
            -1);
}
