/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_bookmark_context_menu_controller.h"

#include <stddef.h>

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/browser/ui/bookmark/bookmark_helper.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/bookmarks/bookmark_merged_surface_service_factory.h"
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
#include "content/public/browser/page_navigator.h"
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
    builder.AddTestingFactory(
        BookmarkMergedSurfaceServiceFactory::GetInstance(),
        BookmarkMergedSurfaceServiceFactory::GetDefaultFactory());
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
      nullptr, nullptr, nullptr, profile_.get(),
      BookmarkLaunchLocation::kSidePanelContextMenu,
      model_->bookmark_bar_node(),
      std::vector<
          raw_ptr<const bookmarks::BookmarkNode, VectorExperimental>>());

  // Show apps command is not present by default.
  sync_preferences::TestingPrefServiceSyncable* prefs =
      profile_->GetTestingPrefService();
  EXPECT_FALSE(prefs->IsManagedPreference(
      bookmarks::prefs::kShowAppsShortcutInBookmarkBar));
  EXPECT_FALSE(controller.menu_model()
                   ->GetIndexOfCommandId(IDC_BOOKMARK_BAR_SHOW_APPS_SHORTCUT)
                   .has_value());

  // Disabling the shorcut by policy doesn't cause the command to be added.
  prefs->SetManagedPref(bookmarks::prefs::kShowAppsShortcutInBookmarkBar,
                        base::Value(false));
  EXPECT_FALSE(controller.menu_model()
                   ->GetIndexOfCommandId(IDC_BOOKMARK_BAR_SHOW_APPS_SHORTCUT)
                   .has_value());

  // And enabling the shortcut by policy doesn't cause the command to be added.
  prefs->SetManagedPref(bookmarks::prefs::kShowAppsShortcutInBookmarkBar,
                        base::Value(true));
  EXPECT_FALSE(controller.menu_model()
                   ->GetIndexOfCommandId(IDC_BOOKMARK_BAR_SHOW_APPS_SHORTCUT)
                   .has_value());

  // And enabling the shortcut by user doesn't cause the command to be added.
  prefs->RemoveManagedPref(bookmarks::prefs::kShowAppsShortcutInBookmarkBar);
  prefs->SetUserPref(bookmarks::prefs::kShowAppsShortcutInBookmarkBar,
                     base::Value(true));
  EXPECT_FALSE(controller.menu_model()
                   ->GetIndexOfCommandId(IDC_BOOKMARK_BAR_SHOW_APPS_SHORTCUT)
                   .has_value());
}

TEST_F(BraveBookmarkContextMenuControllerTest, AddBraveBookmarksSubmenu) {
  BraveBookmarkContextMenuController controller(
      nullptr, nullptr, nullptr, profile_.get(),
      BookmarkLaunchLocation::kSidePanelFolder, model_->bookmark_bar_node(),
      std::vector<
          raw_ptr<const bookmarks::BookmarkNode, VectorExperimental>>());
  EXPECT_FALSE(controller.menu_model()
                   ->GetIndexOfCommandId(IDC_BOOKMARK_BAR_ALWAYS_SHOW)
                   .has_value());

  EXPECT_FALSE(controller.menu_model()
                   ->GetIndexOfCommandId(IDC_BRAVE_BOOKMARK_BAR_ALWAYS)
                   .has_value());
  EXPECT_FALSE(controller.menu_model()
                   ->GetIndexOfCommandId(IDC_BRAVE_BOOKMARK_BAR_NEVER)
                   .has_value());
  EXPECT_FALSE(controller.menu_model()
                   ->GetIndexOfCommandId(IDC_BRAVE_BOOKMARK_BAR_NTP)
                   .has_value());

  auto submenu_index = controller.menu_model()->GetIndexOfCommandId(
      IDC_BRAVE_BOOKMARK_BAR_SUBMENU);
  EXPECT_TRUE(submenu_index.has_value());
  auto* submenu_model =
      controller.menu_model()->GetSubmenuModelAt(submenu_index.value());
  ASSERT_TRUE(submenu_model);
  EXPECT_EQ(submenu_model->GetCommandIdAt(0), IDC_BRAVE_BOOKMARK_BAR_ALWAYS);
  EXPECT_EQ(submenu_model->GetCommandIdAt(1), IDC_BRAVE_BOOKMARK_BAR_NEVER);
  EXPECT_EQ(submenu_model->GetCommandIdAt(2), IDC_BRAVE_BOOKMARK_BAR_NTP);

  EXPECT_TRUE(controller.IsCommandIdEnabled(IDC_BRAVE_BOOKMARK_BAR_ALWAYS));
  EXPECT_TRUE(controller.IsCommandIdEnabled(IDC_BRAVE_BOOKMARK_BAR_NEVER));
  EXPECT_TRUE(controller.IsCommandIdEnabled(IDC_BRAVE_BOOKMARK_BAR_NTP));

  EXPECT_TRUE(controller.IsCommandIdVisible(IDC_BRAVE_BOOKMARK_BAR_ALWAYS));
  EXPECT_TRUE(controller.IsCommandIdVisible(IDC_BRAVE_BOOKMARK_BAR_NEVER));
  EXPECT_TRUE(controller.IsCommandIdVisible(IDC_BRAVE_BOOKMARK_BAR_NTP));

  auto* bookmark_submenu_model = controller.GetBookmarkSubmenuModel();
  EXPECT_EQ(controller.GetLabelForCommandId(IDC_BRAVE_BOOKMARK_BAR_ALWAYS),
            bookmark_submenu_model->GetLabelForCommandId(
                IDC_BRAVE_BOOKMARK_BAR_ALWAYS));
  EXPECT_EQ(controller.GetLabelForCommandId(IDC_BRAVE_BOOKMARK_BAR_NEVER),
            bookmark_submenu_model->GetLabelForCommandId(
                IDC_BRAVE_BOOKMARK_BAR_NEVER));
  EXPECT_EQ(
      controller.GetLabelForCommandId(IDC_BRAVE_BOOKMARK_BAR_NTP),
      bookmark_submenu_model->GetLabelForCommandId(IDC_BRAVE_BOOKMARK_BAR_NTP));

  // Default state is NTP only.
  EXPECT_FALSE(controller.IsCommandIdChecked(IDC_BRAVE_BOOKMARK_BAR_ALWAYS));
  EXPECT_FALSE(controller.IsCommandIdChecked(IDC_BRAVE_BOOKMARK_BAR_NEVER));
  EXPECT_TRUE(controller.IsCommandIdChecked(IDC_BRAVE_BOOKMARK_BAR_NTP));

  // Set state as Always.
  brave::SetBookmarkState(brave::BookmarkBarState::kAlways,
                          profile_->GetPrefs());
  EXPECT_TRUE(controller.IsCommandIdChecked(IDC_BRAVE_BOOKMARK_BAR_ALWAYS));
  EXPECT_FALSE(controller.IsCommandIdChecked(IDC_BRAVE_BOOKMARK_BAR_NEVER));
  EXPECT_FALSE(controller.IsCommandIdChecked(IDC_BRAVE_BOOKMARK_BAR_NTP));

  // Set state as Never.
  brave::SetBookmarkState(brave::BookmarkBarState::kNever,
                          profile_->GetPrefs());
  EXPECT_FALSE(controller.IsCommandIdChecked(IDC_BRAVE_BOOKMARK_BAR_ALWAYS));
  EXPECT_TRUE(controller.IsCommandIdChecked(IDC_BRAVE_BOOKMARK_BAR_NEVER));
  EXPECT_FALSE(controller.IsCommandIdChecked(IDC_BRAVE_BOOKMARK_BAR_NTP));
}
