/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_drag_controller.h"

#include "brave/browser/ui/tabs/features.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"

class TabDragControllerTest : public InProcessBrowserTest {
 public:
  ~TabDragControllerTest() override = default;

  TabDragControllerTest() {
    scoped_feature_list_.InitAndEnableFeature(tabs::features::kBraveSplitView);
  }

 protected:
  void AppendTab(Browser* browser) {
    chrome::AddTabAt(browser, GURL(), -1, true);
  }

  tab_groups::TabGroupId AddTabToNewGroup(Browser* browser, int tab_index) {
    return browser->tab_strip_model()->AddToNewGroup({tab_index});
  }

  TabStrip* GetTabStripForBrowser(Browser* browser) {
    BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
    return browser_view->tabstrip();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
// It's flaky. Upstream also runs group header detach test only on Windows.
// See
// DetachToBrowserTabDragControllerTest.MAYBE_DragGroupHeaderToSeparateWindow
#define MAYBE_DragGroupHeaderToSeparateWindow \
  DISABLED_DragGroupHeaderToSeparateWindow
#else
#define MAYBE_DragGroupHeaderToSeparateWindow DragGroupHeaderToSeparateWindow
#endif

// Browser test for https://github.com/brave/brave-browser/issues/39486
IN_PROC_BROWSER_TEST_F(TabDragControllerTest,
                       MAYBE_DragGroupHeaderToSeparateWindow) {
  ASSERT_TRUE(browser()->tab_strip_model()->SupportsTabGroups());
  tab_groups::TabGroupId group = AddTabToNewGroup(browser(), 0);
  AppendTab(browser());

  TabStrip* tab_strip = GetTabStripForBrowser(browser());
  EXPECT_EQ(tab_strip->tab_at(0)->group().value(), group);
  EXPECT_EQ(tab_strip->tab_at(1)->group(), std::nullopt);

  TabGroupHeader* tab_group_header = tab_strip->group_header(group);
  gfx::Point tab_group_header_center(tab_group_header->width() / 2,
                                     tab_group_header->height() / 2);
  const ui::MouseEvent mouse_pressed_event(
      ui::EventType::kMousePressed, tab_group_header_center,
      tab_group_header_center, base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON,
      ui::EF_LEFT_MOUSE_BUTTON);
  tab_strip->StopAnimating(true);
  tab_strip->MaybeStartDrag(tab_group_header, mouse_pressed_event,
                            tab_strip->GetSelectionModel());

  static const int kDragDistance = 20;
  gfx::Point tab_group_drag_point(tab_group_header->width() + kDragDistance,
                                  tab_group_header->height() + kDragDistance);
  ui::MouseEvent mouse_dragged_event(
      ui::EventType::kMouseDragged, tab_group_drag_point, tab_group_drag_point,
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, 0);
  std::ignore = tab_strip->ContinueDrag(tab_group_header, mouse_dragged_event);
  tab_strip->EndDrag(EndDragReason::END_DRAG_COMPLETE);
}
