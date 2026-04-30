/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_compact_horizontal_tabs_layout.h"

#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/features.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/pointer/touch_ui_controller.h"

namespace tabs {
namespace {

void ExpectDefaultLayoutConstants() {
  EXPECT_EQ(compact_horizontal_tabs_layout::kLocationBarHeightDefault,
            GetLayoutConstant(LayoutConstant::kLocationBarHeight));
  EXPECT_EQ(compact_horizontal_tabs_layout::kTabVerticalSpacingDefault,
            GetLayoutConstant(LayoutConstant::kTabStripPadding));
  EXPECT_EQ(compact_horizontal_tabs_layout::kTabstripToolbarOverlapDefault,
            GetLayoutConstant(LayoutConstant::kTabstripToolbarOverlap));
  EXPECT_EQ(compact_horizontal_tabs_layout::kTabStripControlsHeightDeltaDefault,
            GetHorizontalTabControlsDelta());
}

}  // namespace

// Verifies that `GetLayoutConstant` selects compact vs default metrics based
// on the `#brave-compact-horizontal-tabs` flag and touch UI state. Both sides
// of the guard condition are covered below.
class BraveCompactHorizontalTabsLayoutTest : public ::testing::Test {
 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Compact flag on + touch UI off: `GetLayoutConstant` should return the
// compact metrics from `brave_compact_horizontal_tabs_layout.h`.
TEST_F(BraveCompactHorizontalTabsLayoutTest, SelectsCompactLayoutConstants) {
  scoped_feature_list_.InitWithFeatures(
      {tabs::kBraveCompactHorizontalTabs, tabs::kBraveHorizontalTabsUpdate},
      {});
  ui::TouchUiController::TouchUiScoperForTesting touch_ui_scoper(
      /*touch_ui_enabled=*/false);

  ASSERT_FALSE(ui::TouchUiController::Get()->touch_ui());

  EXPECT_EQ(compact_horizontal_tabs_layout::kLocationBarHeight,
            GetLayoutConstant(LayoutConstant::kLocationBarHeight));
  EXPECT_EQ(compact_horizontal_tabs_layout::kTabVerticalSpacing,
            GetLayoutConstant(LayoutConstant::kTabStripPadding));
  EXPECT_EQ(compact_horizontal_tabs_layout::kTabstripToolbarOverlap,
            GetLayoutConstant(LayoutConstant::kTabstripToolbarOverlap));
  EXPECT_EQ(compact_horizontal_tabs_layout::kTabStripControlsHeightDelta,
            GetHorizontalTabControlsDelta());
}

// Compact flag off: `GetLayoutConstant` should return the default metrics
// regardless of touch UI (exercises the `!UseCompactHorizontalTabs()` side of
// the guard in `ShouldUseCompactHorizontalTabsForNonTouchUI`).
TEST_F(BraveCompactHorizontalTabsLayoutTest,
       FlagDisabledReturnsDefaultLayoutConstants) {
  scoped_feature_list_.InitWithFeatures({tabs::kBraveHorizontalTabsUpdate},
                                        {tabs::kBraveCompactHorizontalTabs});
  ui::TouchUiController::TouchUiScoperForTesting touch_ui_scoper(
      /*touch_ui_enabled=*/false);

  ASSERT_FALSE(ui::TouchUiController::Get()->touch_ui());

  ExpectDefaultLayoutConstants();
}

// Compact flag on but touch UI active: compact metrics are intentionally
// suppressed so touch targets stay usable; defaults should be returned
// (exercises the `!touch_ui()` side of the guard).
TEST_F(BraveCompactHorizontalTabsLayoutTest,
       TouchUiReturnsDefaultLayoutConstants) {
  scoped_feature_list_.InitWithFeatures(
      {tabs::kBraveCompactHorizontalTabs, tabs::kBraveHorizontalTabsUpdate},
      {});
  ui::TouchUiController::TouchUiScoperForTesting touch_ui_scoper(
      /*touch_ui_enabled=*/true);

  ASSERT_TRUE(ui::TouchUiController::Get()->touch_ui());

  ExpectDefaultLayoutConstants();
}

}  // namespace tabs
