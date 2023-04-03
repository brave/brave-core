/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/functional/bind.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel.h"
#include "brave/browser/ui/views/sidebar/sidebar_container_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_control_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_items_contents_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_items_scroll_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/components/ai_chat/features.h"
#include "brave/components/playlist/common/features.h"
#include "brave/components/sidebar/sidebar_item.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock-matchers.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/point.h"

using ::testing::Eq;
using ::testing::Ne;
using ::testing::Optional;

namespace sidebar {

class SidebarBrowserTest : public InProcessBrowserTest {
 public:
  SidebarBrowserTest() = default;
  ~SidebarBrowserTest() override = default;

  void PreRunTestOnMainThread() override {
    InProcessBrowserTest::PreRunTestOnMainThread();

    auto* service = SidebarServiceFactory::GetForProfile(browser()->profile());
    // Enable sidebar explicitely because sidebar option is different based on
    // channel.
    service->SetSidebarShowOption(
        SidebarService::ShowSidebarOption::kShowAlways);
  }

  BraveBrowser* brave_browser() const {
    return static_cast<BraveBrowser*>(browser());
  }

  SidebarModel* model() const { return controller()->model(); }
  TabStripModel* tab_model() const { return browser()->tab_strip_model(); }

  SidebarController* controller() const {
    return brave_browser()->sidebar_controller();
  }

  views::View* GetVerticalTabsContainer() const {
    auto* view = BrowserView::GetBrowserViewForBrowser(browser());
    return static_cast<BraveBrowserView*>(view)->vertical_tab_strip_host_view_;
  }

  views::Widget* GetEventDetectWidget() {
    auto* sidebar_container_view =
        static_cast<SidebarContainerView*>(controller()->sidebar());
    return sidebar_container_view->GetEventDetectWidget()->widget_.get();
  }

  raw_ptr<SidebarItemsContentsView> GetSidebarItemsContentsView(
      SidebarController* controller) const {
    auto* sidebar_container_view =
        static_cast<SidebarContainerView*>(controller->sidebar());
    auto sidebar_control_view = sidebar_container_view->sidebar_control_view_;
    auto sidebar_scroll_view = sidebar_control_view->sidebar_items_view_;
    auto sidebar_items_contents_view = sidebar_scroll_view->contents_view_;
    DCHECK(sidebar_items_contents_view);

    return sidebar_items_contents_view;
  }

  void SimulateSidebarItemClickAt(int index) const {
    auto sidebar_items_contents_view =
        GetSidebarItemsContentsView(controller());

    auto* item = sidebar_items_contents_view->children()[index];
    DCHECK(item);

    const gfx::Point origin(0, 0);
    ui::MouseEvent event(ui::ET_MOUSE_PRESSED, origin, origin,
                         ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON, 0);
    sidebar_items_contents_view->OnItemPressed(item, event);
  }

  SidebarControlView* GetSidebarControlView() const {
    return GetSidebarContainerView()->sidebar_control_view_;
  }

  SidebarContainerView* GetSidebarContainerView() const {
    return static_cast<SidebarContainerView*>(controller()->sidebar());
  }

  BraveSidePanel* GetSidePanel() const {
    return GetSidebarContainerView()->side_panel_;
  }

  bool IsSidebarUIOnLeft() const {
    return GetSidebarContainerView()->sidebar_on_left_ &&
           !GetSidePanel()->IsRightAligned() &&
           GetSidebarControlView()->sidebar_on_left_;
  }

  void WaitUntil(base::RepeatingCallback<bool()> condition) {
    if (condition.Run())
      return;

    base::RepeatingTimer scheduler;
    scheduler.Start(FROM_HERE, base::Milliseconds(100),
                    base::BindLambdaForTesting([this, &condition]() {
                      if (condition.Run())
                        run_loop_->Quit();
                    }));
    Run();
  }

  void Run() {
    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop()->Run();
  }

  base::RunLoop* run_loop() const { return run_loop_.get(); }

  std::unique_ptr<base::RunLoop> run_loop_;
};

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, BasicTest) {
  // Initially, active index is not set.
  EXPECT_THAT(model()->active_index(), Eq(absl::nullopt));

  // Check sidebar UI is initalized properly.
  EXPECT_TRUE(!!controller()->sidebar());

  browser()->command_controller()->ExecuteCommand(IDC_TOGGLE_SIDEBAR);
  WaitUntil(
      base::BindLambdaForTesting([&]() { return !!model()->active_index(); }));
  // Check active index is non-null.
  EXPECT_THAT(model()->active_index(), Ne(absl::nullopt));

  browser()->command_controller()->ExecuteCommand(IDC_TOGGLE_SIDEBAR);
  WaitUntil(
      base::BindLambdaForTesting([&]() { return !model()->active_index(); }));
  // Check active index is null.
  EXPECT_THAT(model()->active_index(), Eq(absl::nullopt));

  // Currently we have 4 default items.
  EXPECT_EQ(4UL, model()->GetAllSidebarItems().size());
  // Activate item that opens in panel.
  controller()->ActivateItemAt(2);
  EXPECT_THAT(model()->active_index(), Optional(2u));
  EXPECT_TRUE(controller()->IsActiveIndex(2));

  // Try to activate item at index 1.
  // Default item at index 1 opens in new tab. So, sidebar active index is not
  // changed. Still active index is 2.
  const auto item = model()->GetAllSidebarItems()[1];
  EXPECT_FALSE(item.open_in_panel);
  controller()->ActivateItemAt(1);
  EXPECT_THAT(model()->active_index(), Optional(2u));

  // Setting absl::nullopt means deactivate current active tab.
  controller()->ActivateItemAt(absl::nullopt);
  EXPECT_THAT(model()->active_index(), Eq(absl::nullopt));

  controller()->ActivateItemAt(2);

  // Remove Item at index 0 change active index from 3 to 2.
  SidebarServiceFactory::GetForProfile(browser()->profile())->RemoveItemAt(0);
  EXPECT_EQ(3UL, model()->GetAllSidebarItems().size());
  EXPECT_THAT(model()->active_index(), Optional(1u));

  // If current active tab is not NTP, we can add current url to sidebar.
  EXPECT_TRUE(CanAddCurrentActiveTabToSidebar(browser()));

  // If current active tab is NTP, we can't add current url to sidebar.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("brave://newtab/")));
  EXPECT_FALSE(CanAddCurrentActiveTabToSidebar(browser()));

  // Check |BrowserView::find_bar_host_view_| is the last child view.
  // If not, findbar dialog is not positioned properly.
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto find_bar_host_view_index =
      browser_view->GetIndexOf(browser_view->find_bar_host_view());
  EXPECT_THAT(find_bar_host_view_index,
              Optional(browser_view->children().size() - 1));
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, WebTypePanelTest) {
  // By default, sidebar has 4 items.
  EXPECT_EQ(4UL, model()->GetAllSidebarItems().size());

  // Add an item
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("brave://settings/")));
  int current_tab_index = tab_model()->active_index();
  EXPECT_EQ(0, current_tab_index);
  EXPECT_TRUE(CanAddCurrentActiveTabToSidebar(browser()));
  controller()->AddItemWithCurrentTab();
  // Verify new size
  EXPECT_EQ(5UL, model()->GetAllSidebarItems().size());

  // Load NTP in a new tab and activate it. (tab index 1)
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("brave://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  current_tab_index = tab_model()->active_index();
  EXPECT_EQ(1, current_tab_index);

  // Activate sidebar item(brave://settings) and check existing first tab is
  // activated.
  auto item = model()->GetAllSidebarItems()[4];
  controller()->ActivateItemAt(4);
  EXPECT_EQ(0, tab_model()->active_index());
  EXPECT_EQ(tab_model()->GetWebContentsAt(0)->GetVisibleURL(), item.url);

  // Activate second sidebar item(wallet) and check it's loaded at current tab.
  item = model()->GetAllSidebarItems()[1];
  controller()->ActivateItemAt(1);
  EXPECT_EQ(0, tab_model()->active_index());
  EXPECT_EQ(tab_model()->GetWebContentsAt(0)->GetVisibleURL(), item.url);
  // New tab is not created.
  EXPECT_EQ(2, tab_model()->count());
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, IterateBuiltInWebTypeTest) {
  // Click builtin wallet item and it's loaded at current active tab.
  auto item = model()->GetAllSidebarItems()[1];
  EXPECT_FALSE(controller()->DoesBrowserHaveOpenedTabForItem(item));
  SimulateSidebarItemClickAt(1);
  EXPECT_TRUE(controller()->DoesBrowserHaveOpenedTabForItem(item));
  EXPECT_EQ(0, tab_model()->active_index());
  EXPECT_EQ(tab_model()->GetWebContentsAt(0)->GetVisibleURL().host(),
            item.url.host());

  // Create NTP and click wallet item. Then wallet tab(index 0) is activated.
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("brave://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  // NTP is active tab.
  EXPECT_EQ(1, tab_model()->active_index());
  SimulateSidebarItemClickAt(1);
  // Wallet tab is active tab.
  EXPECT_EQ(0, tab_model()->active_index());
  EXPECT_EQ(tab_model()->GetWebContentsAt(0)->GetVisibleURL().host(),
            item.url.host());

  // Create NTP.
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("brave://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  // NTP is active tab and load wallet on it.
  EXPECT_EQ(2, tab_model()->active_index());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), item.url));

  // Click wallet item and then first wallet tab(tab index 0) is activated.
  SimulateSidebarItemClickAt(1);
  EXPECT_EQ(0, tab_model()->active_index());

  // Click wallet item and then second wallet tab(index 2) is activated.
  SimulateSidebarItemClickAt(1);
  EXPECT_EQ(2, tab_model()->active_index());

  // Click wallet item and then first wallet tab(index 0) is activated.
  SimulateSidebarItemClickAt(1);
  EXPECT_EQ(0, tab_model()->active_index());

  // Checking windows' activation state is flaky in browser tests.
#if !BUILDFLAG(IS_MAC)
  auto* browser2 = CreateBrowser(browser()->profile());
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return browser2->window()->IsActive(); }));

  // |browser2| doesn't have any wallet tab. So, clicking wallet sidebar item
  // activates other browser's first wallet tab.
  static_cast<BraveBrowser*>(browser2)->sidebar_controller()->ActivateItemAt(1);

  // Wait till browser() is activated.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return browser()->window()->IsActive(); }));

  EXPECT_EQ(0, tab_model()->active_index());
#endif
}

// Test sidebar's initial horizontal option is set properly.
IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, PRE_InitialHorizontalOptionTest) {
  auto* prefs = browser()->profile()->GetPrefs();

  // Check default horizontal option is left-sided.
  EXPECT_FALSE(prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment));
  EXPECT_TRUE(IsSidebarUIOnLeft());

  // Set right-sided for next testing.
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, true);
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, InitialHorizontalOptionTest) {
  auto* prefs = browser()->profile()->GetPrefs();

  // Check horizontal option is right-sided.
  EXPECT_TRUE(prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment));
  EXPECT_FALSE(IsSidebarUIOnLeft());
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, EventDetectWidgetTest) {
  auto* widget = GetEventDetectWidget();
  auto* service = SidebarServiceFactory::GetForProfile(browser()->profile());
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* contents_container = browser_view->contents_container();
  auto* prefs = browser()->profile()->GetPrefs();

  // Check widget is located on left side when sidebar on left.
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, false);
  service->SetSidebarShowOption(
      SidebarService::ShowSidebarOption::kShowOnMouseOver);
  EXPECT_EQ(contents_container->GetBoundsInScreen().x(),
            widget->GetWindowBoundsInScreen().x());

  // Check widget is located on right side when sidebar on right.
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, true);
  EXPECT_EQ(contents_container->GetBoundsInScreen().right(),
            widget->GetWindowBoundsInScreen().right());
}

class SidebarBrowserTestWithPlaylist : public SidebarBrowserTest {
 public:
  SidebarBrowserTestWithPlaylist() {
    feature_list_.InitAndEnableFeature(playlist::features::kPlaylist);
  }
  ~SidebarBrowserTestWithPlaylist() override = default;

  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(SidebarBrowserTestWithPlaylist, Incognito) {
  // There should be no crash with incognito.
  auto* private_browser = CreateIncognitoBrowser(browser()->profile());
  ASSERT_TRUE(private_browser);

  auto* sidebar_service =
      SidebarServiceFactory::GetForProfile(browser()->profile());
  const auto& items = sidebar_service->items();
  auto iter = base::ranges::find_if(items, [](const auto& item) {
    return item.type == SidebarItem::Type::kTypeBuiltIn &&
           item.built_in_item_type == SidebarItem::BuiltInItemType::kPlaylist;
  });
  ASSERT_NE(iter, items.end());

  auto sidebar_items_contents_view = GetSidebarItemsContentsView(
      static_cast<BraveBrowser*>(private_browser)->sidebar_controller());
  EXPECT_FALSE(sidebar_items_contents_view->children()
                   .at(std::distance(items.begin(), iter))
                   ->GetEnabled());

  // Try Adding an item
  sidebar_service->AddItem(sidebar::SidebarItem::Create(
      GURL("http://foo.bar/"), u"title", SidebarItem::Type::kTypeWeb,
      SidebarItem::BuiltInItemType::kNone, false));

  // Try moving an item
  sidebar_service->MoveItem(sidebar_service->items().size() - 1, 0);

  // Try Remove an item
  sidebar_service->RemoveItemAt(0);
}

class SidebarBrowserTestWithAIChat : public SidebarBrowserTest {
 public:
  SidebarBrowserTestWithAIChat() {
    feature_list_.InitAndEnableFeature(ai_chat::features::kAIChat);
  }
  ~SidebarBrowserTestWithAIChat() override = default;

  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(SidebarBrowserTestWithAIChat, TabSpecificPanel) {
  // Collect item indexes for test
  constexpr auto kGlobalItemType = SidebarItem::BuiltInItemType::kBookmarks;
  constexpr auto kTabSpecificItemType = SidebarItem::BuiltInItemType::kChatUI;
  auto global_item_index = model()->GetIndexOf(kGlobalItemType);
  ASSERT_TRUE(global_item_index.has_value());
  auto tab_specific_item_index = model()->GetIndexOf(kTabSpecificItemType);
  ASSERT_TRUE(tab_specific_item_index.has_value());
  // Open 2 more tabs
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("brave://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("brave://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  ASSERT_EQ(tab_model()->GetTabCount(), 3);
  // Open a "global" panel from Tab 0
  tab_model()->ActivateTabAt(0);
  SimulateSidebarItemClickAt(global_item_index.value());
  // Open a "tab specific" panel from Tab 1
  tab_model()->ActivateTabAt(1);
  SimulateSidebarItemClickAt(tab_specific_item_index.value());
  // Tab Specific panel should be open when Tab 1 is active
  EXPECT_EQ(model()->active_index(), tab_specific_item_index);
  // Global panel should be open when Tab 0 is active
  tab_model()->ActivateTabAt(0);
  EXPECT_EQ(model()->active_index(), global_item_index);
  // Global panel should be open when Tab 2 is active
  tab_model()->ActivateTabAt(2);
  EXPECT_EQ(model()->active_index(), global_item_index);
}

class SidebarBrowserTestWithVerticalTabs : public SidebarBrowserTest {
 public:
  SidebarBrowserTestWithVerticalTabs() {
    feature_list_.InitAndEnableFeature(tabs::features::kBraveVerticalTabs);
  }
  ~SidebarBrowserTestWithVerticalTabs() override = default;

  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(SidebarBrowserTestWithVerticalTabs,
                       SidebarRightSideTest) {
  // Sidebar is on left by default
  EXPECT_TRUE(IsSidebarUIOnLeft());

  brave::ToggleVerticalTabStrip(browser());
  ASSERT_TRUE(tabs::utils::ShouldShowVerticalTabs(browser()));

  auto* prefs = browser()->profile()->GetPrefs();

  auto* vertical_tabs_container = GetVerticalTabsContainer();
  auto* sidebar_container =
      static_cast<SidebarContainerView*>(controller()->sidebar());

  // Sidebar will be moved to the right when enabling vertical tab strip.
  EXPECT_FALSE(IsSidebarUIOnLeft());

  // Check if vertical tabs is located at first and sidebar is located on the
  // right side.
  EXPECT_LT(vertical_tabs_container->GetBoundsInScreen().x(),
            sidebar_container->GetBoundsInScreen().x());

  // Changed to sidebar on left side again.
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, false);
  EXPECT_TRUE(IsSidebarUIOnLeft());

  // Check if vertical tabs is located first and sidebar is following it.
  EXPECT_EQ(vertical_tabs_container->GetBoundsInScreen().right(),
            sidebar_container->GetBoundsInScreen().x());

  // Check sidebar position option is synced between normal and private window.
  auto* private_browser = CreateIncognitoBrowser(browser()->profile());
  auto* private_prefs = private_browser->profile()->GetPrefs();
  EXPECT_EQ(prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment),
            private_prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment));
  EXPECT_FALSE(prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment));
  private_prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, true);
  EXPECT_TRUE(prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment));
}

}  // namespace sidebar
