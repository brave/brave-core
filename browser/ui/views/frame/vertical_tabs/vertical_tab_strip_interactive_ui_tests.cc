/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_widget_delegate_view.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "chrome/test/interaction/interactive_browser_test.h"
#include "content/public/test/browser_test.h"
#include "ui/base/test/ui_controls.h"

class VerticalTabStripDragAndDropInteractiveUITest
    : public InteractiveBrowserTest {
 public:
  VerticalTabStripDragAndDropInteractiveUITest() {
    feature_list_.InitAndEnableFeature(features::kBraveRoundedCornersByDefault);
  }
  ~VerticalTabStripDragAndDropInteractiveUITest() override = default;

  BraveBrowserView* browser_view() {
    return static_cast<BraveBrowserView*>(browser()->window());
  }

  void ToggleVerticalTabStrip() {
    brave::ToggleVerticalTabStrip(browser());
    RunScheduledLayouts();
  }

  void AppendTab(Browser* b) { chrome::AddTabAt(b, GURL(), -1, true); }

  TabStrip* GetTabStrip(Browser* b) {
    return BrowserView::GetBrowserViewForBrowser(b)
        ->horizontal_tab_strip_for_testing();
  }

  Tab* GetTabAt(Browser* b, int index) { return GetTabStrip(b)->tab_at(index); }

  gfx::Point GetCenterPointInScreen(views::View* view) {
    return view->GetBoundsInScreen().CenterPoint();
  }

  void PressTabAt(Browser* browser, int index) {
    ASSERT_TRUE(ui_test_utils::SendMouseMoveSync(
        GetCenterPointInScreen(GetTabAt(browser, index))));
    ASSERT_TRUE(ui_test_utils::SendMouseEventsSync(ui_controls::LEFT,
                                                   ui_controls::DOWN));
  }

  void ReleaseMouse() {
    ASSERT_TRUE(
        ui_test_utils::SendMouseEventsSync(ui_controls::LEFT, ui_controls::UP));
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
    EXPECT_TRUE(base::test::RunUntil([&]() { return moved; }));
  }

  bool IsDraggingTabStrip(Browser* b) {
    return GetTabStrip(b)->GetDragContext()->GetDragController() != nullptr;
  }

  // InteractiveBrowserTest:
  void SetUpOnMainThread() override {
    InteractiveBrowserTest::SetUpOnMainThread();

    ToggleVerticalTabStrip();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(VerticalTabStripDragAndDropInteractiveUITest,
                       DragTabToReorder) {
  // Pre-conditions ------------------------------------------------------------
  AppendTab(browser());

  auto* widget_delegate_view =
      browser_view()->vertical_tab_strip_widget_delegate_view();
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

  EXPECT_TRUE(base::test::RunUntil([&]() {
    return pressed_tab == GetTabAt(browser(), 1);
    ;
  }));

  EXPECT_TRUE(IsDraggingTabStrip(browser()));
  ReleaseMouse();
  EXPECT_TRUE(
      base::test::RunUntil([&]() { return !IsDraggingTabStrip(browser()); }));
  GetTabStrip(browser())->StopAnimating();  // Drag-and-drop doesn't start when
                                            // animation is running.
  {
    // Regression test for https://github.com/brave/brave-browser/issues/28488
    // Check if the tab is positioned properly after drag-and-drop.
    auto* moved_tab = GetTabAt(browser(), 1);
    EXPECT_TRUE(region_view->GetBoundsInScreen().Contains(
        moved_tab->GetBoundsInScreen()));
  }
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripDragAndDropInteractiveUITest,
                       DragTabToDetach) {
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
        int same_profile = 0;
        GlobalBrowserCollection::GetInstance()->ForEach(
            [&same_profile, this](BrowserWindowInterface* bwi) {
              if (bwi->GetProfile() == browser()->profile()) {
                ++same_profile;
              }
              return true;
            });
        EXPECT_EQ(2, same_profile);
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

IN_PROC_BROWSER_TEST_F(VerticalTabStripDragAndDropInteractiveUITest, DragURL) {
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("https://brave.com/")));

  auto* location_icon_view =
      browser_view()->GetLocationBarView()->location_icon_view();

  gfx::Point center = GetCenterPointInScreen(location_icon_view);
  ASSERT_TRUE(ui_test_utils::SendMouseMoveSync(center));
  ASSERT_TRUE(
      ui_test_utils::SendMouseEventsSync(ui_controls::LEFT, ui_controls::DOWN));

  gfx::Point drag_target = location_icon_view->GetBoundsInScreen().origin();
  drag_target.Offset(-3, 0);

  // Test if dragging a URL on browser cause a crash. When this happens, the
  // browser root view could try inserting a new tab with the given URL.
  // https://github.com/brave/brave-browser/issues/28592
  bool done = false;
  ui_controls::SendMouseMoveNotifyWhenDone(
      drag_target.x(), drag_target.y(), base::BindLambdaForTesting([&]() {
        done = true;
        ui_controls::SendMouseEvents(ui_controls::LEFT, ui_controls::UP);
      }));
  EXPECT_TRUE(base::test::RunUntil([&]() { return done; }));
}
