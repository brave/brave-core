// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/tabs/brave_tab.h"

#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip_layout_helper.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "brave/components/tabs/public/tree_tab_node_id.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "chrome/browser/ui/browser_window/test/mock_browser_window_interface.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/tabs/fake_tab_slot_controller.h"
#include "chrome/browser/ui/views/tabs/tab/tab_close_button.h"
#include "chrome/browser/ui/views/tabs/tab_style_views.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/tabs/public/mock_tab_interface.h"
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
  MOCK_METHOD(bool, IsVerticalTabsFloating, (), (const, override));
  MOCK_METHOD(bool,
              IsVerticalTabsAnimatingButNotFinalState,
              (),
              (const, override));
  MOCK_METHOD(BrowserWindowInterface*,
              GetBrowserWindowInterface,
              (),
              (override));

  MOCK_METHOD(const tabs::TreeTabNode*,
              GetTreeTabNode,
              (const tree_tab::TreeTabNodeId& id),
              (const, override));
};

class MockTabInterfaceWithWeakPtr : public tabs::MockTabInterface {
 public:
  using tabs::MockTabInterface::MockTabInterface;
  ~MockTabInterfaceWithWeakPtr() override = default;

  base::WeakPtr<tabs::TabInterface> GetWeakPtr() override {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  base::WeakPtrFactory<MockTabInterfaceWithWeakPtr> weak_ptr_factory_{this};
};

class BraveTabTest : public ChromeViewsTestBase {
 public:
  BraveTabTest() = default;
  ~BraveTabTest() override = default;

  void LayoutAndCheckBorder(BraveTab* tab, const gfx::Rect& bounds) {
    tab->SetBoundsRect(bounds);
    views::test::RunScheduledLayout(tab);

    auto insets = tab->tab_style()->GetContentsInsets();
    int left_inset = insets.left();
    left_inset += BraveTab::kExtraLeftPadding;
    EXPECT_EQ(left_inset, tab->GetInsets().left());
  }

  void TestIconVisibilityAcrossWidths(
      MockTabSlotController& tab_slot_controller,
      int tab_handle_id,
      bool is_pinned,
      bool expect_centered,
      const std::string& description_prefix) {
    constexpr int kMaxWidth = 240;  // Typical max width for vertical tabs
    constexpr int kMinWidth = tabs::kVerticalTabMinWidth;  // 32px
    constexpr int kStep = 20;

    auto widget =
        CreateTestWidget(views::Widget::InitParams::CLIENT_OWNS_WIDGET);
    auto tab = std::make_unique<BraveTab>(tabs::TabHandle(tab_handle_id),
                                          &tab_slot_controller);
    if (is_pinned) {
      tab_slot_controller.set_active_tab(nullptr);
    } else {
      tab_slot_controller.set_active_tab(tab.get());
    }
    widget->SetContentsView(std::move(tab));
    auto* tab_ptr = static_cast<BraveTab*>(widget->GetContentsView());

    if (is_pinned) {
      tabs::TabData data;
      data.pinned = true;
      tab_ptr->SetData(std::move(data));
    }

    for (int width = kMaxWidth; width > kMinWidth; width -= kStep) {
      tab_ptr->SetBoundsRect({0, 0, width, 50});
      tab_ptr->UpdateIconVisibility();

      EXPECT_TRUE(tab_ptr->showing_icon())
          << description_prefix << " icon should be visible at width " << width;
      EXPECT_EQ(expect_centered, tab_ptr->center_icon_for_test())
          << description_prefix << " icon centering should be "
          << (expect_centered ? "enabled" : "disabled") << " at width "
          << width;
    }
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

// Tests that a pinned vertical tab's icon is centered when floating from
// "completely hidden" collapsed mode. This is the regression fix: before the
// fix, the floating check ran first and set center_icon_ = false, even though
// the icon should remain centered in this mode.
//
// The center_icon_ = true block only fires when:
//   (showing_icon_ || showing_alert_indicator_) && !ShouldRenderAsNormalTab()
// Tab widths are derived from GetTabSizeInfo().pinned_tab_width and
// kPinnedTabExtraWidthToRenderAsNormal (30, from tab.cc) to straddle the exact
// boundary of ShouldRenderAsNormalTab().
//
// See: UpdateIconVisibility() in brave_tab.cc
TEST_F(BraveTabTest,
       PinnedTabIconCenteredWhenFloatingFromCompletelyHiddenMode) {
  // kBraveVerticalTabHideCompletely is FEATURE_ENABLED_BY_DEFAULT, so
  // kVerticalTabsHideCompletelyWhenCollapsed is already registered.
  TestingProfile profile;
  profile.GetPrefs()->SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  profile.GetPrefs()->SetBoolean(
      brave_tabs::kVerticalTabsHideCompletelyWhenCollapsed, true);

  testing::NiceMock<MockBrowserWindowInterface> mock_browser_window;
  EXPECT_CALL(mock_browser_window, GetProfile())
      .WillRepeatedly(testing::Return(&profile));
  EXPECT_CALL(testing::Const(mock_browser_window), GetProfile())
      .WillRepeatedly(testing::Return(&profile));
  EXPECT_CALL(mock_browser_window, GetType())
      .WillRepeatedly(testing::Return(BrowserWindowInterface::TYPE_NORMAL));

  testing::NiceMock<MockTabSlotController> tab_slot_controller;
  EXPECT_CALL(tab_slot_controller, GetBrowserWindowInterface())
      .WillRepeatedly(testing::Return(&mock_browser_window));
  // Tab is floating (user hovered over the completely-hidden collapsed
  // sidebar).
  EXPECT_CALL(tab_slot_controller, IsVerticalTabsFloating())
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(tab_slot_controller, IsVerticalTabsAnimatingButNotFinalState())
      .WillRepeatedly(testing::Return(false));

  auto widget = CreateTestWidget(views::Widget::InitParams::CLIENT_OWNS_WIDGET);
  auto tab =
      std::make_unique<BraveTab>(tabs::TabHandle(1), &tab_slot_controller);

  BraveTab* tab_ptr = static_cast<BraveTab*>(tab.get());
  widget->SetContentsView(std::move(tab));

  // data().show_icon defaults to true, so Tab::UpdateIconVisibility() sets
  // showing_icon_ = true for this pinned tab.
  tabs::TabData data;
  data.pinned = true;
  tab_ptr->SetData(std::move(data));

  // Compute the exact boundary used by Tab::ShouldRenderAsNormalTab():
  //   width >= pinned_tab_width + kPinnedTabExtraWidthToRenderAsNormal
  // kPinnedTabExtraWidthToRenderAsNormal is defined as 30 in tab.cc.
  const int pinned_tab_width = tab_ptr->GetTabSizeInfo().pinned_tab_width;
  constexpr int kPinnedTabExtraWidthToRenderAsNormal = 30;
  const int render_as_normal_threshold =
      pinned_tab_width + kPinnedTabExtraWidthToRenderAsNormal;

  // One pixel below the threshold: ShouldRenderAsNormalTab() == false, so the
  // center_icon_ = true block fires and the completely-hidden early return
  // preserves it.
  tab_ptr->SetBoundsRect({0, 0, render_as_normal_threshold - 1, 50});

  ASSERT_FALSE(tab_ptr->ShouldRenderAsNormalTab())
      << "Precondition failed: width " << render_as_normal_threshold - 1
      << " should be below the render-as-normal threshold "
      << render_as_normal_threshold;

  tab_ptr->UpdateIconVisibility();

  // After the fix: center_icon_ is set to true before the early return for
  // completely-hidden floating, so the "floating → center_icon_ = false"
  // block is never reached.
  EXPECT_TRUE(tab_ptr->center_icon_for_test())
      << "Pinned tab icon should be centered when floating from "
         "completely-hidden mode";

  // Exactly at the threshold: ShouldRenderAsNormalTab() == true, so the
  // center_icon_ = true block is skipped and center_icon_ stays false.
  tab_ptr->SetBoundsRect({0, 0, render_as_normal_threshold, 50});

  ASSERT_TRUE(tab_ptr->ShouldRenderAsNormalTab())
      << "Precondition failed: width " << render_as_normal_threshold
      << " should be at or above the render-as-normal threshold";

  tab_ptr->UpdateIconVisibility();

  EXPECT_FALSE(tab_ptr->center_icon_for_test())
      << "center_icon_ should remain false when ShouldRenderAsNormalTab() "
         "is true, since the center_icon_ = true block is guarded by "
         "!ShouldRenderAsNormalTab()";
}

// Test that icon visibility is correctly set when vertical tabs are animating
// to collapsed state across various tab widths. This prevents icon flickering
// when the collapse animation occurs during the transition from max to min
// width.
// See: UpdateIconVisibility() in brave_tab.cc
TEST_F(BraveTabTest, IconVisibilityWhenVerticalTabsAnimating) {
  // Set up a profile with vertical tabs enabled
  TestingProfile profile;
  profile.GetPrefs()->SetBoolean(brave_tabs::kVerticalTabsEnabled, true);

  // Set up mock browser window interface
  testing::NiceMock<MockBrowserWindowInterface> mock_browser_window;
  // Mock both const and non-const overloads of GetProfile()
  EXPECT_CALL(mock_browser_window, GetProfile())
      .WillRepeatedly(testing::Return(&profile));
  EXPECT_CALL(testing::Const(mock_browser_window), GetProfile())
      .WillRepeatedly(testing::Return(&profile));
  EXPECT_CALL(mock_browser_window, GetType())
      .WillRepeatedly(testing::Return(BrowserWindowInterface::TYPE_NORMAL));

  // Set up mock tab slot controller
  testing::NiceMock<MockTabSlotController> tab_slot_controller;
  EXPECT_CALL(tab_slot_controller, GetBrowserWindowInterface())
      .WillRepeatedly(testing::Return(&mock_browser_window));
  EXPECT_CALL(tab_slot_controller, IsVerticalTabsAnimatingButNotFinalState())
      .WillRepeatedly(testing::Return(true));

  // Test unpinned tab: icon visible but not centered during animation
  TestIconVisibilityAcrossWidths(tab_slot_controller, /*tab_handle_id=*/1,
                                 /*is_pinned=*/false,
                                 /*expect_centered=*/false, "Unpinned tab");

  // Test pinned tab during floating animation: icon visible but not centered
  EXPECT_CALL(tab_slot_controller, IsVerticalTabsFloating())
      .WillRepeatedly(testing::Return(true));
  TestIconVisibilityAcrossWidths(tab_slot_controller, /*tab_handle_id=*/2,
                                 /*is_pinned=*/true,
                                 /*expect_centered=*/false,
                                 "Pinned tab (floating)");

  // Test pinned tab during non-floating animation: icon visible and centered
  EXPECT_CALL(tab_slot_controller, IsVerticalTabsFloating())
      .WillRepeatedly(testing::Return(false));
  TestIconVisibilityAcrossWidths(tab_slot_controller, /*tab_handle_id=*/3,
                                 /*is_pinned=*/true,
                                 /*expect_centered=*/true,
                                 "Pinned tab (non-floating)");
}

class BraveTabTestWithTreeTab : public BraveTabTest {
 public:
  BraveTabTestWithTreeTab() {
    feature_list_.InitAndEnableFeature(tabs::kBraveTreeTab);
  }
  ~BraveTabTestWithTreeTab() override = default;

 private:
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(BraveTabTestWithTreeTab, TreeToggleButtonVisibleInsteadOfCloseButton) {
  testing::NiceMock<MockTabSlotController> tab_slot_controller;
  auto node_id = tree_tab::TreeTabNodeId::GenerateNew();
  tabs::TreeTabNodeTabCollection collection(
      node_id, std::make_unique<MockTabInterfaceWithWeakPtr>(),
      base::DoNothing(), base::DoNothing());
  ASSERT_EQ(collection.node().height(), 0);

  // When tree tab has descendants, the tree toggle button should be visible.
  EXPECT_CALL(tab_slot_controller, GetTreeTabNode(testing::_))
      .WillRepeatedly(testing::Return(&collection.node()));

  BraveTab tab(tabs::TabHandle(1), &tab_slot_controller);
  tab.set_tree_tab_node(node_id);
  tab_slot_controller.set_active_tab(&tab);
  ASSERT_TRUE(tab.GetTreeTabNode());
  ASSERT_EQ(tab.GetTreeTabNode()->id(), node_id);
  ASSERT_EQ(tab.GetTreeTabNode()->height(), 0);

  // Tab has no tree_tab_node (FakeTabSlotController returns empty node and
  // height 0), so the tree toggle button should be hidden.
  tab.SetBoundsRect({0, 0, 200, 50});
  views::test::RunScheduledLayout(&tab);

  // Close button should remain visible when tree toggle is hidden.
  ASSERT_TRUE(tab.showing_close_button_for_test());
  EXPECT_FALSE(tab.tree_toggle_button_->GetVisible());
  EXPECT_TRUE(tab.close_button_for_test()->GetVisible());

  // Now tree tab has descendants, so the tree toggle button should be visible.
  collection.node().set_height_for_test(100);
  tab.UpdateIconVisibility();
  tab.LayoutTreeToggleButton();

  ASSERT_TRUE(tab.showing_close_button_for_test());
  EXPECT_TRUE(tab.tree_toggle_button_->GetVisible());
  EXPECT_FALSE(tab.close_button_for_test()->GetVisible());
}

TEST_F(BraveTabTestWithTreeTab,
       TreeToggleButtonAlwaysVisibleWhenCollapsedAndHasDescendants) {
  testing::NiceMock<MockTabSlotController> tab_slot_controller;
  auto node_id = tree_tab::TreeTabNodeId::GenerateNew();
  tabs::TreeTabNodeTabCollection collection(
      node_id, std::make_unique<MockTabInterfaceWithWeakPtr>(),
      base::DoNothing(), base::DoNothing());
  ASSERT_EQ(collection.node().height(), 0);

  // When tree tab has descendants, the tree toggle button should be visible.
  EXPECT_CALL(tab_slot_controller, GetTreeTabNode(testing::_))
      .WillRepeatedly(testing::Return(&collection.node()));

  BraveTab tab(tabs::TabHandle(1), &tab_slot_controller);
  tab.set_tree_tab_node(node_id);
  ASSERT_TRUE(tab.GetTreeTabNode());
  ASSERT_EQ(tab.GetTreeTabNode()->id(), node_id);
  ASSERT_EQ(tab.GetTreeTabNode()->height(), 0);
  ASSERT_FALSE(tab.GetTreeTabNode()->collapsed());

  // Set narrow width so that show_close_button_ = false.
  tab.SetBoundsRect({0, 0, 10, 50});
  views::test::RunScheduledLayout(&tab);
  ASSERT_FALSE(tab.IsActive());
  ASSERT_FALSE(tab.showing_close_button_for_test());

  // When there's no descendants, the tree toggle button should be hidden.
  EXPECT_FALSE(tab.tree_toggle_button_->GetVisible());

  // Now tree tab has descendants. But not collapsed. So the button is still
  // hidden.
  collection.node().set_height_for_test(100);
  tab.UpdateIconVisibility();
  tab.LayoutTreeToggleButton();
  ASSERT_FALSE(tab.showing_close_button_for_test());
  EXPECT_FALSE(tab.tree_toggle_button_->GetVisible());

  // Now tree tab is collapsed. So the button should be visible.
  collection.node().set_collapsed(true);
  tab.UpdateIconVisibility();
  tab.LayoutTreeToggleButton();
  ASSERT_FALSE(tab.showing_close_button_for_test());
  EXPECT_TRUE(tab.tree_toggle_button_->GetVisible());
}
