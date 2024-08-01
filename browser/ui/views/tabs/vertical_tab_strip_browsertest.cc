/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_tab_menu_model.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"
#include "brave/browser/ui/views/tabs/brave_compound_tab_container.h"
#include "brave/browser/ui/views/tabs/brave_tab_context_menu_contents.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip.h"
#include "brave/browser/ui/views/tabs/switches.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/components/constants/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/tab_strip_region_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "chrome/browser/ui/views/tabs/tab_search_button.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "ui/base/test/ui_controls.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_manager.h"

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
  observation_.Observe(
      browser->exclusive_access_manager()->fullscreen_controller());
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
  BrowserNonClientFrameView* browser_non_client_frame_view() {
    return browser_view()->frame()->GetFrameView();
  }
  const BrowserNonClientFrameView* browser_non_client_frame_view() const {
    return browser_view()->frame()->GetFrameView();
  }

  void ToggleVerticalTabStrip() {
    brave::ToggleVerticalTabStrip(browser());
    browser_non_client_frame_view()->DeprecatedLayoutImmediately();
  }

  TabStrip* GetTabStrip(Browser* browser) {
    BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
    return browser_view->tabstrip();
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
                    base::BindLambdaForTesting([this, &condition]() {
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

 private:
  std::unique_ptr<base::RunLoop> run_loop_;
};

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, ToggleVerticalTabStrip) {
  // Pre-conditions
  // The default orientation is horizontal.
  ASSERT_FALSE(tabs::utils::ShouldShowVerticalTabs(browser()));
  ASSERT_EQ(browser_view()->GetWidget(),
            browser_view()->tabstrip()->GetWidget());

  // Show vertical tab strip. This will move tabstrip to its own widget.
  ToggleVerticalTabStrip();
  EXPECT_TRUE(tabs::utils::ShouldShowVerticalTabs(browser()));
  EXPECT_NE(browser_view()->GetWidget(),
            browser_view()->tabstrip()->GetWidget());

  // Hide vertical tab strip and restore to the horizontal tabstrip.
  ToggleVerticalTabStrip();
  EXPECT_FALSE(tabs::utils::ShouldShowVerticalTabs(browser()));
  EXPECT_EQ(browser_view()->GetWidget(),
            browser_view()->tabstrip()->GetWidget());
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

  ASSERT_TRUE(tabs::utils::ShouldShowVerticalTabs(browser()));
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
      browser_view()->tab_strip_region_view()->new_tab_button()->GetVisible());

  ToggleVerticalTabStrip();
  EXPECT_FALSE(
      browser_view()->tab_strip_region_view()->new_tab_button()->GetVisible());

  ToggleVerticalTabStrip();
  EXPECT_TRUE(
      browser_view()->tab_strip_region_view()->new_tab_button()->GetVisible());
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, MinHeight) {
  ToggleVerticalTabStrip();

  // Add a tab to flush cached min size.
  chrome::AddTabAt(browser(), {}, -1, true);

  const auto browser_view_min_size = browser_view()->GetMinimumSize();
  const auto browser_non_client_frame_view_min_size =
      browser_view()->frame()->GetFrameView()->GetMinimumSize();

  // Add tabs as much as it can grow mih height of tab strip.
  auto tab_strip_min_height =
      browser_view()->tab_strip_region_view()->GetMinimumSize().height();
  for (int i = 0; i < 10; i++) {
    chrome::AddTabAt(browser(), {}, -1, true);
  }
  ASSERT_LE(tab_strip_min_height,
            browser_view()->tab_strip_region_view()->GetMinimumSize().height());

  // TabStrip's min height shouldn't affect that of browser window.
  EXPECT_EQ(browser_view_min_size.height(),
            browser_view()->GetMinimumSize().height());
  EXPECT_EQ(browser_non_client_frame_view_min_size.height(),
            browser_view()->frame()->GetFrameView()->GetMinimumSize().height());
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, VisualState) {
  ToggleVerticalTabStrip();

  // Pre-condition: Floating mode is enabled by default.
  using State = VerticalTabStripRegionView::State;
  ASSERT_TRUE(tabs::utils::IsFloatingVerticalTabsEnabled(browser()));
  auto* widget_delegate_view =
      browser_view()->vertical_tab_strip_widget_delegate_view_.get();
  ASSERT_TRUE(widget_delegate_view);

  auto* region_view = widget_delegate_view->vertical_tab_strip_region_view();
  ASSERT_TRUE(region_view);
  ASSERT_EQ(State::kExpanded, region_view->state());

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
    ui::MouseEvent event(ui::ET_MOUSE_ENTERED, gfx::PointF(), gfx::PointF(), {},
                         {}, {});
    region_view->OnMouseEntered(event);
    EXPECT_EQ(State::kFloating, region_view->state());
  }

  // Check if mouse exiting make tab strip collapsed.
  {
    base::AutoReset resetter(&region_view->mouse_events_for_test_, true);
    ui::MouseEvent event(ui::ET_MOUSE_EXITED, gfx::PointF(), gfx::PointF(), {},
                         {}, {});
    region_view->OnMouseExited(event);
    EXPECT_EQ(State::kCollapsed, region_view->state());
  }

  // When floating mode is disabled, it shouldn't be triggered.
  prefs->SetBoolean(brave_tabs::kVerticalTabsFloatingEnabled, false);
  {
    base::AutoReset resetter(&region_view->mouse_events_for_test_, true);
    ui::MouseEvent event(ui::ET_MOUSE_ENTERED, gfx::PointF(), gfx::PointF(), {},
                         {}, {});
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
  auto* fullscreen_controller =
      browser_view()->GetExclusiveAccessManager()->fullscreen_controller();
  {
    auto observer = FullscreenNotificationObserver(browser());
    fullscreen_controller->ToggleBrowserFullscreenMode();
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
    fullscreen_controller->ToggleBrowserFullscreenMode();
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

  chrome::AddTabAt(browser(), {}, -1, true);

  auto* widget_delegate_view =
      browser_view()->vertical_tab_strip_widget_delegate_view();
  ASSERT_TRUE(widget_delegate_view);

  auto* region_view = widget_delegate_view->vertical_tab_strip_region_view();
  ASSERT_TRUE(region_view);
  ASSERT_EQ(VerticalTabStripRegionView::State::kExpanded, region_view->state());

  auto* model = browser()->tab_strip_model();
  ASSERT_EQ(2, model->count());
  model->SetTabPinned(0, true);

  browser_view()->tabstrip()->StopAnimating(/* layout= */ true);

  // Test if every tabs are laid out inside tab strip region -------------------
  // This is a regression test for
  // https://github.com/brave/brave-browser/issues/28084
  for (int i = 0; i < model->count(); i++) {
    auto* tab = GetTabAt(browser(), i);
    EXPECT_TRUE(GetBoundsInScreen(region_view, region_view->GetLocalBounds())
                    .Contains(GetBoundsInScreen(tab, tab->GetLocalBounds())));
  }
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, ScrollBarVisibility) {
  ToggleVerticalTabStrip();

  auto* prefs = browser()->profile()->GetPrefs();
  auto* pref = prefs->FindPreference(brave_tabs::kVerticalTabsShowScrollbar);

  // Check if the default value is false
  EXPECT_TRUE(pref && pref->IsDefaultValue());
  EXPECT_FALSE(prefs->GetBoolean(brave_tabs::kVerticalTabsShowScrollbar));

  auto get_tab_container = [&]() {
    return views::AsViewClass<BraveTabStrip>(browser_view()->tabstrip())
        ->GetTabContainerForTesting();
  };

  auto* brave_tab_container =
      views::AsViewClass<BraveCompoundTabContainer>(get_tab_container());
  EXPECT_TRUE(brave_tab_container);
  EXPECT_EQ(views::ScrollView::ScrollBarMode::kHiddenButEnabled,
            brave_tab_container->scroll_view_->GetVerticalScrollBarMode());

  // Turn on the prefs and checks if scrollbar becomes visible
  prefs->SetBoolean(brave_tabs::kVerticalTabsShowScrollbar, true);
  EXPECT_EQ(views::ScrollView::ScrollBarMode::kEnabled,
            brave_tab_container->scroll_view_->GetVerticalScrollBarMode());

  // Turning off and on vertical tabs and see if the visibility persists.
  ToggleVerticalTabStrip();
  ToggleVerticalTabStrip();
  brave_tab_container =
      views::AsViewClass<BraveCompoundTabContainer>(get_tab_container());
  EXPECT_EQ(views::ScrollView::ScrollBarMode::kEnabled,
            brave_tab_container->scroll_view_->GetVerticalScrollBarMode());

  // Checks if scrollbar is hidden when the pref is turned off.
  prefs->SetBoolean(brave_tabs::kVerticalTabsShowScrollbar, false);
  EXPECT_EQ(views::ScrollView::ScrollBarMode::kHiddenButEnabled,
            brave_tab_container->scroll_view_->GetVerticalScrollBarMode());
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, ExpandedState) {
  // Given that kVerticalTabsExpandedStatePerWindow is false,
  auto* prefs = browser()->profile()->GetPrefs();
  ASSERT_FALSE(
      prefs->GetBoolean(brave_tabs::kVerticalTabsExpandedStatePerWindow));

  // When clicking the toggle button,
  using State = VerticalTabStripRegionView::State;
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
  using State = VerticalTabStripRegionView::State;
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
};

IN_PROC_BROWSER_TEST_F(VerticalTabStripStringBrowserTest, ContextMenuString) {
  // Pre-conditions ------------------------------------------------------------
  auto create_tab_context_menu_contents = [&]() {
    return std::make_unique<BraveTabContextMenuContents>(
        GetTabAt(browser(), 0),
        static_cast<BraveBrowserTabStripController*>(
            browser_view()->tabstrip()->controller()),
        /* index= */ 0);
  };

  auto get_all_labels = [&]() {
    auto menu_contents = create_tab_context_menu_contents();
    std::vector<std::u16string> labels;
    for (auto i = 0u; i < menu_contents->model_->GetItemCount(); i++) {
      labels.push_back(menu_contents->model_->GetLabelAt(i));
    }
    return labels;
  };

  {
    auto context_menu_contents = create_tab_context_menu_contents();
    ASSERT_FALSE(get_all_labels().empty());
  }

  // Tests ---------------------------------------------------------------------
  {
    // Check if there's no "Below" in context menu labels when it's horizontal
    // tab strip
    auto context_menu_contents = create_tab_context_menu_contents();
    EXPECT_TRUE(base::ranges::none_of(get_all_labels(), [](const auto& label) {
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
    auto context_menu_contents = create_tab_context_menu_contents();
    EXPECT_TRUE(base::ranges::none_of(get_all_labels(), [](const auto& label) {
#if BUILDFLAG(IS_MAC)
      return base::Contains(label, u"Right") || base::Contains(label, u"Left");
#else
      return base::Contains(label, u"right") ||
          base::Contains(label, u"left");
#endif
    }));
  }
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, OriginalTabSearchButton) {
  auto* widget_delegate_view =
      browser_view()->vertical_tab_strip_widget_delegate_view();
  ASSERT_TRUE(widget_delegate_view);

  auto* region_view = widget_delegate_view->vertical_tab_strip_region_view();
  ASSERT_TRUE(region_view);

  auto* tab_search_container =
      region_view->original_region_view_->tab_search_container();
  if (!tab_search_container) {
    return;
  }

  auto* original_tab_search_button = tab_search_container->tab_search_button();
  if (!original_tab_search_button) {
    // On Windows 10, the button is on the window frame and vertical tab strip
    // does nothing to it.
    return;
  }

  ASSERT_TRUE(original_tab_search_button->GetVisible());

  // The button should be hidden when using vertical tab strip
  ToggleVerticalTabStrip();
  EXPECT_FALSE(original_tab_search_button->GetVisible());

  // The button should reappear when getting back to horizontal tab strip.
  ToggleVerticalTabStrip();
  EXPECT_TRUE(original_tab_search_button->GetVisible());

  // Turn off the button with a preference.
  browser()->profile()->GetPrefs()->SetBoolean(kTabsSearchShow, false);
  EXPECT_FALSE(original_tab_search_button->GetVisible());

  // Turn on and off vertical tab strip
  ToggleVerticalTabStrip();
  ToggleVerticalTabStrip();

  // the original tab search button should stay hidden
  EXPECT_FALSE(original_tab_search_button->GetVisible());
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
    return GetTabStrip(b)->GetDragContext()->IsDragSessionActive();
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

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_WIN)
// TODO(sko) On Linux test environment, the test doesn't work well
// TODO(sko) On Windows CI, SendMouse() doesn't work.
#define MAYBE_DragTabToReorder DISABLED_DragTabToReorder
#else
#define MAYBE_DragTabToReorder DragTabToReorder
#endif

IN_PROC_BROWSER_TEST_F(VerticalTabStripDragAndDropBrowserTest,
                       MAYBE_DragTabToReorder) {
  // Pre-conditions ------------------------------------------------------------
  chrome::AddTabAt(browser(), {}, -1, true);

  auto* widget_delegate_view =
      browser_view()->vertical_tab_strip_widget_delegate_view_.get();
  ASSERT_TRUE(widget_delegate_view);

  auto* region_view = widget_delegate_view->vertical_tab_strip_region_view();
  ASSERT_TRUE(region_view);
  ASSERT_EQ(VerticalTabStripRegionView::State::kExpanded, region_view->state());

  // Drag and drop a tab to reorder it -----------------------------------------
  GetTabStrip(browser())->StopAnimating(
      /* layout= */ true);  // Drag-and-drop doesn't start when animation is
                            // running.
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
  GetTabStrip(browser())->StopAnimating(
      true);  // Drag-and-drop doesn't start when animation is running.
  {
    // Regression test for https://github.com/brave/brave-browser/issues/28488
    // Check if the tab is positioned properly after drag-and-drop.
    auto* moved_tab = GetTabAt(browser(), 1);
    EXPECT_TRUE(GetBoundsInScreen(region_view, region_view->GetLocalBounds())
                    .Contains(GetBoundsInScreen(moved_tab,
                                                moved_tab->GetLocalBounds())));
  }
}

// TODO(sko) On Linux test environment, the test doesn't work well
// TODO(sko) On Windows CI, SendMouse() doesn't work.
// TODO(sko) As of Dec, 2023 this test is flaky on Mac CI.
#define MAYBE_DragTabToDetach DISABLED_DragTabToDetach

IN_PROC_BROWSER_TEST_F(VerticalTabStripDragAndDropBrowserTest,
                       MAYBE_DragTabToDetach) {
  // Pre-conditions ------------------------------------------------------------
  chrome::AddTabAt(browser(), {}, -1, true);

  // Drag a tab out of tab strip to create browser -----------------------------
  GetTabStrip(browser())->StopAnimating(
      true);  // Drag-and-drop doesn't start when animation is running.
  PressTabAt(browser(), 0);
  gfx::Point point_out_of_tabstrip =
      GetCenterPointInScreen(GetTabAt(browser(), 0));
  point_out_of_tabstrip.set_x(point_out_of_tabstrip.x() +
                              2 * GetTabAt(browser(), 0)->width());
  MoveMouseTo(point_out_of_tabstrip, base::BindLambdaForTesting([&]() {
                // Creating new browser during drag-and-drop will create
                // a nested run loop. So we should do things within callback.
                auto* browser_list = BrowserList::GetInstance();
                EXPECT_EQ(
                    2, base::ranges::count_if(*browser_list, [&](Browser* b) {
                      return b->profile() == browser()->profile();
                    }));
                ReleaseMouse();
                auto* new_browser = browser_list->GetLastActive();
                new_browser->window()->Close();
              }));
}

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
// TODO(sko) On Linux test environment, the test doesn't work well
// TODO(sko) On Windows CI, SendMouse() doesn't work.
#define MAYBE_DragURL DISABLED_DragURL
#else
#define MAYBE_DragURL DragURL
#endif

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

class VerticalTabStripWithScrollableTabBrowserTest
    : public VerticalTabStripBrowserTest {
 public:
  VerticalTabStripWithScrollableTabBrowserTest()
      : feature_list_(features::kScrollableTabStrip) {}

  ~VerticalTabStripWithScrollableTabBrowserTest() override = default;

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(VerticalTabStripWithScrollableTabBrowserTest, Sanity) {
  // Make sure browser works with both vertical tab and scrollable tab strip
  // https://github.com/brave/brave-browser/issues/28877
  ToggleVerticalTabStrip();
  Browser::Create(Browser::CreateParams(browser()->profile(), true));
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
  EXPECT_FALSE(tabs::utils::SupportsVerticalTabs(browser()));

  EXPECT_FALSE(tabs::utils::ShouldShowVerticalTabs(browser()));
  // Even when we toggle on the tab strip, this state should persist.
  ToggleVerticalTabStrip();
  EXPECT_FALSE(tabs::utils::ShouldShowVerticalTabs(browser()));
}

class VerticalTabStripScrollBarFlagTest : public VerticalTabStripBrowserTest {
 public:
  VerticalTabStripScrollBarFlagTest()
      : feature_list_(tabs::features::kBraveVerticalTabScrollBar) {}

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
