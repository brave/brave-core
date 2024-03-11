/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_context_menu_contents.h"

#include <memory>

#include "brave/browser/ui/tabs/brave_tab_menu_model.h"
#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/browser_tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"

class BraveTabContextMenuContentsTest : public InProcessBrowserTest {
 public:
  BraveTabContextMenuContentsTest() = default;
  ~BraveTabContextMenuContentsTest() override = default;

 protected:
  std::unique_ptr<BraveTabContextMenuContents> CreateMenu() {
    TabStrip* tabstrip =
        BrowserView::GetBrowserViewForBrowser(browser())->tabstrip();
    return std::make_unique<BraveTabContextMenuContents>(
        tabstrip->tab_at(0),
        static_cast<BraveBrowserTabStripController*>(
            BrowserView::GetBrowserViewForBrowser(browser())
                ->tabstrip()
                ->controller()),
        0);
  }

  Browser* CreateBrowser(bool incognito) {
    if (incognito) {
      return chrome::OpenEmptyWindow(
          browser()->profile()->GetPrimaryOTRProfile(/*create_if_needed=*/true),
          /*should_trigger_session_restore=*/false);
    }
    return chrome::OpenEmptyWindow(browser()->profile(),
                                   /*should_trigger_session_restore=*/false);
  }

  void AddTabs(Browser* browser, int new_tab_count, int pinned_tab_count) {
    std::vector expected = {browser->tab_strip_model()->GetWebContentsAt(0)};
    for (int i = 0; i < new_tab_count; ++i) {
      expected.push_back(chrome::AddAndReturnTabAt(browser, GURL(),
                                                   /*index=*/-1,
                                                   /*foreground=*/false));
    }
    for (int i = 0; i < pinned_tab_count; ++i) {
      browser->tab_strip_model()->SetTabPinned(i, true);
    }
  }

  std::vector<content::WebContents*> GetWebContentses(Browser* browser) {
    std::vector<content::WebContents*> web_contentses;
    const auto count = browser->tab_strip_model()->count();
    for (int i = 0; i < count; i++) {
      web_contentses.push_back(browser->tab_strip_model()->GetWebContentsAt(i));
    }
    return web_contentses;
  }
};

IN_PROC_BROWSER_TEST_F(BraveTabContextMenuContentsTest, Basics) {
  auto menu = CreateMenu();

  // All items are disable state when there is only one tab.
  EXPECT_FALSE(menu->IsCommandIdEnabled(BraveTabMenuModel::CommandRestoreTab));
  EXPECT_FALSE(
      menu->IsCommandIdEnabled(BraveTabMenuModel::CommandBookmarkAllTabs));

  chrome::NewTab(browser());
  // Still restore tab menu is disabled because there is no closed tab.
  EXPECT_FALSE(menu->IsCommandIdEnabled(BraveTabMenuModel::CommandRestoreTab));
  // Bookmark all tabs item is enabled if the number of tabs are 2 or more.
  EXPECT_TRUE(
      menu->IsCommandIdEnabled(BraveTabMenuModel::CommandBookmarkAllTabs));

  // When a tab is closed, restore tab menu item is enabled.
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("brave://version/")));
  chrome::CloseTab(browser());
  EXPECT_TRUE(menu->IsCommandIdEnabled(BraveTabMenuModel::CommandRestoreTab));
  EXPECT_FALSE(
      menu->IsCommandIdEnabled(BraveTabMenuModel::CommandBookmarkAllTabs));
}

IN_PROC_BROWSER_TEST_F(BraveTabContextMenuContentsTest,
                       BringAllTabsToThisWindow_VisibleWhenOtherBrowserExists) {
  auto menu = CreateMenu();
  auto is_command_visible = [&]() {
    return menu->IsCommandIdVisible(
        BraveTabMenuModel::CommandBringAllTabsToThisWindow);
  };

  // No other browser exists, so the command is not visible.
  EXPECT_FALSE(is_command_visible());

  // Open a new browser and the command becomes visible.
  auto* new_browser = CreateBrowser(/*incognito=*/false);
  ASSERT_FALSE(new_browser->tab_strip_model()->empty());
  EXPECT_TRUE(is_command_visible());

  // Close the new browser and the command becomes invisible again.
  CloseBrowserSynchronously(new_browser);
  EXPECT_FALSE(is_command_visible());

  // New incognito window shouldn't affect the visibility of the command.
  new_browser = CreateBrowser(/*incognito=*/true);
  ASSERT_FALSE(new_browser->tab_strip_model()->empty());
  EXPECT_FALSE(is_command_visible());
}

IN_PROC_BROWSER_TEST_F(BraveTabContextMenuContentsTest,
                       BringAllTabsToThisWindow_TabsInOrder) {
  // Prepare a new browser with multiple tabs.
  auto* new_browser = CreateBrowser(/*incognito=*/false);
  constexpr auto kNewTabCount = 4;
  constexpr auto kPinnedTabCount = 2;
  AddTabs(new_browser, kNewTabCount, kPinnedTabCount);
  ASSERT_EQ(new_browser->tab_strip_model()->count(), kNewTabCount + 1);
  ASSERT_EQ(new_browser->tab_strip_model()->IndexOfFirstNonPinnedTab(),
            kPinnedTabCount);
  auto expected = GetWebContentses(new_browser);

  auto* tab_strip_model = browser()->tab_strip_model();
  AddTabs(browser(), /*new_tab_count*/ 1, /*pinned_tab*/ 1);
  ASSERT_EQ(tab_strip_model->count(), 2);
  ASSERT_TRUE(tab_strip_model->IsTabPinned(0));
  ASSERT_FALSE(tab_strip_model->IsTabPinned(1));

  // All other unpinned tab should be appended after the existing tabs.
  expected.insert(expected.begin() + kPinnedTabCount,
                  tab_strip_model->GetWebContentsAt(1));
  expected.insert(expected.begin(), tab_strip_model->GetWebContentsAt(0));

  // Bring all tabs to this browser
  auto menu = CreateMenu();
  ASSERT_TRUE(menu->IsCommandIdVisible(
      BraveTabMenuModel::CommandBringAllTabsToThisWindow));
  menu->ExecuteCommand(BraveTabMenuModel::CommandBringAllTabsToThisWindow,
                       /*event_flags=*/0);

  // The tabs should be moved to the current browser in the order.
  EXPECT_EQ(tab_strip_model->IndexOfFirstNonPinnedTab(), kPinnedTabCount + 1);
  EXPECT_THAT(GetWebContentses(browser()), testing::ElementsAreArray(expected));
}

IN_PROC_BROWSER_TEST_F(BraveTabContextMenuContentsTest,
                       BringAllTabsToThisWindow_MultipleWindows) {
  auto* new_browser_1 = CreateBrowser(/*incognito=*/false);
  AddTabs(new_browser_1, /*new_tab_count=*/2, /*pinned_tab_count=*/0);
  auto tab_count = new_browser_1->tab_strip_model()->count();

  auto* new_browser_2 = CreateBrowser(/*incognito=*/false);
  AddTabs(new_browser_2, /*new_tab_count=*/3, /*pinned_tab_count=*/0);
  tab_count += new_browser_2->tab_strip_model()->count();

  auto* incognito_browser = CreateBrowser(/*incognito=*/true);
  AddTabs(incognito_browser, /*new_tab_count=*/4, /*pinned_tab_count=*/0);
  const auto incognito_tab_count =
      incognito_browser->tab_strip_model()->count();

  auto menu = CreateMenu();
  ASSERT_TRUE(menu->IsCommandIdVisible(
      BraveTabMenuModel::CommandBringAllTabsToThisWindow));
  menu->ExecuteCommand(BraveTabMenuModel::CommandBringAllTabsToThisWindow,
                       /*event_flags=*/0);

  EXPECT_EQ(browser()->tab_strip_model()->count(), tab_count + 1);
  EXPECT_EQ(incognito_browser->tab_strip_model()->count(), incognito_tab_count);
}
