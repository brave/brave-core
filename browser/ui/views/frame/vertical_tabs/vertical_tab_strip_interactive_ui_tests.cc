/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/auto_reset.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/run_until.h"
#include "base/test/test_future.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_container_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/ui_test_utils.h"
#include "chrome/test/interaction/interactive_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "ui/base/models/dialog_model.h"
#include "ui/events/event.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/bubble/bubble_dialog_model_host.h"
#include "ui/views/test/widget_activation_waiter.h"
#include "ui/views/widget/widget.h"

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

// A bubble anchored inside the browser window - a permission prompt, page
// info, ... - deactivates the browser widget when it takes activation. That's
// not the user leaving the window, and collapsing a floating strip there is
// harmful: the collapse animation lays out BrowserView on every frame, each
// layout re-anchors the open bubble, and re-anchoring re-arms the bubble's
// input protection, so the bubble silently drops clicks for the duration of
// the animation plus the cooldown.
IN_PROC_BROWSER_TEST_F(VerticalTabStripInteractiveUITest,
                       FloatingStateSurvivesOwnBubbleActivation) {
  ToggleVerticalTabStrip();

  auto* region_view = browser_view()
                          ->vertical_tab_strip_container_view()
                          ->vertical_tab_strip_region_view();
  ASSERT_TRUE(region_view);

  browser()->GetProfile()->GetOriginalProfile()->GetPrefs()->SetBoolean(
      brave_tabs::kVerticalTabsCollapsed, true);
  {
    base::AutoReset resetter(&region_view->mouse_events_for_test_, true);
    ui::MouseEvent event(ui::EventType::kMouseEntered, gfx::PointF(),
                         gfx::PointF(), {}, {}, {});
    region_view->OnMouseEntered(event);
  }
  ASSERT_EQ(State::kFloating, region_view->state());

  auto bubble_delegate = std::make_unique<views::BubbleDialogModelHost>(
      ui::DialogModel::Builder().Build(), browser_view()->toolbar(),
      views::BubbleBorder::TOP_LEFT, /*autosize=*/true,
      /*owned_by_widget=*/false);
  std::unique_ptr<views::Widget> bubble =
      views::BubbleDialogDelegate::CreateBubble(bubble_delegate.get());
  bubble->Show();
  views::test::WaitForWidgetActive(bubble.get(), true);

  auto* browser_widget = browser_view()->GetWidget();
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return !browser_widget->IsActive(); }));
  ASSERT_TRUE(bubble->IsActive());

  // The collapse decision is deferred by a task; let it run.
  base::test::TestFuture<void> flushed;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, flushed.GetCallback());
  ASSERT_TRUE(flushed.Wait());

  EXPECT_EQ(State::kFloating, region_view->state());
}

// The counterpart: activating a different window really is the user leaving,
// and a floating strip must still collapse then.
IN_PROC_BROWSER_TEST_F(VerticalTabStripInteractiveUITest,
                       FloatingStateCollapsesWhenAnotherWindowActivates) {
  ToggleVerticalTabStrip();

  auto* region_view = browser_view()
                          ->vertical_tab_strip_container_view()
                          ->vertical_tab_strip_region_view();
  ASSERT_TRUE(region_view);

  browser()->GetProfile()->GetOriginalProfile()->GetPrefs()->SetBoolean(
      brave_tabs::kVerticalTabsCollapsed, true);
  {
    base::AutoReset resetter(&region_view->mouse_events_for_test_, true);
    ui::MouseEvent event(ui::EventType::kMouseEntered, gfx::PointF(),
                         gfx::PointF(), {}, {}, {});
    region_view->OnMouseEntered(event);
  }
  ASSERT_EQ(State::kFloating, region_view->state());

  Browser* other_browser = CreateBrowser(browser()->GetProfile());
  views::test::WaitForWidgetActive(
      BrowserView::GetBrowserViewForBrowser(other_browser)->GetWidget(), true);
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !browser_view()->GetWidget()->IsActive(); }));

  EXPECT_TRUE(base::test::RunUntil(
      [&]() { return region_view->state() == State::kCollapsed; }));
}
