/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_container_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "chrome/test/interaction/interactive_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "ui/gfx/geometry/point.h"

using State = BraveVerticalTabStripRegionView::State;

class VerticalTabStripInteractiveUITest : public InteractiveBrowserTest {
 public:
  VerticalTabStripInteractiveUITest() {
    feature_list_.InitAndEnableFeature(features::kBraveRoundedCornersByDefault);
  }

  BraveBrowserView* browser_view() {
    return static_cast<BraveBrowserView*>(
        BrowserWindow::FromBrowser(browser()));
  }

  void ToggleVerticalTabStrip() { brave::ToggleVerticalTabStrip(browser()); }

  // macOS drops the move without a window hint when the cursor isn't over one.
  [[nodiscard]] bool MoveMouseTo(const gfx::Point& point_in_screen) {
    return ui_test_utils::SendMouseMoveSync(
        point_in_screen, browser_view()->GetWidget()->GetNativeWindow());
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

#if BUILDFLAG(IS_MAC)
// Fullscreen test flaky on macOS: https://crbug.com/41393319
#define MAYBE_TabFullscreenUpdatesHostViewBounds \
  DISABLED_TabFullscreenUpdatesHostViewBounds
#else
#define MAYBE_TabFullscreenUpdatesHostViewBounds \
  TabFullscreenUpdatesHostViewBounds
#endif

IN_PROC_BROWSER_TEST_F(VerticalTabStripInteractiveUITest,
                       MAYBE_TabFullscreenUpdatesHostViewBounds) {
  ToggleVerticalTabStrip();

  auto* host_view = browser_view()->vertical_tab_strip_host_view_for_testing();
  ASSERT_TRUE(host_view);

  auto* region_view = browser_view()
                          ->vertical_tab_strip_container_view()
                          ->vertical_tab_strip_region_view();
  ASSERT_TRUE(region_view);
  ASSERT_EQ(State::kExpanded, region_view->state());
  ASSERT_GT(host_view->GetPreferredSize().width(), 0);

  auto* fullscreen_controller =
      ExclusiveAccessManager::From(browser())->fullscreen_controller();
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();

  {
    ui_test_utils::FullscreenWaiter waiter(browser(), {.tab_fullscreen = true});
    fullscreen_controller->EnterFullscreenModeForTab(
        web_contents->GetPrimaryMainFrame());
    waiter.Wait();
  }
  ASSERT_TRUE(fullscreen_controller->IsTabFullscreen());

  // The vertical tab strip should no longer be reserving any space for the
  // host view while in tab fullscreen.
  EXPECT_TRUE(base::test::RunUntil(
      [&]() { return host_view->GetPreferredSize().width() == 0; }));

  // Tab fullscreen shouldn't have changed the pref-based expanded/collapsed
  // state - it's a temporary, layout-only change.
  EXPECT_EQ(State::kExpanded, region_view->state());

  {
    ui_test_utils::FullscreenWaiter waiter(
        browser(), ui_test_utils::FullscreenWaiter::kNoFullscreen);
    fullscreen_controller->ExitFullscreenModeForTab(web_contents);
    waiter.Wait();
  }
  ASSERT_FALSE(fullscreen_controller->IsTabFullscreen());

  // Exiting tab fullscreen should restore the previously allocated space.
  EXPECT_TRUE(base::test::RunUntil(
      [&]() { return host_view->GetPreferredSize().width() > 0; }));
  EXPECT_EQ(State::kExpanded, region_view->state());
}

#if BUILDFLAG(IS_MAC)
// Fullscreen test flaky on macOS: https://crbug.com/41393319
#define MAYBE_BrowserFullscreenUpdatesHostViewBounds \
  DISABLED_BrowserFullscreenUpdatesHostViewBounds
#else
#define MAYBE_BrowserFullscreenUpdatesHostViewBounds \
  BrowserFullscreenUpdatesHostViewBounds
#endif

IN_PROC_BROWSER_TEST_F(VerticalTabStripInteractiveUITest,
                       MAYBE_BrowserFullscreenUpdatesHostViewBounds) {
  ToggleVerticalTabStrip();

  auto* host_view = browser_view()->vertical_tab_strip_host_view_for_testing();
  ASSERT_TRUE(host_view);

  auto* region_view = browser_view()
                          ->vertical_tab_strip_container_view()
                          ->vertical_tab_strip_region_view();
  ASSERT_TRUE(region_view);
  ASSERT_EQ(State::kExpanded, region_view->state());
  ASSERT_GT(host_view->GetPreferredSize().width(), 0);

  auto* fullscreen_controller =
      ExclusiveAccessManager::From(browser())->fullscreen_controller();

  {
    ui_test_utils::FullscreenWaiter waiter(browser(),
                                           {.browser_fullscreen = true});
    fullscreen_controller->ToggleBrowserFullscreenMode(/*user_initiated=*/true);
    waiter.Wait();
  }
  ASSERT_TRUE(fullscreen_controller->IsFullscreenForBrowser());

  // Vertical tab strip should be invisible on browser fullscreen.
  EXPECT_TRUE(base::test::RunUntil(
      [&]() { return host_view->GetPreferredSize().width() == 0; }));

  {
    ui_test_utils::FullscreenWaiter waiter(browser(),
                                           {.browser_fullscreen = false});
    fullscreen_controller->ToggleBrowserFullscreenMode(/*user_initiated=*/true);
    waiter.Wait();
  }
  ASSERT_FALSE(fullscreen_controller->IsFullscreenForBrowser());

  // Exiting browser fullscreen restores the pre-fullscreen expanded state
  // and makes the strip visible again.
  EXPECT_EQ(State::kExpanded, region_view->state());
  EXPECT_TRUE(region_view->GetVisible());
  EXPECT_TRUE(base::test::RunUntil(
      [&]() { return host_view->GetPreferredSize().width() > 0; }));
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripInteractiveUITest, FloatingOnHover) {
  ToggleVerticalTabStrip();

  auto* region_view = browser_view()
                          ->vertical_tab_strip_container_view()
                          ->vertical_tab_strip_region_view();
  ASSERT_TRUE(region_view);
  ASSERT_TRUE(
      VerticalTabController::From(browser())->IsFloatingVerticalTabsEnabled());

  // On macOS, moves over native subviews such as the web contents never reach
  // the views hierarchy, so the cursor has to leave the strip over a view.
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return !browser_view()->toolbar()->GetBoundsInScreen().IsEmpty();
  }));
  const gfx::Point outside =
      browser_view()->toolbar()->GetBoundsInScreen().CenterPoint();
  ASSERT_FALSE(region_view->GetBoundsInScreen().Contains(outside));
  ASSERT_TRUE(MoveMouseTo(outside));

  auto* prefs = browser()->GetProfile()->GetOriginalProfile()->GetPrefs();
  prefs->SetBoolean(brave_tabs::kVerticalTabsCollapsed, true);
  ASSERT_EQ(State::kCollapsed, region_view->state());
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !region_view->GetBoundsInScreen().IsEmpty(); }));

  // Hovering the collapsed strip makes it float.
  ASSERT_TRUE(MoveMouseTo(region_view->GetBoundsInScreen().CenterPoint()));
  EXPECT_TRUE(base::test::RunUntil(
      [&]() { return region_view->state() == State::kFloating; }));
  EXPECT_EQ(1, region_view->GetInsets().width());

  // Moving the cursor away collapses it again.
  ASSERT_TRUE(MoveMouseTo(outside));
  EXPECT_TRUE(base::test::RunUntil(
      [&]() { return region_view->state() == State::kCollapsed; }));
  EXPECT_EQ(0, region_view->GetInsets().width());

  // With floating mode disabled, hovering doesn't float the strip.
  prefs->SetBoolean(brave_tabs::kVerticalTabsFloatingEnabled, false);
  ASSERT_TRUE(MoveMouseTo(region_view->GetBoundsInScreen().CenterPoint()));
  EXPECT_NE(State::kFloating, region_view->state());
}
