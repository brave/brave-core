/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/tab_strip_region_view.h"
#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_manager.h"

#if BUILDFLAG(IS_WIN)
#include "chrome/browser/ui/views/frame/glass_browser_frame_view.h"
#endif

#if BUILDFLAG(IS_MAC)
#include "ui/views/widget/native_widget_mac.h"
#endif

#if defined(USE_AURA)
#include "chrome/browser/ui/views/frame/opaque_browser_frame_view.h"
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
  if (run_loop_.running())
    run_loop_.Quit();
}

void FullscreenNotificationObserver::Wait() {
  if (observed_change_)
    return;

  run_loop_.Run();
}

}  // namespace

class VerticalTabStripBrowserTest : public InProcessBrowserTest {
 public:
  VerticalTabStripBrowserTest()
      : feature_list_(tabs::features::kBraveVerticalTabs) {}

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
    browser_non_client_frame_view()->Layout();
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
      return static_cast<const GlassBrowserFrameView*>(
                 browser_non_client_frame_view())
          ->window_title_for_testing()
          ->GetVisible();
    }
#endif

#if defined(USE_AURA)
    return static_cast<const OpaqueBrowserFrameView*>(
               browser_non_client_frame_view())
        ->ShouldShowWindowTitle();
#endif
  }

  base::test::ScopedFeatureList feature_list_;
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
  // Pre-condition: Window title is "hidden" by default on vertical tabs
  ASSERT_TRUE(tabs::utils::ShouldShowVerticalTabs(browser()));
  ASSERT_FALSE(tabs::utils::ShouldShowWindowTitleForVerticalTabs(browser()));
  ASSERT_FALSE(browser_view()->ShouldShowWindowTitle());
  ASSERT_FALSE(IsWindowTitleViewVisible());

  // Show window title bar
  brave::ToggleWindowTitleVisibilityForVerticalTabs(browser());
  browser_non_client_frame_view()->Layout();
  EXPECT_TRUE(tabs::utils::ShouldShowWindowTitleForVerticalTabs(browser()));
  EXPECT_TRUE(browser_view()->ShouldShowWindowTitle());
  EXPECT_GE(browser_non_client_frame_view()->GetTopInset(/*restored=*/false),
            0);
  EXPECT_TRUE(IsWindowTitleViewVisible());

  // Hide window title bar
  brave::ToggleWindowTitleVisibilityForVerticalTabs(browser());
  browser_non_client_frame_view()->Layout();
  EXPECT_FALSE(tabs::utils::ShouldShowWindowTitleForVerticalTabs(browser()));
  EXPECT_FALSE(browser_view()->ShouldShowWindowTitle());
#if !BUILDFLAG(IS_LINUX)
  // TODO(sko) For now, we can't hide window title bar entirely on Linux.
  // We're using a minimum height for it.
  EXPECT_EQ(0,
            browser_non_client_frame_view()->GetTopInset(/*restored=*/false));
#endif
  EXPECT_FALSE(IsWindowTitleViewVisible());
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
  for (int i = 0; i < 10; i++)
    chrome::AddTabAt(browser(), {}, -1, true);
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

  auto* region_view = widget_delegate_view->region_view_.get();
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

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, SidebarAlignment) {
  // Pre-condition: sidebar is on the left by default.
  auto* prefs = browser()->profile()->GetPrefs();
  ASSERT_TRUE(prefs->FindPreference(prefs::kSidePanelHorizontalAlignment)
                  ->IsDefaultValue());
  ASSERT_FALSE(prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment));

  // When enabling vertical tab strip, sidebar moves to the right.
  ToggleVerticalTabStrip();
  EXPECT_FALSE(prefs->FindPreference(prefs::kSidePanelHorizontalAlignment)
                   ->IsDefaultValue());
  EXPECT_TRUE(prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment));

  // When disabling vertical tab strip, sidebar should be restored to the
  // default position.
  ToggleVerticalTabStrip();
  EXPECT_TRUE(prefs->FindPreference(prefs::kSidePanelHorizontalAlignment)
                  ->IsDefaultValue());
  EXPECT_FALSE(prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment));

  // When user explicitly set position, sidebar shouldn't move.
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, false);
  EXPECT_FALSE(prefs->FindPreference(prefs::kSidePanelHorizontalAlignment)
                   ->IsDefaultValue());
  ToggleVerticalTabStrip();
  EXPECT_FALSE(prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment));

  // Turning off vertical tab strip also shouldn't affect sidebar's position.
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, true);
  ToggleVerticalTabStrip();
  ToggleVerticalTabStrip();
  EXPECT_FALSE(prefs->FindPreference(prefs::kSidePanelHorizontalAlignment)
                   ->IsDefaultValue());
  EXPECT_TRUE(prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment));
}

#if BUILDFLAG(IS_MAC)
// Mac test bots are not able to enter fullscreen.
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
  fullscreen_controller->ToggleBrowserFullscreenMode();
  { FullscreenNotificationObserver(browser()).Wait(); }

  // Vertical tab strip should be visible on browser fullscreen.
  ASSERT_TRUE(fullscreen_controller->IsFullscreenForBrowser());
  ASSERT_TRUE(browser_view()->IsFullscreen());
  EXPECT_TRUE(browser_view()
                  ->vertical_tab_strip_host_view_->GetPreferredSize()
                  .width());

  fullscreen_controller->ToggleBrowserFullscreenMode();
  { FullscreenNotificationObserver(browser()).Wait(); }
  ASSERT_FALSE(fullscreen_controller->IsFullscreenForBrowser());
  ASSERT_FALSE(browser_view()->IsFullscreen());

  // Vertical tab strip should become invisible on tab fullscreen.
  fullscreen_controller->EnterFullscreenModeForTab(browser_view()
                                                       ->browser()
                                                       ->tab_strip_model()
                                                       ->GetActiveWebContents()
                                                       ->GetPrimaryMainFrame());
  { FullscreenNotificationObserver(browser()).Wait(); }
  ASSERT_TRUE(fullscreen_controller->IsTabFullscreen());

  base::RunLoop run_loop;
  auto wait_until = base::BindLambdaForTesting(
      [&](base::RepeatingCallback<bool()> predicate) {
        if (predicate.Run())
          return;

        base::RepeatingTimer scheduler;
        scheduler.Start(FROM_HERE, base::Milliseconds(100),
                        base::BindLambdaForTesting([&]() {
                          if (predicate.Run())
                            run_loop.Quit();
                        }));
        run_loop.Run();
      });

  wait_until.Run(base::BindLambdaForTesting([&]() {
    return !browser_view()
                ->vertical_tab_strip_host_view_->GetPreferredSize()
                .width();
  }));
}
