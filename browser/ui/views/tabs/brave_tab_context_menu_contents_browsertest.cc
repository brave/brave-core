/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_menu_model.h"
#include "brave/browser/ui/views/tabs/brave_tab_context_menu_contents.h"
#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/browser_tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "url/gurl.h"

using BraveTabContextMenuContentsTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveTabContextMenuContentsTest, Basics) {
  TabStrip* tabstrip =
      BrowserView::GetBrowserViewForBrowser(browser())->tabstrip();
  BraveTabContextMenuContents menu(
      tabstrip->tab_at(0),
      static_cast<BraveBrowserTabStripController*>(
          BrowserView::GetBrowserViewForBrowser(
              browser())->tabstrip()->controller()),
      0);

  // All items are disable state when there is only one tab.
  EXPECT_FALSE(menu.IsCommandIdEnabled(
      BraveTabMenuModel::CommandRestoreTab));
  EXPECT_FALSE(menu.IsCommandIdEnabled(
      BraveTabMenuModel::CommandBookmarkAllTabs));

  chrome::NewTab(browser());
  // Still restore tab menu is disabled because there is no closed tab.
  EXPECT_FALSE(menu.IsCommandIdEnabled(
      BraveTabMenuModel::CommandRestoreTab));
  // Bookmark all tabs item is enabled if the number of tabs are 2 or more.
  EXPECT_TRUE(menu.IsCommandIdEnabled(
      BraveTabMenuModel::CommandBookmarkAllTabs));

  // When a tab is closed, restore tab menu item is enabled.
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("brave://version/")));
  chrome::CloseTab(browser());
  EXPECT_TRUE(menu.IsCommandIdEnabled(
      BraveTabMenuModel::CommandRestoreTab));
  EXPECT_FALSE(menu.IsCommandIdEnabled(
      BraveTabMenuModel::CommandBookmarkAllTabs));
}
