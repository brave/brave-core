/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/features.h"
#include "src/chrome/browser/ui/views/tabs/tab_drag_controller_interactive_uitest.cc"

class DetachToBrowserTabDragControllerTestWithSplitViewEnabled
    : public DetachToBrowserTabDragControllerTest {
 public:
  DetachToBrowserTabDragControllerTestWithSplitViewEnabled() {
    scoped_feature_list_.InitAndEnableFeature(
        tabs::features::kBraveSplitView);
  }
  private:
    base::test::ScopedFeatureList scoped_feature_list_;
};

// Creates two browsers, then drags a group from one to the other.
IN_PROC_BROWSER_TEST_P(DetachToBrowserTabDragControllerTestWithSplitViewEnabled,
                       DragGroupHeaderToSeparateWindow) {
  ASSERT_TRUE(browser()->tab_strip_model()->SupportsTabGroups());

  TabStrip* tab_strip = GetTabStripForBrowser(browser());
  TabStripModel* model = browser()->tab_strip_model();
  AddTabsAndResetBrowser(browser(), 1);
  tab_groups::TabGroupId group = model->AddToNewGroup({0, 1});
  tab_groups::TabGroupColorId group_color = tab_strip->GetGroupColorId(group);
  StopAnimating(tab_strip);

  // Create another browser.
  Browser* browser2 = CreateAnotherBrowserAndResize();
  TabStrip* tab_strip2 = GetTabStripForBrowser(browser2);
  TabStripModel* model2 = browser2->tab_strip_model();
  StopAnimating(tab_strip2);

  // Drag the group by its header into the second browser.
  DragToDetachGroupAndNotify(tab_strip,
                             base::BindOnce(&DragAllToSeparateWindowStep2, this,
                                            tab_strip, tab_strip2),
                             group);
  ASSERT_TRUE(ReleaseInput());

  // Expect the group to be in browser2, but with a new tab_groups::TabGroupId.
  EXPECT_EQ("100 0 1", IDString(model2));
  std::vector<tab_groups::TabGroupId> groups2 =
      model2->group_model()->ListTabGroups();
  EXPECT_EQ(1u, groups2.size());
  EXPECT_EQ(model2->group_model()->GetTabGroup(groups2[0])->ListTabs(),
            gfx::Range(1, 3));
  EXPECT_EQ(groups2[0], group);
  EXPECT_EQ(tab_strip2->GetGroupColorId(groups2[0]), group_color);
}

INSTANTIATE_TEST_SUITE_P(
    TabDragging,
    DetachToBrowserTabDragControllerTestWithSplitViewEnabled,
    ::testing::Combine(
        /*kSplitTabStrip=*/::testing::Bool(),
        /*kTearOffWebAppTabOpensWebAppWindow=*/::testing::Values(false),
        /*input_source=*/::testing::Values("mouse"),
        /*kAllowWindowDragUsingSystemDragDrop=*/::testing::Bool()));