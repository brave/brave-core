/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/brave_compact_horizontal_tabs_layout.h"
#include "chrome/browser/ui/tabs/features.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/pointer/mock_touch_ui_controller.h"
#include "ui/gfx/geometry/insets.h"

TEST(BraveLayoutConstantsTest, BraveValueTest) {
  ui::MockTouchUiController controller;
  EXPECT_FALSE(controller.touch_ui());

  EXPECT_EQ(gfx::Insets(4), GetLayoutInsets(TOOLBAR_BUTTON));
  EXPECT_EQ(8, GetLayoutConstant(LayoutConstant::kToolbarCornerRadius));
  EXPECT_EQ(28, GetLayoutConstant(LayoutConstant::kToolbarButtonHeight));
  EXPECT_EQ(4,
            GetLayoutConstant(LayoutConstant::kLocationBarChildCornerRadius));
  EXPECT_EQ(GetLayoutConstant(LayoutConstant::kLocationBarElementPadding),
            GetLayoutConstant(
                LayoutConstant::kLocationBarTrailingDecorationEdgePadding));
}

// Tab strip + toolbar (URL bar row) should stay within the compact horizontal
// tabs design budget (~60 DIP) when the flag is on and touch UI is off.
TEST(BraveLayoutConstantsTest, CompactHorizontalTabsCombinedChromeHeight) {
  ui::MockTouchUiController controller;
  EXPECT_FALSE(controller.touch_ui());

  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeatures(
      {tabs::kBraveCompactHorizontalTabs, tabs::kBraveHorizontalTabsUpdate},
      {});

  const int overlap =
      GetLayoutConstant(LayoutConstant::kTabstripToolbarOverlap);
  const int tab_strip = GetLayoutConstant(LayoutConstant::kTabStripHeight);
  const gfx::Insets toolbar_insets = GetLayoutInsets(TOOLBAR_INTERIOR_MARGIN);
  const int location_bar =
      GetLayoutConstant(LayoutConstant::kLocationBarHeight);
  const int toolbar_row = toolbar_insets.height() + location_bar;
  const int combined = tab_strip + toolbar_row - overlap;

  EXPECT_EQ(overlap,
            tabs::compact_horizontal_tabs_layout::kTabstripToolbarOverlap);
  EXPECT_EQ(tab_strip,
            tabs::compact_horizontal_tabs_layout::kTabStripLayoutHeightTarget);
  EXPECT_LE(combined, 60);
}
