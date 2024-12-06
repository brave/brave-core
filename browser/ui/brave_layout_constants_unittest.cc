/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/layout_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/pointer/mock_touch_ui_controller.h"
#include "ui/gfx/geometry/insets.h"

TEST(BraveLayoutConstantsTest, BraveValueTest) {
  ui::MockTouchUiController controller;
  EXPECT_FALSE(controller.touch_ui());

  EXPECT_EQ(gfx::Insets(5), GetLayoutInsets(TOOLBAR_BUTTON));
  EXPECT_EQ(28, GetLayoutConstant(TOOLBAR_BUTTON_HEIGHT));
  EXPECT_EQ(4, GetLayoutConstant(LOCATION_BAR_CHILD_CORNER_RADIUS));
  EXPECT_EQ(GetLayoutConstant(LOCATION_BAR_ELEMENT_PADDING),
            GetLayoutConstant(LOCATION_BAR_TRAILING_DECORATION_EDGE_PADDING));
}
