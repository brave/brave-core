/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <optional>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/i18n/rtl.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/sidebar/buildflags/buildflags.h"
#include "brave/browser/ui/sidebar/features.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/sidebar/sidebar_web_panel_controller.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/frame/split_view/brave_contents_container_view.h"
#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view.h"
#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view_mini_toolbar.h"
#include "brave/browser/ui/views/side_panel/side_panel_resize_widget.h"
#include "brave/browser/ui/views/sidebar/sidebar_container_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_control_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_items_contents_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_items_scroll_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/browser/ui/views/toolbar/side_panel_button.h"
#include "brave/common/pref_names.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_talk/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/constants/brave_switches.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"
#include "brave/components/sidebar/browser/constants.h"
#include "brave/components/sidebar/browser/pref_names.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "brave/components/sidebar/common/features.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/frame/multi_contents_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
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
#include "ui/display/screen.h"
#include "ui/display/test/test_screen.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/gfx/geometry/point.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/features.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "brave/components/playlist/core/common/features.h"
#endif

using ::testing::Eq;
using ::testing::Ne;
using ::testing::Optional;

namespace {
ui::MouseEvent GetDummyEvent() {
  return ui::MouseEvent(ui::EventType::kMouseMoved, gfx::PointF(),
                        gfx::PointF(), base::TimeTicks::Now(), 0, 0);
}
}  // namespace

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

  SidebarModel* model() const { return controller()->model(); }
  TabStripModel* tab_model() const { return browser()->tab_strip_model(); }

  SidebarController* controller() const {
    return browser()->GetFeatures().sidebar_controller();
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

  views::Widget* GetSidePanelResizeWidget() {
#if BUILDFLAG(ENABLE_SIDEBAR_V2)
    NOTREACHED() << "No resize widget in v2";
#else
    return GetSidePanel()->resize_widget_->GetWidget();
#endif
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

  SidePanel* GetSidePanel() const {
    return GetSidebarContainerView()->side_panel_;
  }

  bool IsSidebarUIOnLeft() const {
    if (IsV2Enabled()) {
      return GetSidebarContainerView()->sidebar_on_left_ &&
             GetSidebarControlView()->sidebar_on_left_;
    }

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
    auto sidebar_items_contents_view =
        GetSidebarItemsContentsView(controller());
    EXPECT_NE(std::nullopt,
              sidebar_items_contents_view->CalculateTargetDragIndicatorIndex(
                  screen_position));
  }

  base::RunLoop* run_loop() const { return run_loop_.get(); }

  size_t GetDefaultItemCount() const {
    // kDefaultBuiltInItemTypes.size() already accounts for buildflags
    // (ENABLE_BRAVE_TALK, ENABLE_AI_CHAT) at compile time.
    auto item_count =
        std::size(SidebarServiceFactory::kDefaultBuiltInItemTypes) -
        1 /* for history*/;
#if BUILDFLAG(ENABLE_PLAYLIST)
    if (!base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
      item_count -= 1;
    }
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
    if (!ai_chat::features::IsAIChatEnabled()) {
      item_count -= 1;
    }
#endif

#if !BUILDFLAG(ENABLE_BRAVE_WALLET)
    item_count -= 1;
#endif

    return item_count;
  }

  int GetFirstPanelItemIndex() {
    auto const items = model()->GetAllSidebarItems();
    auto const iter =
        std::ranges::find(items, true, &SidebarItem::open_in_panel);
    return std::distance(items.cbegin(), iter);
  }

  int GetFirstWebItemIndex() {
    const auto items = model()->GetAllSidebarItems();
    auto const iter =
        std::ranges::find(items, false, &SidebarItem::open_in_panel);
    return std::distance(items.cbegin(), iter);
  }

  // ===== V2-aware helpers =====

  bool IsV2Enabled() const {
    return base::FeatureList::IsEnabled(sidebar::features::kSidebarV2);
  }

  raw_ptr<views::View, DanglingUntriaged> item_added_bubble_anchor_ = nullptr;
  std::unique_ptr<base::RunLoop> run_loop_;
  base::WeakPtrFactory<SidebarBrowserTest> weak_factory_{this};
};

// Parameterized test fixture to test both Sidebar V1 and V2.
// Test parameter: bool - false = V1 (default), true = V2 (kSidebarV2 enabled)
//
// Tests using this fixture fall into three categories:
//
// Category A: Tests that work in both V1 and V2 without changes
//   - Tests UI/model/controller logic that's identical across versions
//   - No conditional logic needed (runs same assertions for both V1 and V2)
//
// Category B: Mixed tests with version-specific sections
//   - Tests behavior that differs between V1 and V2
//   - Uses if (IsV2Enabled()) conditionals to handle version-specific
//     assertions
//
// Category C: V1-only tests (skip in V2)
//   - Tests V1-specific functionality that doesn't exist in V2
//   - Uses GTEST_SKIP() to skip when IsV2Enabled() is true
//
// This parameterized test will be removed when V2 is enabled by default.
class SidebarBrowserTestV1AndV2 : public SidebarBrowserTest,
                                  public testing::WithParamInterface<bool> {
 public:
  SidebarBrowserTestV1AndV2() {
    if (GetParam()) {  // true = Enable V2
      scoped_features_.InitAndEnableFeature(sidebar::features::kSidebarV2);
    } else {  // false = V1 (default behavior)
      scoped_features_.InitAndDisableFeature(sidebar::features::kSidebarV2);
    }
  }
  ~SidebarBrowserTestV1AndV2() override = default;

 private:
  base::test::ScopedFeatureList scoped_features_;
};

// Category B: Mixed test (panel-specific sections guarded with conditionals)
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2, BasicTest) {
  EXPECT_TRUE(!!GetSidePanelToolbarButton()->context_menu_controller());

  // Initially, active index is not set.
  EXPECT_THAT(model()->active_index(), Eq(std::nullopt));

  // Check sidebar UI is initalized properly.
  EXPECT_TRUE(!!controller()->sidebar());

  // IDC_TOGGLE_SIDEBAR command doesn't work for V2 now.
  // In V1, SidebarContainerView listens panel open/close event.
  // In V2, SidebarController will do that.
  if (!IsV2Enabled()) {
    browser()->command_controller()->ExecuteCommand(IDC_TOGGLE_SIDEBAR);
    WaitUntil(base::BindLambdaForTesting(
        [&]() { return !!model()->active_index(); }));
    // Check active index is non-null.
    EXPECT_THAT(model()->active_index(), Ne(std::nullopt));

    browser()->command_controller()->ExecuteCommand(IDC_TOGGLE_SIDEBAR);
    WaitUntil(
        base::BindLambdaForTesting([&]() { return !model()->active_index(); }));
    // Check active index is null.
    EXPECT_THAT(model()->active_index(), Eq(std::nullopt));
  }

  auto expected_count = GetDefaultItemCount();
  EXPECT_EQ(expected_count, model()->GetAllSidebarItems().size());

  // Activate item that opens in panel.
  const size_t first_panel_item_index = GetFirstPanelItemIndex();
  const auto& first_panel_item =
      controller()->model()->GetAllSidebarItems()[first_panel_item_index];

  if (IsV2Enabled()) {
    controller()->ActivateItemAt(
        model()->GetIndexOf(first_panel_item.built_in_item_type));
  } else {
    controller()->ActivatePanelItem(first_panel_item.built_in_item_type);
  }
  WaitUntil(
      base::BindLambdaForTesting([&]() { return !!model()->active_index(); }));
  EXPECT_THAT(model()->active_index(), Optional(first_panel_item_index));
  EXPECT_TRUE(controller()->IsActiveIndex(first_panel_item_index));

  // Get first index of item that opens in a new tab (not panel).
  // Note: Web-type items (kBraveTalk, kWallet) may not exist if their
  // respective features are disabled.
  const size_t first_web_item_index = GetFirstWebItemIndex();
  int active_item_index = first_panel_item_index;
  if (first_web_item_index < model()->GetAllSidebarItems().size()) {
    const auto item = model()->GetAllSidebarItems()[first_web_item_index];
    EXPECT_FALSE(item.open_in_panel);
    controller()->ActivateItemAt(first_web_item_index);
  }
  EXPECT_THAT(model()->active_index(), Optional(active_item_index));

  // Setting std::nullopt means deactivate current active tab.
  if (IsV2Enabled()) {
    controller()->ActivateItemAt(std::nullopt);
  } else {
    controller()->DeactivateCurrentPanel();
  }
  WaitUntil(
      base::BindLambdaForTesting([&]() { return !model()->active_index(); }));
  EXPECT_THAT(model()->active_index(), Eq(std::nullopt));

  if (IsV2Enabled()) {
    controller()->ActivateItemAt(
        model()->GetIndexOf(first_panel_item.built_in_item_type));
  } else {
    controller()->ActivatePanelItem(first_panel_item.built_in_item_type);
  }
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
  --expected_count;

  // Wait for sidebar item removal to complete before navigation.
  WaitUntil(base::BindLambdaForTesting([&]() {
    return model()->GetAllSidebarItems().size() == expected_count;
  }));
  EXPECT_EQ(expected_count, model()->GetAllSidebarItems().size());
  EXPECT_THAT(model()->active_index(), Optional(active_item_index));

  // Navigate to a non-NTP page first to ensure we can add it to sidebar
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("brave://settings/")));

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

// Category A:
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2, WebTypePanelTest) {
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
      std::ranges::find(items, GURL("chrome://settings/"), &SidebarItem::url);
  EXPECT_NE(items.end(), iter);
  controller()->ActivateItemAt(std::distance(items.begin(), iter));
  EXPECT_EQ(0, tab_model()->active_index());
  EXPECT_EQ(tab_model()->GetWebContentsAt(0)->GetVisibleURL(), iter->url);

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  // Activate second sidebar item(wallet) and check it's loaded at current tab.
  iter = std::ranges::find(items, SidebarItem::BuiltInItemType::kWallet,
                           &SidebarItem::built_in_item_type);
  EXPECT_NE(items.end(), iter);
  controller()->ActivateItemAt(std::distance(items.begin(), iter));
  EXPECT_EQ(0, tab_model()->active_index());
  EXPECT_EQ(tab_model()->GetWebContentsAt(0)->GetVisibleURL(), iter->url);
#endif
  // New tab is not created.
  EXPECT_EQ(2, tab_model()->count());
}

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
// Category A:
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2, IterateBuiltInWebTypeTest) {
  // Click builtin wallet item and it's loaded at current active tab.
  const auto items = model()->GetAllSidebarItems();
  const auto wallet_item_iter =
      std::ranges::find(items, SidebarItem::BuiltInItemType::kWallet,
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
  browser2->GetFeatures().sidebar_controller()->ActivateItemAt(
      wallet_item_index);

  // Wait till browser() is activated.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return browser()->window()->IsActive(); }));

  EXPECT_EQ(0, tab_model()->active_index());
#endif
}
#endif  // BUILDFLAG(ENABLE_BRAVE_WALLET)

// Category A:
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2,
                       BookmarksPanelShownAfterReadingListTest) {
  if (IsV2Enabled()) {
    controller()->ActivateItemAt(
        model()->GetIndexOf(SidebarItem::BuiltInItemType::kReadingList));
  } else {
    auto* panel_ui = browser()->GetFeatures().side_panel_ui();
    panel_ui->Show(SidePanelEntryId::kReadingList);
  }

  // Check reading list panel is activated.
  auto reading_list_item_index =
      model()->GetIndexOf(SidebarItem::BuiltInItemType::kReadingList);
  ASSERT_TRUE(reading_list_item_index.has_value());
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return controller()->IsActiveIndex(reading_list_item_index); }));

  // Check bookmarks panel is activated.
  if (IsV2Enabled()) {
    controller()->ActivateItemAt(
        model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks));
  } else {
    auto* panel_ui = browser()->GetFeatures().side_panel_ui();
    panel_ui->Show(SidePanelEntryId::kBookmarks);
  }

  auto bookmarks_item_index =
      model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks);
  ASSERT_TRUE(bookmarks_item_index.has_value());
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return controller()->IsActiveIndex(bookmarks_item_index); }));
}

// Category C:
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2,
                       PRE_LastlyUsedSidePanelItemTest) {
  if (IsV2Enabled()) {
    GTEST_SKIP() << "Panel state persistence is V1-specific";
  }

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

// Category C:
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2, LastlyUsedSidePanelItemTest) {
  if (IsV2Enabled()) {
    GTEST_SKIP() << "Panel state persistence is V1-specific";
  }

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

// Category C:
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2, DefaultEntryTest) {
  if (IsV2Enabled()) {
    GTEST_SKIP() << "Default panel entry testing is V1-specific";
  }

  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  auto bookmark_item_index =
      model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks);
  panel_ui->Show(SidePanelEntryId::kBookmarks);

  // Wait till bookmark panel is activated.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return controller()->IsActiveIndex(bookmark_item_index); }));

  panel_ui->Close(SidePanelEntry::PanelType::kContent);
  WaitUntil(base::BindLambdaForTesting([&]() {
    return !panel_ui->GetCurrentEntryId(SidePanelEntry::PanelType::kContent)
                .has_value();
  }));

  // Remove bookmarks and check it's gone.
  SidebarServiceFactory::GetForProfile(browser()->profile())
      ->RemoveItemAt(*bookmark_item_index);
  EXPECT_FALSE(!!model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks));

  // Open panel w/o entry id.
  panel_ui->Toggle();
  WaitUntil(base::BindLambdaForTesting([&]() {
    return panel_ui->GetCurrentEntryId(SidePanelEntry::PanelType::kContent)
        .has_value();
  }));
  // Check bookmark panel is not opened again as it's deleted item.
  EXPECT_NE(SidePanelEntryId::kBookmarks,
            panel_ui->GetCurrentEntryId(SidePanelEntry::PanelType::kContent));
}

// Category A:
// Test sidebar's initial horizontal option is set properly.
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2,
                       PRE_InitialHorizontalOptionTest) {
  auto* prefs = browser()->profile()->GetPrefs();

  // Check default horizontal option is right-sided.
  EXPECT_TRUE(prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment));
  EXPECT_FALSE(IsSidebarUIOnLeft());

  // Set left-sided for next testing.
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, false);
}

// Category A:
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2, InitialHorizontalOptionTest) {
  auto* prefs = browser()->profile()->GetPrefs();

  // Check horizontal option is right-sided.
  EXPECT_FALSE(prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment));
  EXPECT_TRUE(IsSidebarUIOnLeft());
}

// Category A:
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2, ItemDragIndicatorCalcTest) {
  auto sidebar_items_contents_view = GetSidebarItemsContentsView(controller());
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

class SidebarBrowserWithSplitViewTest
    : public SidebarBrowserTest,
      public testing::WithParamInterface<bool> {
 public:
  SidebarBrowserWithSplitViewTest() = default;
  ~SidebarBrowserWithSplitViewTest() override = default;

  void SetUpOnMainThread() override {
    SidebarBrowserTest::SetUpOnMainThread();
    browser()->profile()->GetPrefs()->SetBoolean(kWebViewRoundedCorners,
                                                 GetParam());
  }

  void NewSplitTab() {
    chrome::NewSplitTab(browser(),
                        split_tabs::SplitTabCreatedSource::kTabContextMenu);
  }

  // Use this when left split view is active.
  views::View* GetStartSplitContentsView() {
    return browser_view()
        ->GetBraveMultiContentsView()
        ->GetActiveContentsContainerView();
  }

  // Use this when left split view is active.
  views::View* GetEndSplitContentsView() {
    return browser_view()
        ->GetBraveMultiContentsView()
        ->GetInactiveContentsContainerView();
  }

  BraveBrowserView* browser_view() {
    return BraveBrowserView::From(
        BrowserView::GetBrowserViewForBrowser(browser()));
  }

 private:
  display::test::TestScreen screen{/*create_display=*/true,
                                   /*register_screen=*/true};
};

IN_PROC_BROWSER_TEST_P(SidebarBrowserWithSplitViewTest,
                       ShowSidebarOnMouseOverTest) {
  auto scoped_mode = gfx::AnimationTestApi::SetRichAnimationRenderMode(
      gfx::Animation::RichAnimationRenderMode::FORCE_DISABLED);

  auto* service = SidebarServiceFactory::GetForProfile(browser()->profile());
  service->SetSidebarShowOption(
      SidebarService::ShowSidebarOption::kShowOnMouseOver);

  // To put sidebar right position after changing show option.
  browser_view()->DeprecatedLayoutImmediately();

  auto* browser_view = static_cast<BraveBrowserView*>(
      BrowserView::GetBrowserViewForBrowser(browser()));

  auto* prefs = browser()->profile()->GetPrefs();
  auto* sidebar_container = GetSidebarContainerView();
  auto* screen = display::Screen::Get();

  auto contents_area_view_rect =
      browser_view->GetBoundingBoxInScreenForMouseOverHandling();
  EXPECT_EQ(browser_view->width(), contents_area_view_rect.width());

  // Check sidebar is not shown.
  EXPECT_FALSE(sidebar_container->IsSidebarVisible());

  // Set mouse position inside the mouse hover area to check sidebar UI is shown
  // with that mouse position when sidebar is on right side.
  gfx::Point mouse_position = contents_area_view_rect.top_right();
  mouse_position.Offset(-2, 2);
  screen->SetCursorScreenPointForTesting(mouse_position);
  browser_view->HandleBrowserWindowMouseEvent(GetDummyEvent());
  EXPECT_TRUE(sidebar_container->IsSidebarVisible());

  // Check when sidebar on left.
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, false);

  // Hide sidebar.
  HideSidebar(true);
  EXPECT_FALSE(sidebar_container->IsSidebarVisible());

  // Set mouse position inside the mouse hover area to check sidebar UI is shown
  // with that mouse position when sidebar is on left side.
  mouse_position = contents_area_view_rect.origin();
  mouse_position.Offset(2, 2);
  screen->SetCursorScreenPointForTesting(mouse_position);
  browser_view->HandleBrowserWindowMouseEvent(GetDummyEvent());
  EXPECT_TRUE(sidebar_container->IsSidebarVisible());

  // Hide sidebar.
  HideSidebar(true);
  EXPECT_FALSE(sidebar_container->IsSidebarVisible());

  // Check with the space between window border and contents.
  // We have that space with rounded corners.
  // When mouse moves into that space, sidebar should be visible.
  mouse_position = contents_area_view_rect.origin();
  screen->SetCursorScreenPointForTesting(mouse_position);
  browser_view->HandleBrowserWindowMouseEvent(GetDummyEvent());
  EXPECT_TRUE(sidebar_container->IsSidebarVisible());

  // Hide sidebar.
  HideSidebar(true);
  EXPECT_FALSE(sidebar_container->IsSidebarVisible());

  // Test with split view.
  // With sidebar on left, only left split view contents' left side hot
  // corner should trigger split view.
  NewSplitTab();
  auto* tab_strip_model = browser()->tab_strip_model();
  EXPECT_EQ(2, tab_strip_model->count());

  // Make left split view as active to make secondary container put
  // at right side(end).
  tab_strip_model->ActivateTabAt(0);
  auto* left_split_view = GetStartSplitContentsView();
  auto* right_split_view = GetEndSplitContentsView();

  // Check left split view's left hot corner handles.
  mouse_position = left_split_view->GetLocalBounds().origin();
  mouse_position.Offset(2, 2);
  views::View::ConvertPointToScreen(left_split_view, &mouse_position);
  screen->SetCursorScreenPointForTesting(mouse_position);
  browser_view->HandleBrowserWindowMouseEvent(GetDummyEvent());
  EXPECT_TRUE(sidebar_container->IsSidebarVisible());

  // Hide sidebar.
  HideSidebar(true);
  EXPECT_FALSE(sidebar_container->IsSidebarVisible());

  // Check right split view's left hot corner doesn't handle.
  mouse_position = right_split_view->GetLocalBounds().origin();
  mouse_position.Offset(2, 2);
  views::View::ConvertPointToScreen(right_split_view, &mouse_position);
  screen->SetCursorScreenPointForTesting(mouse_position);
  browser_view->HandleBrowserWindowMouseEvent(GetDummyEvent());
  EXPECT_FALSE(sidebar_container->IsSidebarVisible());
}

INSTANTIATE_TEST_SUITE_P(
    /* no prefix */,
    SidebarBrowserWithSplitViewTest,
    ::testing::Bool());

class SidebarBrowserWithWebPanelTest
    : public SidebarBrowserTest,
      public testing::WithParamInterface<bool> {
 public:
  SidebarBrowserWithWebPanelTest() {
    if (GetParam()) {
      scoped_features_.InitAndEnableFeature(
          sidebar::features::kSidebarWebPanel);
    }
  }
  ~SidebarBrowserWithWebPanelTest() override = default;

  BraveMultiContentsView* GetBraveMultiContentsView() {
    return browser_view()->GetBraveMultiContentsView();
  }

  BraveBrowserView* browser_view() {
    return BraveBrowserView::From(
        BrowserView::GetBrowserViewForBrowser(browser()));
  }

  bool IsWebPanelEnabled() const {
    return base::FeatureList::IsEnabled(features::kSidebarWebPanel);
  }

  SidebarWebPanelController* web_panel_controller() {
    return controller()->GetWebPanelController();
  }

 private:
  base::test::ScopedFeatureList scoped_features_;
};

IN_PROC_BROWSER_TEST_P(SidebarBrowserWithWebPanelTest, WebPanelTest) {
  auto contents_container_view_for_web_panel =
      GetBraveMultiContentsView()->contents_container_view_for_web_panel_;
  if (!IsWebPanelEnabled()) {
    EXPECT_FALSE(contents_container_view_for_web_panel);

    // Even web panel feature is disabled, sidebar could have web panel
    // type because it could be added when that feature enabled.
    // In this sitaution, web panel type should work like web type that
    // load its url in a tab.
    // Test it works in that way after adding web panel type.
    auto* sidebar_service =
        SidebarServiceFactory::GetForProfile(browser()->profile());
    GURL item_url("http://foo.bar/");
    sidebar_service->AddItem(sidebar::SidebarItem::Create(
        item_url, u"title", SidebarItem::Type::kTypeWeb,
        SidebarItem::BuiltInItemType::kNone, /*open_in_panel*/ true));
    EXPECT_NE(tab_model()->GetActiveWebContents()->GetVisibleURL(), item_url);
    // Above item is added at last.
    controller()->ActivateItemAt(sidebar_service->items().size() - 1);
    EXPECT_EQ(tab_model()->GetActiveWebContents()->GetVisibleURL(), item_url);

    // Test toggle existing panel doesn't have any issue even web panel type
    // exists.
    auto* panel_ui = browser()->GetFeatures().side_panel_ui();
    panel_ui->Show(SidePanelEntryId::kCustomizeChrome);
    ASSERT_TRUE(
        base::test::RunUntil([&]() { return GetSidePanel()->GetVisible(); }));
    panel_ui->Close(SidePanelEntry::PanelType::kContent);
    ASSERT_TRUE(
        base::test::RunUntil([&]() { return !GetSidePanel()->GetVisible(); }));
    return;
  }

  EXPECT_TRUE(contents_container_view_for_web_panel);
  EXPECT_FALSE(contents_container_view_for_web_panel->GetVisible());
  EXPECT_GT(GetBraveMultiContentsView()->web_panel_width_, 0);
  EXPECT_EQ(GetBraveMultiContentsView()->GetWebPanelWidth(), 0);
  EXPECT_EQ(contents_container_view_for_web_panel->width(), 0);
  EXPECT_FALSE(GetBraveMultiContentsView()->web_panel_on_left_);
  EXPECT_EQ(
      GetBraveMultiContentsView()->width(),
      GetBraveMultiContentsView()->GetActiveContentsContainerView()->width());

  contents_container_view_for_web_panel->SetVisible(true);
  EXPECT_GT(GetBraveMultiContentsView()->GetWebPanelWidth(), 0);
  GetBraveMultiContentsView()->InvalidateLayout();
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return contents_container_view_for_web_panel->width() ==
           GetBraveMultiContentsView()->web_panel_width_;
  }));
  EXPECT_EQ(contents_container_view_for_web_panel->bounds().origin(),
            GetBraveMultiContentsView()
                ->GetActiveContentsContainerView()
                ->bounds()
                .top_right());

  GetBraveMultiContentsView()->SetWebPanelOnLeft(true);
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return contents_container_view_for_web_panel->bounds().top_right() ==
           GetBraveMultiContentsView()
               ->GetActiveContentsContainerView()
               ->bounds()
               .origin();
  }));

  contents_container_view_for_web_panel->SetVisible(false);

  // To prevent item added bubble launching.
  auto* prefs = browser()->profile()->GetPrefs();
  prefs->SetInteger(sidebar::kSidebarItemAddedFeedbackBubbleShowCount, 3);

  // Add two web panel items and check panel is shown by activating it.
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("https://brave.com/")));
  EXPECT_TRUE(CanAddCurrentActiveTabToSidebar(browser()));
  controller()->AddItemWithCurrentTab();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), GURL("https://basicattentiontoken.com/")));
  EXPECT_TRUE(CanAddCurrentActiveTabToSidebar(browser()));
  controller()->AddItemWithCurrentTab();
  EXPECT_FALSE(GetBraveMultiContentsView()->IsWebPanelVisible());

  // Activate web panel item and check panel gets visible.
  // We have 1 tabs now and will check another pinned tab is created for web
  // panel.
  auto* tab_strip_model = browser()->tab_strip_model();
  EXPECT_EQ(1, tab_strip_model->count());

  const int web_panel_item_index = model()->GetAllSidebarItems().size() - 1;
  EXPECT_TRUE(
      model()->GetAllSidebarItems()[web_panel_item_index].is_web_panel_type());
  EXPECT_TRUE(model()
                  ->GetAllSidebarItems()[web_panel_item_index - 1]
                  .is_web_panel_type());
  controller()->ActivateItemAt(web_panel_item_index - 1);
  EXPECT_TRUE(GetBraveMultiContentsView()->IsWebPanelVisible());
  EXPECT_TRUE(
      contents_container_view_for_web_panel->mini_toolbar()->GetVisible());

  // Activate another web panel and check panel is still visible.
  controller()->ActivateItemAt(web_panel_item_index);
  EXPECT_TRUE(GetBraveMultiContentsView()->IsWebPanelVisible());
  EXPECT_TRUE(
      contents_container_view_for_web_panel->mini_toolbar()->GetVisible());

  // Now we have another pinned tab for web panel at 0.
  EXPECT_EQ(2, tab_strip_model->count());
  auto* tab_for_web_panel = tab_strip_model->GetTabAtIndex(0);
  EXPECT_TRUE(tab_for_web_panel->IsPinned());
  EXPECT_FALSE(tab_for_web_panel->IsActivated());
  EXPECT_EQ(tab_for_web_panel->GetContents(),
            web_panel_controller()->panel_contents());

  // Toggle web panel item and check panel gets hidden.
  controller()->ActivateItemAt(web_panel_item_index);
  EXPECT_FALSE(web_panel_controller()->panel_contents());
  EXPECT_FALSE(GetBraveMultiContentsView()->IsWebPanelVisible());
  EXPECT_EQ(1, tab_strip_model->count());

  // Open web panel.
  controller()->ActivateItemAt(web_panel_item_index);
  EXPECT_TRUE(GetBraveMultiContentsView()->IsWebPanelVisible());
  EXPECT_EQ(2, tab_strip_model->count());
  tab_for_web_panel = tab_strip_model->GetTabAtIndex(0);
  EXPECT_EQ(tab_for_web_panel->GetContents(),
            web_panel_controller()->panel_contents());

  // Web panel contents should be treated as active contents without
  // explicitly forcing focus (avoids macOS focus teardown issues).
  EXPECT_EQ(
      web_panel_controller()->panel_contents(),
      contents_container_view_for_web_panel->contents_view()->GetWebContents());

  // Check tab contents test with split view.
  // Create split view with tab at 1.
  tab_strip_model->ActivateTabAt(1);
  chrome::NewSplitTab(browser(),
                      split_tabs::SplitTabCreatedSource::kTabContextMenu);
  EXPECT_EQ(2, tab_strip_model->active_index());
  EXPECT_TRUE(tab_strip_model->GetTabAtIndex(1)->IsSplit());
  EXPECT_TRUE(tab_strip_model->GetTabAtIndex(2)->IsSplit());
  EXPECT_FALSE(GetBraveMultiContentsView()->is_web_panel_active_);

  // Cache current inactive split tab contents and check it's not changed for
  // inactive contents view when web panel is activated. Only active contents
  // view is changed to panel's contents as
  // BraveMultiContentsView::GetActiveContentsView() will give panel's contents.
  auto* inactive_split_tab_contents =
      GetBraveMultiContentsView()->GetInactiveContentsView()->GetWebContents();
  tab_strip_model->ActivateTabAt(0);
  EXPECT_TRUE(GetBraveMultiContentsView()->is_web_panel_active_);
  EXPECT_EQ(0, tab_strip_model->active_index());
  EXPECT_EQ(
      web_panel_controller()->panel_contents(),
      GetBraveMultiContentsView()->GetActiveContentsView()->GetWebContents());
  EXPECT_EQ(
      inactive_split_tab_contents,
      GetBraveMultiContentsView()->GetInactiveContentsView()->GetWebContents());

  // Check web panel is closed by closing its pinned tab.
  tab_for_web_panel->Close();
  EXPECT_FALSE(web_panel_controller()->panel_contents());
  EXPECT_FALSE(GetBraveMultiContentsView()->IsWebPanelVisible());
  EXPECT_EQ(2, tab_strip_model->count());
}

INSTANTIATE_TEST_SUITE_P(
    /* no prefix */,
    SidebarBrowserWithWebPanelTest,
    ::testing::Bool());

// Category A:
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2, HideSidebarUITest) {
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

// Category A:
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2,
                       ItemAddedBubbleAnchorViewTest) {
  auto* sidebar_service =
      SidebarServiceFactory::GetForProfile(browser()->profile());
  auto sidebar_items_contents_view = GetSidebarItemsContentsView(controller());
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

// Category A:
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2, ItemActivatedScrollTest) {
  // To prevent item added bubble launching.
  auto* prefs = browser()->profile()->GetPrefs();
  prefs->SetInteger(sidebar::kSidebarItemAddedFeedbackBubbleShowCount, 3);

  auto bookmark_item_index =
      model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks);
  ASSERT_TRUE(bookmark_item_index.has_value());

  auto* sidebar_service =
      SidebarServiceFactory::GetForProfile(browser()->profile());
  auto* scroll_view = GetSidebarItemsScrollView(controller());

  // Move bookmark item at zero index to make it hidden.
  sidebar_service->MoveItem(*bookmark_item_index, 0);
  bookmark_item_index = 0;
  AddItemsTillScrollable(scroll_view, sidebar_service);

  // Check bookmarks item is hidden.
  EXPECT_TRUE(NeedScrollForItemAt(*bookmark_item_index, scroll_view));

  // Open bookmark panel.
  if (IsV2Enabled()) {
    // Use panel->Show() when V2 is enabled by default.
    // Item should be responded by panel activation.
    controller()->ActivateItemAt(
        model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks));
  } else {
    browser()->GetFeatures().side_panel_ui()->Show(
        SidePanelEntryId::kBookmarks);
  }

  // Wait till bookmarks item is visible.
  WaitUntil(base::BindLambdaForTesting([&]() {
    return !NeedScrollForItemAt(*bookmark_item_index, scroll_view);
  }));
  EXPECT_TRUE(controller()->IsActiveIndex(bookmark_item_index));
}

// Category A:
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2, ItemAddedScrollTest) {
  // To prevent item added bubble launching.
  auto* prefs = browser()->profile()->GetPrefs();
  prefs->SetInteger(sidebar::kSidebarItemAddedFeedbackBubbleShowCount, 3);

  auto* sidebar_service =
      SidebarServiceFactory::GetForProfile(browser()->profile());
  auto* scroll_view = GetSidebarItemsScrollView(controller());
  auto sidebar_items_contents_view = GetSidebarItemsContentsView(controller());

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

// Category A:
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2, PRE_PrefsMigrationTest) {
  // Prepare temporarily changed condition.
  auto* prefs = browser()->profile()->GetPrefs();
  prefs->SetBoolean(sidebar::kSidebarAlignmentChangedTemporarily, true);
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, true);
}

// Category A:
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2, PrefsMigrationTest) {
  // Check all prefs are changed to default.
  auto* prefs = browser()->profile()->GetPrefs();
  EXPECT_TRUE(prefs->FindPreference(prefs::kSidePanelHorizontalAlignment)
                  ->IsDefaultValue());
  EXPECT_TRUE(prefs->FindPreference(prefs::kSidePanelHorizontalAlignment)
                  ->IsDefaultValue());
}

// Category C: V1-only test (skip in V2)
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2, SidePanelResizeTest) {
  if (IsV2Enabled()) {
    GTEST_SKIP() << "Panel resize is V1-specific (V2 doesn't manage panel in "
                    "sidebar container)";
  }

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

// Category C: V1-only (unmanaged panel entry test)
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2, UnManagedPanelEntryTest) {
  if (IsV2Enabled()) {
    GTEST_SKIP() << "Unmanaged panel entry testing is V1-specific";
  }

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
  panel_ui->Close(SidePanelEntry::PanelType::kContent);
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !GetSidePanel()->GetVisible(); }));
  EXPECT_FALSE(
      !!panel_ui->GetCurrentEntryId(SidePanelEntry::PanelType::kContent));

  // Remove bookmarks and check it's gone.
  SidebarServiceFactory::GetForProfile(browser()->profile())
      ->RemoveItemAt(bookmark_item_index);
  EXPECT_FALSE(!!model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks));

  // Show bookmarks entry again and wait till sidebar panel gets visible
  panel_ui->Show(SidePanelEntryId::kBookmarks);
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return GetSidePanel()->GetVisible(); }));
  EXPECT_EQ(SidePanelEntryId::kBookmarks,
            panel_ui->GetCurrentEntryId(SidePanelEntry::PanelType::kContent));
}

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
// Category C: V1-only (unmanaged panel after item deletion)
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2,
                       OpenUnManagedPanelAfterDeletingDefaultWebTypeItem) {
  if (IsV2Enabled()) {
    GTEST_SKIP() << "Unmanaged panel testing is V1-specific";
  }

  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  const auto items = model()->GetAllSidebarItems();
  const auto wallet_item_iter =
      std::ranges::find(items, SidebarItem::BuiltInItemType::kWallet,
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
#endif  // BUILDFLAG(ENABLE_BRAVE_WALLET)

// Category C: V1-only (tab-specific vs global panels)
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2,
                       TabSpecificAndGlobalPanelsTest) {
  if (IsV2Enabled()) {
    GTEST_SKIP() << "Tab-specific vs global panel testing is V1-specific";
  }

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
  EXPECT_FALSE(
      panel_ui->IsSidePanelShowing(SidePanelEntry::PanelType::kContent));

  // Open global panel when active tab index is 1.
  panel_ui->Show(SidePanelEntryId::kBookmarks);
  WaitUntil(base::BindLambdaForTesting([&]() {
    return panel_ui->GetCurrentEntryId(SidePanelEntry::PanelType::kContent) ==
           SidePanelEntryId::kBookmarks;
  }));

  // Contextual panel should be set when activate tab at 0.
  tab_model()->ActivateTabAt(0);
  WaitUntil(base::BindLambdaForTesting([&]() {
    return panel_ui->GetCurrentEntryId(SidePanelEntry::PanelType::kContent) ==
           SidePanelEntryId::kCustomizeChrome;
  }));

  // Global panel should be set when activate tab at 1.
  tab_model()->ActivateTabAt(1);
  WaitUntil(base::BindLambdaForTesting([&]() {
    return panel_ui->GetCurrentEntryId(SidePanelEntry::PanelType::kContent) ==
           SidePanelEntryId::kBookmarks;
  }));
}

// Category A:
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2, DisabledItemsTest) {
  auto* guest_browser = CreateGuestBrowser();
  auto* controller = guest_browser->GetFeatures().sidebar_controller();
  auto* model = controller->model();
  for (const auto& item : model->GetAllSidebarItems()) {
    // Check disabled builtin items are not included in guest browser's items
    // list.
    if (item.is_built_in_type()) {
      EXPECT_FALSE(IsDisabledItemForGuest(item.built_in_item_type));
    }
  }

  auto* private_browser = CreateIncognitoBrowser(browser()->profile());
  controller = private_browser->GetFeatures().sidebar_controller();
  model = controller->model();
  for (const auto& item : model->GetAllSidebarItems()) {
    // Check disabled builtin items are not included in private browser's items
    // list.
    if (item.is_built_in_type()) {
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

#if BUILDFLAG(ENABLE_AI_CHAT)
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
    WaitUntil(base::BindLambdaForTesting([&]() {
      return !!panel_ui->GetCurrentEntryId(SidePanelEntry::PanelType::kContent);
    }));

    EXPECT_EQ(SidePanelEntryId::kChatUI,
              panel_ui->GetCurrentEntryId(SidePanelEntry::PanelType::kContent));
  }
  testing::Mock::VerifyAndClearExpectations(&observer_);

  panel_ui->Close(SidePanelEntry::PanelType::kContent);
  EXPECT_FALSE(
      panel_ui->IsSidePanelShowing(SidePanelEntry::PanelType::kContent));

  // Check one shot panel is not opened anymore.
  EXPECT_CALL(observer_, OnActiveIndexChanged(testing::_, testing::_)).Times(0);
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("https://www.brave.com/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  EXPECT_FALSE(
      panel_ui->IsSidePanelShowing(SidePanelEntry::PanelType::kContent));
  testing::Mock::VerifyAndClearExpectations(&observer_);

  observation_.Reset();
}

INSTANTIATE_TEST_SUITE_P(
    /* no prefix */,
    SidebarBrowserTestWithkSidebarShowAlwaysOnStable,
    ::testing::Bool());
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#if BUILDFLAG(ENABLE_PLAYLIST)
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
      SidebarServiceFactory::GetForProfile(private_browser->profile());
  const auto& items = sidebar_service->items();
  auto iter = std::ranges::find_if(items, [](const auto& item) {
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
#endif  // BUILDFLAG(ENABLE_PLAYLIST)

#if BUILDFLAG(ENABLE_AI_CHAT)

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
  ASSERT_EQ(tab_model()->count(), 2);

  // Open contextual panel from Tab 0.
  tab_model()->ActivateTabAt(0);
  SimulateSidebarItemClickAt(tab_specific_item_index.value());
  EXPECT_EQ(model()->active_index(), tab_specific_item_index);

  // Delete Tab 0 and check model doesn't have active index.
  tab_model()->DetachAndDeleteWebContentsAt(0);
  EXPECT_FALSE(!!model()->active_index());
  ASSERT_EQ(tab_model()->count(), 1);

  // Create two more tab for test below.
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("brave://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("brave://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  ASSERT_EQ(tab_model()->count(), 3);

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
  ASSERT_EQ(tab_model()->count(), 3);

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
  EXPECT_EQ(SidePanelEntryId::kChatUI,
            panel_ui->GetCurrentEntryId(SidePanelEntry::PanelType::kContent));
  EXPECT_TRUE(GetSidePanel()->GetVisible());
  // Tab Specific panel should be open when Tab 1 is active
  EXPECT_EQ(model()->active_index(), tab_specific_item_index);

  // Global panel should be open when Tab 0 is active
  tab_model()->ActivateTabAt(0);
  EXPECT_EQ(SidePanelEntryId::kBookmarks,
            panel_ui->GetCurrentEntryId(SidePanelEntry::PanelType::kContent));
  EXPECT_TRUE(model()->active_index().has_value());
  EXPECT_EQ(model()->active_index(),
            model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks));

  // Global panel should be open when Tab 2 is active
  tab_model()->ActivateTabAt(2);
  EXPECT_EQ(SidePanelEntryId::kBookmarks,
            panel_ui->GetCurrentEntryId(SidePanelEntry::PanelType::kContent));
  EXPECT_TRUE(model()->active_index().has_value());
  EXPECT_EQ(model()->active_index(),
            model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks));

  // Check tab specific panel item is still activated after moving to another
  // browser.
  tab_model()->ActivateTabAt(1);
  EXPECT_EQ(model()->active_index(), tab_specific_item_index);

  auto* browser2 = CreateBrowser(browser()->profile());
  auto* browser2_model = browser2->GetFeatures().sidebar_controller()->model();
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
  ASSERT_EQ(tab_model()->count(), 3);
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

#endif  // BUILDFLAG(ENABLE_AI_CHAT)

// Category A:
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2, SidebarRightSideTest) {
  // Sidebar is on right by default
  EXPECT_FALSE(IsSidebarUIOnLeft());

  brave::ToggleVerticalTabStrip(browser());
  ASSERT_TRUE(tabs::utils::ShouldShowBraveVerticalTabs(browser()));

  auto* prefs = browser()->profile()->GetPrefs();
  auto* vertical_tabs_container = GetVerticalTabsContainer();
  views::View* sidebar_container = GetSidebarContainerView();

  // Check if vertical tabs is located at first and sidebar is located on the
  // right side.
  EXPECT_LT(vertical_tabs_container->GetBoundsInScreen().x(),
            sidebar_container->GetBoundsInScreen().x());

  // Changed to sidebar on left side.
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, false);
  EXPECT_TRUE(IsSidebarUIOnLeft());

  int expected_sidebar_x = vertical_tabs_container->GetBoundsInScreen().right();
  if (BraveContentsViewUtil::GetRoundedCornersWebViewMargin(browser())) {
    expected_sidebar_x += 1;
  }

  // Check if vertical tabs is located first and sidebar is following it.
  EXPECT_EQ(sidebar_container->GetBoundsInScreen().x(), expected_sidebar_x);

  // Check sidebar position option is synced between normal and private window.
  auto* private_browser = CreateIncognitoBrowser(browser()->profile());
  auto* private_prefs = private_browser->profile()->GetPrefs();
  EXPECT_EQ(prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment),
            private_prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment));
  EXPECT_FALSE(prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment));
  private_prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, true);
  EXPECT_TRUE(prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment));
}

// Category C: V1-only (SidebarContainerView observation test)
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2,
                       SidebarContainerDoesNotObserveToolbarHeightEntry) {
  if (IsV2Enabled()) {
    GTEST_SKIP() << "SidebarContainerView observation is V1-specific";
  }

  // This test verifies that SidebarContainerView doesn't observe
  // SidePanelEntry with kToolbar type, as it only observes kContent type
  // entries per AddSidePanelEntryObservation() implementation.

  auto* sidebar_container = GetSidebarContainerView();

  // Get the tab registry and create a kToolbar type entry
  auto* registry = SidePanelRegistry::From(browser()->GetActiveTabInterface());
  ASSERT_TRUE(registry);

  // Create a kToolbar type SidePanelEntry
  std::unique_ptr<SidePanelEntry> toolbar_entry =
      std::make_unique<SidePanelEntry>(
          SidePanelEntry::PanelType::kToolbar,
          SidePanelEntry::Key(SidePanelEntry::Id::kAboutThisSite),
          base::BindRepeating([](SidePanelEntryScope&) {
            return std::make_unique<views::View>();
          }),
          /*default_content_width_callback=*/base::NullCallback());

  auto* toolbar_entry_ptr = toolbar_entry.get();
  registry->Register(std::move(toolbar_entry));

  // Show the toolbar entry
  auto* coordinator = SidePanelCoordinator::From(browser());
  coordinator->SetNoDelaysForTesting(true);
  coordinator->Show(SidePanelEntry::Id::kAboutThisSite);

  // Verify that SidebarContainerView is NOT observing the kToolbar entry.
  EXPECT_FALSE(toolbar_entry_ptr->IsBeingObservedBy(sidebar_container));
}

// Category B:
// Tests that sidebar container, side panel, control view, and contents
// container are positioned correctly in RTL mode.
//
// Three fixes work together:
//   - SetMirrored(false) on SidebarContainerView/SidebarControlView prevents
//     the Views framework from flipping internal layout in RTL.
//   - SetFlipCanvasOnPaintForRTLUI(false) on buttons prevents icon flipping.
//   - GetMirroredRect(contents_bounds) in BraveBrowserViewLayout ensures the
//     contents container is placed correctly next to the sidebar in RTL.
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2, SidebarLayoutInRTLTest) {
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* sidebar_container = GetSidebarContainerView();
  auto* control_view = GetSidebarControlView();
  auto* contents_view = browser_view->contents_container();

  // Needs invalidation to apply RTL mode layout.
  base::i18n::ScopedRTLForTesting scoped_rtl(/*rtl=*/true);
  browser_view->InvalidateLayout();
  RunScheduledLayouts();

  // --- Right-aligned sidebar (default) ---
  ASSERT_FALSE(IsSidebarUIOnLeft());

  // As we set mirrored rect for sidebar and contents during the layout,
  // Each views' mirrored bounds are what we're seeing in RTL mode.
  EXPECT_LE(contents_view->GetMirroredBounds().right(),
            sidebar_container->GetMirroredBounds().x());

  if (IsV2Enabled()) {
    GTEST_SKIP() << "Below panel position observation is V1-specific";
  }

  // Open the side panel to verify panel/control positioning within the
  // sidebar container.
  browser()->command_controller()->ExecuteCommand(IDC_TOGGLE_SIDEBAR);
  RunScheduledLayouts();
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return GetSidePanel()->width() == kDefaultSidePanelWidth; }));

  // As container disabled mirroring(SetMirrored(false)),
  // their bounds are what we're seeing.
  EXPECT_LE(GetSidePanel()->bounds().right(), control_view->bounds().x());

  // --- Left-aligned sidebar in RTL mode ---
  auto* prefs = browser()->profile()->GetPrefs();
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, false);
  ASSERT_TRUE(IsSidebarUIOnLeft());
  RunScheduledLayouts();

  // As container disabled mirroring(SetMirrored(false)),
  // their bounds are what we're seeing.
  EXPECT_LE(control_view->bounds().right(), GetSidePanel()->bounds().x());

  // As we set mirrored rect for sidebar and contents during the layout,
  // Each views' mirrored bounds are what we're seeing in RTL mode.
  EXPECT_LE(sidebar_container->GetMirroredBounds().right(),
            contents_view->GetMirroredBounds().x());
}

// Category C: V1-only (Panel test)
// Tests that side panel resize direction is correct in RTL mode.
// The OnResize sign convention is flipped in RTL compared to LTR because the
// drag direction on screen is reversed.
IN_PROC_BROWSER_TEST_P(SidebarBrowserTestV1AndV2, SidePanelResizeInRTLTest) {
  if (IsV2Enabled()) {
    GTEST_SKIP() << "Panel resize test is V1-specific";
  }

  // Open side panel (right-aligned by default).
  browser()->command_controller()->ExecuteCommand(IDC_TOGGLE_SIDEBAR);
  RunScheduledLayouts();
  int expected_width = kDefaultSidePanelWidth;
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return GetSidePanel()->width() == expected_width; }));

  // In LTR with right-aligned sidebar: negative resize_amount increases width.
  GetSidePanel()->OnResize(-20, true);
  expected_width += 20;
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return GetSidePanel()->width() == expected_width; }));

  // Enable RTL mode.
  base::i18n::ScopedRTLForTesting scoped_rtl(/*rtl=*/true);
  BrowserView::GetBrowserViewForBrowser(browser())->InvalidateLayout();
  RunScheduledLayouts();

  // In RTL with right-aligned sidebar: positive resize_amount increases width
  // (sign is flipped because screen drag direction reverses in RTL).
  GetSidePanel()->OnResize(20, true);
  expected_width += 20;
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return GetSidePanel()->width() == expected_width; }));
}

// Instantiate parameterized tests for both V1 and V2
INSTANTIATE_TEST_SUITE_P(
    /* no prefix */,
    SidebarBrowserTestV1AndV2,
    ::testing::Bool(),  // false = V1, true = V2
    [](const testing::TestParamInfo<bool>& info) {
      return info.param ? "V2" : "V1";
    });

#if BUILDFLAG(ENABLE_SIDEBAR_V2)
// In V2 the upstream side panel is a direct child of browser_view, positioned
// by CalculateSideBarLayout.  Verify that when the panel is open it sits
// between the contents container and the sidebar control, NOT outside it.
// Covers both the default right-side and the explicitly set left-side layouts.
IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, SidebarV2PanelPositionTest) {
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* panel = browser_view->contents_height_side_panel();
  panel->DisableAnimationsForTesting();
  SidebarContainerView* sidebar = GetSidebarContainerView();
  auto* prefs = browser()->profile()->GetPrefs();

  browser()->GetFeatures().side_panel_ui()->Toggle();
  RunScheduledLayouts();

  ASSERT_TRUE(panel->GetVisible());
  ASSERT_TRUE(sidebar->IsSidebarVisible());

  // --- Sidebar on right (default LTR: kSidePanelHorizontalAlignment = true)
  ASSERT_FALSE(sidebar->sidebar_on_left());

  // Panel sits immediately left of the sidebar control:
  //   [contents] [panel] [sidebar_control]
  EXPECT_EQ(panel->bounds().right(), sidebar->bounds().x())
      << "panel=" << panel->bounds().ToString()
      << " sidebar=" << sidebar->bounds().ToString();

  // --- Sidebar on left (kSidePanelHorizontalAlignment = false)
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, false);
  RunScheduledLayouts();

  ASSERT_TRUE(sidebar->sidebar_on_left());

  // Panel sits immediately right of the sidebar control:
  //   [sidebar_control] [panel] [contents]
  EXPECT_EQ(sidebar->bounds().right(), panel->bounds().x())
      << "sidebar=" << sidebar->bounds().ToString()
      << " panel=" << panel->bounds().ToString();
}

// Verify that the sidebar item active state in SidebarModel is updated:
// - When clicking a panel item via the sidebar UI.
// - When the side panel is opened or closed via the side panel UI directly
//   (e.g. toolbar toggle button), which bypasses SidebarController.
//   In V1, SidebarContainerView monitors panel show/hide events and asks
//   SidebarController to update the active state. In V2,
//   SidebarContainerView does not do that, so BraveSidePanelCoordinator
//   handles it in Show() and Close().
IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, SidebarV2ActiveItemStateSync) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  panel_ui->DisableAnimationsForTesting();

  const auto bookmark_item_index =
      model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks);
  ASSERT_TRUE(bookmark_item_index.has_value());

  // Initially no item is active.
  EXPECT_FALSE(model()->active_index());

  // Clicking a panel item via the sidebar UI activates it in the model.
  SimulateSidebarItemClickAt(*bookmark_item_index);
  EXPECT_EQ(model()->active_index(), bookmark_item_index);

  // Deactivate by closing the panel.
  panel_ui->Close(SidePanelEntry::PanelType::kContent);
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !model()->active_index().has_value(); }));

  // Opening the side panel via the panel UI (e.g. toolbar toggle button path)
  // also activates the corresponding sidebar item in the model.
  panel_ui->Show(SidePanelEntryId::kBookmarks);
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return controller()->IsActiveIndex(bookmark_item_index); }));

  // Closing the side panel via the panel UI deactivates the item in the model.
  panel_ui->Close(SidePanelEntry::PanelType::kContent);
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !model()->active_index().has_value(); }));

  // Toggling the panel open (as the toolbar button does) activates the
  // last-used sidebar item in the model.
  panel_ui->Toggle();
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return controller()->IsActiveIndex(bookmark_item_index); }));

  // Wait for the panel to be fully shown before toggling closed, so that
  // BraveSidePanelCoordinator::Toggle() sees IsSidePanelShowing() == true
  // and takes the close branch instead of the show branch.
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return panel_ui->IsSidePanelShowing(SidePanelEntry::PanelType::kContent);
  }));

  // Toggling the panel closed deactivates the item in the model.
  panel_ui->Toggle();
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !model()->active_index().has_value(); }));
}

// Verify that the upstream SidePanelHeader is never added when a sidebar panel
// is opened in V2, so Brave can render its own header.
IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, SidebarV2NoUpstreamHeaderTest) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* side_panel = browser_view->contents_height_side_panel();
  side_panel->DisableAnimationsForTesting();

  panel_ui->Toggle();
  ASSERT_TRUE(base::test::RunUntil([&]() { return side_panel->GetVisible(); }));

  EXPECT_EQ(nullptr, side_panel->GetHeaderView<views::View>())
      << "Upstream SidePanelHeader should not be present after V2 panel open";

  // Also verify CustomizeChrome panel does not get an upstream header.
  panel_ui->Show(SidePanelEntryId::kCustomizeChrome);
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return panel_ui->IsSidePanelEntryShowing(
        SidePanelEntry::Key(SidePanelEntryId::kCustomizeChrome));
  }));

  EXPECT_EQ(nullptr, side_panel->GetHeaderView<views::View>())
      << "Upstream SidePanelHeader should not be present for CustomizeChrome "
         "panel";
}

#endif  // BUILDFLAG(ENABLE_SIDEBAR_V2)

}  // namespace sidebar
