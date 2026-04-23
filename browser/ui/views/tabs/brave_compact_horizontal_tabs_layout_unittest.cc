/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/brave_compact_horizontal_tabs_layout.h"

#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/features.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/pointer/touch_ui_controller.h"

namespace tabs {

// Verifies that when `#brave-compact-horizontal-tabs` is enabled and touch UI
// is off, `GetLayoutConstant` returns the compact metrics defined in
// `brave_compact_horizontal_tabs_layout.h`.
class BraveCompactHorizontalTabsLayoutTest : public ::testing::Test {
 public:
  BraveCompactHorizontalTabsLayoutTest() {
    scoped_feature_list_.InitWithFeatures(
        {tabs::kBraveCompactHorizontalTabs, tabs::kBraveHorizontalTabsUpdate},
        {});
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  ui::TouchUiController::TouchUiScoperForTesting touch_ui_scoper_{
      /*touch_ui_enabled=*/false};
};

TEST_F(BraveCompactHorizontalTabsLayoutTest, SelectsCompactLayoutConstants) {
  ASSERT_FALSE(ui::TouchUiController::Get()->touch_ui());

  EXPECT_EQ(compact_horizontal_tabs_layout::kLocationBarHeight,
            GetLayoutConstant(LayoutConstant::kLocationBarHeight));
  EXPECT_EQ(compact_horizontal_tabs_layout::kTabVerticalSpacing,
            GetLayoutConstant(LayoutConstant::kTabStripPadding));
  EXPECT_EQ(compact_horizontal_tabs_layout::kTabstripToolbarOverlap,
            GetLayoutConstant(LayoutConstant::kTabstripToolbarOverlap));
}

}  // namespace tabs
