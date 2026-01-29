/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_tab_menu_model.h"
#include "brave/browser/ui/tabs/brave_tab_menu_model_factory.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"
#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip_layout_helper.h"
#include "brave/browser/ui/views/tabs/switches.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/common/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/tab_group_sync/tab_group_sync_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/horizontal_tab_strip_region_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "chrome/browser/ui/views/tabs/tab_context_menu_controller.h"
#include "chrome/browser/ui/views/tabs/tab_search_button.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "third_party/skia/include/core/SkPath.h"
#include "ui/base/test/ui_controls.h"
#include "ui/display/screen.h"
#include "ui/display/test/test_screen.h"
#include "ui/events/event.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_manager.h"
#include "ui/views/test/views_test_utils.h"

#if BUILDFLAG(IS_WIN)
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/frame/browser_frame_view_win.h"
#endif

#if BUILDFLAG(IS_MAC)
#include "ui/views/widget/native_widget_mac.h"
#endif

#if defined(USE_AURA)
#include "chrome/browser/ui/views/frame/opaque_browser_frame_view.h"
#include "ui/aura/test/ui_controls_aurawin.h"
#include "ui/aura/window.h"
#endif

#if BUILDFLAG(IS_OZONE)
#include "ui/ozone/public/ozone_platform.h"
#include "ui/platform_window/common/platform_window_defaults.h"
#endif

namespace {

class FullscreenNotificationObserver : public FullscreenObserver {
 public:
  explicit FullscreenNotificationObserver(Browser* browser);

  FullscreenNotificationObserver(const FullscreenNotificationObserver&) =
      delete;
  FullscreenNotificationObserver& operator=(
      const FullscreenNotificationObserver&) = delete;

  ~FullscreenNotificationObserver() override;

  // Runs a loop until a fullscreen change is seen (unless one has already been
  // observed, in which case it returns immediately).
  void Wait();

  // FullscreenObserver:
  void OnFullscreenStateChanged() override;

 protected:
  bool observed_change_ = false;
  base::ScopedObservation<FullscreenController, FullscreenObserver>
      observation_{this};
  base::RunLoop run_loop_;
};

FullscreenNotificationObserver::FullscreenNotificationObserver(
    Browser* browser) {
  observation_.Observe(browser->GetFeatures()
                           .exclusive_access_manager()
                           ->fullscreen_controller());
}

FullscreenNotificationObserver::~FullscreenNotificationObserver() = default;

void FullscreenNotificationObserver::OnFullscreenStateChanged() {
  observed_change_ = true;
  if (run_loop_.running()) {
    run_loop_.Quit();
  }
}

void FullscreenNotificationObserver::Wait() {
  if (observed_change_) {
    return;
  }

  run_loop_.Run();
}

}  // namespace

class VerticalTabStripBrowserTest : public InProcessBrowserTest {
 public:
  VerticalTabStripBrowserTest() = default;
  ~VerticalTabStripBrowserTest() override = default;

  const BraveBrowserView* browser_view() const {
    return static_cast<BraveBrowserView*>(browser()->window());
  }
  BraveBrowserView* browser_view() {
    return static_cast<BraveBrowserView*>(browser()->window());
  }
  BrowserFrameView* browser_non_client_frame_view() {
    return browser_view()->browser_widget()->GetFrameView();
  }
  const BrowserFrameView* browser_non_client_frame_view() const {
    return browser_view()->browser_widget()->GetFrameView();
  }

  void ToggleVerticalTabStrip() {
    brave::ToggleVerticalTabStrip(browser());
    browser_non_client_frame_view()->DeprecatedLayoutImmediately();
  }

  void AppendTab(Browser* browser) {
    chrome::AddTabAt(browser, GURL(), -1, true);
  }

  tab_groups::TabGroupId AddTabToNewGroup(Browser* browser, int tab_index) {
    return browser->tab_strip_model()->AddToNewGroup({tab_index});
  }

  void AddTabToExistingGroup(Browser* browser,
                             int tab_index,
                             tab_groups::TabGroupId group) {
    ASSERT_TRUE(browser->tab_strip_model()->SupportsTabGroups());
    browser->tab_strip_model()->AddToExistingGroup({tab_index}, group);
  }

  TabStrip* GetTabStrip(Browser* browser) {
    return BrowserView::GetBrowserViewForBrowser(browser)
        ->horizontal_tab_strip_for_testing();
  }

  Tab* GetTabAt(Browser* browser, int index) {
    return GetTabStrip(browser)->tab_at(index);
  }

  gfx::Rect GetBoundsInScreen(views::View* view, const gfx::Rect& rect) {
    auto bounds_in_screen = rect;
    views::View::ConvertRectToScreen(view, &bounds_in_screen);
    return bounds_in_screen;
  }

  // Returns the visibility of window title is actually changed by a frame or
  // a widget. If we can't access to actual title view, returns a value which
  // window title will be synchronized to.
  bool IsWindowTitleViewVisible() const {
#if BUILDFLAG(IS_MAC)
    auto* native_widget = static_cast<const views::NativeWidgetMac*>(
        browser_view()->GetWidget()->native_widget_private());
    if (!native_widget->has_overridden_window_title_visibility()) {
      // Returns default visibility
      return browser_view()
          ->GetWidget()
          ->widget_delegate()
          ->ShouldShowWindowTitle();
    }

    return native_widget->GetOverriddenWindowTitleVisibility();
#elif BUILDFLAG(IS_WIN)
    if (browser_view()->GetWidget()->ShouldUseNativeFrame()) {
      return static_cast<const BrowserFrameViewWin*>(
                 browser_non_client_frame_view())
          ->GetViewByID(VIEW_ID_WINDOW_TITLE)
          ->GetVisible();
    }
#endif

#if defined(USE_AURA)
    return static_cast<const OpaqueBrowserFrameView*>(
               browser_non_client_frame_view())
        ->ShouldShowWindowTitle();
#endif
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
    RunLoop();
  }

  void RunLoop() {
    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop_->Run();
  }

  void InvalidateAndRunLayoutForVerticalTabStrip() {
    auto* widget_delegate_view =
        browser_view()->vertical_tab_strip_widget_delegate_view_.get();
    ASSERT_TRUE(widget_delegate_view);
    widget_delegate_view->vertical_tab_strip_region_view()->InvalidateLayout();
    views::test::RunScheduledLayout(
        widget_delegate_view->vertical_tab_strip_region_view());
  }

 protected:
  HorizontalTabStripRegionView* tab_strip_region_view() {
    return views::AsViewClass<HorizontalTabStripRegionView>(
        BrowserView::GetBrowserViewForBrowser(browser())->tab_strip_view());
  }

 private:
  std::unique_ptr<base::RunLoop> run_loop_;
  base::test::ScopedFeatureList feature_list_{
      features::kBraveRoundedCornersByDefault};
};

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, ToggleVerticalTabStrip) {
  // Pre-conditions
  // The default orientation is horizontal.
  ASSERT_FALSE(tabs::utils::ShouldShowBraveVerticalTabs(browser()));
  ASSERT_EQ(browser_view()->GetWidget(),
            browser_view()->horizontal_tab_strip_for_testing()->GetWidget());

  // Show vertical tab strip. This will move tabstrip to its own widget.
  ToggleVerticalTabStrip();
  EXPECT_TRUE(tabs::utils::ShouldShowBraveVerticalTabs(browser()));
  EXPECT_NE(browser_view()->GetWidget(),
            browser_view()->horizontal_tab_strip_for_testing()->GetWidget());

  // Hide vertical tab strip and restore to the horizontal tabstrip.
  ToggleVerticalTabStrip();
  EXPECT_FALSE(tabs::utils::ShouldShowBraveVerticalTabs(browser()));
  EXPECT_EQ(browser_view()->GetWidget(),
            browser_view()->horizontal_tab_strip_for_testing()->GetWidget());
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, WindowTitle) {
  ToggleVerticalTabStrip();

#if BUILDFLAG(IS_LINUX)
  browser()->profile()->GetPrefs()->SetBoolean(prefs::kUseCustomChromeFrame,
                                               true);
#endif

  // Pre-condition: Window title visibility differs per platform
#if BUILDFLAG(IS_WIN)
  constexpr bool kWindowTitleVisibleByDefault = true;
#else
  constexpr bool kWindowTitleVisibleByDefault = false;
#endif

  ASSERT_TRUE(tabs::utils::ShouldShowBraveVerticalTabs(browser()));
  ASSERT_EQ(kWindowTitleVisibleByDefault,
            tabs::utils::ShouldShowWindowTitleForVerticalTabs(browser()));
  ASSERT_EQ(kWindowTitleVisibleByDefault,
            browser_view()->ShouldShowWindowTitle());
  ASSERT_EQ(kWindowTitleVisibleByDefault, IsWindowTitleViewVisible());

  auto check_if_window_title_gets_visible = [&]() {
    // Show window title bar
    brave::ToggleWindowTitleVisibilityForVerticalTabs(browser());
    browser_non_client_frame_view()->DeprecatedLayoutImmediately();
    EXPECT_TRUE(tabs::utils::ShouldShowWindowTitleForVerticalTabs(browser()));
    EXPECT_TRUE(browser_view()->ShouldShowWindowTitle());
    EXPECT_GE(browser_non_client_frame_view()->GetTopInset(/*restored=*/false),
              0);
    EXPECT_TRUE(IsWindowTitleViewVisible());
  };

  if constexpr (!kWindowTitleVisibleByDefault) {
    check_if_window_title_gets_visible();
  }

  // Hide window title bar
  brave::ToggleWindowTitleVisibilityForVerticalTabs(browser());
  browser_non_client_frame_view()->DeprecatedLayoutImmediately();
  EXPECT_FALSE(tabs::utils::ShouldShowWindowTitleForVerticalTabs(browser()));
  EXPECT_FALSE(browser_view()->ShouldShowWindowTitle());
#if !BUILDFLAG(IS_LINUX)
  // TODO(sko) For now, we can't hide window title bar entirely on Linux.
  // We're using a minimum height for it.
  EXPECT_EQ(0,
            browser_non_client_frame_view()->GetTopInset(/*restored=*/false));
#endif
  EXPECT_FALSE(IsWindowTitleViewVisible());

  if constexpr (kWindowTitleVisibleByDefault) {
    check_if_window_title_gets_visible();
  }
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, NewTabVisibility) {
  EXPECT_TRUE(
      tab_strip_region_view()->new_tab_button_for_testing()->GetVisible());

  ToggleVerticalTabStrip();
  EXPECT_FALSE(
      tab_strip_region_view()->new_tab_button_for_testing()->GetVisible());

  ToggleVerticalTabStrip();
  EXPECT_TRUE(
      tab_strip_region_view()->new_tab_button_for_testing()->GetVisible());
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, MinHeight) {
  ToggleVerticalTabStrip();

  // Add a tab to flush cached min size.
  AppendTab(browser());

  const auto browser_view_min_size = browser_view()->GetMinimumSize();
  const auto browser_non_client_frame_view_min_size =
      browser_view()->browser_widget()->GetFrameView()->GetMinimumSize();

  // Add tabs as much as it can grow mih height of tab strip.
  auto tab_strip_min_height =
      tab_strip_region_view()->GetMinimumSize().height();
  for (int i = 0; i < 10; i++) {
    AppendTab(browser());
  }
  ASSERT_LE(tab_strip_min_height,
            tab_strip_region_view()->GetMinimumSize().height());

  // TabStrip's min height shouldn't affect that of browser window.
  EXPECT_EQ(browser_view_min_size.height(),
            browser_view()->GetMinimumSize().height());
  EXPECT_EQ(browser_non_client_frame_view_min_size.height(),
            browser_view()
                ->browser_widget()
                ->GetFrameView()
                ->GetMinimumSize()
                .height());
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, VisualState) {
  ToggleVerticalTabStrip();

  // Pre-condition: Floating mode is enabled by default.
  using State = BraveVerticalTabStripRegionView::State;
  ASSERT_TRUE(tabs::utils::IsFloatingVerticalTabsEnabled(browser()));
  auto* widget_delegate_view =
      browser_view()->vertical_tab_strip_widget_delegate_view_.get();
  ASSERT_TRUE(widget_delegate_view);

  auto* region_view = widget_delegate_view->vertical_tab_strip_region_view();
  ASSERT_TRUE(region_view);
  ASSERT_EQ(State::kExpanded, region_view->state());

  // When rounded corners is on(it's default now),
  // region view's border inset is changed during the floating.
  // See BraveVerticalTabStripRegionView::UpdateBorder() for
  // border inset calculation.
  const int inset_for_expanded_collapsed = -2;
  const int inset_for_floating = 1;
  EXPECT_EQ(inset_for_expanded_collapsed, region_view->GetInsets().width());

  // Try Expanding / collapsing
  auto* prefs = browser()->profile()->GetOriginalProfile()->GetPrefs();
  prefs->SetBoolean(brave_tabs::kVerticalTabsCollapsed, true);
  EXPECT_EQ(State::kCollapsed, region_view->state());
  prefs->SetBoolean(brave_tabs::kVerticalTabsCollapsed, false);
  EXPECT_EQ(State::kExpanded, region_view->state());
  prefs->SetBoolean(brave_tabs::kVerticalTabsCollapsed, true);

  // Check if mouse hover triggers floating mode.
  {
    base::AutoReset resetter(&region_view->mouse_events_for_test_, true);
    ui::MouseEvent event(ui::EventType::kMouseEntered, gfx::PointF(),
                         gfx::PointF(), {}, {}, {});
    region_view->OnMouseEntered(event);
    EXPECT_EQ(State::kFloating, region_view->state());
    EXPECT_EQ(inset_for_floating, region_view->GetInsets().width());
  }

  // Check if mouse exiting make tab strip collapsed.
  {
    base::AutoReset resetter(&region_view->mouse_events_for_test_, true);
    ui::MouseEvent event(ui::EventType::kMouseExited, gfx::PointF(),
                         gfx::PointF(), {}, {}, {});
    region_view->OnMouseExited(event);
    EXPECT_EQ(State::kCollapsed, region_view->state());
    EXPECT_EQ(inset_for_expanded_collapsed, region_view->GetInsets().width());
  }

  // When floating mode is disabled, it shouldn't be triggered.
  prefs->SetBoolean(brave_tabs::kVerticalTabsFloatingEnabled, false);
  {
    base::AutoReset resetter(&region_view->mouse_events_for_test_, true);
    ui::MouseEvent event(ui::EventType::kMouseEntered, gfx::PointF(),
                         gfx::PointF(), {}, {}, {});
    region_view->OnMouseEntered(event);
    EXPECT_NE(State::kFloating, region_view->state());
  }
}

#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
// * Mac test bots are not able to enter fullscreen.
// * On Linux this test is flaky.
#define MAYBE_Fullscreen DISABLED_Fullscreen
#else
#define MAYBE_Fullscreen Fullscreen
#endif

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, MAYBE_Fullscreen) {
  ToggleVerticalTabStrip();
  ASSERT_TRUE(browser_view()
                  ->vertical_tab_strip_host_view_->GetPreferredSize()
                  .width());
  auto* fullscreen_controller = browser_view()
                                    ->browser()
                                    ->GetFeatures()
                                    .exclusive_access_manager()
                                    ->fullscreen_controller();
  {
    auto observer = FullscreenNotificationObserver(browser());
    fullscreen_controller->ToggleBrowserFullscreenMode(/*user_initiated=*/true);
    observer.Wait();
  }

  // Vertical tab strip should be invisible on browser fullscreen.
  ASSERT_TRUE(fullscreen_controller->IsFullscreenForBrowser());
  ASSERT_TRUE(browser_view()->IsFullscreen());
  EXPECT_FALSE(browser_view()
                   ->vertical_tab_strip_host_view_->GetPreferredSize()
                   .width());

  {
    auto observer = FullscreenNotificationObserver(browser());
    fullscreen_controller->ToggleBrowserFullscreenMode(/*user_initiated=*/true);
    observer.Wait();
  }
  ASSERT_FALSE(fullscreen_controller->IsFullscreenForBrowser());
  ASSERT_FALSE(browser_view()->IsFullscreen());

  {
    auto observer = FullscreenNotificationObserver(browser());
    // Vertical tab strip should become invisible on tab fullscreen.
    fullscreen_controller->EnterFullscreenModeForTab(
        browser_view()
            ->browser()
            ->tab_strip_model()
            ->GetActiveWebContents()
            ->GetPrimaryMainFrame());

    observer.Wait();
  }
  ASSERT_TRUE(fullscreen_controller->IsTabFullscreen());
  if (!browser_view()
           ->vertical_tab_strip_host_view_->GetPreferredSize()
           .width()) {
    return;
  }

  base::RunLoop run_loop;
  auto wait_until = base::BindLambdaForTesting(
      [&](base::RepeatingCallback<bool()> predicate) {
        if (predicate.Run()) {
          return;
        }

        base::RepeatingTimer scheduler;
        scheduler.Start(FROM_HERE, base::Milliseconds(100),
                        base::BindLambdaForTesting([&]() {
                          if (predicate.Run()) {
                            run_loop.Quit();
                          } else {
                            LOG(ERROR) << browser_view()
                                              ->vertical_tab_strip_host_view_
                                              ->GetPreferredSize()
                                              .width();
                          }
                        }));
        run_loop.Run();
      });

  wait_until.Run(base::BindLambdaForTesting([&]() {
    return !browser_view()
                ->vertical_tab_strip_host_view_->GetPreferredSize()
                .width();
  }));
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, LayoutSanity) {
  // Pre-conditions ------------------------------------------------------------

  ToggleVerticalTabStrip();

  AppendTab(browser());

  auto* widget_delegate_view =
      browser_view()->vertical_tab_strip_widget_delegate_view();
  ASSERT_TRUE(widget_delegate_view);

  auto* region_view = widget_delegate_view->vertical_tab_strip_region_view();
  ASSERT_TRUE(region_view);
  ASSERT_EQ(BraveVerticalTabStripRegionView::State::kExpanded,
            region_view->state());

  auto* model = browser()->tab_strip_model();
  ASSERT_EQ(2, model->count());
  model->SetTabPinned(0, true);

  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();

  // Test if every tabs are laid out inside tab strip region -------------------
  // This is a regression test for
  // https://github.com/brave/brave-browser/issues/28084
  const auto region_view_bounds =
      GetBoundsInScreen(region_view, region_view->GetLocalBounds());
  for (int i = 0; i < model->count(); i++) {
    auto* tab = GetTabAt(browser(), i);
    const auto tab_bounds = GetBoundsInScreen(tab, tab->GetLocalBounds());
    EXPECT_TRUE(region_view_bounds.Contains(tab_bounds))
        << "Region view bounds: " << region_view_bounds.ToString()
        << " vs. Tab bounds: " << tab_bounds.ToString();
  }

  // Check resize area is top-most view.
  auto resize_area_index = region_view->GetIndexOf(region_view->resize_area_);
  EXPECT_TRUE(resize_area_index.has_value() &&
              resize_area_index == region_view->children().size() - 1);
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest,
                       LayoutAfterFirstTabCreation) {
  ToggleVerticalTabStrip();

  auto* widget_delegate_view =
      browser_view()->vertical_tab_strip_widget_delegate_view();
  ASSERT_TRUE(widget_delegate_view);

  auto* region_view = widget_delegate_view->vertical_tab_strip_region_view();
  ASSERT_TRUE(region_view);
  ASSERT_EQ(BraveVerticalTabStripRegionView::State::kExpanded,
            region_view->state());

  auto* model = browser()->tab_strip_model();
  model->SetTabPinned(0, true);
  ASSERT_EQ(1, model->count());

  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();

  // At this point, the contents_view_height already contains spacing after the
  // last pinned tab
  int contents_view_height = region_view->original_region_view_->height();
  AppendTab(browser());  // Add first unpinned tab
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
  InvalidateAndRunLayoutForVerticalTabStrip();

  auto* brave_tab_container = views::AsViewClass<BraveTabContainer>(
      views::AsViewClass<BraveTabStrip>(
          browser_view()->horizontal_tab_strip_for_testing())
          ->GetTabContainerForTesting());
  ASSERT_FALSE(brave_tab_container->GetTabAtModelIndex(1)->data().pinned);
  EXPECT_EQ(brave_tab_container->GetPinnedTabsAreaBottom(),
            brave_tab_container->GetIdealBounds(1).y() -
                /*spacing before the first unpinned tab*/
                tabs::kMarginForVerticalTabContainers)
      << "The firs unpinned tabs y should be aligned to the pinned tab + "
         "separator's bottom";
  contents_view_height += tabs::kPinnedUnpinnedSeparatorHeight;
  contents_view_height += tabs::kVerticalTabsSpacing +
                          tabs::kVerticalTabHeight + tabs::kVerticalTabsSpacing;
  EXPECT_EQ(contents_view_height, region_view->original_region_view_->height());

  // Check first unpinned tab's position in floating mode.
  region_view->SetState(BraveVerticalTabStripRegionView::State::kFloating);
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
  InvalidateAndRunLayoutForVerticalTabStrip();

  EXPECT_EQ(contents_view_height, region_view->original_region_view_->height());
  region_view->SetState(BraveVerticalTabStripRegionView::State::kExpanded);
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
  InvalidateAndRunLayoutForVerticalTabStrip();

  // Check if separator is laid out correctly
  EXPECT_TRUE(brave_tab_container->separator_->GetVisible());
  EXPECT_EQ(brave_tab_container->separator_->bounds().y(),
            brave_tab_container->GetPinnedTabsAreaBottom() -
                tabs::kPinnedUnpinnedSeparatorHeight);

  AppendTab(browser());  // Add second unpinned tab
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
  InvalidateAndRunLayoutForVerticalTabStrip();

  // When second tab is added, height should be increased with tab height plus
  // tab spacing.
  contents_view_height +=
      (tabs::kVerticalTabHeight + tabs::kVerticalTabsSpacing);
  ASSERT_EQ(region_view->original_region_view_->height(), contents_view_height);
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, ScrollBarMode) {
  ToggleVerticalTabStrip();

  auto* prefs = browser()->profile()->GetPrefs();
  auto* pref = prefs->FindPreference(brave_tabs::kVerticalTabsShowScrollbar);

  // Check if the default value is false
  EXPECT_TRUE(pref && pref->IsDefaultValue());
  EXPECT_FALSE(prefs->GetBoolean(brave_tabs::kVerticalTabsShowScrollbar));

  auto* brave_tab_container = views::AsViewClass<BraveTabContainer>(
      views::AsViewClass<BraveTabStrip>(
          browser_view()->horizontal_tab_strip_for_testing())
          ->GetTabContainerForTesting());

  EXPECT_TRUE(brave_tab_container);
  EXPECT_EQ(views::ScrollView::ScrollBarMode::kHiddenButEnabled,
            brave_tab_container->GetScrollBarMode());

  // Turn on the prefs and checks if scrollbar becomes visible
  prefs->SetBoolean(brave_tabs::kVerticalTabsShowScrollbar, true);
  EXPECT_EQ(views::ScrollView::ScrollBarMode::kEnabled,
            brave_tab_container->GetScrollBarMode());

  // Turning off and on vertical tabs and see if the visibility persists.
  ToggleVerticalTabStrip();
  ToggleVerticalTabStrip();
  EXPECT_EQ(views::ScrollView::ScrollBarMode::kEnabled,
            brave_tab_container->GetScrollBarMode());

  // Checks if scrollbar is hidden when the pref is turned off.
  prefs->SetBoolean(brave_tabs::kVerticalTabsShowScrollbar, false);
  EXPECT_EQ(views::ScrollView::ScrollBarMode::kHiddenButEnabled,
            brave_tab_container->GetScrollBarMode());
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest,
                       ScrollBarDisabledWhenHorizontal) {
  // Pre-condition: horizontal tab strip
  ASSERT_FALSE(tabs::utils::ShouldShowBraveVerticalTabs(browser()));

  auto* brave_tab_container = views::AsViewClass<BraveTabContainer>(
      views::AsViewClass<BraveTabStrip>(
          browser_view()->horizontal_tab_strip_for_testing())
          ->GetTabContainerForTesting());

  EXPECT_TRUE(brave_tab_container);
  // Scrollbar should be disabled when not in vertical tab mode
  EXPECT_EQ(views::ScrollView::ScrollBarMode::kDisabled,
            brave_tab_container->GetScrollBarMode());
  EXPECT_FALSE(brave_tab_container->scroll_bar_->GetVisible());

  // Even if the pref is enabled, scrollbar should be disabled in horizontal
  // mode
  auto* prefs = browser()->profile()->GetPrefs();
  prefs->SetBoolean(brave_tabs::kVerticalTabsShowScrollbar, true);
  EXPECT_EQ(views::ScrollView::ScrollBarMode::kDisabled,
            brave_tab_container->GetScrollBarMode());
  EXPECT_FALSE(brave_tab_container->scroll_bar_->GetVisible());
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest,
                       ScrollBarVisibilityWithManyTabs) {
  ToggleVerticalTabStrip();

  auto* prefs = browser()->profile()->GetPrefs();
  prefs->SetBoolean(brave_tabs::kVerticalTabsShowScrollbar, true);

  auto* brave_tab_container = views::AsViewClass<BraveTabContainer>(
      views::AsViewClass<BraveTabStrip>(
          browser_view()->horizontal_tab_strip_for_testing())
          ->GetTabContainerForTesting());

  EXPECT_TRUE(brave_tab_container);
  EXPECT_EQ(views::ScrollView::ScrollBarMode::kEnabled,
            brave_tab_container->GetScrollBarMode());
  // Scrollbar should be invisible as max scroll offset is 0
  EXPECT_EQ(0, brave_tab_container->GetMaxScrollOffset());
  EXPECT_FALSE(brave_tab_container->scroll_bar_->GetVisible());

  // Add many tabs to trigger scrollbar visibility
  // The scrollbar should be visible when content height exceeds viewport
  for (int i = 0; i < 50; i++) {
    AppendTab(browser());
  }

  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
  EXPECT_GT(brave_tab_container->GetMaxScrollOffset(), 0);

  // After adding many tabs, scrollbar mode should still be enabled
  EXPECT_EQ(views::ScrollView::ScrollBarMode::kEnabled,
            brave_tab_container->GetScrollBarMode());

  // And scrollbar should be visible as max scroll offset is greater than 0
  EXPECT_TRUE(brave_tab_container->scroll_bar_->GetVisible());
}

// Due to flakiness, this test is disabled
IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest,
                       DISABLED_ScrollBarBoundsWithPinnedTabs) {
  ToggleVerticalTabStrip();

  auto* prefs = browser()->profile()->GetPrefs();
  prefs->SetBoolean(brave_tabs::kVerticalTabsShowScrollbar, true);

  auto* brave_tab_container = views::AsViewClass<BraveTabContainer>(
      views::AsViewClass<BraveTabStrip>(
          browser_view()->horizontal_tab_strip_for_testing())
          ->GetTabContainerForTesting());

  EXPECT_TRUE(brave_tab_container);

  // Add many tabs to make scrollbar visible
  for (int i = 0; i < 30; i++) {
    AppendTab(browser());
  }
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();

  ASSERT_TRUE(brave_tab_container->scroll_bar_->GetVisible());

  views::ScrollBar* scroll_bar = brave_tab_container->scroll_bar_.get();
  EXPECT_TRUE(scroll_bar);
  EXPECT_TRUE(scroll_bar->GetVisible());

  // Get initial scrollbar bounds when no pinned tabs
  gfx::Rect initial_bounds = scroll_bar->bounds();
  EXPECT_GT(initial_bounds.height(), 0);
  EXPECT_EQ(initial_bounds.y(),
            0);  // Should start from top when no pinned tabs
  int initial_pinned_area_bottom =
      brave_tab_container->GetPinnedTabsAreaBottom();
  EXPECT_EQ(initial_pinned_area_bottom, 0);

  auto* model = browser()->tab_strip_model();

  // Pin first tab and check if scrollbar bounds are updated
  model->SetTabPinned(0, true);
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
  InvalidateAndRunLayoutForVerticalTabStrip();

  // Get pinned area bottom after pinning
  // Note: GetPinnedTabsAreaBottom() uses GetIdealBounds() which may not be
  // updated immediately, so we check the actual scrollbar bounds instead
  int pinned_area_bottom = brave_tab_container->GetPinnedTabsAreaBottom();
  ASSERT_GE(pinned_area_bottom, 0);

  // At least verify that scrollbar bounds changed
  gfx::Rect bounds_after_pinning = scroll_bar->bounds();
  EXPECT_GT(bounds_after_pinning.y(), initial_bounds.y());

  // Verify scrollbar bounds are updated
  EXPECT_EQ(bounds_after_pinning.y(), pinned_area_bottom);
  // Height should be container height minus pinned area bottom
  EXPECT_EQ(bounds_after_pinning.height(),
            brave_tab_container->height() - pinned_area_bottom);

  // Pin more tabs and verify bounds continue to update
  while (brave_tab_container->GetPinnedTabsAreaBottom() <= pinned_area_bottom) {
    model->SetTabPinned(model->IndexOfFirstNonPinnedTab(), true);
    browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
    InvalidateAndRunLayoutForVerticalTabStrip();
  }
  pinned_area_bottom = brave_tab_container->GetPinnedTabsAreaBottom();

  // After pinning multiple tabs, scrollbar should be positioned below pinned
  // area
  gfx::Rect bounds_after_pinning_multiple = scroll_bar->bounds();
  EXPECT_EQ(bounds_after_pinning_multiple.y(), pinned_area_bottom);
  EXPECT_EQ(bounds_after_pinning_multiple.height(),
            brave_tab_container->height() - pinned_area_bottom);

  // Unpin all tabs and verify bounds return to initial state
  while (model->IndexOfFirstNonPinnedTab() != 0) {
    model->SetTabPinned(0, false);
  }
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
  InvalidateAndRunLayoutForVerticalTabStrip();
  pinned_area_bottom = brave_tab_container->GetPinnedTabsAreaBottom();
  ASSERT_EQ(pinned_area_bottom, 0);

  // After unpinning, scrollbar should return to top when no pinned tabs
  gfx::Rect bounds_after_unpinning = scroll_bar->bounds();
  EXPECT_EQ(bounds_after_unpinning.y(), 0);
  EXPECT_EQ(bounds_after_unpinning.height(), brave_tab_container->height());
}

// Due to flakiness, this test is disabled
IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest,
                       DISABLED_ScrollBarThumbState) {
  ToggleVerticalTabStrip();

  auto* prefs = browser()->profile()->GetPrefs();
  prefs->SetBoolean(brave_tabs::kVerticalTabsShowScrollbar, true);

  auto* brave_tab_container = views::AsViewClass<BraveTabContainer>(
      views::AsViewClass<BraveTabStrip>(
          browser_view()->horizontal_tab_strip_for_testing())
          ->GetTabContainerForTesting());

  EXPECT_TRUE(brave_tab_container);

  // Add many tabs to make scrollbar visible and enable scrolling
  for (int i = 0; i < 30; i++) {
    AppendTab(browser());
  }
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
  InvalidateAndRunLayoutForVerticalTabStrip();
  views::ScrollBar* scroll_bar = brave_tab_container->scroll_bar_.get();
  ASSERT_TRUE(scroll_bar->GetVisible());
  const int track_bounds_y = scroll_bar->GetTrackBounds().y();

  EXPECT_TRUE(scroll_bar);
  EXPECT_TRUE(scroll_bar->GetVisible());

  // Get initial thumb state
  EXPECT_EQ(scroll_bar->GetMinPosition(), 0);
  EXPECT_GT(scroll_bar->GetMaxPosition(), 0);

  // ## Scroll to middle
  int middle_offset = brave_tab_container->GetMaxScrollOffset() / 2;
  ASSERT_GT(middle_offset, 0);

  brave_tab_container->SetScrollOffset(middle_offset);
  ASSERT_EQ(scroll_bar->GetTrackBounds().y(), track_bounds_y)
      << "Track bounds y should not changed";

  // Verify scroll offset was set correctly
  ASSERT_EQ(brave_tab_container->scroll_offset_, middle_offset);

  int position_at_middle = scroll_bar->GetPosition();
  EXPECT_GT(position_at_middle, 0) << scroll_bar->GetTrackBounds().y();
  EXPECT_LT(position_at_middle, scroll_bar->GetMaxPosition());

  // ## Scroll to maximum
  brave_tab_container->SetScrollOffset(
      brave_tab_container->GetMaxScrollOffset());
  ASSERT_EQ(scroll_bar->GetTrackBounds().y(), track_bounds_y)
      << "Track bounds y should not changed";

  // Verify scroll offset was set correctly
  ASSERT_EQ(brave_tab_container->scroll_offset_,
            brave_tab_container->GetMaxScrollOffset());
  EXPECT_GT(scroll_bar->GetPosition(), position_at_middle);

  // ## Scroll back to top
  brave_tab_container->SetScrollOffset(0);
  ASSERT_EQ(scroll_bar->GetTrackBounds().y(), track_bounds_y)
      << "Track bounds y should not changed";
  // Verify scroll offset was reset
  ASSERT_EQ(brave_tab_container->scroll_offset_, 0);
  EXPECT_EQ(scroll_bar->GetPosition(), 0);

  // Verify min position remains 0
  EXPECT_EQ(scroll_bar->GetMinPosition(), 0);
  EXPECT_GT(scroll_bar->GetMaxPosition(), 0);
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, RichAnimationIsDisabled) {
  // Regression test for https://github.com/brave/brave-browser/issues/52044
  // Given that rich animation is disabled,
  auto scoped_mode = gfx::AnimationTestApi::SetRichAnimationRenderMode(
      gfx::Animation::RichAnimationRenderMode::FORCE_DISABLED);
  ASSERT_FALSE(gfx::Animation::ShouldRenderRichAnimation());

  ToggleVerticalTabStrip();

  auto* brave_tab_container = views::AsViewClass<BraveTabContainer>(
      views::AsViewClass<BraveTabStrip>(
          browser_view()->horizontal_tab_strip_for_testing())
          ->GetTabContainerForTesting());

  EXPECT_TRUE(brave_tab_container);

  // Add many tabs to make scrollbar visible and enable scrolling
  for (int i = 0; i < 30; i++) {
    AppendTab(browser());
  }
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
  brave_tab_container->SetScrollOffset(
      brave_tab_container->GetMaxScrollOffset());
  InvalidateAndRunLayoutForVerticalTabStrip();

  // When closing the last tab from the scrollable vertical tab strip,
  // It should not fall to infinite loop.
  auto* model = browser()->tab_strip_model();
  model->CloseWebContentsAt(model->count() - 1,
                            TabCloseTypes::CLOSE_USER_GESTURE);
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest,
                       BraveTabContainerSeparator) {
  auto* brave_tab_container = views::AsViewClass<BraveTabContainer>(
      views::AsViewClass<BraveTabStrip>(
          browser_view()->horizontal_tab_strip_for_testing())
          ->GetTabContainerForTesting());
  EXPECT_FALSE(brave_tab_container->separator_->GetVisible());

  auto* model = browser()->tab_strip_model();
  model->SetTabPinned(0, true);
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
  EXPECT_FALSE(brave_tab_container->separator_->GetVisible());

  AppendTab(browser());
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
  EXPECT_FALSE(brave_tab_container->separator_->GetVisible());

  ToggleVerticalTabStrip();
  EXPECT_TRUE(brave_tab_container->separator_->GetVisible());

  auto* tab_strip = browser_view()->horizontal_tab_strip_for_testing();
  EXPECT_EQ(
      tab_strip->tab_at(0)->bounds().bottom() + tabs::kVerticalTabsSpacing,
      brave_tab_container->separator_->bounds().y());

  model->SetTabPinned(0, false);
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
  EXPECT_FALSE(brave_tab_container->separator_->GetVisible());

  // Add enough pinned tabs to move separator bounds by creating unpinned tab
  // and pinning it. Check separator bounds is updated properly after pinning
  // new tab.
  for (int i = 0; i < 20; i++) {
    AppendTab(browser());
    model->SetTabPinned(model->count() - 1, true);
    browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
    EXPECT_EQ(tab_strip->tab_at(model->IndexOfFirstNonPinnedTab() - 1)
                      ->bounds()
                      .bottom() +
                  tabs::kVerticalTabsSpacing,
              brave_tab_container->separator_->bounds().y());
  }

  // Check separator bounds by unpinning all tabs.
  const int tab_count = model->count();
  for (int i = 0; i < tab_count; i++) {
    model->SetTabPinned(i, false);
    browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();

    const int first_unpinned_tab_index = model->IndexOfFirstNonPinnedTab();
    if (first_unpinned_tab_index == 0) {
      EXPECT_FALSE(brave_tab_container->separator_->GetVisible());
    } else {
      EXPECT_EQ(
          tab_strip->tab_at(first_unpinned_tab_index - 1)->bounds().bottom() +
              tabs::kVerticalTabsSpacing,
          brave_tab_container->separator_->bounds().y());
    }
  }
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, ExpandedState) {
  // Given that kVerticalTabsExpandedStatePerWindow is false,
  auto* prefs = browser()->profile()->GetPrefs();
  ASSERT_FALSE(
      prefs->GetBoolean(brave_tabs::kVerticalTabsExpandedStatePerWindow));

  // When clicking the toggle button,
  using State = BraveVerticalTabStripRegionView::State;
  auto* region_view_1 = browser_view()
                            ->vertical_tab_strip_widget_delegate_view_
                            ->vertical_tab_strip_region_view();
  ASSERT_TRUE(region_view_1);
  ASSERT_EQ(State::kExpanded, region_view_1->state());

  region_view_1->GetToggleButtonForTesting().button_controller()->NotifyClick();
  EXPECT_EQ(State::kCollapsed, region_view_1->state());
  EXPECT_TRUE(prefs->GetBoolean(brave_tabs::kVerticalTabsCollapsed));

  // it affects all browsers.
  auto* region_view_2 =
      static_cast<BraveBrowserView*>(
          Browser::Create(Browser::CreateParams(browser()->profile(), true))
              ->window())
          ->vertical_tab_strip_widget_delegate_view_
          ->vertical_tab_strip_region_view();
  EXPECT_EQ(State::kCollapsed, region_view_2->state());

  // Given that kVerticalTabsExpandedStatePerWindow is true,
  prefs->SetBoolean(brave_tabs::kVerticalTabsExpandedStatePerWindow, true);

  // When clicking the toggle button,
  region_view_1->GetToggleButtonForTesting().button_controller()->NotifyClick();

  // it affects only the browser
  EXPECT_EQ(State::kExpanded, region_view_1->state());
  EXPECT_FALSE(prefs->GetBoolean(brave_tabs::kVerticalTabsCollapsed));
  EXPECT_EQ(State::kCollapsed, region_view_2->state());

  // Check expanded state is toggled via command.
  auto* command_controller = browser()->command_controller();
  command_controller->ExecuteCommandWithDisposition(
      IDC_TOGGLE_VERTICAL_TABS_EXPANDED, WindowOpenDisposition::CURRENT_TAB);
  EXPECT_EQ(State::kCollapsed, region_view_1->state());

  // And new browser should follow the preference.
  prefs->SetBoolean(brave_tabs::kVerticalTabsCollapsed, true);
  auto* region_view_3 =
      static_cast<BraveBrowserView*>(
          Browser::Create(Browser::CreateParams(browser()->profile(), true))
              ->window())
          ->vertical_tab_strip_widget_delegate_view_
          ->vertical_tab_strip_region_view();
  EXPECT_EQ(State::kCollapsed, region_view_3->state());
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, ExpandedWidth) {
  // Given that kVerticalTabsExpandedStatePerWindow is false,
  auto* prefs = browser()->profile()->GetPrefs();
  ASSERT_FALSE(
      prefs->GetBoolean(brave_tabs::kVerticalTabsExpandedStatePerWindow));

  // When setting the expanded width,
  using State = BraveVerticalTabStripRegionView::State;
  auto* region_view_1 = browser_view()
                            ->vertical_tab_strip_widget_delegate_view_
                            ->vertical_tab_strip_region_view();
  ASSERT_TRUE(region_view_1);
  ASSERT_EQ(State::kExpanded, region_view_1->state());

  region_view_1->SetExpandedWidth(100);
  EXPECT_EQ(100, region_view_1->expanded_width_);
  EXPECT_EQ(100, prefs->GetValue(brave_tabs::kVerticalTabsExpandedWidth));

  // it affects all browsers.
  auto* region_view_2 =
      static_cast<BraveBrowserView*>(
          Browser::Create(Browser::CreateParams(browser()->profile(), true))
              ->window())
          ->vertical_tab_strip_widget_delegate_view_
          ->vertical_tab_strip_region_view();
  EXPECT_EQ(100, region_view_2->expanded_width_);

  // Given that kVerticalTabsExpandedStatePerWindow is true,
  prefs->SetBoolean(brave_tabs::kVerticalTabsExpandedStatePerWindow, true);

  // When clicking the toggle button,
  region_view_1->SetExpandedWidth(200);

  // it affects only the browser
  EXPECT_EQ(200, region_view_1->expanded_width_);
  EXPECT_EQ(200, prefs->GetValue(brave_tabs::kVerticalTabsExpandedWidth));
  EXPECT_EQ(100, region_view_2->expanded_width_);

  // And new browser should follow the preference.
  prefs->SetBoolean(brave_tabs::kVerticalTabsCollapsed, true);
  auto* region_view_3 =
      static_cast<BraveBrowserView*>(
          Browser::Create(Browser::CreateParams(browser()->profile(), true))
              ->window())
          ->vertical_tab_strip_widget_delegate_view_
          ->vertical_tab_strip_region_view();
  EXPECT_EQ(200, region_view_3->expanded_width_);
}

class VerticalTabStripStringBrowserTest : public VerticalTabStripBrowserTest {
 public:
  using VerticalTabStripBrowserTest::VerticalTabStripBrowserTest;
  ~VerticalTabStripStringBrowserTest() override = default;

  // VerticalTabStripBrowserTest:
  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII("lang", "en");
    VerticalTabStripBrowserTest::SetUp();
  }

  std::unique_ptr<TabContextMenuController> CreateMenuControllerAt(
      int tab_index) {
    auto* controller = static_cast<BraveBrowserTabStripController*>(
        BrowserView::GetBrowserViewForBrowser(browser())
            ->horizontal_tab_strip_for_testing()
            ->controller());

    auto context_menu_controller =
        std::make_unique<TabContextMenuController>(tab_index, controller);

    return context_menu_controller;
  }

  ui::SimpleMenuModel* CreateMenuModelAt(
      TabContextMenuController* context_menu_controller,
      int tab_index) {
    brave::BraveTabMenuModelFactory factory;
    auto model =
        factory.Create(context_menu_controller,
                       browser()->GetFeatures().tab_menu_model_delegate(),
                       browser()->tab_strip_model(), tab_index);

    auto* model_ptr = model.get();
    context_menu_controller->LoadModel(std::move(model));

    return model_ptr;
  }
};

IN_PROC_BROWSER_TEST_F(VerticalTabStripStringBrowserTest, ContextMenuString) {
  // Pre-conditions ------------------------------------------------------------
  auto get_all_labels = [&]() {
    auto menu = CreateMenuControllerAt(/*tab_index=*/0);
    auto* menu_model = CreateMenuModelAt(menu.get(), /*tab_index=*/0);
    std::vector<std::u16string> labels;
    for (auto i = 0u; i < menu_model->GetItemCount(); i++) {
      labels.push_back(menu_model->GetLabelAt(i));
    }
    return labels;
  };

  {
    ASSERT_FALSE(get_all_labels().empty());
  }

  // Tests ---------------------------------------------------------------------
  {
    // Check if there's no "Below" in context menu labels when it's horizontal
    // tab strip
    EXPECT_TRUE(std::ranges::none_of(get_all_labels(), [](const auto& label) {
#if BUILDFLAG(IS_MAC)
      return base::Contains(label, u"Below");
#else
      return base::Contains(label, u"below");
#endif
    }));
  }

  ToggleVerticalTabStrip();
  {
    // Check if there's no "Right" or "Left" in context menu labels when it's
    // vertical tab strip. When this fails, we should revisit
    // BraveTabMenuModel::GetLabelAt().
    EXPECT_TRUE(std::ranges::none_of(get_all_labels(), [](const auto& label) {
#if BUILDFLAG(IS_MAC)
      return base::Contains(label, u"Right") || base::Contains(label, u"Left");
#else
      return base::Contains(label, u"right") ||
          base::Contains(label, u"left");
#endif
    }));
  }
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, PinningGroupedTab) {
  auto* tab_groups_service =
      tab_groups::TabGroupSyncServiceFactory::GetForProfile(
          browser()->profile());
  ASSERT_TRUE(tab_groups_service);
  tab_groups_service->SetIsInitializedForTesting(true);

  // Regression check for https://github.com/brave/brave-browser/issues/40201
  ToggleVerticalTabStrip();

  AppendTab(browser());
  AppendTab(browser());
  AppendTab(browser());

  tab_groups::TabGroupId group = AddTabToNewGroup(browser(), 0);
  AddTabToExistingGroup(browser(), 1, group);
  AddTabToExistingGroup(browser(), 2, group);
  AddTabToExistingGroup(browser(), 3, group);

  browser()->tab_strip_model()->SetTabPinned(1, true);
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
  EXPECT_EQ(GetTabStrip(browser())->tab_at(0)->group(), std::nullopt);

  browser()->tab_strip_model()->SetTabPinned(2, true);
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
  EXPECT_EQ(GetTabStrip(browser())->tab_at(1)->group(), std::nullopt);

  ASSERT_TRUE(GetTabStrip(browser())->tab_at(2)->group().has_value());
  EXPECT_EQ(GetTabStrip(browser())->tab_at(2)->group().value(), group);
  ASSERT_TRUE(GetTabStrip(browser())->tab_at(3)->group().has_value());
  EXPECT_EQ(GetTabStrip(browser())->tab_at(3)->group().value(), group);
}

class VerticalTabStripDragAndDropBrowserTest
    : public VerticalTabStripBrowserTest {
 public:
  using VerticalTabStripBrowserTest::VerticalTabStripBrowserTest;
  ~VerticalTabStripDragAndDropBrowserTest() override = default;

  gfx::Point GetCenterPointInScreen(views::View* view) {
    return GetBoundsInScreen(view, view->GetLocalBounds()).CenterPoint();
  }

  void PressTabAt(Browser* browser, int index) {
    ASSERT_TRUE(ui_test_utils::SendMouseMoveSync(
        GetCenterPointInScreen(GetTabAt(browser, index))));
    ASSERT_TRUE(ui_test_utils::SendMouseEventsSync(ui_controls::LEFT,
                                                   ui_controls::DOWN));
  }

  void ReleaseMouse() {
    ASSERT_TRUE(
        ui_controls::SendMouseEvents(ui_controls::LEFT, ui_controls::UP));
  }

  void MoveMouseTo(
      const gfx::Point& point_in_screen,
      base::OnceClosure task_on_mouse_moved = base::NullCallback()) {
    bool moved = false;
    ui_controls::SendMouseMoveNotifyWhenDone(
        point_in_screen.x(), point_in_screen.y(),
        base::BindLambdaForTesting([&]() {
          moved = true;
          if (task_on_mouse_moved) {
            std::move(task_on_mouse_moved).Run();
          }
        }));
    WaitUntil(base::BindLambdaForTesting([&]() { return moved; }));
  }

  bool IsDraggingTabStrip(Browser* b) {
    return GetTabStrip(b)->GetDragContext()->GetDragController() != nullptr;
  }

  // VerticalTabStripBrowserTest:
  void SetUpOnMainThread() override {
    VerticalTabStripBrowserTest::SetUpOnMainThread();

#if BUILDFLAG(IS_WIN)
    aura::test::EnableUIControlsAuraWin();

    auto* widget_delegate_view =
        browser_view()->vertical_tab_strip_widget_delegate_view_.get();
    ASSERT_TRUE(widget_delegate_view);
#endif  // defined(IS_WIN)

#if BUILDFLAG(IS_OZONE)
    // Notifies the platform that test config is needed. For Wayland, for
    // example, makes it possible to use emulated input.
    ui::test::EnableTestConfigForPlatformWindows();

    ui::OzonePlatform::InitParams params;
    params.single_process = true;
    ui::OzonePlatform::InitializeForUI(params);
#endif

#if !BUILDFLAG(IS_WIN)
    ui_controls::EnableUIControls();
#endif

    ToggleVerticalTabStrip();

#if BUILDFLAG(IS_WIN)
    // Sometimes, the window is not activated and it causes flakiness. In order
    // to make sure the window is the front, do these.
    browser()->window()->Minimize();
    browser()->window()->Restore();
    browser()->window()->Activate();
#endif
  }
};

// Before we have our own interactive ui tests, we need to disable this test as
// it's flaky when running test suits.
#define MAYBE_DragTabToReorder DISABLED_DragTabToReorder

IN_PROC_BROWSER_TEST_F(VerticalTabStripDragAndDropBrowserTest,
                       MAYBE_DragTabToReorder) {
  // Pre-conditions ------------------------------------------------------------
  AppendTab(browser());

  auto* widget_delegate_view =
      browser_view()->vertical_tab_strip_widget_delegate_view_.get();
  ASSERT_TRUE(widget_delegate_view);

  auto* region_view = widget_delegate_view->vertical_tab_strip_region_view();
  ASSERT_TRUE(region_view);
  ASSERT_EQ(BraveVerticalTabStripRegionView::State::kExpanded,
            region_view->state());

  // Drag and drop a tab to reorder it -----------------------------------------
  GetTabStrip(browser())->StopAnimating();  // Drag-and-drop doesn't start when
                                            // animation is running.
  auto* pressed_tab = GetTabAt(browser(), 0);
  PressTabAt(browser(), 0);
  auto point_to_move_to = GetCenterPointInScreen(GetTabAt(browser(), 1));
  point_to_move_to.set_y(point_to_move_to.y() + pressed_tab->height());
  for (gfx::Point pos = GetCenterPointInScreen(pressed_tab);
       pos != point_to_move_to; pos.set_y(pos.y() + 1)) {
    MoveMouseTo(pos);
  }

  if (!IsDraggingTabStrip(browser())) {
    // Even when we try to simulate drag-n-drop, some CI node seems to fail
    // to enter drag-n-drop mode. In this case, we can't proceed to further test
    // so just return.
    return;
  }

  WaitUntil(base::BindLambdaForTesting(
      [&]() { return pressed_tab == GetTabAt(browser(), 1); }));

  EXPECT_TRUE(IsDraggingTabStrip(browser()));
  ReleaseMouse();
  GetTabStrip(browser())->StopAnimating();  // Drag-and-drop doesn't start when
                                            // animation is running.
  {
    // Regression test for https://github.com/brave/brave-browser/issues/28488
    // Check if the tab is positioned properly after drag-and-drop.
    auto* moved_tab = GetTabAt(browser(), 1);
    EXPECT_TRUE(GetBoundsInScreen(region_view, region_view->GetLocalBounds())
                    .Contains(GetBoundsInScreen(moved_tab,
                                                moved_tab->GetLocalBounds())));
  }
}

// Before we have our own interactive ui tests, we need to disable this test as
// it's flaky when running test suits.
#define MAYBE_DragTabToDetach DISABLED_DragTabToDetach

IN_PROC_BROWSER_TEST_F(VerticalTabStripDragAndDropBrowserTest,
                       MAYBE_DragTabToDetach) {
  // Pre-conditions ------------------------------------------------------------
  AppendTab(browser());

  // Drag a tab out of tab strip to create browser -----------------------------
  GetTabStrip(browser())->StopAnimating();  // Drag-and-drop doesn't start when
                                            // animation is running.
  PressTabAt(browser(), 0);
  gfx::Point point_out_of_tabstrip =
      GetCenterPointInScreen(GetTabAt(browser(), 0));
  point_out_of_tabstrip.set_x(point_out_of_tabstrip.x() +
                              2 * GetTabAt(browser(), 0)->width());
  MoveMouseTo(
      point_out_of_tabstrip, base::BindLambdaForTesting([&]() {
        // Creating new browser during drag-and-drop will create
        // a nested run loop. So we should do things within callback.
        auto* browser_list = BrowserList::GetInstance();
        EXPECT_EQ(2, std::ranges::count_if(*browser_list, [&](Browser* b) {
                    return b->profile() == browser()->profile();
                  }));
        auto* new_browser = GetLastActiveBrowserWindowInterfaceWithAnyProfile();
        auto* browser_view = BrowserView::GetBrowserViewForBrowser(new_browser);
        auto* tab = browser_view->horizontal_tab_strip_for_testing()->tab_at(0);
        ASSERT_TRUE(tab);
        // During the tab detaching, mouse should be over the dragged
        // tab.
        EXPECT_TRUE(tab->IsMouseHovered());
        EXPECT_TRUE(tab->dragging());
        ReleaseMouse();
        new_browser->GetWindow()->Close();
      }));
}

// Before we have our own interactive ui tests, we need to disable this test as
// it's flaky when running test suits.
#define MAYBE_DragURL DISABLED_DragURL

IN_PROC_BROWSER_TEST_F(VerticalTabStripDragAndDropBrowserTest, MAYBE_DragURL) {
  // Pre-conditions ------------------------------------------------------------
  auto convert_point_in_screen = [&](views::View* view,
                                     const gfx::Point& point) {
    auto point_in_screen = point;
    views::View::ConvertPointToScreen(view, &point_in_screen);
    return point_in_screen;
  };

  auto press_view = [&](views::View* view) {
    ASSERT_TRUE(ui_test_utils::SendMouseMoveSync(
        convert_point_in_screen(view, view->GetLocalBounds().CenterPoint())));
    ASSERT_TRUE(ui_test_utils::SendMouseEventsSync(ui_controls::LEFT,
                                                   ui_controls::DOWN));
  };

  auto drag_mouse_to_point_and_drop = [&](const gfx::Point& point_in_screen) {
    bool moved = false;
    ui_controls::SendMouseMoveNotifyWhenDone(
        point_in_screen.x(), point_in_screen.y(),
        base::BindLambdaForTesting([&]() {
          moved = true;
          ui_controls::SendMouseEvents(ui_controls::LEFT, ui_controls::UP);
        }));

    WaitUntil(base::BindLambdaForTesting([&]() { return moved; }));
  };

  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("https://brave.com/")));

  // Test if dragging a URL on browser cause a crash. When this happens, the
  // browser root view could try inserting a new tab with the given URL.
  // https://github.com/brave/brave-browser/issues/28592
  auto* location_icon_view =
      browser_view()->GetLocationBarView()->location_icon_view();
  press_view(location_icon_view);

  auto position_to_drag_to =
      convert_point_in_screen(location_icon_view, location_icon_view->origin());
  position_to_drag_to.set_x(position_to_drag_to.x() - 3);
  drag_mouse_to_point_and_drop(
      position_to_drag_to);  // This shouldn't end up in a crash
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, Sanity) {
  // Make sure browser works with both vertical tab and scrollable tab strip
  // https://github.com/brave/brave-browser/issues/28877
  ToggleVerticalTabStrip();
  Browser::Create(Browser::CreateParams(browser()->profile(), true));
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, ToggleWithGroups) {
  // Deflake the test by setting TabGroupSyncService initialized.
  tab_groups::TabGroupSyncService* service =
      tab_groups::TabGroupSyncServiceFactory::GetForProfile(
          browser()->profile());
  service->SetIsInitializedForTesting(true);

  // Make sure browser works with both vertical tab and scrollable tab strip
  // even with groups.
  // https://github.com/brave/brave-browser/issues/46615
  AddTabToNewGroup(browser(), 0);
  ToggleVerticalTabStrip();  // To vertical tab strip
  ToggleVerticalTabStrip();  // To horizontal tab strip
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, ScrollOffset) {
  ToggleVerticalTabStrip();

  auto* brave_tab_container = views::AsViewClass<BraveTabContainer>(
      views::AsViewClass<BraveTabStrip>(
          browser_view()->horizontal_tab_strip_for_testing())
          ->GetTabContainerForTesting());
  ASSERT_TRUE(brave_tab_container);

  auto* model = browser()->tab_strip_model();
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();

  // Pre-condition: With only one tab, max scroll offset should be 0
  ASSERT_EQ(1, model->count());
  EXPECT_EQ(0, brave_tab_container->GetMaxScrollOffset());

  // Adding tabs until they hit the height of the tab strip. When they exceed
  // the height, max scroll offset should be greater than 0.
  while (brave_tab_container->GetMaxScrollOffset() <= 0) {
    AppendTab(browser());
    browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
  }

  // ## Basic test -------------------------------------------------------------
  // Max scroll offset should be the total height of unpinned tabs minus the
  // height of the container.
  auto unpinned_tabs_total_height = [](int unpinned_tab_count) {
    return unpinned_tab_count *
               (tabs::kVerticalTabHeight + tabs::kVerticalTabsSpacing) -
           tabs::kVerticalTabsSpacing +
           2 * tabs::kMarginForVerticalTabContainers;
  };

  auto available_height = [&]() {
    return brave_tab_container->height() -
           brave_tab_container->GetPinnedTabsAreaBottom();
  };
  EXPECT_EQ(unpinned_tabs_total_height(model->count()) - available_height(),
            brave_tab_container->GetMaxScrollOffset());

  // When adding foreground tabs, the current scroll offset should be updated
  // so that the new active tab is visible.
  EXPECT_EQ(brave_tab_container->scroll_offset_,
            brave_tab_container->GetMaxScrollOffset());

  // ## Pinning a tab test -----------------------------------------------------
  // Add a few more tabs for further testing
  while (brave_tab_container->GetMaxScrollOffset() <
         5 * tabs::kVerticalTabHeight) {
    AppendTab(browser());
    browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
    InvalidateAndRunLayoutForVerticalTabStrip();
  }
  // Make sure that the container has a reasonable height.
  ASSERT_GT(brave_tab_container->height(), 40);

  // When pin a tab from the last, pinned tabs area should be updated.
  const int max_scroll_offset_before_pinning =
      brave_tab_container->GetMaxScrollOffset();
  int scroll_offset_before_pinning = brave_tab_container->scroll_offset_;
  ASSERT_EQ(max_scroll_offset_before_pinning, scroll_offset_before_pinning)
      << "As we added active tabs at the end, the scroll offset should be "
         "equal to the max scroll offset";

  model->SetTabPinned(model->count() - 1, true);
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
  ASSERT_EQ(1, model->IndexOfFirstNonPinnedTab());
  EXPECT_EQ(brave_tab_container->GetPinnedTabsAreaBottom(),
            tabs::kVerticalTabHeight +
                2 * tabs::kMarginForVerticalTabContainers +
                tabs::kPinnedUnpinnedSeparatorHeight);

  // Also max scroll offset should be updated.
  EXPECT_EQ(max_scroll_offset_before_pinning - tabs::kVerticalTabHeight -
                tabs::kVerticalTabsSpacing +
                brave_tab_container->GetPinnedTabsAreaBottom(),
            brave_tab_container->GetMaxScrollOffset());
  EXPECT_EQ(unpinned_tabs_total_height(model->count() -
                                       model->IndexOfFirstNonPinnedTab()) -
                available_height(),
            brave_tab_container->GetMaxScrollOffset());

  // Pin the last tab again, so that the max scroll offset could be smaller
  scroll_offset_before_pinning = brave_tab_container->scroll_offset_;
  model->SetTabPinned(model->count() - 1, true);
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();

  // Then, current scroll offset should be clamped to the max scroll offset.
  EXPECT_GT(scroll_offset_before_pinning, brave_tab_container->scroll_offset_);
  EXPECT_EQ(brave_tab_container->scroll_offset_,
            brave_tab_container->GetMaxScrollOffset());

  // ## Unpin the tab
  model->SetTabPinned(0, false);
  model->SetTabPinned(0, false);
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
  ASSERT_EQ(0, brave_tab_container->GetPinnedTabsAreaBottom());

  // Max scroll offset should be restored after unpinning
  EXPECT_EQ(max_scroll_offset_before_pinning,
            brave_tab_container->GetMaxScrollOffset());
  EXPECT_EQ(unpinned_tabs_total_height(model->count()) - available_height(),
            brave_tab_container->GetMaxScrollOffset());

  // ## Removing a tab
  // Scroll offset should be updated
  int scroll_offset_before_removing = brave_tab_container->scroll_offset_;
  model->SelectLastTab();
  model->CloseWebContentsAt(0, TabCloseTypes::CLOSE_USER_GESTURE);
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
  ASSERT_EQ(model->GetIndexOfWebContents(model->GetActiveWebContents()),
            model->count() - 1);

  // Max scroll offset should be updated
  EXPECT_EQ(unpinned_tabs_total_height(model->count()) - available_height(),
            brave_tab_container->GetMaxScrollOffset());
  // also the current scroll offset should be clamped to the max scroll offset.
  EXPECT_GT(scroll_offset_before_removing, brave_tab_container->scroll_offset_);
  EXPECT_EQ(brave_tab_container->scroll_offset_,
            brave_tab_container->GetMaxScrollOffset());
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, ClipPathOnScrollOffset) {
  // https://github.com/brave/brave-browser/issues/51734
  ToggleVerticalTabStrip();

  auto* brave_tab_container = views::AsViewClass<BraveTabContainer>(
      views::AsViewClass<BraveTabStrip>(
          browser_view()->horizontal_tab_strip_for_testing())
          ->GetTabContainerForTesting());
  ASSERT_TRUE(brave_tab_container);

  auto* model = browser()->tab_strip_model();
  model->SetTabPinned(0, true);

  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();

  // Add enough tabs to make the tab strip scrollable
  while (brave_tab_container->GetMaxScrollOffset() <=
         5 * tabs::kVerticalTabHeight) {
    AppendTab(browser());
    browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();

    InvalidateAndRunLayoutForVerticalTabStrip();
  }
  const int container_height = brave_tab_container->height();
  ASSERT_GT(container_height, 40);

  const int pinned_tabs_area_bottom =
      brave_tab_container->GetPinnedTabsAreaBottom();

  ASSERT_GT(pinned_tabs_area_bottom, 0);
  ASSERT_NE(brave_tab_container->scroll_offset_, 0);

  // Set scroll offset to 0 (top)
  brave_tab_container->SetScrollOffset(0);
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();

  // Verify that UpdateClipPathForSlotViews() was called by checking clip paths
  // All unpinned tabs should have clip path set when pinned tabs exist
  // The clip path should match the visible area bounds
  gfx::Rect expected_clip_bounds_in_container(
      0, pinned_tabs_area_bottom, brave_tab_container->width(),
      container_height - pinned_tabs_area_bottom);

  for (int i = 0; i < model->count(); ++i) {
    Tab* tab = GetTabAt(browser(), i);
    if (tab->data().pinned) {
      // Pinned tabs should not have clip path
      EXPECT_TRUE(tab->clip_path().isEmpty())
          << "Pinned tab at index " << i << " should not have clip path";
      continue;
    }

    // Unpinned tabs should have clip path set (when pinned tabs exist)
    // The clip path should match the visible area bounds in tab's coordinate
    // system
    EXPECT_FALSE(tab->clip_path().isEmpty())
        << "Unpinned tab at index " << i << " should have clip path";

    // Verify the clip path bounds match the expected visible area
    SkRect clip_bounds_sk = tab->clip_path().computeTightBounds();
    gfx::RectF clip_bounds_in_tab_f = gfx::SkRectToRectF(clip_bounds_sk);
    gfx::Rect clip_bounds_in_tab = gfx::ToEnclosingRect(clip_bounds_in_tab_f);

    // Convert expected clip bounds from container to tab coordinate system
    gfx::Rect expected_clip_bounds_in_tab = views::View::ConvertRectToTarget(
        brave_tab_container, tab, expected_clip_bounds_in_container);

    // The clip path bounds should match the expected bounds
    EXPECT_EQ(clip_bounds_in_tab, expected_clip_bounds_in_tab)
        << "Unpinned tab at index " << i << " should have clip path";
  }

  // Set scroll offset to maximum (bottom)
  const int max_offset = brave_tab_container->GetMaxScrollOffset();
  brave_tab_container->SetScrollOffset(max_offset);
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();

  // Verify clip paths are updated after scrolling to bottom
  // The clip path should still match the visible area bounds
  for (int i = 0; i < model->count(); ++i) {
    Tab* tab = GetTabAt(browser(), i);
    if (tab->data().pinned) {
      // Pinned tabs should not have clip path
      EXPECT_TRUE(tab->clip_path().isEmpty())
          << "Pinned tab at index " << i << " should not have clip path";
      continue;
    }

    // Unpinned tabs should have clip path set (when pinned tabs exist)
    EXPECT_FALSE(tab->clip_path().isEmpty())
        << "Unpinned tab at index " << i << " should have clip path";

    // Verify the clip path bounds match the expected visible area
    SkRect clip_bounds_sk = tab->clip_path().computeTightBounds();
    gfx::RectF clip_bounds_in_tab_f = gfx::SkRectToRectF(clip_bounds_sk);
    gfx::Rect clip_bounds_in_tab = gfx::ToEnclosingRect(clip_bounds_in_tab_f);

    // Convert expected clip bounds from container to tab coordinate system
    gfx::Rect expected_clip_bounds_in_tab = views::View::ConvertRectToTarget(
        brave_tab_container, tab, expected_clip_bounds_in_container);

    // The clip path bounds should match the expected bounds
    EXPECT_EQ(clip_bounds_in_tab, expected_clip_bounds_in_tab)
        << "Unpinned tab at index " << i << " should have clip path";
  }
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest,
                       GetMaxScrollOffsetWithGroups) {
  // Test GetMaxScrollOffset with tab groups
  ToggleVerticalTabStrip();

  auto* brave_tab_container = views::AsViewClass<BraveTabContainer>(
      views::AsViewClass<BraveTabStrip>(
          browser_view()->horizontal_tab_strip_for_testing())
          ->GetTabContainerForTesting());
  ASSERT_TRUE(brave_tab_container);

  // Deflake the test by setting TabGroupSyncService initialized.
  tab_groups::TabGroupSyncService* service =
      tab_groups::TabGroupSyncServiceFactory::GetForProfile(
          browser()->profile());
  service->SetIsInitializedForTesting(true);

  auto* model = browser()->tab_strip_model();
  browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();

  // Create enough tabs to make the tab strip scrollable
  while (brave_tab_container->GetMaxScrollOffset() <=
         5 * tabs::kVerticalTabHeight) {
    AppendTab(browser());
    browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();

    InvalidateAndRunLayoutForVerticalTabStrip();
  }

  int last_max_scroll_offset = brave_tab_container->GetMaxScrollOffset();
  tab_groups::TabGroupId group1 =
      AddTabToNewGroup(browser(), model->count() - 1);
  EXPECT_GT(brave_tab_container->GetMaxScrollOffset(), last_max_scroll_offset)
      << "When adding a tab to a group, max scroll offset should increase, as "
         "group header should be visible";
  last_max_scroll_offset = brave_tab_container->GetMaxScrollOffset();

  // Collapse the group
  browser_view()
      ->horizontal_tab_strip_for_testing()
      ->controller()
      ->ToggleTabGroupCollapsedState(group1);
  ASSERT_TRUE(browser_view()
                  ->horizontal_tab_strip_for_testing()
                  ->controller()
                  ->IsGroupCollapsed(group1));

  EXPECT_EQ(brave_tab_container->GetMaxScrollOffset(),
            last_max_scroll_offset - tabs::kVerticalTabHeight -
                tabs::kVerticalTabsSpacing)
      << "When collapsing a group, max scroll offset should decrease, by the "
         "height of the contained tabs and spacing";

  // Even though all tabs in the group are invisible, the group should be
  // considered as the last visible slot view.
  // https://github.com/brave/brave-browser/issues/51635#issuecomment-3702630411
  EXPECT_EQ(brave_tab_container->FindVisibleUnpinnedSlotViews()
                .second->GetTabSlotViewType(),
            TabSlotView::ViewType::kTabGroupHeader);
}

// * Non-type argument of 'float' or 'double' for template is unsupported
// * Passing template as argument of IN_PROC_BROWSER_TEST_F is not working
// > thus, use macro instead.
#define VERTICAL_TAB_STRIP_DPI_TEST(RATIO, DPI)                           \
  class DPI##VerticalTabStripBrowserTest                                  \
      : public VerticalTabStripBrowserTest {                              \
   public:                                                                \
    void SetUp() override {                                               \
      base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(          \
          "force-device-scale-factor", base::NumberToString(RATIO));      \
      VerticalTabStripBrowserTest::SetUp();                               \
    }                                                                     \
    void SetUpOnMainThread() override {                                   \
      VerticalTabStripBrowserTest::SetUpOnMainThread();                   \
      /* Start up with vertical tab enabled - there shouldn't be crash */ \
      ToggleVerticalTabStrip();                                           \
    }                                                                     \
  };                                                                      \
                                                                          \
  IN_PROC_BROWSER_TEST_F(DPI##VerticalTabStripBrowserTest, DPI) {         \
    /* Manipulate size and state */                                       \
    auto* prefs = browser()->profile()->GetOriginalProfile()->GetPrefs(); \
    browser_view()->Maximize();                                           \
    prefs->SetBoolean(brave_tabs::kVerticalTabsCollapsed, true);          \
    prefs->SetBoolean(brave_tabs::kVerticalTabsCollapsed, false);         \
    prefs->SetBoolean(brave_tabs::kVerticalTabsCollapsed, true);          \
                                                                          \
    browser_view()->Restore();                                            \
    prefs->SetBoolean(brave_tabs::kVerticalTabsCollapsed, true);          \
    prefs->SetBoolean(brave_tabs::kVerticalTabsCollapsed, false);         \
    prefs->SetBoolean(brave_tabs::kVerticalTabsCollapsed, true);          \
                                                                          \
    /* Get back to horizontal tab strip - there shouldn't be crash */     \
    ToggleVerticalTabStrip();                                             \
  }

// Available DPIs on Windows
VERTICAL_TAB_STRIP_DPI_TEST(1.00f, Dpi100)
VERTICAL_TAB_STRIP_DPI_TEST(1.25f, Dpi125)
VERTICAL_TAB_STRIP_DPI_TEST(1.50f, Dpi150)
VERTICAL_TAB_STRIP_DPI_TEST(1.75f, Dpi175)
VERTICAL_TAB_STRIP_DPI_TEST(2.00f, Dpi200)
VERTICAL_TAB_STRIP_DPI_TEST(2.25f, Dpi225)
VERTICAL_TAB_STRIP_DPI_TEST(2.50f, Dpi250)
VERTICAL_TAB_STRIP_DPI_TEST(3.00f, Dpi300)
VERTICAL_TAB_STRIP_DPI_TEST(3.50f, Dpi350)

#undef VERTICAL_TAB_STRIP_DPI_TEST

class VerticalTabStripSwitchTest : public VerticalTabStripBrowserTest {
 public:
  using VerticalTabStripBrowserTest::VerticalTabStripBrowserTest;
  ~VerticalTabStripSwitchTest() override = default;

  // VerticalTabStripBrowserTest:
  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        tabs::switches::kDisableVerticalTabsSwitch);
    VerticalTabStripBrowserTest::SetUp();
  }
};

IN_PROC_BROWSER_TEST_F(VerticalTabStripSwitchTest, DisableSwitch) {
  EXPECT_FALSE(tabs::utils::SupportsBraveVerticalTabs(browser()));

  EXPECT_FALSE(tabs::utils::ShouldShowBraveVerticalTabs(browser()));
  // Even when we toggle on the tab strip, this state should persist.
  ToggleVerticalTabStrip();
  EXPECT_FALSE(tabs::utils::ShouldShowBraveVerticalTabs(browser()));
}

class VerticalTabStripScrollBarFlagTest : public VerticalTabStripBrowserTest {
 public:
  VerticalTabStripScrollBarFlagTest()
      : feature_list_(tabs::kBraveVerticalTabScrollBar) {}

  ~VerticalTabStripScrollBarFlagTest() override = default;

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(VerticalTabStripScrollBarFlagTest, MigrationTest) {
  auto* prefs = browser()->profile()->GetPrefs();
  auto* pref = prefs->FindPreference(brave_tabs::kVerticalTabsShowScrollbar);
  ASSERT_TRUE(pref);

  // Check if pref is set to true when user turned on the feature flag.
  EXPECT_FALSE(pref->IsDefaultValue());
  EXPECT_TRUE(prefs->GetBoolean(brave_tabs::kVerticalTabsShowScrollbar));
}

class VerticalTabStripHideCompletelyTest : public VerticalTabStripBrowserTest {
 public:
  VerticalTabStripHideCompletelyTest()
      : feature_list_(tabs::kBraveVerticalTabHideCompletely) {}

  ~VerticalTabStripHideCompletelyTest() override = default;

  void SetHideCompletelyWhenCollapsed(bool hide) {
    browser()->profile()->GetPrefs()->SetBoolean(
        brave_tabs::kVerticalTabsHideCompletelyWhenCollapsed, hide);
  }

  void SetUpOnMainThread() override {
    VerticalTabStripBrowserTest::SetUpOnMainThread();

    SetHideCompletelyWhenCollapsed(true);
  }

  ui::MouseEvent GetDummyEvent() {
    return ui::MouseEvent(ui::EventType::kMouseMoved, gfx::PointF(),
                          gfx::PointF(), base::TimeTicks::Now(), 0, 0);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
  display::test::TestScreen screen{/*create_display=*/true,
                                   /*register_screen=*/true};
};

IN_PROC_BROWSER_TEST_F(VerticalTabStripHideCompletelyTest, GetMinimumWidth) {
  // Given vertical tab strip is enabled and collapsed with the flag is on
  ToggleVerticalTabStrip();
  auto* widget_delegate_view =
      browser_view()->vertical_tab_strip_widget_delegate_view_.get();
  ASSERT_TRUE(widget_delegate_view);

  auto* region_view = widget_delegate_view->vertical_tab_strip_region_view();
  ASSERT_TRUE(region_view);

  region_view->ToggleState();
  ASSERT_EQ(BraveVerticalTabStripRegionView::State::kCollapsed,
            region_view->state());

  // The minimum width of the region view should be 0px as it's hidden
  // completely when collapsed.
  EXPECT_EQ(0, region_view->GetMinimumSize().width());

  // When the preference is disabled, the minimum width should be back to
  // 41px(w/o rounded corners) or 38px(with rounded corners) due to region
  // view's difference. See BraveVerticalTabStripRegionView::UpdateBorder().
  SetHideCompletelyWhenCollapsed(false);

  // As rounded corners is on by default minimum size is 38px.
  EXPECT_EQ(38, region_view->GetMinimumSize().width());

  // 41px w/o rounded corners.
  browser()->profile()->GetPrefs()->SetBoolean(kWebViewRoundedCorners, false);
  EXPECT_EQ(41, region_view->GetMinimumSize().width());

  region_view->ToggleState();
  ASSERT_EQ(BraveVerticalTabStripRegionView::State::kExpanded,
            region_view->state());

  // When expanded, minimum size should not be affected when the hide
  // completely option changes.
  const auto minimum_size_no_collapsed = region_view->GetMinimumSize().width();
  SetHideCompletelyWhenCollapsed(true);
  EXPECT_EQ(minimum_size_no_collapsed, region_view->GetMinimumSize().width());
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripHideCompletelyTest, ShouldBeInvisible) {
  ToggleVerticalTabStrip();

  const gfx::AnimationTestApi::RenderModeResetter render_mode_resetter =
      gfx::AnimationTestApi::SetRichAnimationRenderMode(
          gfx::Animation::RichAnimationRenderMode::FORCE_DISABLED);

  auto* widget_delegate_view =
      browser_view()->vertical_tab_strip_widget_delegate_view_.get();
  ASSERT_TRUE(widget_delegate_view);

  auto* region_view = widget_delegate_view->vertical_tab_strip_region_view();
  ASSERT_TRUE(region_view);

  region_view->ToggleState();
  ASSERT_EQ(BraveVerticalTabStripRegionView::State::kCollapsed,
            region_view->state());

  // When collapsed, it should be invisible.
  EXPECT_FALSE(region_view->GetVisible());

  const bool rounded_corners =
      BraveBrowserView::ShouldUseBraveWebViewRoundedCornersForContents(
          browser());
#if BUILDFLAG(IS_MAC)
  // On Mac, host view is moved by 1px to prevent vertical tab overlap
  // with frame border. If failed see
  // BraveBrowserViewLayout::AddVerticalTabFrameBorderInsets();
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return browser_view()
        ->vertical_tab_strip_host_view_->GetContentsBounds()
        .IsEmpty();
  }));
  EXPECT_EQ(browser_view()->vertical_tab_strip_host_view_->GetInsets().width(),
            1);

  // Check contents container has 1px insets for frame border.
  // frame border(1px) + rounded corners padding(4px).
  EXPECT_EQ(browser_view()->contents_container()->x(), rounded_corners ? 5 : 1);
#else
  // Check contents container doesn't have insets for frame border.
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return browser_view()->contents_container()->x() ==
           (rounded_corners ? 4 : 0);
  }));
#endif

  region_view->ToggleState();
  ASSERT_EQ(BraveVerticalTabStripRegionView::State::kExpanded,
            region_view->state());

  // When expanded, it should get visible again.
  EXPECT_TRUE(region_view->GetVisible());

  // When we turn off the preference, it should be visible even when collapsed.
  region_view->ToggleState();
  ASSERT_EQ(BraveVerticalTabStripRegionView::State::kCollapsed,
            region_view->state());
  ASSERT_FALSE(region_view->GetVisible());
  SetHideCompletelyWhenCollapsed(false);
  EXPECT_TRUE(region_view->GetVisible());
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripHideCompletelyTest,
                       ShowVerticalTabOnMouseOverTest) {
  auto scoped_mode = gfx::AnimationTestApi::SetRichAnimationRenderMode(
      gfx::Animation::RichAnimationRenderMode::FORCE_DISABLED);

  ToggleVerticalTabStrip();

  auto* widget_delegate_view =
      browser_view()->vertical_tab_strip_widget_delegate_view_.get();
  ASSERT_TRUE(widget_delegate_view);

  auto* region_view = widget_delegate_view->vertical_tab_strip_region_view();
  ASSERT_TRUE(region_view);
  auto* vertical_tab_widget = region_view->GetWidget();

  // Collapse the region view so it's hidden completely.
  SetHideCompletelyWhenCollapsed(true);
  region_view->ToggleState();
  EXPECT_EQ(BraveVerticalTabStripRegionView::State::kCollapsed,
            region_view->state());
  EXPECT_FALSE(region_view->GetVisible());
  EXPECT_FALSE(region_view->GetVisible());

  auto contents_area_view_rect =
      browser_view()->GetBoundingBoxInScreenForMouseOverHandling();
  EXPECT_EQ(browser_view()->width(), contents_area_view_rect.width());

  // Check region view is not visible.
  EXPECT_FALSE(region_view->GetVisible());

  auto* screen = display::Screen::Get();

  // Set mouse position inside hot corner area to check region view is shown
  // with that mouse position.
  gfx::Point mouse_position = contents_area_view_rect.origin();
  mouse_position.Offset(2, 2);
  screen->SetCursorScreenPointForTesting(mouse_position);
  browser_view()->HandleBrowserWindowMouseEvent(GetDummyEvent());
  EXPECT_TRUE(vertical_tab_widget->IsVisible());
  EXPECT_TRUE(region_view->GetVisible());

  // Completely hide again to test mouse position outside hot corner area.
  region_view->ToggleState();
  EXPECT_EQ(BraveVerticalTabStripRegionView::State::kExpanded,
            region_view->state());
  EXPECT_TRUE(vertical_tab_widget->IsVisible());
  EXPECT_TRUE(region_view->GetVisible());

  region_view->ToggleState();
  EXPECT_EQ(BraveVerticalTabStripRegionView::State::kCollapsed,
            region_view->state());
  EXPECT_FALSE(vertical_tab_widget->IsVisible());
  EXPECT_FALSE(region_view->GetVisible());

  // Set mouse position outside of hot corner area to check region view is not
  // shown with that mouse position.
  mouse_position = contents_area_view_rect.origin();
  mouse_position.Offset(10, 2);
  screen->SetCursorScreenPointForTesting(mouse_position);
  browser_view()->HandleBrowserWindowMouseEvent(GetDummyEvent());
  EXPECT_FALSE(vertical_tab_widget->IsVisible());
  EXPECT_FALSE(region_view->GetVisible());
}
