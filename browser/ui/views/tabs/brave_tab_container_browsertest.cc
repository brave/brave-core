/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_container.h"

#include <algorithm>

#include "base/location.h"
#include "base/run_loop.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/brave_tab_strip_region_view.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_root_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/horizontal_tab_strip_region_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/browser/ui/views/tabs/tab_strip_control_button.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event_constants.h"
#include "ui/events/test/event_generator.h"
#include "ui/views/repeat_controller.h"
#include "ui/views/view_utils.h"
#include "ui/views/widget/widget_utils.h"
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

  BraveHorizontalTabStripRegionView* tab_strip_region() {
    views::View* tab_strip = browser_view()->horizontal_tab_strip_for_testing();
    return views::AsViewClass<BraveHorizontalTabStripRegionView>(
        tab_strip->parent());
  }

  // Grows the strip so at least |n| scroll-button steps can apply before
  // clamping (viewport/4 as step is often greater than the max offset after
  // the first few overflow tabs).
  void GrowHorizontalStripUntilMaxOffsetAtLeastNTimesStep(
      BraveTabContainer* container,
      int n,
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    for (int i = 0; i < 100; i++) {
      const int max_offset = container->GetMaxScrollOffsetForTesting();
      const int step = container->GetHorizontalTabScrollStep();
      if (step > 0 && max_offset >= n * step) {
        return;
      }
      AppendTab();
      StopAnimatingAndLayout();
    }
    GTEST_FAIL()
        << "Failed to get enough scroll headroom for repeat/hold test.";
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    // Horizontal scrolling requires kBraveScrollableTabStrip and this pref.
    browser()->profile()->GetPrefs()->SetBoolean(
        brave_tabs::kScrollableHorizontalTabStrip, true);
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(HorizontalScrollableTabStripBrowserTest,
                       GetScrollDirectionIsHorizontal) {
  // When the feature and scrollable-horizontal-tab-strip pref are enabled, the
  // scroll direction should be horizontal.
  auto* tab_strip = views::AsViewClass<BraveTabStrip>(
      browser_view()->horizontal_tab_strip_for_testing());
  BraveTabContainer* container = views::AsViewClass<BraveTabContainer>(
      tab_strip->GetTabContainerForTesting());
  ASSERT_TRUE(container);

  auto direction = container->GetScrollDirection();
  ASSERT_TRUE(direction.has_value());
  EXPECT_EQ(direction.value(), views::LayoutOrientation::kHorizontal);
}

IN_PROC_BROWSER_TEST_F(
    HorizontalScrollableTabStripBrowserTest,
    GetScrollDirectionNotHorizontalWhenScrollableHorizontalPrefDisabled) {
  // kBraveScrollableTabStrip is enabled in the fixture, but horizontal strip
  // scrolling is still gated by kScrollableHorizontalTabStrip.
  browser()->profile()->GetPrefs()->SetBoolean(
      brave_tabs::kScrollableHorizontalTabStrip, false);
  RunScheduledLayouts();

  auto* tab_strip = views::AsViewClass<BraveTabStrip>(
      browser_view()->horizontal_tab_strip_for_testing());
  BraveTabContainer* container = views::AsViewClass<BraveTabContainer>(
      tab_strip->GetTabContainerForTesting());
  ASSERT_TRUE(container);

  EXPECT_NE(std::optional<views::LayoutOrientation>(
                views::LayoutOrientation::kHorizontal),
            container->GetScrollDirection());
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
  EXPECT_EQ(0, container->GetMaxScrollOffsetForTesting());
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

  EXPECT_EQ(0, container->GetMaxScrollOffsetForTesting())
      << "Pinned + one unpinned tab with enough space should not be scrollable";
}

IN_PROC_BROWSER_TEST_F(HorizontalScrollableTabStripBrowserTest,
                       MaxScrollOffsetPositiveWithManyTabs) {
  auto* tab_strip = views::AsViewClass<BraveTabStrip>(
      browser_view()->horizontal_tab_strip_for_testing());
  BraveTabContainer* container = views::AsViewClass<BraveTabContainer>(
      tab_strip->GetTabContainerForTesting());
  ASSERT_TRUE(container);

  while (container->GetMaxScrollOffsetForTesting() == 0) {
    AppendTab();
    StopAnimatingAndLayout();
  }
  // In case of failure, the test will hang.
}

IN_PROC_BROWSER_TEST_F(HorizontalScrollableTabStripBrowserTest,
                       AddingNewTabShouldScrollToBeVisible) {
  auto* tab_strip = views::AsViewClass<BraveTabStrip>(
      browser_view()->horizontal_tab_strip_for_testing());
  BraveTabContainer* container = views::AsViewClass<BraveTabContainer>(
      tab_strip->GetTabContainerForTesting());
  ASSERT_TRUE(container);

  while (container->GetMaxScrollOffsetForTesting() == 0) {
    AppendTab();
    StopAnimatingAndLayout();
  }
  // Adding foreground tab should scroll to be visible.
  EXPECT_EQ(container->GetScrollOffsetForTesting(),
            container->GetMaxScrollOffsetForTesting());

  container->SetScrollOffsetForTesting(0);

  // Also adding background tab should scroll to be visible.
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground=*/false);
  StopAnimatingAndLayout();
  EXPECT_EQ(container->GetScrollOffsetForTesting(),
            container->GetMaxScrollOffsetForTesting());
}

IN_PROC_BROWSER_TEST_F(HorizontalScrollableTabStripBrowserTest,
                       ActiveTabScrollsIntoViewWhenSelectingLast) {
  auto* tab_strip = views::AsViewClass<BraveTabStrip>(
      browser_view()->horizontal_tab_strip_for_testing());
  BraveTabContainer* container = views::AsViewClass<BraveTabContainer>(
      tab_strip->GetTabContainerForTesting());
  ASSERT_TRUE(container);
  auto* model = browser()->tab_strip_model();

  while (container->GetMaxScrollOffsetForTesting() <= 200) {
    AppendTab();
    StopAnimatingAndLayout();
  }

  model->SelectTabAt(0);
  StopAnimatingAndLayout();
  EXPECT_EQ(container->GetScrollOffsetForTesting(), 0);

  // After activating the last tab, the scroll offset should be the maximum.
  model->SelectLastTab();
  StopAnimatingAndLayout();

  EXPECT_EQ(model->active_index(), static_cast<int>(model->count() - 1));
  EXPECT_EQ(container->GetScrollOffsetForTesting(),
            container->GetMaxScrollOffsetForTesting());
}

IN_PROC_BROWSER_TEST_F(HorizontalScrollableTabStripBrowserTest,
                       ScrollOffsetClampedWhenTabRemoved) {
  auto* tab_strip = views::AsViewClass<BraveTabStrip>(
      browser_view()->horizontal_tab_strip_for_testing());
  BraveTabContainer* container = views::AsViewClass<BraveTabContainer>(
      tab_strip->GetTabContainerForTesting());
  ASSERT_TRUE(container);
  auto* model = browser()->tab_strip_model();

  while (container->GetMaxScrollOffsetForTesting() < 200) {
    AppendTab();
    StopAnimatingAndLayout();
  }

  model->SelectLastTab();
  StopAnimatingAndLayout();
  const int scroll_before = container->GetScrollOffsetForTesting();

  model->CloseWebContentsAt(0, TabCloseTypes::CLOSE_USER_GESTURE);
  StopAnimatingAndLayout();

  EXPECT_LT(container->GetScrollOffsetForTesting(), scroll_before)
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

IN_PROC_BROWSER_TEST_F(HorizontalScrollableTabStripBrowserTest,
                       HorizontalScrollButtonsHiddenWhenPrefOff) {
  auto* tab_strip = views::AsViewClass<BraveTabStrip>(
      browser_view()->horizontal_tab_strip_for_testing());
  BraveTabContainer* container = views::AsViewClass<BraveTabContainer>(
      tab_strip->GetTabContainerForTesting());
  ASSERT_TRUE(container);

  browser()->profile()->GetPrefs()->SetBoolean(
      brave_tabs::kShowHorizontalTabScrollButtons, false);

  while (container->GetMaxScrollOffsetForTesting() == 0) {
    AppendTab();
    StopAnimatingAndLayout();
  }

  BraveHorizontalTabStripRegionView* region = tab_strip_region();
  ASSERT_TRUE(region);
  ASSERT_TRUE(region->tab_scroll_previous_for_testing());
  EXPECT_FALSE(region->tab_scroll_previous_for_testing()->GetVisible());
}

IN_PROC_BROWSER_TEST_F(HorizontalScrollableTabStripBrowserTest,
                       HorizontalScrollButtonsHiddenWhenPrefOnButNoOverflow) {
  auto* tab_strip = views::AsViewClass<BraveTabStrip>(
      browser_view()->horizontal_tab_strip_for_testing());
  BraveTabContainer* container = views::AsViewClass<BraveTabContainer>(
      tab_strip->GetTabContainerForTesting());
  ASSERT_TRUE(container);

  browser()->profile()->GetPrefs()->SetBoolean(
      brave_tabs::kShowHorizontalTabScrollButtons, true);
  StopAnimatingAndLayout();

  ASSERT_EQ(0, container->GetMaxScrollOffsetForTesting())
      << "Default tab count should not overflow the strip";

  BraveHorizontalTabStripRegionView* region = tab_strip_region();
  ASSERT_TRUE(region);
  ASSERT_TRUE(region->tab_scroll_previous_for_testing());
  EXPECT_FALSE(region->tab_scroll_previous_for_testing()->GetVisible());
}

IN_PROC_BROWSER_TEST_F(HorizontalScrollableTabStripBrowserTest,
                       HorizontalScrollButtonsVisibleWhenPrefOnAndOverflow) {
  auto* tab_strip = views::AsViewClass<BraveTabStrip>(
      browser_view()->horizontal_tab_strip_for_testing());
  BraveTabContainer* container = views::AsViewClass<BraveTabContainer>(
      tab_strip->GetTabContainerForTesting());
  ASSERT_TRUE(container);

  // Grow overflow with scroll buttons off first (same pattern as
  // HorizontalScrollButtonsHiddenWhenPrefOff). Adding many tabs while the
  // scroll-button chrome is visible can change active-tab sizing so much that
  // the strip no longer reports overflow, which makes ScrollTabsBy a no-op.
  browser()->profile()->GetPrefs()->SetBoolean(
      brave_tabs::kShowHorizontalTabScrollButtons, false);
  while (container->GetMaxScrollOffsetForTesting() == 0) {
    AppendTab();
    StopAnimatingAndLayout();
  }

  browser()->profile()->GetPrefs()->SetBoolean(
      brave_tabs::kShowHorizontalTabScrollButtons, true);
  StopAnimatingAndLayout();

  BraveHorizontalTabStripRegionView* region = tab_strip_region();
  ASSERT_TRUE(region);
  ASSERT_TRUE(region->tab_scroll_previous_for_testing());
  EXPECT_TRUE(region->tab_scroll_previous_for_testing()->GetVisible());

  ASSERT_GT(container->GetMaxScrollOffsetForTesting(), 0);
  container->SetScrollOffsetForTesting(
      container->GetMaxScrollOffsetForTesting());
  StopAnimatingAndLayout();
  EXPECT_TRUE(region->tab_scroll_previous_for_testing()->GetEnabled());
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !region->tab_scroll_next_for_testing()->GetEnabled(); }));

  ASSERT_TRUE(container->ShouldShowHorizontalScrollButton());
  container->SetScrollOffsetForTesting(0);
  StopAnimatingAndLayout();
  ASSERT_FALSE(container->CanScrollTabsStart());
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return region->tab_scroll_previous_for_testing()->GetEnabled();
  }));

  container->SetScrollOffsetForTesting(
      container->GetMaxScrollOffsetForTesting());
  StopAnimatingAndLayout();
  const int scroll_at_end = container->GetScrollOffsetForTesting();
  ASSERT_GT(scroll_at_end, 0);
  const int step = container->GetHorizontalTabScrollStep();
  ASSERT_GT(step, 0);
  container->ScrollTabsBy(step);
  StopAnimatingAndLayout();
  EXPECT_LT(container->GetScrollOffsetForTesting(), scroll_at_end);
}

IN_PROC_BROWSER_TEST_F(HorizontalScrollableTabStripBrowserTest,
                       HorizontalScrollButtonsHiddenWithVerticalTabs) {
  browser()->profile()->GetPrefs()->SetBoolean(
      brave_tabs::kShowHorizontalTabScrollButtons, true);

  auto* tab_strip = views::AsViewClass<BraveTabStrip>(
      browser_view()->horizontal_tab_strip_for_testing());
  BraveTabContainer* container = views::AsViewClass<BraveTabContainer>(
      tab_strip->GetTabContainerForTesting());
  ASSERT_TRUE(container);

  while (container->GetMaxScrollOffsetForTesting() == 0) {
    AppendTab();
    StopAnimatingAndLayout();
  }

  browser()->profile()->GetPrefs()->SetBoolean(brave_tabs::kVerticalTabsEnabled,
                                               true);
  StopAnimatingAndLayout();

  BraveHorizontalTabStripRegionView* region = tab_strip_region();
  ASSERT_TRUE(region);
  ASSERT_TRUE(region->tab_scroll_previous_for_testing());
  EXPECT_FALSE(region->tab_scroll_previous_for_testing()->GetVisible());
}

IN_PROC_BROWSER_TEST_F(
    HorizontalScrollableTabStripBrowserTest,
    HorizontalScrollPreviousButtonSingleClickScrollsByOneStep) {
  auto* tab_strip = views::AsViewClass<BraveTabStrip>(
      browser_view()->horizontal_tab_strip_for_testing());
  BraveTabContainer* container = views::AsViewClass<BraveTabContainer>(
      tab_strip->GetTabContainerForTesting());
  ASSERT_TRUE(container);

  browser()->profile()->GetPrefs()->SetBoolean(
      brave_tabs::kShowHorizontalTabScrollButtons, true);
  StopAnimatingAndLayout();
  GrowHorizontalStripUntilMaxOffsetAtLeastNTimesStep(container, 2);

  BraveHorizontalTabStripRegionView* region = tab_strip_region();
  ASSERT_TRUE(region);
  const int max_scroll_offset = container->GetMaxScrollOffsetForTesting();
  ASSERT_GT(max_scroll_offset, 0);
  // Exercise the leading (back) control at max scroll. The trailing (forward)
  // button is disabled at the end; growing many tabs often leaves the last
  // tab active with the strip scrolled to the end.
  browser()->tab_strip_model()->ActivateTabAt(
      std::max(0, browser()->tab_strip_model()->count() - 1));
  container->SetScrollOffsetForTesting(max_scroll_offset);
  StopAnimatingAndLayout();

  TabStripControlButton* back = region->tab_scroll_previous_for_testing();
  ASSERT_TRUE(back);
  ASSERT_TRUE(back->GetVisible());
  ASSERT_TRUE(back->GetEnabled());

  const int step = container->GetHorizontalTabScrollStep();
  ASSERT_GT(step, 0);

  const int before = container->GetScrollOffsetForTesting();

  ui::test::EventGenerator event_generator(
      views::GetRootWindow(browser_view()->GetWidget()),
      browser_view()->GetNativeWindow());
  event_generator.MoveMouseTo(back->GetBoundsInScreen().CenterPoint());
  event_generator.PressLeftButton();
  event_generator.ReleaseLeftButton();
  StopAnimatingAndLayout();

  EXPECT_EQ(before - container->GetScrollOffsetForTesting(), step)
      << "before: " << before
      << " after: " << container->GetScrollOffsetForTesting()
      << " One press+release should match one scroll action (not double on "
         "press, "
         "repeater should not have fired).";
}

IN_PROC_BROWSER_TEST_F(
    HorizontalScrollableTabStripBrowserTest,
    HorizontalScrollPreviousButtonHoldScrollsByMoreThanOneStep) {
  auto* tab_strip = views::AsViewClass<BraveTabStrip>(
      browser_view()->horizontal_tab_strip_for_testing());
  BraveTabContainer* container = views::AsViewClass<BraveTabContainer>(
      tab_strip->GetTabContainerForTesting());
  ASSERT_TRUE(container);

  browser()->profile()->GetPrefs()->SetBoolean(
      brave_tabs::kShowHorizontalTabScrollButtons, false);
  browser()->profile()->GetPrefs()->SetBoolean(
      brave_tabs::kShowHorizontalTabScrollButtons, true);
  StopAnimatingAndLayout();
  GrowHorizontalStripUntilMaxOffsetAtLeastNTimesStep(container, 2);

  const int max_scroll_offset = container->GetMaxScrollOffsetForTesting();
  const int step = container->GetHorizontalTabScrollStep();
  ASSERT_GT(step, 0);
  ASSERT_GE(max_scroll_offset, 2 * step);

  BraveHorizontalTabStripRegionView* region = tab_strip_region();
  ASSERT_TRUE(region);
  browser()->tab_strip_model()->ActivateTabAt(
      std::max(0, browser()->tab_strip_model()->count() - 1));
  container->SetScrollOffsetForTesting(max_scroll_offset);
  StopAnimatingAndLayout();

  TabStripControlButton* back = region->tab_scroll_previous_for_testing();
  ASSERT_TRUE(back);
  ASSERT_TRUE(back->GetVisible());
  ASSERT_TRUE(back->GetEnabled());

  const int before = container->GetScrollOffsetForTesting();

  ui::test::EventGenerator event_generator(
      views::GetRootWindow(browser_view()->GetWidget()),
      browser_view()->GetNativeWindow());
  event_generator.MoveMouseTo(back->GetBoundsInScreen().CenterPoint());
  event_generator.PressLeftButton();
  base::RunLoop run_loop;
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(),
      views::RepeatController::GetInitialWaitForTesting() +
          views::RepeatController::GetRepeatingWaitForTesting() * 2);
  run_loop.Run();
  event_generator.ReleaseLeftButton();
  StopAnimatingAndLayout();

  const int after = container->GetScrollOffsetForTesting();
  EXPECT_GT(before - after, step)
      << "Holding should run the repeat timer and scroll more than one action.";
}
