/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/bookmarks/brave_bookmark_context_menu.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/bookmark/brave_bookmark_prefs.h"
#include "brave/browser/ui/toolbar/brave_bookmark_context_menu_controller.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/bookmarks/bookmark_merged_surface_service_factory.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/bookmarks/managed_bookmark_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/controls/menu/submenu_view.h"
#include "ui/views/test/scoped_views_test_helper.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

class BraveBookmarkContextMenuTest : public testing::Test {
 public:
  BraveBookmarkContextMenuTest() : model_(nullptr) {}

  void SetUp() override {
    TestingProfile::Builder profile_builder;
    profile_builder.AddTestingFactory(
        BookmarkModelFactory::GetInstance(),
        BookmarkModelFactory::GetDefaultFactory());
    profile_builder.AddTestingFactory(
        ManagedBookmarkServiceFactory::GetInstance(),
        ManagedBookmarkServiceFactory::GetDefaultFactory());
    profile_builder.AddTestingFactory(
        BookmarkMergedSurfaceServiceFactory::GetInstance(),
        BookmarkMergedSurfaceServiceFactory::GetDefaultFactory());
    profile_ = profile_builder.Build();

    model_ = BookmarkModelFactory::GetForBrowserContext(profile_.get());
    prefs_ = std::make_unique<TestingPrefServiceSimple>();

    brave::bookmarks::prefs::RegisterProfilePrefs(prefs_->registry());

    bookmarks::test::WaitForBookmarkModelToLoad(model_);
  }

 protected:
  std::unique_ptr<BraveBookmarkContextMenu> CreateBookmarkContextMenu() {
    auto menu = std::make_unique<BraveBookmarkContextMenu>(
        nullptr, nullptr, profile_.get(), BookmarkLaunchLocation::kNone,
        nullptr,
        std::vector<
            raw_ptr<const bookmarks::BookmarkNode, VectorExperimental>>(),
        false);
    menu->GetControllerForTesting()->SetPrefsForTesting(prefs_.get());
    return menu;
  }

  content::BrowserTaskEnvironment task_environment_;
  views::ScopedViewsTestHelper views_test_helper_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<TestingPrefServiceSimple> prefs_;
  raw_ptr<BookmarkModel> model_;
};

TEST_F(BraveBookmarkContextMenuTest, OverrideBookmarkAlwaysShowMenu) {
  auto bookmark_context_menu = CreateBookmarkContextMenu();
  EXPECT_FALSE(bookmark_context_menu->menu()->GetMenuItemByID(
      IDC_BOOKMARK_BAR_ALWAYS_SHOW));

  auto* menu = bookmark_context_menu->menu()->GetMenuItemByID(
      IDC_BRAVE_BOOKMARK_BAR_SUBMENU);
  EXPECT_TRUE(menu);
  auto* submenu = menu->GetSubmenu();
  EXPECT_EQ(submenu->GetMenuItemAt(0)->GetCommand(),
            IDC_BRAVE_BOOKMARK_BAR_ALWAYS);
  EXPECT_EQ(submenu->GetMenuItemAt(1)->GetCommand(),
            IDC_BRAVE_BOOKMARK_BAR_NEVER);
  EXPECT_EQ(submenu->GetMenuItemAt(2)->GetCommand(),
            IDC_BRAVE_BOOKMARK_BAR_NTP);
  EXPECT_EQ(submenu->GetMenuItems().size(), 3u);
}

TEST_F(BraveBookmarkContextMenuTest, AddShowAllBookmarksButtonMenu) {
  auto bookmark_context_menu = CreateBookmarkContextMenu();
  auto* menu = bookmark_context_menu->menu()->GetMenuItemByID(
      IDC_TOGGLE_ALL_BOOKMARKS_BUTTON_VISIBILITY);
  EXPECT_TRUE(menu);

  EXPECT_TRUE(bookmark_context_menu->IsCommandEnabled(
      IDC_TOGGLE_ALL_BOOKMARKS_BUTTON_VISIBILITY));
  EXPECT_FALSE(bookmark_context_menu->IsCommandVisible(
      IDC_TOGGLE_ALL_BOOKMARKS_BUTTON_VISIBILITY));
  EXPECT_TRUE(bookmark_context_menu->IsItemChecked(
      IDC_TOGGLE_ALL_BOOKMARKS_BUTTON_VISIBILITY));
}

TEST_F(BraveBookmarkContextMenuTest, ShowAllBookmarksButtonMenuCheckedState) {
  auto bookmark_context_menu = CreateBookmarkContextMenu();
  ASSERT_TRUE(bookmark_context_menu->IsItemChecked(
      IDC_TOGGLE_ALL_BOOKMARKS_BUTTON_VISIBILITY));

  prefs_->SetBoolean(brave::bookmarks::prefs::kShowAllBookmarksButton, false);
  EXPECT_FALSE(bookmark_context_menu->IsItemChecked(
      IDC_TOGGLE_ALL_BOOKMARKS_BUTTON_VISIBILITY));

  prefs_->SetBoolean(brave::bookmarks::prefs::kShowAllBookmarksButton, true);
  EXPECT_TRUE(bookmark_context_menu->IsItemChecked(
      IDC_TOGGLE_ALL_BOOKMARKS_BUTTON_VISIBILITY));
}

TEST_F(BraveBookmarkContextMenuTest,
       ShowAllBookmarksButtonMenuVisibilityState) {
  auto bookmark_context_menu = CreateBookmarkContextMenu();
  EXPECT_FALSE(bookmark_context_menu->IsCommandVisible(
      IDC_TOGGLE_ALL_BOOKMARKS_BUTTON_VISIBILITY));

  std::u16string title = u"xyz";
  GURL url("https://www.xyz.com");
  const BookmarkNode* node =
      model_->AddNewURL(model_->other_node(), 0, title, url);
  EXPECT_TRUE(bookmark_context_menu->IsCommandVisible(
      IDC_TOGGLE_ALL_BOOKMARKS_BUTTON_VISIBILITY));

  model_->Remove(node, bookmarks::metrics::BookmarkEditSource::kOther,
                 FROM_HERE);
  EXPECT_FALSE(bookmark_context_menu->IsCommandVisible(
      IDC_TOGGLE_ALL_BOOKMARKS_BUTTON_VISIBILITY));
}
