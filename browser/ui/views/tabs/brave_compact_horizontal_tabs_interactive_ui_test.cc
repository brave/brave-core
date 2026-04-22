/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/brave_compact_horizontal_tabs_layout.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/test/interaction/interactive_browser_test.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/pointer/mock_touch_ui_controller.h"
#include "ui/base/pointer/touch_ui_controller.h"

// CUJ: #brave-compact-horizontal-tabs drives smaller toolbar/tab metrics when
// the flag is on and touch UI is off (see layout_constants.cc).

class BraveCompactHorizontalTabsInteractiveTest
    : public InteractiveBrowserTest {
 public:
  BraveCompactHorizontalTabsInteractiveTest() {
    scoped_feature_list_.InitWithFeatures(
        {tabs::kBraveCompactHorizontalTabs, tabs::kBraveHorizontalTabsUpdate},
        {});
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveCompactHorizontalTabsInteractiveTest,
                       SelectsCompactLayoutConstants) {
  RunTestSequence(Do([] {
    ui::MockTouchUiController no_touch(
        ui::TouchUiController::TouchUiState::kDisabled);
    ASSERT_FALSE(ui::TouchUiController::Get()->touch_ui());
    using tabs::compact_horizontal_tabs_layout;
    EXPECT_EQ(compact_horizontal_tabs_layout::kLocationBarHeight,
              GetLayoutConstant(LayoutConstant::kLocationBarHeight));
    EXPECT_EQ(compact_horizontal_tabs_layout::kTabVerticalSpacing,
              GetLayoutConstant(LayoutConstant::kTabStripPadding));
    EXPECT_EQ(compact_horizontal_tabs_layout::kTabstripToolbarOverlap,
              GetLayoutConstant(LayoutConstant::kTabstripToolbarOverlap));
  }));
}
