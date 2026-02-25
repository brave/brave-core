// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/sidebar/sidebar_container_view.h"

#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/side_panel/side_panel.h"
#include "brave/browser/ui/views/sidebar/sidebar_button_view.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/browser/ui/views/toolbar/side_panel_button.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/sidebar/browser/pref_names.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

class SidebarContainerViewBrowserTest : public InProcessBrowserTest {
 public:
  SidebarContainerViewBrowserTest() = default;
  SidebarContainerViewBrowserTest(const SidebarContainerViewBrowserTest&) =
      delete;
  SidebarContainerViewBrowserTest& operator=(
      const SidebarContainerViewBrowserTest&) = delete;
  ~SidebarContainerViewBrowserTest() override = default;

  BraveBrowserView* brave_browser_view() {
    return static_cast<BraveBrowserView*>(
        BraveBrowserView::GetBrowserViewForBrowser(browser()));
  }

  sidebar::SidebarService* GetService() {
    return sidebar::SidebarServiceFactory::GetForProfile(browser()->profile());
  }

  SidebarContainerView* sidebar() {
    auto* controller = browser()->GetFeatures().sidebar_controller();
    return static_cast<SidebarContainerView*>(controller->sidebar());
  }

  SidePanelButton* toolbar_button() {
    return static_cast<BraveToolbarView*>(
               BrowserView::GetBrowserViewForBrowser(browser())->toolbar())
        ->side_panel_button();
  }
};

IN_PROC_BROWSER_TEST_F(SidebarContainerViewBrowserTest,
                       ButtonIsShownByDefault) {
  EXPECT_LT(0u, GetService()->items().size());
  EXPECT_TRUE(sidebar());
  EXPECT_TRUE(toolbar_button());
  EXPECT_TRUE(toolbar_button()->GetVisible());
  EXPECT_EQ(GetLayoutConstant(LayoutConstant::kToolbarButtonHeight),
            toolbar_button()->height());
}

IN_PROC_BROWSER_TEST_F(SidebarContainerViewBrowserTest, ButtonIsHiddenByPref) {
  EXPECT_TRUE(toolbar_button()->GetVisible());

  // When the pref is false, the button should be hidden.
  browser()->profile()->GetPrefs()->SetBoolean(kShowSidePanelButton, false);
  EXPECT_FALSE(toolbar_button()->GetVisible());

  // Reenabling it should show the button again.
  browser()->profile()->GetPrefs()->SetBoolean(kShowSidePanelButton, true);
  EXPECT_TRUE(toolbar_button()->GetVisible());
}

IN_PROC_BROWSER_TEST_F(SidebarContainerViewBrowserTest,
                       ButtonIsHiddenWithoutPanelItems) {
  EXPECT_TRUE(toolbar_button()->GetVisible());

  // Removing all items should make the button hide.
  auto items_count = GetService()->items().size();
  while (items_count > 0) {
    GetService()->RemoveItemAt(--items_count);
  }
  EXPECT_FALSE(toolbar_button()->GetVisible());

  // Adding a new default item should cause the button to become visible again.
  GetService()->AddItem(sidebar::SidebarItem::Create(
      u"Test", sidebar::SidebarItem::Type::kTypeBuiltIn,
      sidebar::SidebarItem::BuiltInItemType::kReadingList, true));
  EXPECT_EQ(1u, GetService()->items().size());
  EXPECT_TRUE(toolbar_button()->GetVisible());
}

// Verifies that when "Show Sidebar" is set to Never, opening a side panel
// shows only the panel content without the sidebar control view (icon strip).
// Regression test for https://github.com/brave/brave-browser/issues/51271
IN_PROC_BROWSER_TEST_F(SidebarContainerViewBrowserTest,
                       ShowNeverHidesControlViewWhenPanelOpens) {
  // Set sidebar show option to "Never".
  GetService()->SetSidebarShowOption(
      sidebar::SidebarService::ShowSidebarOption::kShowNever);
  EXPECT_FALSE(sidebar()->IsSidebarVisible());

  // Open a built-in side panel entry (e.g., bookmarks).
  auto* side_panel_ui = browser()->GetFeatures().side_panel_ui();
  ASSERT_TRUE(side_panel_ui);
  side_panel_ui->Show(SidePanelEntryId::kBookmarks);

  // The side panel should be visible, but the sidebar control view
  // (icon strip) should remain hidden per the user's preference.
  EXPECT_TRUE(sidebar()->side_panel()->GetVisible());
  EXPECT_FALSE(sidebar()->IsSidebarVisible());
}

// Verifies that switching from ShowAlways to ShowNever while a panel is open
// hides the control view but keeps the panel visible.
IN_PROC_BROWSER_TEST_F(SidebarContainerViewBrowserTest,
                       SwitchToShowNeverWhilePanelOpen) {
  // Start with ShowAlways and open a panel.
  GetService()->SetSidebarShowOption(
      sidebar::SidebarService::ShowSidebarOption::kShowAlways);
  EXPECT_TRUE(sidebar()->IsSidebarVisible());

  auto* side_panel_ui = browser()->GetFeatures().side_panel_ui();
  ASSERT_TRUE(side_panel_ui);
  side_panel_ui->Show(SidePanelEntryId::kBookmarks);

  EXPECT_TRUE(sidebar()->IsSidebarVisible());
  EXPECT_TRUE(sidebar()->side_panel()->GetVisible());

  // Switch to ShowNever — control should hide, panel should stay.
  GetService()->SetSidebarShowOption(
      sidebar::SidebarService::ShowSidebarOption::kShowNever);
  EXPECT_FALSE(sidebar()->IsSidebarVisible());
  EXPECT_TRUE(sidebar()->side_panel()->GetVisible());

  // Switch back to ShowAlways — control should reappear alongside panel.
  GetService()->SetSidebarShowOption(
      sidebar::SidebarService::ShowSidebarOption::kShowAlways);
  EXPECT_TRUE(sidebar()->IsSidebarVisible());
  EXPECT_TRUE(sidebar()->side_panel()->GetVisible());
}
