// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/tabs/tab_style_views.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/numerics/safe_conversions.h"
#include "brave/browser/ui/views/tabs/brave_tab.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip_layout_helper.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/tabs/fake_tab_slot_controller.h"
#include "chrome/browser/ui/views/tabs/tab/tab_icon.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "components/tabs/public/tab_interface.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkRRect.h"
#include "ui/events/event.h"
#include "ui/gfx/favicon_size.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/views/test/views_test_utils.h"
#include "ui/views/widget/widget.h"

namespace {

// Test controller that allows overriding GetTabsInSplit for testing.
class TestTabSlotController : public FakeTabSlotController {
 public:
  TestTabSlotController() = default;
  ~TestTabSlotController() override = default;

  void SetTabsInSplit(std::vector<Tab*> tabs) {
    split_tabs_.assign(tabs.begin(), tabs.end());
  }
  void ClearSplitTabs() { split_tabs_.clear(); }

  // FakeTabSlotController:
  std::vector<Tab*> GetTabsInSplit(const Tab* tab) override {
    return {split_tabs_.begin(), split_tabs_.end()};
  }
  Tab* GetAdjacentTab(const Tab* tab, int offset) override {
    const auto it = std::ranges::find(split_tabs_, tab);
    if (it == split_tabs_.end()) {
      return nullptr;
    }
    const auto index = std::distance(split_tabs_.begin(), it) + offset;
    if (index < 0 || index >= static_cast<int>(split_tabs_.size())) {
      return nullptr;
    }
    return split_tabs_[index];
  }

 private:
  std::vector<raw_ptr<Tab>> split_tabs_;
};

}  // namespace

// Note: These tests verify IsHovering() behavior indirectly through
// CalculateTargetColors() because IsHovering() is not part of the public
// TabStyleViews interface. We test the observable effect of IsHovering() on
// tab background colors rather than calling the method directly.
class BraveTabStyleIsHoveringTest : public ChromeViewsTestBase {
 public:
  BraveTabStyleIsHoveringTest() = default;
  ~BraveTabStyleIsHoveringTest() override = default;

  void SetUp() override {
    ChromeViewsTestBase::SetUp();
    controller_ = std::make_unique<TestTabSlotController>();
    widget_ = CreateTestWidget(views::Widget::InitParams::CLIENT_OWNS_WIDGET);

    // Create a container view for tabs.
    auto container = std::make_unique<views::View>();
    container_ = container.get();
    widget_->SetContentsView(std::move(container));
  }

  void TearDown() override {
    container_ = nullptr;
    controller_->ClearSplitTabs();
    widget_.reset();
    ChromeViewsTestBase::TearDown();
  }

  // Creates a tab and adds it to the container for testing.
  Tab* CreateTab(int id) {
    auto tab =
        std::make_unique<BraveTab>(tabs::TabHandle(id), controller_.get());
    return container_->AddChildView(std::move(tab));
  }

  // Ensures the tab has non-zero size so hover simulation works on Linux.
  // Tab::MaybeUpdateHoverStatus ignores hover when event.location().y() >=
  // height() - kHoverCardOverlap; with height 0, (0,0) fails. Call before
  // SimulateMouseEnter.
  void EnsureTabHasSizeForHover(Tab* tab) {
    const int tab_height = GetLayoutConstant(LayoutConstant::kTabHeight);
    tab->SetBounds(0, 0, 200, tab_height);
  }

  void SimulateMouseEnter(Tab* tab) {
    EnsureTabHasSizeForHover(tab);
    gfx::Point center = tab->GetLocalBounds().CenterPoint();
    ui::MouseEvent enter_event(ui::EventType::kMouseEntered, center, center,
                               base::TimeTicks(), 0, 0);
    tab->OnMouseEntered(enter_event);
  }

  void SimulateMouseExit(Tab* tab) {
    ui::MouseEvent exit_event(ui::EventType::kMouseExited, gfx::Point(),
                              gfx::Point(), base::TimeTicks(), 0, 0);
    tab->OnMouseExited(exit_event);
  }

 protected:
  std::unique_ptr<TestTabSlotController> controller_;
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<views::View> container_;
};

// Verifies that hovering a single tab changes its background color and
// un-hovering returns it to the original color.
TEST_F(BraveTabStyleIsHoveringTest, IsHovering_SingleTab) {
  Tab* tab = CreateTab(1);

  // Tab is not hovered by default.
  EXPECT_FALSE(tab->mouse_hovered());

  // Capture baseline (not hovered) color.
  TabStyle::TabColors not_hovered_colors =
      tab->tab_style_views()->CalculateTargetColors();
  SkColor not_hovered_color = not_hovered_colors.background_color;
  EXPECT_NE(not_hovered_color, gfx::kPlaceholderColor);

  SimulateMouseEnter(tab);

  EXPECT_TRUE(tab->mouse_hovered());
  EXPECT_TRUE(tab->IsHovering());

  // Verify hover state changes the color.
  TabStyle::TabColors hovered_colors =
      tab->tab_style_views()->CalculateTargetColors();
  SkColor hovered_color = hovered_colors.background_color;
  EXPECT_NE(hovered_color, not_hovered_color);

  SimulateMouseExit(tab);

  EXPECT_FALSE(tab->mouse_hovered());

  // After leaving, should return to original color.
  TabStyle::TabColors final_colors =
      tab->tab_style_views()->CalculateTargetColors();
  EXPECT_EQ(final_colors.background_color, not_hovered_color);
}

// Verifies that split tabs use transparent background when neither is hovered.
TEST_F(BraveTabStyleIsHoveringTest, IsHovering_SplitTabs_NeitherHovered) {
  Tab* tab1 = CreateTab(1);
  Tab* tab2 = CreateTab(2);

  // Set up split tabs.
  split_tabs::SplitTabId split_id = split_tabs::SplitTabId::GenerateNew();
  tab1->SetSplit(split_id);
  tab2->SetSplit(split_id);

  std::vector<Tab*> split_tabs = {tab1, tab2};
  controller_->SetTabsInSplit(split_tabs);

  EXPECT_FALSE(tab1->mouse_hovered());
  EXPECT_FALSE(tab2->mouse_hovered());
  EXPECT_FALSE(tab1->IsHovering());
  EXPECT_FALSE(tab2->IsHovering());

  // Neither tab should have hover background color (both use transparent for
  // non-hovered split tabs).
  TabStyle::TabColors colors1 =
      tab1->tab_style_views()->CalculateTargetColors();
  TabStyle::TabColors colors2 =
      tab2->tab_style_views()->CalculateTargetColors();
  EXPECT_EQ(colors1.background_color, SK_ColorTRANSPARENT);
  EXPECT_EQ(colors2.background_color, SK_ColorTRANSPARENT);
}

// Verifies that when hovering the second tab in a split, only that tab's
// color changes (Brave behavior - not both tabs like upstream).
TEST_F(BraveTabStyleIsHoveringTest, IsHovering_SplitTabs_SecondTabHovered) {
  Tab* tab1 = CreateTab(1);
  Tab* tab2 = CreateTab(2);

  // Set up split tabs.
  split_tabs::SplitTabId split_id = split_tabs::SplitTabId::GenerateNew();
  tab1->SetSplit(split_id);
  tab2->SetSplit(split_id);

  std::vector<Tab*> split_tabs = {tab1, tab2};
  controller_->SetTabsInSplit(split_tabs);

  // Capture baseline colors (neither hovered).
  TabStyle::TabColors baseline1 =
      tab1->tab_style_views()->CalculateTargetColors();
  TabStyle::TabColors baseline2 =
      tab2->tab_style_views()->CalculateTargetColors();

  SimulateMouseEnter(tab2);

  EXPECT_FALSE(tab1->mouse_hovered());
  EXPECT_TRUE(tab2->mouse_hovered());
  EXPECT_FALSE(tab1->IsHovering());
  EXPECT_TRUE(tab2->IsHovering());

  // Only tab2's color should change.
  TabStyle::TabColors colors1 =
      tab1->tab_style_views()->CalculateTargetColors();
  TabStyle::TabColors colors2 =
      tab2->tab_style_views()->CalculateTargetColors();

  EXPECT_EQ(colors1.background_color, baseline1.background_color);
  EXPECT_NE(colors2.background_color, baseline2.background_color);
}

// Verifies that when moving hover between split tabs, colors update correctly
// so only the currently hovered tab has the hover color.
TEST_F(BraveTabStyleIsHoveringTest, IsHovering_SplitTabs_HoverTransition) {
  Tab* tab1 = CreateTab(1);
  Tab* tab2 = CreateTab(2);

  split_tabs::SplitTabId split_id = split_tabs::SplitTabId::GenerateNew();
  tab1->SetSplit(split_id);
  tab2->SetSplit(split_id);

  std::vector<Tab*> split_tabs = {tab1, tab2};
  controller_->SetTabsInSplit(split_tabs);

  // Capture baseline colors (neither hovered).
  TabStyle::TabColors baseline1 =
      tab1->tab_style_views()->CalculateTargetColors();
  TabStyle::TabColors baseline2 =
      tab2->tab_style_views()->CalculateTargetColors();

  SimulateMouseEnter(tab1);

  TabStyle::TabColors colors1 =
      tab1->tab_style_views()->CalculateTargetColors();
  TabStyle::TabColors colors2 =
      tab2->tab_style_views()->CalculateTargetColors();

  // tab1 should change, tab2 should not.
  EXPECT_NE(colors1.background_color, baseline1.background_color);
  EXPECT_EQ(colors2.background_color, baseline2.background_color);

  // Move hover to second tab (first tab exits, second tab enters).
  SimulateMouseExit(tab1);
  SimulateMouseEnter(tab2);

  EXPECT_FALSE(tab1->mouse_hovered());
  EXPECT_TRUE(tab2->mouse_hovered());

  colors1 = tab1->tab_style_views()->CalculateTargetColors();
  colors2 = tab2->tab_style_views()->CalculateTargetColors();

  // tab1 should return to baseline, tab2 should change.
  EXPECT_EQ(colors1.background_color, baseline1.background_color);
  EXPECT_NE(colors2.background_color, baseline2.background_color);

  SimulateMouseExit(tab2);

  EXPECT_FALSE(tab1->mouse_hovered());
  EXPECT_FALSE(tab2->mouse_hovered());

  colors1 = tab1->tab_style_views()->CalculateTargetColors();
  colors2 = tab2->tab_style_views()->CalculateTargetColors();

  // Both should return to baselines.
  EXPECT_EQ(colors1.background_color, baseline1.background_color);
  EXPECT_EQ(colors2.background_color, baseline2.background_color);
}

class BraveTabStyleSplitTabTest : public BraveTabStyleIsHoveringTest {
 protected:
  SkPath GetShape(Tab* tab) {
    return tab->tab_style_views()->GetPath(
        TabStyle::PathType::kHighlight, /*scale=*/1.0f,
        {.render_units = TabStyle::RenderUnits::kDips});
  }

  gfx::RectF GetShapeBounds(Tab* tab) {
    return gfx::SkRectToRectF(GetShape(tab).getBounds());
  }

  // Sets up |tab1| and |tab2| as a split and lays them out next to each other
  // in the horizontal tab strip.
  void SetUpHorizontalSplitTabs(Tab* tab1, Tab* tab2, int width) {
    split_tabs::SplitTabId split_id = split_tabs::SplitTabId::GenerateNew();
    tab1->SetSplit(split_id);
    tab2->SetSplit(split_id);
    controller_->SetTabsInSplit({tab1, tab2});

    const int height = GetLayoutConstant(LayoutConstant::kTabStripHeight);
    tab1->SetBounds(0, 0, width, height);
    tab2->SetBounds(width - TabStyle::Get()->GetTabOverlap(), 0, width, height);
    views::test::RunScheduledLayout(tab1);
    views::test::RunScheduledLayout(tab2);
  }

  // Verifies that the shapes of the split tabs in the horizontal tab strip are
  // inset from the tile's bounding box and their corners are concentric with
  // the tile's corners.
  void ExpectHorizontalSplitTabsAreInsetInTile(Tab* tab1, Tab* tab2) {
    // Matches kPaddingForHorizontalTabInTile and the additional vertical
    // padding for split tabs in horizontal_tab_style_views.cc.
    constexpr int kHorizontalPadding = 4;
    const int vertical_inset = tabs::kHorizontalSplitViewTabVerticalSpacing -
                               tabs::kHorizontalTabGap +
                               tabs::GetHorizontalTabVerticalSpacing();

    const gfx::RectF shape1 = GetShapeBounds(tab1);
    EXPECT_EQ(tabs::kHorizontalTabInset + kHorizontalPadding, shape1.x());
    EXPECT_EQ(tab1->width() - tabs::kHorizontalTabInset, shape1.right());
    EXPECT_EQ(vertical_inset, shape1.y());
    EXPECT_EQ(tab1->height() - vertical_inset, shape1.bottom());

    const gfx::RectF shape2 = GetShapeBounds(tab2);
    EXPECT_EQ(tabs::kHorizontalTabInset, shape2.x());
    EXPECT_EQ(tab2->width() - tabs::kHorizontalTabInset - kHorizontalPadding,
              shape2.right());
    EXPECT_EQ(vertical_inset, shape2.y());
    EXPECT_EQ(tab2->height() - vertical_inset, shape2.bottom());

    const int tile_radius =
        tabs::GetSplitViewTileCornerRadius(/*is_vertical_tab=*/false);
    const SkVector expected_radii = SkVector::Make(
        tile_radius - kHorizontalPadding,
        tile_radius -
            (vertical_inset - tabs::kHorizontalSplitViewTileVerticalMargin));
    for (Tab* tab : {tab1, tab2}) {
      SkRRect rrect;
      ASSERT_TRUE(GetShape(tab).isRRect(&rrect));
      for (auto corner :
           {SkRRect::kUpperLeft_Corner, SkRRect::kUpperRight_Corner,
            SkRRect::kLowerRight_Corner, SkRRect::kLowerLeft_Corner}) {
        EXPECT_EQ(expected_radii, rrect.radii(corner));
      }
    }
  }
};

// Verifies that pinned tabs in a horizontal split view tile are inset so that
// the tile's bounding box doesn't touch them, that their corners are concentric
// with the tile's corners, and that their favicons stay centered in the inset
// shape.
TEST_F(BraveTabStyleSplitTabTest, PinnedSplitTabsAreInsetInTile) {
  if (!tabs::HorizontalTabsUpdateEnabled()) {
    GTEST_SKIP();
  }

  Tab* tab1 = CreateTab(1);
  Tab* tab2 = CreateTab(2);

  tabs::TabData data;
  data.pinned = true;
  tab1->SetDataForTesting(data);
  tab2->SetDataForTesting(data);

  SetUpHorizontalSplitTabs(tab1, tab2,
                           TabStyle::Get()->GetPinnedWidth(/*is_split=*/true));
  ExpectHorizontalSplitTabsAreInsetInTile(tab1, tab2);

  for (Tab* tab : {tab1, tab2}) {
    const TabIcon* icon = tab->GetTabIconForTesting();
    ASSERT_TRUE(icon->GetVisible());
    // The icon's bounds include insets around the favicon.
    const int favicon_x = icon->x() + icon->GetInsets().left();
    EXPECT_EQ(base::ClampRound(GetShapeBounds(tab).CenterPoint().x()),
              favicon_x + gfx::kFaviconSize / 2);
  }
}

// Verifies that unpinned tabs in a horizontal split view tile are inset so that
// the tile's bounding box doesn't touch them, and that their corners are
// concentric with the tile's corners.
TEST_F(BraveTabStyleSplitTabTest, UnpinnedSplitTabsAreInsetInTile) {
  if (!tabs::HorizontalTabsUpdateEnabled()) {
    GTEST_SKIP();
  }

  Tab* tab1 = CreateTab(1);
  Tab* tab2 = CreateTab(2);
  SetUpHorizontalSplitTabs(
      tab1, tab2, TabStyle::Get()->GetStandardWidth(/*is_split=*/true));
  ExpectHorizontalSplitTabsAreInsetInTile(tab1, tab2);
}
