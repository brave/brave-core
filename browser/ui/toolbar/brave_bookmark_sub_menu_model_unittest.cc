/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <stddef.h>

#include <memory>
#include <string>
#include <utility>

#include "brave/browser/ui/toolbar/brave_bookmark_sub_menu_model.h"

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/browser/brave_local_state_prefs.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/test_browser_window.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;
using content::BrowserThread;

namespace {

class TestSimpleMenuDelegate : public ui::SimpleMenuModel::Delegate {
 public:
  TestSimpleMenuDelegate() = default;

  TestSimpleMenuDelegate(const TestSimpleMenuDelegate&) = delete;
  TestSimpleMenuDelegate& operator=(const TestSimpleMenuDelegate&) = delete;

  ~TestSimpleMenuDelegate() override = default;

  bool IsCommandIdChecked(int command_id) const override { return false; }

  bool IsCommandIdEnabled(int command_id) const override { return true; }

  void ExecuteCommand(int command_id, int event_flags) override {}
};

}  // namespace

class BraveBookmarkSubMenuModelUnitTest : public testing::Test {
 public:
  BraveBookmarkSubMenuModelUnitTest() {}

  void SetUp() override {
    TestingProfile::Builder builder;
    builder.AddTestingFactory(BookmarkModelFactory::GetInstance(),
                              BookmarkModelFactory::GetDefaultFactory());
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(prefs->registry());

    RegisterLocalState(test_local_state_.registry());
    TestingBrowserProcess::GetGlobal()->SetLocalState(&test_local_state_);
    builder.SetPrefService(std::move(prefs));
    profile_ = builder.Build();
    model_ = BookmarkModelFactory::GetForBrowserContext(profile_.get());
    bookmarks::test::WaitForBookmarkModelToLoad(model_);
  }

  ui::SimpleMenuModel::Delegate* delegate() { return &delegate_; }

  Browser* GetBrowser() {
    if (!browser_) {
      Browser::CreateParams params(profile_.get(), true);
      test_window_ = std::make_unique<TestBrowserWindow>();
      params.window = test_window_.get();
      browser_.reset(Browser::Create(params));
    }
    return browser_.get();
  }
  void TearDown() override {
    browser_.reset();
    profile_.reset();
    TestingBrowserProcess::GetGlobal()->SetLocalState(nullptr);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestSimpleMenuDelegate delegate_;
  std::unique_ptr<Browser> browser_;
  std::unique_ptr<TestBrowserWindow> test_window_;
  std::unique_ptr<TestingProfile> profile_;
  raw_ptr<BookmarkModel> model_ = nullptr;
  TestingPrefServiceSimple test_local_state_;
};

TEST_F(BraveBookmarkSubMenuModelUnitTest, Build) {
  BraveBookmarkSubMenuModel model(delegate(), GetBrowser());
  EXPECT_GT(model.GetItemCount(), 0u);
  EXPECT_FALSE(model.GetIndexOfCommandId(IDC_SHOW_BOOKMARK_BAR).has_value());
  EXPECT_FALSE(model.GetIndexOfCommandId(IDC_SHOW_BOOKMARK_BAR).has_value());
  EXPECT_FALSE(
      model.GetIndexOfCommandId(IDC_BRAVE_BOOKMARK_BAR_ALWAYS).has_value());
  EXPECT_FALSE(
      model.GetIndexOfCommandId(IDC_BRAVE_BOOKMARK_BAR_NEVER).has_value());
  EXPECT_FALSE(
      model.GetIndexOfCommandId(IDC_BRAVE_BOOKMARK_BAR_NTP).has_value());

  auto submenu_index =
      model.GetIndexOfCommandId(IDC_BRAVE_BOOKMARK_BAR_SUBMENU);
  EXPECT_TRUE(submenu_index.has_value());
  auto* submenu_model = model.GetSubmenuModelAt(submenu_index.value());
  ASSERT_TRUE(submenu_model);

  EXPECT_EQ(submenu_model->GetCommandIdAt(0), IDC_BRAVE_BOOKMARK_BAR_ALWAYS);
  EXPECT_EQ(submenu_model->GetCommandIdAt(1), IDC_BRAVE_BOOKMARK_BAR_NEVER);
  EXPECT_EQ(submenu_model->GetCommandIdAt(2), IDC_BRAVE_BOOKMARK_BAR_NTP);
  EXPECT_EQ(submenu_model->GetItemCount(), 3u);
}
