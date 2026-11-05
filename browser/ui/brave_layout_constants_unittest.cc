/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/brave_ui_features.h"
#include "chrome/browser/ui/layout_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/pointer/mock_touch_ui_controller.h"
#include "ui/gfx/geometry/insets.h"

TEST(BraveLayoutConstantsTest, BraveValueTest) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(features::kBraveToolbarsSmallIcons);
  ui::MockTouchUiController controller;
  EXPECT_FALSE(controller.touch_ui());

  EXPECT_EQ(gfx::Insets(4), GetLayoutInsets(TOOLBAR_BUTTON));
  EXPECT_EQ(gfx::Insets::VH(4, 4),
            GetLayoutInsets(LOCATION_BAR_PAGE_ACTION_ICON_PADDING));
  EXPECT_EQ(20, GetLayoutConstant(LayoutConstant::kToolbarButtonIconSize));
  EXPECT_EQ(20,
            GetLayoutConstant(LayoutConstant::kLocationBarTrailingIconSize));
  EXPECT_EQ(8, GetLayoutConstant(LayoutConstant::kToolbarCornerRadius));
  EXPECT_EQ(28, GetLayoutConstant(LayoutConstant::kToolbarButtonHeight));
  EXPECT_EQ(6,
            GetLayoutConstant(LayoutConstant::kLocationBarChildCornerRadius));
  EXPECT_EQ(6, GetLayoutConstant(LayoutConstant::kLocationBarChipPadding));
  EXPECT_EQ(2, GetLayoutConstant(LayoutConstant::kLocationBarElementPadding));
  EXPECT_EQ(0, GetLayoutConstant(
                   LayoutConstant::kLocationBarTrailingDecorationEdgePadding));
  EXPECT_EQ(16, GetLayoutConstant(LayoutConstant::kTabCloseButtonSize));
  EXPECT_EQ(16, GetLayoutConstant(LayoutConstant::kTabAlertIndicatorIconWidth));
  EXPECT_EQ(20,
            GetLayoutConstant(LayoutConstant::kVerticalTabStripButtonIconSize));
}

TEST(BraveLayoutConstantsTest, ToolbarsSmallIconsTest) {
  base::test::ScopedFeatureList feature_list(
      features::kBraveToolbarsSmallIcons);
  ui::MockTouchUiController controller;
  EXPECT_FALSE(controller.touch_ui());

  // Buttons keep their size: icon + insets still adds up to 28.
  EXPECT_EQ(gfx::Insets(5), GetLayoutInsets(TOOLBAR_BUTTON));
  EXPECT_EQ(gfx::Insets::VH(5, 5),
            GetLayoutInsets(LOCATION_BAR_PAGE_ACTION_ICON_PADDING));
  EXPECT_EQ(18, GetLayoutConstant(LayoutConstant::kToolbarButtonIconSize));
  EXPECT_EQ(18,
            GetLayoutConstant(LayoutConstant::kLocationBarTrailingIconSize));
  EXPECT_EQ(28, GetLayoutConstant(LayoutConstant::kToolbarButtonHeight));
  EXPECT_EQ(18,
            GetLayoutConstant(LayoutConstant::kVerticalTabStripButtonIconSize));
}
