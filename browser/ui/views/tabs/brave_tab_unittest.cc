// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/tabs/brave_tab.h"

#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/tabs/fake_tab_slot_controller.h"
#include "chrome/browser/ui/views/tabs/tab_close_button.h"
#include "chrome/browser/ui/views/tabs/tab_style_views.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/views/test/views_test_utils.h"

class MockTabSlotController : public FakeTabSlotController {
 public:
  MockTabSlotController() = default;
  ~MockTabSlotController() override = default;

  // FakeTabSlotController overrides:
  MOCK_METHOD(void, CloseTab, (Tab * tab, CloseTabSource source), (override));
};

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
  BraveTab tab(tabs::TabHandle(1), &tab_slot_controller);

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
  BraveTab tab(tabs::TabHandle(1), &tab_slot_controller);
  tab.SetBoundsRect(
      {0, 0, 100, GetLayoutConstant(LayoutConstant::kTabStripHeight)});
  EXPECT_EQ(tab.GetLocalBounds().height() -
                GetLayoutConstant(LayoutConstant::kTabstripToolbarOverlap),
            tab.GetContentsBounds().height());

  SkPath mask = tab.tab_style_views()->GetPath(
      TabStyle::PathType::kFill,
      /* scale */ 1.0, {.render_units = TabStyle::RenderUnits::kDips});
  SkRegion clip_region;
  clip_region.setRect({0, 0, tab.width(), tab.height()});
  SkRegion mask_region;
  ASSERT_TRUE(mask_region.setPath(mask, clip_region));

  // Check outside of tab region.
  gfx::Rect rect(50, 0, 1, 1);
  EXPECT_FALSE(mask_region.intersects(RectToSkIRect(rect)));
  rect.set_y(GetLayoutConstant(LayoutConstant::kTabStripPadding) - 1);
  EXPECT_FALSE(mask_region.intersects(RectToSkIRect(rect)));

  // Check inside of tab region.
  rect.set_y(GetLayoutConstant(LayoutConstant::kTabStripPadding));
  EXPECT_TRUE(mask_region.intersects(RectToSkIRect(rect)));
  rect.set_y(GetLayoutConstant(LayoutConstant::kTabStripPadding) +
             GetLayoutConstant(LayoutConstant::kTabHeight) - 1);
  EXPECT_TRUE(mask_region.intersects(RectToSkIRect(rect)));

  // Check outside of tab region.
  rect.set_y(GetLayoutConstant(LayoutConstant::kTabStripPadding) +
             GetLayoutConstant(LayoutConstant::kTabHeight));
  EXPECT_FALSE(mask_region.intersects(RectToSkIRect(rect)));
}

TEST_F(BraveTabTest, TabStyleTest) {
  FakeTabSlotController tab_slot_controller;
  BraveTab tab(tabs::TabHandle(1), &tab_slot_controller);

  // We use same width for split and non-split tab.
  auto* tab_style = tab.tab_style();
  EXPECT_EQ(tab_style->GetStandardWidth(/*is_split*/ true),
            tab_style->GetStandardWidth(/*is_split*/ false));
  EXPECT_EQ(tab_style->GetMinimumActiveWidth(/*is_split*/ true),
            tab_style->GetMinimumActiveWidth(/*is_split*/ false));
}

TEST_F(BraveTabTest, ShouldAlwaysHideTabCloseButton) {
  FakeTabSlotController tab_slot_controller;
  BraveTab tab(tabs::TabHandle(1), &tab_slot_controller);
  tab_slot_controller.set_active_tab(&tab);

  ASSERT_FALSE(tab_slot_controller.ShouldAlwaysHideCloseButton());
  tab.SetBoundsRect({0, 0, 100, 50});
  views::test::RunScheduledLayout(&tab);
  EXPECT_TRUE(tab.close_button_for_test()->GetVisible());

  tab_slot_controller.set_should_always_hide_close_button(true);
  tab.InvalidateLayout();
  views::test::RunScheduledLayout(&tab);
  EXPECT_FALSE(tab.close_button_for_test()->GetVisible());
}

TEST_F(BraveTabTest, CanCloseTabViaMiddleButtonClick) {
  testing::NiceMock<MockTabSlotController> tab_slot_controller;
  auto tab =
      std::make_unique<BraveTab>(tabs::TabHandle(2), &tab_slot_controller);
  tab_slot_controller.set_active_tab(tab.get());

  // Create a widget to host the tab
  auto widget =
      CreateTestWidget(views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET);
  widget->SetContentsView(std::move(tab));
  auto* tab_ptr = static_cast<BraveTab*>(widget->GetContentsView());
  tab_ptr->SetBoundsRect({0, 0, 100, 50});
  views::test::RunScheduledLayout(tab_ptr);

  // Default should be enabled (true)
  EXPECT_TRUE(tab_slot_controller.CanCloseTabViaMiddleButtonClick());

  // Simulate middle mouse button click - should close the tab
  ui::MouseEvent press_event(ui::EventType::kMousePressed, gfx::Point(50, 25),
                             gfx::Point(), base::TimeTicks(),
                             ui::EF_MIDDLE_MOUSE_BUTTON, 0);
  ui::MouseEvent release_event(
      ui::EventType::kMouseReleased, gfx::Point(50, 25), gfx::Point(),
      base::TimeTicks(), ui::EF_MIDDLE_MOUSE_BUTTON, 0);
  testing::Mock::VerifyAndClearExpectations(&tab_slot_controller);
  EXPECT_CALL(tab_slot_controller,
              CloseTab(tab_ptr, CloseTabSource::kFromMouse))
      .Times(1);
  tab_ptr->OnMousePressed(press_event);
  tab_ptr->OnMouseReleased(release_event);

  // Test disabling middle click to close
  tab_slot_controller.set_can_close_tab_via_middle_button_click(false);
  EXPECT_FALSE(tab_slot_controller.CanCloseTabViaMiddleButtonClick());

  // Simulate middle mouse button click - should NOT close the tab
  testing::Mock::VerifyAndClearExpectations(&tab_slot_controller);
  EXPECT_CALL(tab_slot_controller,
              CloseTab(tab_ptr, CloseTabSource::kFromMouse))
      .Times(0);
  tab_ptr->OnMousePressed(press_event);
  tab_ptr->OnMouseReleased(release_event);

  // Test re-enabling middle click to close
  tab_slot_controller.set_can_close_tab_via_middle_button_click(true);
  EXPECT_TRUE(tab_slot_controller.CanCloseTabViaMiddleButtonClick());

  // Simulate middle mouse button click - should close the tab again
  testing::Mock::VerifyAndClearExpectations(&tab_slot_controller);
  EXPECT_CALL(tab_slot_controller,
              CloseTab(tab_ptr, CloseTabSource::kFromMouse))
      .Times(1);
  tab_ptr->OnMousePressed(press_event);
  tab_ptr->OnMouseReleased(release_event);
}
