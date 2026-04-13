/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_container.h"

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_root_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/horizontal_tab_strip_region_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event_constants.h"
#include "ui/views/view_utils.h"
#include "url/url_constants.h"

class HorizontalScrollableTabStripBrowserTest : public InProcessBrowserTest {
 public:
  HorizontalScrollableTabStripBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(tabs::kBraveScrollableTabStrip);
  }

  BraveBrowserView* browser_view() {
    return static_cast<BraveBrowserView*>(browser()->window());
  }

  void AppendTab() { chrome::AddTabAt(browser(), GURL(), -1, true); }

  BrowserRootView* browser_root_view() {
    return static_cast<BrowserRootView*>(
        browser_view()->GetWidget()->GetRootView());
  }

  // Dispatches a wheel event at the horizontal tab strip, as
  // chrome/browser/ui/views/frame/browser_root_view_browsertest.cc does for
  // WheelTabChange.
  void PerformMouseWheelOnTabStrip(const gfx::Vector2d& offset, int flags) {
    TabStrip* tabstrip = browser_view()->horizontal_tab_strip_for_testing();
    const gfx::Point tabstrip_center = tabstrip->GetLocalBounds().CenterPoint();
    const gfx::Point location = views::View::ConvertPointToTarget(
        tabstrip, browser_root_view(), tabstrip_center);
    const gfx::Point root_location =
        views::View::ConvertPointToScreen(tabstrip, tabstrip_center);

    ui::MouseWheelEvent wheel_event(offset, location, root_location,
                                    ui::EventTimeForNow(), flags,
                                    /*changed_button_flags=*/0);
    browser_root_view()->OnMouseWheel(wheel_event);
  }

  void StopAnimatingAndLayout() {
    browser_view()->horizontal_tab_strip_for_testing()->StopAnimating();
    RunScheduledLayouts();
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(HorizontalScrollableTabStripBrowserTest,
                       GetScrollDirectionIsHorizontal) {
  // When the feature is enabled, the scroll direction should be horizontal.
  auto* tab_strip = views::AsViewClass<BraveTabStrip>(
      browser_view()->horizontal_tab_strip_for_testing());
  BraveTabContainer* container = views::AsViewClass<BraveTabContainer>(
      tab_strip->GetTabContainerForTesting());
  ASSERT_TRUE(container);

  auto direction = container->GetScrollDirection();
  ASSERT_TRUE(direction.has_value());
  EXPECT_EQ(direction.value(), views::LayoutOrientation::kHorizontal);
}

IN_PROC_BROWSER_TEST_F(HorizontalScrollableTabStripBrowserTest,
                       MaxScrollOffsetZeroWithFewTabs) {
  auto* tab_strip = views::AsViewClass<BraveTabStrip>(
      browser_view()->horizontal_tab_strip_for_testing());
  BraveTabContainer* container = views::AsViewClass<BraveTabContainer>(
      tab_strip->GetTabContainerForTesting());
  ASSERT_TRUE(container);
  auto* model = browser()->tab_strip_model();

  ASSERT_EQ(1, model->count());
  StopAnimatingAndLayout();
  EXPECT_EQ(0, container->GetMaxScrollOffset());
}

IN_PROC_BROWSER_TEST_F(HorizontalScrollableTabStripBrowserTest,
                       MaxScrollOffsetZeroWithPinnedAndUnpinnedTab) {
  // With one pinned tab and one unpinned tab, and enough space so that the
  // tab strip does not need to scroll, GetMaxScrollOffset() should be 0.
  auto* tab_strip = views::AsViewClass<BraveTabStrip>(
      browser_view()->horizontal_tab_strip_for_testing());
  BraveTabContainer* container = views::AsViewClass<BraveTabContainer>(
      tab_strip->GetTabContainerForTesting());
  ASSERT_TRUE(container);
  auto* model = browser()->tab_strip_model();

  ASSERT_EQ(1, model->count());
  AppendTab();
  ASSERT_EQ(2, model->count());
  model->SetTabPinned(0, true);
  StopAnimatingAndLayout();

  EXPECT_EQ(0, container->GetMaxScrollOffset())
      << "Pinned + one unpinned tab with enough space should not be scrollable";
}

IN_PROC_BROWSER_TEST_F(HorizontalScrollableTabStripBrowserTest,
                       MaxScrollOffsetPositiveWithManyTabs) {
  auto* tab_strip = views::AsViewClass<BraveTabStrip>(
      browser_view()->horizontal_tab_strip_for_testing());
  BraveTabContainer* container = views::AsViewClass<BraveTabContainer>(
      tab_strip->GetTabContainerForTesting());
  ASSERT_TRUE(container);

  while (container->GetMaxScrollOffset() == 0) {
    AppendTab();
    StopAnimatingAndLayout();
  }
  // In case of failure, the test will hang.
}

IN_PROC_BROWSER_TEST_F(HorizontalScrollableTabStripBrowserTest,
                       ActiveTabScrollsIntoViewWhenSelectingLast) {
  auto* tab_strip = views::AsViewClass<BraveTabStrip>(
      browser_view()->horizontal_tab_strip_for_testing());
  BraveTabContainer* container = views::AsViewClass<BraveTabContainer>(
      tab_strip->GetTabContainerForTesting());
  ASSERT_TRUE(container);
  auto* model = browser()->tab_strip_model();

  while (container->GetMaxScrollOffset() <= 200) {
    AppendTab();
    StopAnimatingAndLayout();
  }

  model->SelectTabAt(0);
  StopAnimatingAndLayout();
  EXPECT_EQ(container->scroll_offset_, 0);

  // After activating the last tab, the scroll offset should be the maximum.
  model->SelectLastTab();
  StopAnimatingAndLayout();

  EXPECT_EQ(model->active_index(), static_cast<int>(model->count() - 1));
  EXPECT_EQ(container->scroll_offset_, container->GetMaxScrollOffset());
}

IN_PROC_BROWSER_TEST_F(HorizontalScrollableTabStripBrowserTest,
                       ScrollOffsetClampedWhenTabRemoved) {
  auto* tab_strip = views::AsViewClass<BraveTabStrip>(
      browser_view()->horizontal_tab_strip_for_testing());
  BraveTabContainer* container = views::AsViewClass<BraveTabContainer>(
      tab_strip->GetTabContainerForTesting());
  ASSERT_TRUE(container);
  auto* model = browser()->tab_strip_model();

  while (container->GetMaxScrollOffset() < 200) {
    AppendTab();
    StopAnimatingAndLayout();
  }

  model->SelectLastTab();
  StopAnimatingAndLayout();
  const int scroll_before = container->scroll_offset_;

  model->CloseWebContentsAt(0, TabCloseTypes::CLOSE_USER_GESTURE);
  StopAnimatingAndLayout();

  EXPECT_LT(container->scroll_offset_, scroll_before)
      << "Scroll should be clamped after closing a tab";
}

IN_PROC_BROWSER_TEST_F(HorizontalScrollableTabStripBrowserTest,
                       CtrlWheelSwitchesTabPlainWheelDoesNot) {
  if (!browser_defaults::kScrollEventChangesTab) {
    GTEST_SKIP() << "Scroll-to-change-tab is disabled on this platform.";
  }

  TabStripModel* model = browser()->tab_strip_model();
  while (model->count() < 2) {
    ASSERT_TRUE(
        AddTabAtIndex(0, GURL(url::kAboutBlankURL), ui::PAGE_TRANSITION_LINK));
  }
  model->ActivateTabAt(1);
  ASSERT_EQ(1, model->active_index());

  const gfx::Vector2d kWheelUp(0, ui::MouseWheelEvent::kWheelDelta);
  const gfx::Vector2d kWheelDown(0, -ui::MouseWheelEvent::kWheelDelta);

  // With kBraveScrollableTabStrip, only Ctrl+wheel should change tabs; plain
  // wheel is left for horizontal tab-strip scrolling.
  PerformMouseWheelOnTabStrip(kWheelUp, /*flags=*/0);
  EXPECT_EQ(1, model->active_index())
      << "Plain wheel should not activate another tab";

  PerformMouseWheelOnTabStrip(kWheelUp, ui::EF_CONTROL_DOWN);
  EXPECT_EQ(0, model->active_index())
      << "Ctrl+wheel should activate the previous tab";

  PerformMouseWheelOnTabStrip(kWheelDown, /*flags=*/0);
  EXPECT_EQ(0, model->active_index())
      << "Plain wheel should not activate the next tab";

  PerformMouseWheelOnTabStrip(kWheelDown, ui::EF_CONTROL_DOWN);
  EXPECT_EQ(1, model->active_index())
      << "Ctrl+wheel should activate the next tab";
}
