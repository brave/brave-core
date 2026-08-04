/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/run_until.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_container_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "chrome/test/interaction/interactive_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"

using State = BraveVerticalTabStripRegionView::State;

class VerticalTabStripInteractiveUITest : public InteractiveBrowserTest {
 public:
  BraveBrowserView* browser_view() {
    return static_cast<BraveBrowserView*>(
        BrowserWindow::FromBrowser(browser()));
  }

  void ToggleVerticalTabStrip() { brave::ToggleVerticalTabStrip(browser()); }
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

  auto* fullscreen_controller = browser()
                                    ->GetFeatures()
                                    .exclusive_access_manager()
                                    ->fullscreen_controller();
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

  auto* fullscreen_controller = browser()
                                    ->GetFeatures()
                                    .exclusive_access_manager()
                                    ->fullscreen_controller();

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
