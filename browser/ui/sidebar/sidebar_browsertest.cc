/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
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
#include "brave/browser/ui/views/side_panel/brave_side_panel_resize_widget.h"
#include "brave/browser/ui/views/sidebar/sidebar_container_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_control_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_items_contents_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_items_scroll_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/browser/ui/views/toolbar/side_panel_button.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/constants/brave_switches.h"
#include "brave/components/playlist/common/features.h"
#include "brave/components/sidebar/browser/constants.h"
#include "brave/components/sidebar/browser/pref_names.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "brave/components/sidebar/common/features.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/search_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock-matchers.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/base/ui_base_features.h"
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

  SidePanelButton* GetSidePanelToolbarButton() const {
    return static_cast<BraveToolbarView*>(
               BrowserView::GetBrowserViewForBrowser(browser())->toolbar())
        ->side_panel_button();
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

  views::Widget* GetSidePanelResizeWidget() {
    return GetSidePanel()->resize_widget_->GetWidget();
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

  SidebarItemsScrollView* GetSidebarItemsScrollView(
      SidebarController* controller) const {
    auto* sidebar_container_view =
        static_cast<SidebarContainerView*>(controller->sidebar());
    auto sidebar_control_view = sidebar_container_view->sidebar_control_view_;
    return sidebar_control_view->sidebar_items_view_;
  }

  // If the item at |index| is panel item, this will return after waiting
  // model's active index is changed as active index could not be not updated
  // synchronously. Panel activation is done via SidePanelCoordinator instead of
  // asking activation to SidebarController directly.
  void SimulateSidebarItemClickAt(size_t index) {
    auto sidebar_items_contents_view =
        GetSidebarItemsContentsView(controller());

    auto* item = sidebar_items_contents_view->children()[index].get();
    DCHECK(item);

    const gfx::Point origin(0, 0);
    ui::MouseEvent event(ui::EventType::kMousePressed, origin, origin,
                         ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON, 0);
    sidebar_items_contents_view->OnItemPressed(item, event);

    if (model()->GetAllSidebarItems()[index].open_in_panel) {
      WaitUntil(base::BindLambdaForTesting(
          [&]() { return model()->active_index() == index; }));
    }
  }

  SidebarControlView* GetSidebarControlView() const {
    return GetSidebarContainerView()->sidebar_control_view_;
  }

  SidebarContainerView* GetSidebarContainerView() const {
    return static_cast<SidebarContainerView*>(controller()->sidebar());
  }

  void CheckOperationFromActiveTabChangedFlagCleared() const {
    EXPECT_FALSE(GetSidebarContainerView()->operation_from_active_tab_change_);
  }

  BraveSidePanel* GetSidePanel() const {
    return GetSidebarContainerView()->side_panel_;
  }

  bool IsSidebarUIOnLeft() const {
    return GetSidebarContainerView()->sidebar_on_left_ &&
           !GetSidePanel()->IsRightAligned() &&
           GetSidebarControlView()->sidebar_on_left_;
  }

  void ShowSidebar(bool show_side_panel) {
    GetSidebarContainerView()->ShowSidebar(show_side_panel);
  }

  void HideSidebar(bool hide_sidebar_control) {
    GetSidebarContainerView()->HideSidebar(hide_sidebar_control);
  }

  void WaitUntil(base::RepeatingCallback<bool()> condition) {
    if (condition.Run()) {
      return;
    }

    base::RepeatingTimer scheduler;
    scheduler.Start(FROM_HERE, base::Milliseconds(100),
                    base::BindLambdaForTesting([this, &condition] {
                      if (condition.Run()) {
                        run_loop_->Quit();
                      }
                    }));
    Run();
  }

  void Run() {
    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop()->Run();
  }

  void SetItemAddedBubbleLaunchedCallback(
      SidebarItemsContentsView* items_contents_view) {
    items_contents_view->item_added_bubble_launched_for_test_ =
        base::BindRepeating(
            &SidebarBrowserTest::ItemAddedBubbleLaunchedCallback,
            weak_factory_.GetWeakPtr());
  }

  void ItemAddedBubbleLaunchedCallback(views::View* anchor) {
    item_added_bubble_anchor_ = anchor;
  }

  void AddItemsTillScrollable(SidebarItemsScrollView* scroll_view,
                              SidebarService* sidebar_service) {
    int url_prefix = 0;
    while (true) {
      sidebar_service->AddItem(sidebar::SidebarItem::Create(
          GURL(base::StrCat(
              {"https://foo/bar_", base::NumberToString(url_prefix)})),
          u"title", SidebarItem::Type::kTypeWeb,
          SidebarItem::BuiltInItemType::kNone, false));
      url_prefix++;
      base::RunLoop().RunUntilIdle();
      // Add items till first item becomes invisible.
      if (scroll_view->NeedScrollForItemAt(0)) {
        break;
      }
    }
  }

  bool NeedScrollForItemAt(size_t index, SidebarItemsScrollView* scroll_view) {
    return scroll_view->NeedScrollForItemAt(index);
  }

  void VerifyTargetDragIndicatorIndexCalc(const gfx::Point& screen_position) {
    auto sidebar_items_contents_view = GetSidebarItemsContentsView(
        static_cast<BraveBrowser*>(browser())->sidebar_controller());
    EXPECT_NE(std::nullopt,
              sidebar_items_contents_view->CalculateTargetDragIndicatorIndex(
                  screen_position));
  }

  base::RunLoop* run_loop() const { return run_loop_.get(); }

  size_t GetDefaultItemCount() const {
    auto item_count =
        std::size(SidebarServiceFactory::kDefaultBuiltInItemTypes) -
        1 /* for history*/;
#if BUILDFLAG(ENABLE_PLAYLIST)
    if (!base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
      item_count -= 1;
    }
#endif

    if (!ai_chat::features::IsAIChatEnabled()) {
      item_count -= 1;
    }

    return item_count;
  }

  int GetFirstPanelItemIndex() {
    auto const items = model()->GetAllSidebarItems();
    auto const iter =
        base::ranges::find(items, true, &SidebarItem::open_in_panel);
    return std::distance(items.cbegin(), iter);
  }

  int GetFirstWebItemIndex() {
    const auto items = model()->GetAllSidebarItems();
    auto const iter =
        base::ranges::find(items, false, &SidebarItem::open_in_panel);
    return std::distance(items.cbegin(), iter);
  }

  raw_ptr<views::View, DanglingUntriaged> item_added_bubble_anchor_ = nullptr;
  std::unique_ptr<base::RunLoop> run_loop_;
  base::WeakPtrFactory<SidebarBrowserTest> weak_factory_{this};
};

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, BasicTest) {
  EXPECT_TRUE(!!GetSidePanelToolbarButton()->context_menu_controller());

  // Initially, active index is not set.
  EXPECT_THAT(model()->active_index(), Eq(std::nullopt));

  // Check sidebar UI is initalized properly.
  EXPECT_TRUE(!!controller()->sidebar());

  browser()->command_controller()->ExecuteCommand(IDC_TOGGLE_SIDEBAR);
  WaitUntil(
      base::BindLambdaForTesting([&]() { return !!model()->active_index(); }));
  // Check active index is non-null.
  EXPECT_THAT(model()->active_index(), Ne(std::nullopt));

  browser()->command_controller()->ExecuteCommand(IDC_TOGGLE_SIDEBAR);
  WaitUntil(
      base::BindLambdaForTesting([&]() { return !model()->active_index(); }));
  // Check active index is null.
  EXPECT_THAT(model()->active_index(), Eq(std::nullopt));

  auto expected_count = GetDefaultItemCount();
  EXPECT_EQ(expected_count, model()->GetAllSidebarItems().size());
  // Activate item that opens in panel.
  const size_t first_panel_item_index = GetFirstPanelItemIndex();
  const auto& first_panel_item =
      controller()->model()->GetAllSidebarItems()[first_panel_item_index];
  controller()->ActivatePanelItem(first_panel_item.built_in_item_type);
  WaitUntil(
      base::BindLambdaForTesting([&]() { return !!model()->active_index(); }));
  EXPECT_THAT(model()->active_index(), Optional(first_panel_item_index));
  EXPECT_TRUE(controller()->IsActiveIndex(first_panel_item_index));

  // Get first index of item that opens in panel.
  const size_t first_web_item_index = GetFirstWebItemIndex();
  const auto item = model()->GetAllSidebarItems()[first_web_item_index];
  EXPECT_FALSE(item.open_in_panel);
  controller()->ActivateItemAt(first_web_item_index);
  int active_item_index = first_panel_item_index;
  EXPECT_THAT(model()->active_index(), Optional(active_item_index));

  // Setting std::nullopt means deactivate current active tab.
  controller()->DeactivateCurrentPanel();
  WaitUntil(
      base::BindLambdaForTesting([&]() { return !model()->active_index(); }));
  EXPECT_THAT(model()->active_index(), Eq(std::nullopt));

  controller()->ActivatePanelItem(first_panel_item.built_in_item_type);
  WaitUntil(
      base::BindLambdaForTesting([&]() { return !!model()->active_index(); }));
  EXPECT_THAT(model()->active_index(), Optional(active_item_index));

  auto* sidebar_service =
      SidebarServiceFactory::GetForProfile(browser()->profile());

  // Move active item to the next index to make sure it's not the first item.
  sidebar_service->MoveItem(first_panel_item_index, first_panel_item_index + 1);
  active_item_index++;
  EXPECT_THAT(model()->active_index(), Eq(active_item_index));

  // Remove Item at index 0 to change active index.
  sidebar_service->RemoveItemAt(0);
  active_item_index--;
  EXPECT_EQ(--expected_count, model()->GetAllSidebarItems().size());
  EXPECT_THAT(model()->active_index(), Optional(active_item_index));

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
  auto expected_count = GetDefaultItemCount();
  EXPECT_EQ(expected_count, model()->GetAllSidebarItems().size());

  // Add an item
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("brave://settings/")));
  int current_tab_index = tab_model()->active_index();
  EXPECT_EQ(0, current_tab_index);
  EXPECT_TRUE(CanAddCurrentActiveTabToSidebar(browser()));
  controller()->AddItemWithCurrentTab();
  // Verify new size
  EXPECT_EQ(++expected_count, model()->GetAllSidebarItems().size());

  // Load NTP in a new tab and activate it. (tab index 1)
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("brave://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  current_tab_index = tab_model()->active_index();
  EXPECT_EQ(1, current_tab_index);

  // Activate sidebar item(brave://settings) and check existing first tab is
  // activated.
  auto items = model()->GetAllSidebarItems();
  auto iter =
      base::ranges::find(items, GURL("chrome://settings/"), &SidebarItem::url);
  EXPECT_NE(items.end(), iter);
  controller()->ActivateItemAt(std::distance(items.begin(), iter));
  EXPECT_EQ(0, tab_model()->active_index());
  EXPECT_EQ(tab_model()->GetWebContentsAt(0)->GetVisibleURL(), iter->url);

  // Activate second sidebar item(wallet) and check it's loaded at current tab.
  iter = base::ranges::find(items, SidebarItem::BuiltInItemType::kWallet,
                            &SidebarItem::built_in_item_type);
  EXPECT_NE(items.end(), iter);
  controller()->ActivateItemAt(std::distance(items.begin(), iter));
  EXPECT_EQ(0, tab_model()->active_index());
  EXPECT_EQ(tab_model()->GetWebContentsAt(0)->GetVisibleURL(), iter->url);
  // New tab is not created.
  EXPECT_EQ(2, tab_model()->count());
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, IterateBuiltInWebTypeTest) {
  // Click builtin wallet item and it's loaded at current active tab.
  const auto items = model()->GetAllSidebarItems();
  const auto wallet_item_iter =
      base::ranges::find(items, SidebarItem::BuiltInItemType::kWallet,
                         &SidebarItem::built_in_item_type);
  ASSERT_NE(wallet_item_iter, items.cend());
  const int wallet_item_index = std::distance(items.cbegin(), wallet_item_iter);
  auto wallet_item = model()->GetAllSidebarItems()[wallet_item_index];
  EXPECT_FALSE(controller()->DoesBrowserHaveOpenedTabForItem(wallet_item));
  SimulateSidebarItemClickAt(wallet_item_index);
  EXPECT_TRUE(controller()->DoesBrowserHaveOpenedTabForItem(wallet_item));
  EXPECT_EQ(0, tab_model()->active_index());
  EXPECT_EQ(tab_model()->GetWebContentsAt(0)->GetVisibleURL().host(),
            wallet_item.url.host());

  // Create NTP and click wallet item. Then wallet tab(index 0) is activated.
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("brave://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  // NTP is active tab.
  EXPECT_EQ(1, tab_model()->active_index());
  SimulateSidebarItemClickAt(wallet_item_index);
  // Wallet tab is active tab.
  EXPECT_EQ(0, tab_model()->active_index());
  EXPECT_EQ(tab_model()->GetWebContentsAt(0)->GetVisibleURL().host(),
            wallet_item.url.host());

  // Create NTP.
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("brave://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  // NTP is active tab and load wallet on it.
  EXPECT_EQ(2, tab_model()->active_index());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), wallet_item.url));

  // Click wallet item and then first wallet tab(tab index 0) is activated.
  SimulateSidebarItemClickAt(wallet_item_index);
  EXPECT_EQ(0, tab_model()->active_index());

  // Click wallet item and then second wallet tab(index 2) is activated.
  SimulateSidebarItemClickAt(wallet_item_index);
  EXPECT_EQ(2, tab_model()->active_index());

  // Click wallet item and then first wallet tab(index 0) is activated.
  SimulateSidebarItemClickAt(wallet_item_index);
  EXPECT_EQ(0, tab_model()->active_index());

  // Checking windows' activation state is flaky in browser tests.
#if !BUILDFLAG(IS_MAC)
  auto* browser2 = CreateBrowser(browser()->profile());
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return browser2->window()->IsActive(); }));

  // |browser2| doesn't have any wallet tab. So, clicking wallet sidebar item
  // activates other browser's first wallet tab.
  static_cast<BraveBrowser*>(browser2)->sidebar_controller()->ActivateItemAt(
      wallet_item_index);

  // Wait till browser() is activated.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return browser()->window()->IsActive(); }));

  EXPECT_EQ(0, tab_model()->active_index());
#endif
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, PRE_LastlyUsedSidePanelItemTest) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  panel_ui->Show(SidePanelEntryId::kBookmarks);

  // Wait till panel UI opens.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return GetSidePanel()->GetVisible(); }));

  // Check bookmarks panel is shown.
  auto bookmark_item_index =
      model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks);
  ASSERT_TRUE(bookmark_item_index.has_value());
  EXPECT_TRUE(controller()->IsActiveIndex(bookmark_item_index));
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, LastlyUsedSidePanelItemTest) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  panel_ui->Toggle();

  // Wait till panel UI opens.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return GetSidePanel()->GetVisible(); }));

  // Check bookmarks item is opened after toggle.
  auto bookmark_item_index =
      model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks);
  ASSERT_TRUE(bookmark_item_index.has_value());
  EXPECT_TRUE(controller()->IsActiveIndex(bookmark_item_index));
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, DefaultEntryTest) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  auto bookmark_item_index =
      model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks);
  panel_ui->Show(SidePanelEntryId::kBookmarks);

  // Wait till bookmark panel is activated.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return controller()->IsActiveIndex(bookmark_item_index); }));

  panel_ui->Close();
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !panel_ui->GetCurrentEntryId().has_value(); }));

  // Remove bookmarks and check it's gone.
  SidebarServiceFactory::GetForProfile(browser()->profile())
      ->RemoveItemAt(*bookmark_item_index);
  EXPECT_FALSE(!!model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks));

  // Open panel w/o entry id.
  panel_ui->Toggle();
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return panel_ui->GetCurrentEntryId().has_value(); }));
  // Check bookmark panel is not opened again as it's deleted item.
  EXPECT_NE(SidePanelEntryId::kBookmarks, panel_ui->GetCurrentEntryId());
}

// Test sidebar's initial horizontal option is set properly.
IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, PRE_InitialHorizontalOptionTest) {
  auto* prefs = browser()->profile()->GetPrefs();

  // Check default horizontal option is right-sided.
  EXPECT_TRUE(prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment));
  EXPECT_FALSE(IsSidebarUIOnLeft());

  // Set left-sided for next testing.
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, false);
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, InitialHorizontalOptionTest) {
  auto* prefs = browser()->profile()->GetPrefs();

  // Check horizontal option is right-sided.
  EXPECT_FALSE(prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment));
  EXPECT_TRUE(IsSidebarUIOnLeft());
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, ItemDragIndicatorCalcTest) {
  auto sidebar_items_contents_view = GetSidebarItemsContentsView(
      static_cast<BraveBrowser*>(browser())->sidebar_controller());
  gfx::Rect contents_view_rect = sidebar_items_contents_view->GetLocalBounds();
  views::View::ConvertRectToScreen(sidebar_items_contents_view,
                                   &contents_view_rect);
  gfx::Point screen_position = contents_view_rect.origin();
  screen_position.Offset(5, 0);

  // Any point from items contents view should have proper drag indicator index.
  for (int i = 0; i < contents_view_rect.height(); ++i) {
    gfx::Point simulated_mouse_drag_point = screen_position;
    simulated_mouse_drag_point.Offset(0, i);
    VerifyTargetDragIndicatorIndexCalc(simulated_mouse_drag_point);
  }
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

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, HideSidebarUITest) {
  auto* service = SidebarServiceFactory::GetForProfile(browser()->profile());
  auto* sidebar_container = GetSidebarContainerView();

  // Set to on mouse over and check sidebar ui is not shown.
  service->SetSidebarShowOption(
      SidebarService::ShowSidebarOption::kShowOnMouseOver);
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return sidebar_container->width() == 0; }));

  // Ask to show sidebar ui and check it's shown.
  ShowSidebar(false);
  const int target_control_view_width =
      GetSidebarControlView()->GetPreferredSize().width();
  EXPECT_GT(target_control_view_width, 0);
  WaitUntil(base::BindLambdaForTesting([&]() {
    return sidebar_container->width() == target_control_view_width;
  }));

  // Ask to hide sidebar ui and check it's not shown.
  HideSidebar(true);
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return sidebar_container->width() == 0; }));
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, ItemAddedBubbleAnchorViewTest) {
  auto* sidebar_service =
      SidebarServiceFactory::GetForProfile(browser()->profile());
  auto sidebar_items_contents_view = GetSidebarItemsContentsView(
      static_cast<BraveBrowser*>(browser())->sidebar_controller());
  SetItemAddedBubbleLaunchedCallback(sidebar_items_contents_view);
  size_t lastly_added_item_index = 0;

  // Add item at last.
  item_added_bubble_anchor_ = nullptr;
  sidebar_service->AddItem(sidebar::SidebarItem::Create(
      GURL("http://foo.bar/"), u"title", SidebarItem::Type::kTypeWeb,
      SidebarItem::BuiltInItemType::kNone, false));

  // Check item is added at last and check that bubble is anchored to
  // it properly.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !!item_added_bubble_anchor_; }));
  lastly_added_item_index = sidebar_items_contents_view->children().size() - 1;
  EXPECT_EQ(item_added_bubble_anchor_,
            sidebar_items_contents_view->children()[lastly_added_item_index]);

  // Add item at index 0.
  item_added_bubble_anchor_ = nullptr;
  sidebar_service->AddItemAtForTesting(
      sidebar::SidebarItem::Create(GURL("http://foo.bar/"), u"title",
                                   SidebarItem::Type::kTypeWeb,
                                   SidebarItem::BuiltInItemType::kNone, false),
      0);

  // Check item is added at first and check that bubble is anchored to
  // it properly.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !!item_added_bubble_anchor_; }));
  lastly_added_item_index = 0;
  EXPECT_EQ(item_added_bubble_anchor_,
            sidebar_items_contents_view->children()[lastly_added_item_index]);
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, ItemActivatedScrollTest) {
  // To prevent item added bubble launching.
  auto* prefs = browser()->profile()->GetPrefs();
  prefs->SetInteger(sidebar::kSidebarItemAddedFeedbackBubbleShowCount, 3);

  auto bookmark_item_index =
      model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks);
  ASSERT_TRUE(bookmark_item_index.has_value());

  auto* sidebar_service =
      SidebarServiceFactory::GetForProfile(browser()->profile());
  auto* scroll_view = GetSidebarItemsScrollView(
      static_cast<BraveBrowser*>(browser())->sidebar_controller());

  // Move bookmark item at zero index to make it hidden.
  sidebar_service->MoveItem(*bookmark_item_index, 0);
  bookmark_item_index = 0;
  AddItemsTillScrollable(scroll_view, sidebar_service);

  // Check bookmarks item is hidden.
  EXPECT_TRUE(NeedScrollForItemAt(*bookmark_item_index, scroll_view));

  // Open bookmark panel.
  browser()->GetFeatures().side_panel_ui()->Show(SidePanelEntryId::kBookmarks);

  // Wait till bookmarks item is visible.
  WaitUntil(base::BindLambdaForTesting([&]() {
    return !NeedScrollForItemAt(*bookmark_item_index, scroll_view);
  }));
  EXPECT_TRUE(controller()->IsActiveIndex(bookmark_item_index));
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, ItemAddedScrollTest) {
  // To prevent item added bubble launching.
  auto* prefs = browser()->profile()->GetPrefs();
  prefs->SetInteger(sidebar::kSidebarItemAddedFeedbackBubbleShowCount, 3);

  auto* sidebar_service =
      SidebarServiceFactory::GetForProfile(browser()->profile());
  auto* scroll_view = GetSidebarItemsScrollView(
      static_cast<BraveBrowser*>(browser())->sidebar_controller());
  auto sidebar_items_contents_view = GetSidebarItemsContentsView(
      static_cast<BraveBrowser*>(browser())->sidebar_controller());

  AddItemsTillScrollable(scroll_view, sidebar_service);

  // Check first item is not visible in scroll view.
  EXPECT_TRUE(NeedScrollForItemAt(0, scroll_view));

  // After inserting item at index 0, it should be visible as sidebar
  // scrolls to make that new item visible. So last item becomes invisible.
  sidebar_service->AddItemAtForTesting(
      sidebar::SidebarItem::Create(GURL("https://abcd"), u"title",
                                   SidebarItem::Type::kTypeWeb,
                                   SidebarItem::BuiltInItemType::kNone, false),
      0);
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !NeedScrollForItemAt(0, scroll_view); }));

  int last_item_index = sidebar_items_contents_view->children().size() - 1;
  EXPECT_TRUE(NeedScrollForItemAt(last_item_index, scroll_view));

  // After inserting item at last, it should be visible as sidebar
  // scrolls to make that new item visible. So fisrt item becomes invisible.
  last_item_index++;
  sidebar_service->AddItem(sidebar::SidebarItem::Create(
      GURL("https://abcdefg"), u"title", SidebarItem::Type::kTypeWeb,
      SidebarItem::BuiltInItemType::kNone, false));
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !NeedScrollForItemAt(last_item_index, scroll_view); }));
  EXPECT_TRUE(NeedScrollForItemAt(0, scroll_view));
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, PRE_PrefsMigrationTest) {
  // Prepare temporarily changed condition.
  auto* prefs = browser()->profile()->GetPrefs();
  prefs->SetBoolean(sidebar::kSidebarAlignmentChangedTemporarily, true);
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, true);
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, PrefsMigrationTest) {
  // Check all prefs are changed to default.
  auto* prefs = browser()->profile()->GetPrefs();
  EXPECT_TRUE(prefs->FindPreference(prefs::kSidePanelHorizontalAlignment)
                  ->IsDefaultValue());
  EXPECT_TRUE(prefs->FindPreference(prefs::kSidePanelHorizontalAlignment)
                  ->IsDefaultValue());
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, SidePanelResizeTest) {
  auto* prefs = browser()->profile()->GetPrefs();
  EXPECT_EQ(kDefaultSidePanelWidth,
            prefs->GetInteger(sidebar::kSidePanelWidth));

  browser()->command_controller()->ExecuteCommand(IDC_TOGGLE_SIDEBAR);

  int expected_panel_width = kDefaultSidePanelWidth;

  // Wait till sidebar animation ends.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return GetSidePanel()->width() == expected_panel_width; }));

  // Test smaller panel width than default(minimum) and check smaller than
  // default is not applied. Positive offset value is for reducing width in
  // right-sided sidebar.
  GetSidePanel()->OnResize(30, true);
  // Check panel width is not changed.
  EXPECT_EQ(expected_panel_width, prefs->GetInteger(sidebar::kSidePanelWidth));
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return GetSidePanel()->width() == kDefaultSidePanelWidth; }));

  // On right-side sidebar position, side panel's x and resize widget's x is
  // same.
  EXPECT_EQ(GetSidePanel()->GetBoundsInScreen().x(),
            GetSidePanelResizeWidget()->GetWindowBoundsInScreen().x());

  // Increase panel width and check resize handle widget's position.
  // Negative offset value is for increasing width in right-sided
  // sidebar.
  GetSidePanel()->OnResize(-20, true);
  expected_panel_width += 20;
  EXPECT_EQ(expected_panel_width, prefs->GetInteger(sidebar::kSidePanelWidth));
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return GetSidePanel()->width() == expected_panel_width; }));
  EXPECT_EQ(GetSidePanel()->GetBoundsInScreen().x(),
            GetSidePanelResizeWidget()->GetWindowBoundsInScreen().x());

  // Set sidebar on left side.
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, false);
  EXPECT_EQ(GetSidePanel()->GetBoundsInScreen().right(),
            GetSidePanelResizeWidget()->GetWindowBoundsInScreen().right());

  // Increase panel width and check width and resize handle position.
  // Positive offset value is for increasing width in left-sided sidebar.
  GetSidePanel()->OnResize(20, true);
  expected_panel_width += 20;
  EXPECT_EQ(expected_panel_width, prefs->GetInteger(sidebar::kSidePanelWidth));
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return GetSidePanel()->width() == expected_panel_width; }));
  EXPECT_EQ(GetSidePanel()->GetBoundsInScreen().right(),
            GetSidePanelResizeWidget()->GetWindowBoundsInScreen().right());

  // Close side panel.
  browser()->command_controller()->ExecuteCommand(IDC_TOGGLE_SIDEBAR);
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !GetSidePanel()->GetVisible(); }));

  // Re-open side panel and check it's opened as wide as lastly used width.
  browser()->command_controller()->ExecuteCommand(IDC_TOGGLE_SIDEBAR);
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return GetSidePanel()->width() == expected_panel_width; }));
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, UnManagedPanelEntryTest) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();

  // Show bookmarks entry and it has active index.
  panel_ui->Show(SidePanelEntryId::kBookmarks);
  // Wait till sidebar show ends.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return GetSidePanel()->width() == kDefaultSidePanelWidth; }));
  EXPECT_TRUE(model()->active_index().has_value());

  // Cache bookmarks entry index to remove it later.
  const auto bookmark_item_index = model()->active_index().value();

  // Close panel and wait till panel closing animation ends. Panel is hidden
  // when closing completes.
  panel_ui->Close();
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !GetSidePanel()->GetVisible(); }));
  EXPECT_FALSE(!!panel_ui->GetCurrentEntryId());

  // Remove bookmarks and check it's gone.
  SidebarServiceFactory::GetForProfile(browser()->profile())
      ->RemoveItemAt(bookmark_item_index);
  EXPECT_FALSE(!!model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks));

  // Show bookmarks entry again and wait till sidebar panel gets visible
  panel_ui->Show(SidePanelEntryId::kBookmarks);
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return GetSidePanel()->GetVisible(); }));
  EXPECT_EQ(SidePanelEntryId::kBookmarks, panel_ui->GetCurrentEntryId());
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest,
                       OpenUnManagedPanelAfterDeletingDefaultWebTypeItem) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  const auto items = model()->GetAllSidebarItems();
  const auto wallet_item_iter =
      base::ranges::find(items, SidebarItem::BuiltInItemType::kWallet,
                         &SidebarItem::built_in_item_type);
  ASSERT_NE(wallet_item_iter, items.cend());
  const int wallet_item_index = std::distance(items.cbegin(), wallet_item_iter);
  SidebarServiceFactory::GetForProfile(browser()->profile())
      ->RemoveItemAt(wallet_item_index);
  EXPECT_FALSE(!!model()->GetIndexOf(SidebarItem::BuiltInItemType::kWallet));

  // Test with upstream's side panel that runs only with chrome://new-tab-page.
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("chrome://new-tab-page/")));
  panel_ui->Show(SidePanelEntryId::kCustomizeChrome);
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return GetSidePanel()->GetVisible(); }));
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, TabSpecificAndGlobalPanelsTest) {
  // Create another tab.
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("brave://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));

  auto* panel_ui = browser()->GetFeatures().side_panel_ui();

  // Open contextual panel to tab at 0.
  tab_model()->ActivateTabAt(0);
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("chrome://new-tab-page/")));
  panel_ui->Show(SidePanelEntryId::kCustomizeChrome);
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return GetSidePanel()->GetVisible(); }));

  // No panel when activated tab at 1 because we don't open any global panel.
  tab_model()->ActivateTabAt(1);
  EXPECT_FALSE(panel_ui->IsSidePanelShowing());

  // Open global panel when active tab indext is 1.
  panel_ui->Show(SidePanelEntryId::kBookmarks);
  WaitUntil(base::BindLambdaForTesting([&]() {
    return panel_ui->GetCurrentEntryId() == SidePanelEntryId::kBookmarks;
  }));

  // Contextual panel should be set when activate tab at 0.
  tab_model()->ActivateTabAt(0);
  WaitUntil(base::BindLambdaForTesting([&]() {
    return panel_ui->GetCurrentEntryId() == SidePanelEntryId::kCustomizeChrome;
  }));

  // Global panel should be set when activate tab at 1.
  tab_model()->ActivateTabAt(1);
  WaitUntil(base::BindLambdaForTesting([&]() {
    return panel_ui->GetCurrentEntryId() == SidePanelEntryId::kBookmarks;
  }));

  // Check per-url contextual panel close.
  // If current tab load another url, customize panel should be hidden.
  tab_model()->ActivateTabAt(0);
  WaitUntil(base::BindLambdaForTesting([&]() {
    return panel_ui->GetCurrentEntryId() == SidePanelEntryId::kCustomizeChrome;
  }));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("brave://newtab/")));

  // After loading another url, customize panel is deregistered and closed.
  EXPECT_FALSE(panel_ui->GetCurrentEntryId());
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, DisabledItemsTest) {
  auto* guest_browser = static_cast<BraveBrowser*>(CreateGuestBrowser());
  auto* controller = guest_browser->sidebar_controller();
  auto* model = controller->model();
  for (const auto& item : model->GetAllSidebarItems()) {
    // Check disabled builtin items are not included in guest browser's items
    // list.
    if (IsBuiltInType(item)) {
      EXPECT_FALSE(IsDisabledItemForGuest(item.built_in_item_type));
    }
  }

  auto* private_browser =
      static_cast<BraveBrowser*>(CreateIncognitoBrowser(browser()->profile()));
  controller = private_browser->sidebar_controller();
  model = controller->model();
  for (const auto& item : model->GetAllSidebarItems()) {
    // Check disabled builtin items are not included in private browser's items
    // list.
    if (IsBuiltInType(item)) {
      EXPECT_FALSE(IsDisabledItemForPrivate(item.built_in_item_type));
    }
  }
}

class MockSidebarModelObserver : public SidebarModel::Observer {
 public:
  MockSidebarModelObserver() = default;
  ~MockSidebarModelObserver() override = default;

  MOCK_METHOD(void,
              OnItemAdded,
              (const SidebarItem& item, size_t index, bool user_gesture),
              (override));
  MOCK_METHOD(void,
              OnItemMoved,
              (const SidebarItem& item, size_t from, size_t to),
              (override));
  MOCK_METHOD(void, OnItemRemoved, (size_t index), (override));
  MOCK_METHOD(void,
              OnActiveIndexChanged,
              (std::optional<size_t> old_index,
               std::optional<size_t> new_index),
              (override));
  MOCK_METHOD(void,
              OnItemUpdated,
              (const SidebarItem& item, const SidebarItemUpdate& update),
              (override));
  MOCK_METHOD(void,
              OnFaviconUpdatedForItem,
              (const SidebarItem& item, const gfx::ImageSkia& image),
              (override));
};

class SidebarBrowserTestWithkSidebarShowAlwaysOnStable
    : public testing::WithParamInterface<bool>,
      public SidebarBrowserTest {
 public:
  SidebarBrowserTestWithkSidebarShowAlwaysOnStable() {
    if (GetParam()) {
      feature_list_.InitAndEnableFeatureWithParameters(
          sidebar::features::kSidebarShowAlwaysOnStable,
          {{"open_one_shot_leo_panel", "true"}});
    } else {
      feature_list_.InitAndEnableFeature(
          sidebar::features::kSidebarShowAlwaysOnStable);
    }
  }
  ~SidebarBrowserTestWithkSidebarShowAlwaysOnStable() override = default;

  void SetUp() override { SidebarBrowserTest::SetUp(); }

  void TearDown() override { SidebarBrowserTest::TearDown(); }

  // For skipping SidebarBrowserTest's PreRunTestOnMainThread
  // as show option is set explicitely there.
  void PreRunTestOnMainThread() override {
    InProcessBrowserTest::PreRunTestOnMainThread();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    SidebarBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kDontShowSidebarOnNonStable);
    command_line->AppendSwitch(switches::kForceFirstRun);
  }

  base::test::ScopedFeatureList feature_list_;
  testing::NiceMock<MockSidebarModelObserver> observer_;
  base::ScopedObservation<SidebarModel, SidebarModel::Observer> observation_{
      &observer_};
};

IN_PROC_BROWSER_TEST_P(SidebarBrowserTestWithkSidebarShowAlwaysOnStable,
                       SidebarShowAlwaysTest) {
  auto* sidebar_service =
      SidebarServiceFactory::GetForProfile(browser()->profile());
  EXPECT_EQ(SidebarService::ShowSidebarOption::kShowAlways,
            sidebar_service->GetSidebarShowOption());

  observation_.Observe(model());

  // Check one shot Leo panel is opened or not based on test parameter.
  if (GetParam()) {
    // If Leo panel is opened, panel active index is changed.
    EXPECT_CALL(observer_, OnActiveIndexChanged(testing::_, testing::_))
        .Times(1);
  } else {
    EXPECT_CALL(observer_, OnActiveIndexChanged(testing::_, testing::_))
        .Times(0);
  }

  // Need to make sure template url service is loaded before testing one-shot
  // leo panel open. As we don't want to show one-shot leo panel open for
  // SERP, SidebarTabHelper uses template url service for checking current
  // url is SERP page or not. If template url service is not loaded, it just
  // does early return w/o opening leo panel.
  // See SidebarTabHelper::PrimaryPageChanged() for more details.
  auto* service =
      TemplateURLServiceFactory::GetForProfile(browser()->profile());
  search_test_utils::WaitForTemplateURLServiceToLoad(service);

  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("https://www.brave.com/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));

  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  if (GetParam()) {
    // Wait till browser has active panel.
    WaitUntil(base::BindLambdaForTesting(
        [&]() { return !!panel_ui->GetCurrentEntryId(); }));

    EXPECT_EQ(SidePanelEntryId::kChatUI, panel_ui->GetCurrentEntryId());
  }
  testing::Mock::VerifyAndClearExpectations(&observer_);

  panel_ui->Close();
  EXPECT_FALSE(panel_ui->IsSidePanelShowing());

  // Check one shot panel is not opened anymore.
  EXPECT_CALL(observer_, OnActiveIndexChanged(testing::_, testing::_)).Times(0);
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("https://www.brave.com/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  EXPECT_FALSE(panel_ui->IsSidePanelShowing());
  testing::Mock::VerifyAndClearExpectations(&observer_);

  observation_.Reset();
}

INSTANTIATE_TEST_SUITE_P(
    /* no prefix */,
    SidebarBrowserTestWithkSidebarShowAlwaysOnStable,
    ::testing::Bool());

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
  auto* private_browser =
      static_cast<BraveBrowser*>(CreateIncognitoBrowser(browser()->profile()));
  ASSERT_TRUE(private_browser);

  auto* sidebar_service =
      SidebarServiceFactory::GetForProfile(private_browser->profile());
  const auto& items = sidebar_service->items();
  auto iter = base::ranges::find_if(items, [](const auto& item) {
    return item.type == SidebarItem::Type::kTypeBuiltIn &&
           item.built_in_item_type == SidebarItem::BuiltInItemType::kPlaylist;
  });

  // Check playlist item is not included in private window.
  EXPECT_EQ(iter, items.end());

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

  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("brave://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  ASSERT_EQ(tab_model()->GetTabCount(), 2);

  // Open contextual panel from Tab 0.
  tab_model()->ActivateTabAt(0);
  SimulateSidebarItemClickAt(tab_specific_item_index.value());
  EXPECT_EQ(model()->active_index(), tab_specific_item_index);

  // Delete Tab 0 and check model doesn't have active index.
  tab_model()->DetachAndDeleteWebContentsAt(0);
  EXPECT_FALSE(!!model()->active_index());
  ASSERT_EQ(tab_model()->GetTabCount(), 1);

  // Create two more tab for test below.
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
  // Tab changed flag should be cleared after ActivateTabAt() executed.
  CheckOperationFromActiveTabChangedFlagCleared();
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

IN_PROC_BROWSER_TEST_F(SidebarBrowserTestWithAIChat,
                       TabSpecificPanelAndUnManagedPanel) {
  // Collect item indexes for test and remove global item.
  constexpr auto kGlobalItemType = SidebarItem::BuiltInItemType::kBookmarks;
  constexpr auto kTabSpecificItemType = SidebarItem::BuiltInItemType::kChatUI;
  auto global_item_index = model()->GetIndexOf(kGlobalItemType);
  ASSERT_TRUE(global_item_index.has_value());
  SidebarServiceFactory::GetForProfile(browser()->profile())
      ->RemoveItemAt(*global_item_index);
  EXPECT_FALSE(!!model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks));

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

  // Open a unmanaged "global" panel from Tab 0
  tab_model()->ActivateTabAt(0);
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  panel_ui->Show(SidePanelEntryId::kBookmarks);
  // Wait till sidebar show ends.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return GetSidePanel()->width() == kDefaultSidePanelWidth; }));
  // Unmanaged entry becomes managed when its panel is shown
  // and it becomes active item.
  EXPECT_TRUE(model()->active_index().has_value());
  EXPECT_EQ(model()->active_index(),
            model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks));

  // Open a "tab specific" panel from Tab 1
  tab_model()->ActivateTabAt(1);
  SimulateSidebarItemClickAt(tab_specific_item_index.value());
  EXPECT_EQ(SidePanelEntryId::kChatUI, panel_ui->GetCurrentEntryId());
  EXPECT_TRUE(GetSidePanel()->GetVisible());
  // Tab Specific panel should be open when Tab 1 is active
  EXPECT_EQ(model()->active_index(), tab_specific_item_index);

  // Global panel should be open when Tab 0 is active
  tab_model()->ActivateTabAt(0);
  EXPECT_EQ(SidePanelEntryId::kBookmarks, panel_ui->GetCurrentEntryId());
  EXPECT_TRUE(model()->active_index().has_value());
  EXPECT_EQ(model()->active_index(),
            model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks));

  // Global panel should be open when Tab 2 is active
  tab_model()->ActivateTabAt(2);
  EXPECT_EQ(SidePanelEntryId::kBookmarks, panel_ui->GetCurrentEntryId());
  EXPECT_TRUE(model()->active_index().has_value());
  EXPECT_EQ(model()->active_index(),
            model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks));

  // Check tab specific panel item is still activated after moving to another
  // browser.
  tab_model()->ActivateTabAt(1);
  EXPECT_EQ(model()->active_index(), tab_specific_item_index);

  auto* browser2 = CreateBrowser(browser()->profile());
  auto* browser2_model =
      static_cast<BraveBrowser*>(browser2)->sidebar_controller()->model();
  auto* browser2_tab_model = browser2->tab_strip_model();

  auto detached_tab = tab_model()->DetachTabAtForInsertion(1);
  browser2_tab_model->AppendTab(std::move(detached_tab), /* foreground */ true);
  EXPECT_EQ(browser2_model->active_index(),
            browser2_model->GetIndexOf(SidebarItem::BuiltInItemType::kChatUI));
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTestWithAIChat,
                       TabSpecificPanelIdxChange) {
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
  // Move global item
  size_t new_global_item_index = (global_item_index > 0u) ? 0u : 1u;
  SidebarServiceFactory::GetForProfile(browser()->profile())
      ->MoveItem(global_item_index.value(), new_global_item_index);
  tab_specific_item_index = model()->GetIndexOf(kTabSpecificItemType);
  // Tab Specific panel should be open when Tab 1 is active
  EXPECT_EQ(model()->active_index(), tab_specific_item_index);
  // Global panel should be open when Tab 0 is active
  tab_model()->ActivateTabAt(0);
  EXPECT_EQ(model()->active_index(), new_global_item_index);
  // Global panel should be open when Tab 2 is active
  tab_model()->ActivateTabAt(2);
  EXPECT_EQ(model()->active_index(), new_global_item_index);
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, SidebarRightSideTest) {
  // Sidebar is on right by default
  EXPECT_FALSE(IsSidebarUIOnLeft());

  brave::ToggleVerticalTabStrip(browser());
  ASSERT_TRUE(tabs::utils::ShouldShowVerticalTabs(browser()));

  auto* prefs = browser()->profile()->GetPrefs();
  auto* vertical_tabs_container = GetVerticalTabsContainer();
  auto* sidebar_container =
      static_cast<SidebarContainerView*>(controller()->sidebar());

  // Check if vertical tabs is located at first and sidebar is located on the
  // right side.
  EXPECT_LT(vertical_tabs_container->GetBoundsInScreen().x(),
            sidebar_container->GetBoundsInScreen().x());

  // Changed to sidebar on left side.
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
