/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_context_menu_contents.h"

#include <memory>

#include "brave/browser/ui/tabs/brave_tab_menu_model.h"
#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tabs/split_tab_menu_model.h"
#include "chrome/browser/ui/tabs/split_tab_metrics.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/browser_tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/core/common/features.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

class BraveTabContextMenuContentsTest : public InProcessBrowserTest {
 public:
  BraveTabContextMenuContentsTest() = default;
  ~BraveTabContextMenuContentsTest() override = default;

 protected:
  std::unique_ptr<BraveTabContextMenuContents> CreateMenuAt(int tab_index) {
    TabStrip* tabstrip =
        BrowserView::GetBrowserViewForBrowser(browser())->tabstrip();
    return std::make_unique<BraveTabContextMenuContents>(
        tabstrip->tab_at(tab_index),
        static_cast<BraveBrowserTabStripController*>(
            BrowserView::GetBrowserViewForBrowser(browser())
                ->tabstrip()
                ->controller()),
        tab_index);
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
  auto menu = CreateMenuAt(0);

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
  auto menu = CreateMenuAt(0);
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
  auto menu = CreateMenuAt(0);
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

  auto menu = CreateMenuAt(0);
  ASSERT_TRUE(menu->IsCommandIdVisible(
      BraveTabMenuModel::CommandBringAllTabsToThisWindow));
  menu->ExecuteCommand(BraveTabMenuModel::CommandBringAllTabsToThisWindow,
                       /*event_flags=*/0);

  EXPECT_EQ(browser()->tab_strip_model()->count(), tab_count + 1);
  EXPECT_EQ(incognito_browser->tab_strip_model()->count(), incognito_tab_count);
}

IN_PROC_BROWSER_TEST_F(BraveTabContextMenuContentsTest,
                       SplitViewMenuCustomizationTest) {
  // Smoke test for normal tab closing to verify split view's tab closing
  // customization doesn't affect original tab closing behavior.
  auto* tab_strip_model = browser()->tab_strip_model();

  // Create another normal tab.
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);

  // Now we have two normal tab at 0 and 1.
  EXPECT_EQ(2, tab_strip_model->count());
  EXPECT_EQ(1, tab_strip_model->active_index());
  {
    // Cache tab at 0's handle to check it's not closed when closing active tab
    // at 1 via tab context menu.
    auto tab0_handle = tab_strip_model->GetTabAtIndex(0)->GetHandle();
    auto menu = CreateMenuAt(1);
    menu->ExecuteCommand(TabStripModel::CommandCloseTab,
                         /*event_flags=*/0);
    EXPECT_EQ(1, tab_strip_model->count());
    EXPECT_EQ(0, tab_strip_model->active_index());
    EXPECT_EQ(tab0_handle, tab_strip_model->GetTabAtIndex(0)->GetHandle());
  }

  // Create another normal tab.
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ false);

  // Now we have two normal tab at 0 and 1.
  EXPECT_EQ(2, tab_strip_model->count());
  EXPECT_EQ(0, tab_strip_model->active_index());
  {
    // Cache tab at 0's handle to check it's not closed when closing inactive
    // tab at 1 via tab context menu.
    auto tab0_handle = tab_strip_model->GetTabAtIndex(0)->GetHandle();
    auto menu = CreateMenuAt(1);
    menu->ExecuteCommand(TabStripModel::CommandCloseTab,
                         /*event_flags=*/0);
    EXPECT_EQ(tab0_handle, tab_strip_model->GetTabAtIndex(0)->GetHandle());
  }

  // Now we have one normal tab at 0.
  EXPECT_EQ(1, tab_strip_model->count());
  EXPECT_EQ(0, tab_strip_model->active_index());

  // Check split view context menu with normal tab.
  {
    auto menu = CreateMenuAt(0);
    auto* menu_model = menu->model_.get();
    auto index =
        menu_model->GetIndexOfCommandId(TabStripModel::CommandAddToSplit);
    EXPECT_TRUE(index.has_value());
    EXPECT_FALSE(menu_model->IsNewFeatureAt(*index));
  }

  // Create another normal tab.
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);

  // Now we have two normal tab at 0 and 1.
  EXPECT_EQ(2, tab_strip_model->count());
  EXPECT_EQ(1, tab_strip_model->active_index());

  // Create split tabs with tab at 1 and 2.
  chrome::NewSplitTab(browser(),
                      split_tabs::SplitTabCreatedSource::kTabContextMenu);

  // Now we have one normal tab at 0 and split tab at 1 and 2.
  EXPECT_EQ(3, tab_strip_model->count());
  EXPECT_EQ(2, tab_strip_model->active_index());

  // split tab's context menu has arrange command.
  {
    auto menu = CreateMenuAt(2);
    auto* menu_model = menu->model_.get();
    auto index =
        menu_model->GetIndexOfCommandId(TabStripModel::CommandArrangeSplit);
    EXPECT_TRUE(index.has_value());
    EXPECT_FALSE(menu_model->IsNewFeatureAt(*index));

    // Execute close tab menu on tab at 2 and check only that active tab is
    // closed from split tab.
    auto tab1_handle = tab_strip_model->GetTabAtIndex(1)->GetHandle();
    menu->ExecuteCommand(TabStripModel::CommandCloseTab,
                         /*event_flags=*/0);
    EXPECT_EQ(2, tab_strip_model->count());
    EXPECT_EQ(1, tab_strip_model->active_index());
    EXPECT_EQ(tab1_handle, tab_strip_model->GetTabAtIndex(1)->GetHandle());
  }

  // Create split tabs with tab at 1 and 2.
  chrome::NewSplitTab(browser(),
                      split_tabs::SplitTabCreatedSource::kTabContextMenu);

  // Now we have one normal tab at 0 and split tab at 1 and 2.
  EXPECT_EQ(3, tab_strip_model->count());
  EXPECT_EQ(2, tab_strip_model->active_index());

  // split tab's context menu has arrange command.
  {
    auto tab2_handle = tab_strip_model->GetTabAtIndex(2)->GetHandle();
    auto menu = CreateMenuAt(1);
    // Execute close tab menu on tab at 1 and check only that inactive tab is
    // closed from split tab.
    menu->ExecuteCommand(TabStripModel::CommandCloseTab,
                         /*event_flags=*/0);
    EXPECT_EQ(2, tab_strip_model->count());
    EXPECT_EQ(1, tab_strip_model->active_index());
    EXPECT_EQ(tab2_handle, tab_strip_model->GetTabAtIndex(1)->GetHandle());
  }

  // Create split tabs with tab at 1 and 2.
  chrome::NewSplitTab(browser(),
                      split_tabs::SplitTabCreatedSource::kTabContextMenu);

  // Now we have one normal tab at 0 and split tab at 1 and 2.
  EXPECT_EQ(3, tab_strip_model->count());
  EXPECT_EQ(2, tab_strip_model->active_index());

  // Close normal tab when split tab is active and check only that normal tab is
  // closed.
  {
    auto normal_tab_handle = tab_strip_model->GetTabAtIndex(0)->GetHandle();
    auto menu = CreateMenuAt(0);
    menu->ExecuteCommand(TabStripModel::CommandCloseTab,
                         /*event_flags=*/0);
    EXPECT_EQ(2, tab_strip_model->count());
    EXPECT_EQ(1, tab_strip_model->active_index());
    EXPECT_NE(normal_tab_handle,
              tab_strip_model->GetTabAtIndex(0)->GetHandle());
    EXPECT_NE(normal_tab_handle,
              tab_strip_model->GetTabAtIndex(1)->GetHandle());
    // Now we have one split view at tab 0 and 1.
  }

  // Create another normal tab at 0.
  chrome::AddTabAt(browser(), GURL(), 0, /*foreground*/ true);

  // Now we have one normal tab at 0 and split tab at 1 and 2.
  EXPECT_EQ(3, tab_strip_model->count());
  EXPECT_EQ(0, tab_strip_model->active_index());

  // Close normal tab when split tab is active and check only that normal tab is
  // closed.
  {
    auto normal_tab_handle = tab_strip_model->GetTabAtIndex(0)->GetHandle();
    auto menu = CreateMenuAt(0);
    menu->ExecuteCommand(TabStripModel::CommandCloseTab,
                         /*event_flags=*/0);
    EXPECT_EQ(2, tab_strip_model->count());
    EXPECT_NE(normal_tab_handle,
              tab_strip_model->GetTabAtIndex(0)->GetHandle());
    EXPECT_NE(normal_tab_handle,
              tab_strip_model->GetTabAtIndex(1)->GetHandle());
    // Now we have one split view at tab 0 and 1.
  }

  // Create another normal tab at 0.
  chrome::AddTabAt(browser(), GURL(), 0, /*foreground*/ false);
  tab_strip_model->ActivateTabAt(2);

  // Now we have one normal tab at 0 and split tab at 1 and 2.
  EXPECT_EQ(3, tab_strip_model->count());
  EXPECT_EQ(2, tab_strip_model->active_index());
  {
    auto menu = CreateMenuAt(2);
    auto* menu_model = menu->model_.get();
    auto index =
        menu_model->GetIndexOfCommandId(TabStripModel::CommandArrangeSplit);
    ASSERT_TRUE(index);
    EXPECT_FALSE(menu_model->IsNewFeatureAt(*index));
    auto* arrange_split_menu_model =
        menu_model->arrange_split_view_submenu_for_testing();
    ASSERT_TRUE(arrange_split_menu_model);
    index = arrange_split_menu_model->GetIndexOfCommandId(
        SplitTabMenuModel::GetCommandIdInt(
            SplitTabMenuModel::CommandId::kReversePosition));
    ASSERT_TRUE(index);
    EXPECT_EQ(l10n_util::GetStringUTF16(IDS_IDC_SWAP_SPLIT_VIEW),
              arrange_split_menu_model->GetLabelAt(*index));
    index = arrange_split_menu_model->GetIndexOfCommandId(
        SplitTabMenuModel::GetCommandIdInt(
            SplitTabMenuModel::CommandId::kExitSplit));
    ASSERT_TRUE(index);
    EXPECT_EQ(l10n_util::GetStringUTF16(IDS_IDC_BREAK_TILE),
              arrange_split_menu_model->GetLabelAt(*index));
  }

  // Non active tab's context menu has swap menu when
  // active tab is split tab.
  {
    auto menu = CreateMenuAt(0);
    auto* menu_model = menu->model_.get();
    auto index = menu_model->GetIndexOfCommandId(
        TabStripModel::CommandSwapWithActiveSplit);
    EXPECT_TRUE(index.has_value());
    EXPECT_FALSE(menu_model->IsNewFeatureAt(*index));
  }
}

#if BUILDFLAG(ENABLE_CONTAINERS)
class BraveTabContextMenuContentsWithContainersTest
    : public BraveTabContextMenuContentsTest {
 public:
  BraveTabContextMenuContentsWithContainersTest() = default;
  ~BraveTabContextMenuContentsWithContainersTest() override = default;

 private:
  base::test::ScopedFeatureList feature_list_{
      containers::features::kContainers};
};

IN_PROC_BROWSER_TEST_F(BraveTabContextMenuContentsWithContainersTest,
                       ContainersSubMenuExists) {
  // Regression test for https://github.com/brave/brave-browser/issues/47808
  // The Containers submenu should be added even if CommandMoveTabsToNewWindow
  // doesn't exist in the tab context menu.
  auto menu = CreateMenuAt(0);
  auto* menu_model = menu->model_.get();
  auto index = menu_model->GetIndexOfCommandId(
      BraveTabMenuModel::CommandOpenInContainer);
  EXPECT_TRUE(index.has_value());

  chrome::NewEmptyWindow(browser()->profile());

  menu = CreateMenuAt(0);
  menu_model = menu->model_.get();
  index = menu_model->GetIndexOfCommandId(
      BraveTabMenuModel::CommandOpenInContainer);
  EXPECT_TRUE(index.has_value());
}
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
