/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_menu_model.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/models/simple_menu_model.h"

using BraveTabMenuModelTest = InProcessBrowserTest;

class Delegate : public ui::SimpleMenuModel::Delegate {
 public:
  Delegate() : execute_count_(0), enable_count_(0) {}

  bool IsCommandIdChecked(int command_id) const override {
    return false;
  }
  bool IsCommandIdEnabled(int command_id) const override {
    ++enable_count_;
    return true;
  }

  void ExecuteCommand(int command_id, int event_flags) override {
    ++execute_count_;
  }

  int execute_count_;
  mutable int enable_count_;
};

// Recursively checks the enabled state and executes a command on every item
// that's not a separator or a submenu parent item. The returned count should
// match the number of times the delegate is called to ensure every item works.
void CountEnabledExecutable(ui::MenuModel* model,
                            int* count) {
  for (int i = 0; i < model->GetItemCount(); ++i) {
    ui::MenuModel::ItemType type = model->GetTypeAt(i);
    switch (type) {
      case ui::MenuModel::TYPE_SEPARATOR:
        continue;
      case ui::MenuModel::TYPE_SUBMENU:
        CountEnabledExecutable(model->GetSubmenuModelAt(i), count);
        break;
      case ui::MenuModel::TYPE_COMMAND:
      case ui::MenuModel::TYPE_CHECK:
      case ui::MenuModel::TYPE_RADIO:
        model->IsEnabledAt(i);  // Check if it's enabled (ignore answer).
        model->ActivatedAt(i);  // Execute it.
        (*count)++;  // Increment the count of executable items seen.
        break;
      default:
        FAIL();  // Ensure every case is tested.
    }
  }
}

IN_PROC_BROWSER_TEST_F(BraveTabMenuModelTest, Basics) {
  Delegate delegate;
  BraveTabMenuModel model(&delegate, browser()->tab_strip_model(), 0);

  // Verify it has items. The number varies by platform, so we don't check
  // the exact number.
  // Chromium uses 5 but we added three more items. So, use 8.
  const int additional_brave_items = 3;
  EXPECT_GT(model.GetItemCount(), 5 + additional_brave_items);

  int item_count = 0;
  CountEnabledExecutable(&model, &item_count);

  // Brave added three more items and it is not countable by |delegate_|
  // because proxy handles them instead of passing |delegate_|.
  // So, plus 3 to chromium's tab menu count.
  EXPECT_GT(item_count, 0);
  EXPECT_EQ(item_count, delegate.execute_count_ + additional_brave_items);
  EXPECT_EQ(item_count, delegate.enable_count_ + additional_brave_items);

  // All items are disable state when there is only one tab.
  EXPECT_FALSE(model.delegate()->IsCommandIdEnabled(
      BraveTabMenuModel::CommandCloseOtherTabs));
  EXPECT_FALSE(model.delegate()->IsCommandIdEnabled(
      BraveTabMenuModel::CommandRestoreTab));
  EXPECT_FALSE(model.delegate()->IsCommandIdEnabled(
      BraveTabMenuModel::CommandBookmarkAllTabs));

  chrome::NewTab(browser());
  // Still close other tabs is disabled because currently running context menu
  // is for tab at zero and it's not selected tab. If other tab is only one and
  // selected, close other tabs item is not enabled.
  EXPECT_FALSE(model.delegate()->IsCommandIdEnabled(
      BraveTabMenuModel::CommandCloseOtherTabs));
  // Still restore tab menu is disabled because there is no closed tab.
  EXPECT_FALSE(model.delegate()->IsCommandIdEnabled(
      BraveTabMenuModel::CommandRestoreTab));
  // Bookmark all tabs item is enabled if the number of tabs are 2 or more.
  EXPECT_TRUE(model.delegate()->IsCommandIdEnabled(
      BraveTabMenuModel::CommandBookmarkAllTabs));

  // If the other tab is un-selected tab, it can be closed by close other tabs
  // menu.
  browser()->tab_strip_model()->ActivateTabAt(0);
  EXPECT_TRUE(model.delegate()->IsCommandIdEnabled(
      BraveTabMenuModel::CommandCloseOtherTabs));

  // If the other tab is pinned tab, close other tabs is disabled.
  browser()->tab_strip_model()->SetTabPinned(1, true);
  EXPECT_FALSE(model.delegate()->IsCommandIdEnabled(
      BraveTabMenuModel::CommandCloseOtherTabs));

  browser()->tab_strip_model()->SetTabPinned(1, false);

  chrome::CloseTab(browser());
  EXPECT_FALSE(model.delegate()->IsCommandIdEnabled(
      BraveTabMenuModel::CommandCloseOtherTabs));
  // When a tab is closed, restore tab menu item is enabled.
  EXPECT_TRUE(model.delegate()->IsCommandIdEnabled(
      BraveTabMenuModel::CommandRestoreTab));
  EXPECT_FALSE(model.delegate()->IsCommandIdEnabled(
      BraveTabMenuModel::CommandBookmarkAllTabs));
}
