// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/tabs/brave_tab.h"

#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/tabs/fake_tab_slot_controller.h"
#include "chrome/browser/ui/views/tabs/tab_style_views.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/views/test/views_test_utils.h"

class BraveTabTest : public ChromeViewsTestBase {
 public:
  BraveTabTest() = default;
  ~BraveTabTest() override = default;

  void LayoutAndCheckBorder(BraveTab* tab, const gfx::Rect& bounds) {
    tab->SetBoundsRect(bounds);
    views::test::RunScheduledLayout(tab);

    auto insets = tab->tab_style_views()->GetContentsInsets();
    int left_inset = insets.left();
    left_inset += BraveTab::kExtraLeftPadding;
    EXPECT_EQ(left_inset, tab->GetInsets().left());
  }
};

TEST_F(BraveTabTest, ExtraPaddingLayoutTest) {
  FakeTabSlotController tab_slot_controller;
  BraveTab tab(&tab_slot_controller);

  // Our tab should have extra padding always.
  // See the comment at BraveTab::GetInsets().
  LayoutAndCheckBorder(&tab, {0, 0, 30, 50});
  LayoutAndCheckBorder(&tab, {0, 0, 50, 50});
  LayoutAndCheckBorder(&tab, {0, 0, 100, 50});
  LayoutAndCheckBorder(&tab, {0, 0, 150, 50});
  LayoutAndCheckBorder(&tab, {0, 0, 30, 50});
}

// Check tab's region inside of vertical padding.
TEST_F(BraveTabTest, TabHeightTest) {
  FakeTabSlotController tab_slot_controller;
  BraveTab tab(&tab_slot_controller);
  tab.SetBoundsRect({0, 0, 100, GetLayoutConstant(TAB_STRIP_HEIGHT)});
  EXPECT_EQ(tab.GetLocalBounds().height() -
                GetLayoutConstant(TABSTRIP_TOOLBAR_OVERLAP),
            tab.GetContentsBounds().height());

  SkPath mask = tab.tab_style_views()->GetPath(TabStyle::PathType::kFill,
                                               /* scale */ 1.0,
                                               /* force_active */ false,
                                               TabStyle::RenderUnits::kDips);
  SkRegion clip_region;
  clip_region.setRect({0, 0, tab.width(), tab.height()});
  SkRegion mask_region;
  ASSERT_TRUE(mask_region.setPath(mask, clip_region));

  // Check outside of tab region.
  gfx::Rect rect(50, 0, 1, 1);
  EXPECT_FALSE(mask_region.intersects(RectToSkIRect(rect)));
  rect.set_y(GetLayoutConstant(TAB_STRIP_PADDING) - 1);
  EXPECT_FALSE(mask_region.intersects(RectToSkIRect(rect)));

  // Check inside of tab region.
  rect.set_y(GetLayoutConstant(TAB_STRIP_PADDING));
  EXPECT_TRUE(mask_region.intersects(RectToSkIRect(rect)));
  rect.set_y(GetLayoutConstant(TAB_STRIP_PADDING) +
             GetLayoutConstant(TAB_HEIGHT) - 1);
  EXPECT_TRUE(mask_region.intersects(RectToSkIRect(rect)));

  // Check outside of tab region.
  rect.set_y(GetLayoutConstant(TAB_STRIP_PADDING) +
             GetLayoutConstant(TAB_HEIGHT));
  EXPECT_FALSE(mask_region.intersects(RectToSkIRect(rect)));
}
