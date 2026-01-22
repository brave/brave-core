// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/tabs/brave_tab_strip_layout_helper.h"

#include <optional>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/ui/tabs/tab_types.h"
#include "chrome/browser/ui/views/tabs/tab_layout_state.h"
#include "chrome/browser/ui/views/tabs/tab_strip_layout_types.h"
#include "chrome/browser/ui/views/tabs/tab_width_constraints.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/rect.h"

namespace tabs {

namespace {

// Helper function to create TabWidthConstraints for testing
TabWidthConstraints MakeTabConstraints(TabPinned pinned,
                                       TabOpen open = TabOpen::kOpen,
                                       TabActive active = TabActive::kInactive,
                                       bool in_group = false) {
  TabSizeInfo size_info;
  size_info.pinned_tab_width = kVerticalTabMinWidth;
  size_info.min_active_width = 56;
  size_info.min_inactive_width = 32;
  size_info.standard_width = 256;

  TabLayoutState state(open, pinned, active, std::nullopt);
  TabWidthConstraints constraints(state, size_info);
  constraints.set_is_tab_in_group(in_group);
  return constraints;
}

}  // namespace

// Tests for CalculatePinnedTabsBoundsInGrid

TEST(BraveTabStripLayoutHelperUnitTest,
     CalculatePinnedTabsBoundsInGrid_FloatingMode_DoesNothing) {
  std::vector<TabWidthConstraints> tabs;
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));

  std::vector<gfx::Rect> result;
  CalculatePinnedTabsBoundsInGrid(tabs, 200, &result);

  // Two pinned tabs should be laid out in grid
  EXPECT_EQ(2u, result.size());
}

TEST(BraveTabStripLayoutHelperUnitTest,
     CalculatePinnedTabsBoundsInGrid_ShouldCalculateOnlyPinnedTabs) {
  std::vector<TabWidthConstraints> tabs;

  // Should only lay out the 2 pinned tabs
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));
  tabs.push_back(MakeTabConstraints(TabPinned::kUnpinned));
  tabs.push_back(MakeTabConstraints(TabPinned::kUnpinned));

  std::vector<gfx::Rect> result;
  CalculatePinnedTabsBoundsInGrid(tabs, 200, &result);

  EXPECT_EQ(2u, result.size());
}

TEST(BraveTabStripLayoutHelperUnitTest,
     CalculatePinnedTabsBoundsInGrid_SinglePinnedTab) {
  std::vector<TabWidthConstraints> tabs;
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));

  std::vector<gfx::Rect> result;
  constexpr int kAvailableWidth = 100;
  CalculatePinnedTabsBoundsInGrid(tabs, kAvailableWidth, &result);

  ASSERT_EQ(1u, result.size());

  // Check the first tab's bounds
  EXPECT_EQ(kMarginForVerticalTabContainers, result[0].x());
  EXPECT_EQ(kMarginForVerticalTabContainers, result[0].y());
  EXPECT_EQ(kVerticalTabHeight, result[0].height());
  EXPECT_EQ(kAvailableWidth - 2 * kMarginForVerticalTabContainers,
            result[0].width());
}

TEST(BraveTabStripLayoutHelperUnitTest,
     CalculatePinnedTabsBoundsInGrid_TwoPinnedTabs_FitInOneRow) {
  std::vector<TabWidthConstraints> tabs;
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));

  // Enough width that allows both tabs to fit in one row
  std::vector<gfx::Rect> result;
  constexpr int kAvailableWidth = 200;
  CalculatePinnedTabsBoundsInGrid(tabs, kAvailableWidth, &result);
  ASSERT_EQ(2u, result.size());

  // Both tabs should be in the same row (y-coordinate should be the same)
  EXPECT_EQ(result[0].y(), result[1].y());

  // Second tab should be to the right of first tab
  EXPECT_GT(result[1].x(), result[0].right());

  // Second tab's right should be the edge of available width minus margin
  EXPECT_EQ(kAvailableWidth - kMarginForVerticalTabContainers,
            result[1].right());

  // All tabs should have the same height
  EXPECT_EQ(result[0].height(), result[1].height());
}

TEST(BraveTabStripLayoutHelperUnitTest,
     CalculatePinnedTabsBoundsInGrid_ThreePinnedTabs_FitInOneRow) {
  std::vector<TabWidthConstraints> tabs;
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));

  // Width that allows all three tabs to fit in one row
  std::vector<gfx::Rect> result;
  constexpr int kAvailableWidth = 200;
  CalculatePinnedTabsBoundsInGrid(tabs, kAvailableWidth, &result);

  ASSERT_EQ(3u, result.size());

  // All tabs should be in the same row
  EXPECT_EQ(result[0].y(), result[1].y());
  EXPECT_EQ(result[1].y(), result[2].y());

  // Tabs should be laid out left to right
  EXPECT_LT(result[0].right(), result[1].x());
  EXPECT_LT(result[1].right(), result[2].x());

  // The last tab's right should be the edge of available width minus margin
  EXPECT_EQ(kAvailableWidth - kMarginForVerticalTabContainers,
            result[2].right() + 1);
}

TEST(BraveTabStripLayoutHelperUnitTest,
     CalculatePinnedTabsBoundsInGrid_MultiplePinnedTabs_WrapToNewRow) {
  std::vector<TabWidthConstraints> tabs;
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));

  // Narrow width that forces wrapping - the width is set so that only 2 tabs
  // fit per row
  std::vector<gfx::Rect> result;
  CalculatePinnedTabsBoundsInGrid(tabs, 100, &result);

  ASSERT_EQ(4u, result.size());

  // First two tabs should be in the first row
  EXPECT_EQ(result[0].y(), result[1].y());

  // Third tab should be in a new row (different y-coordinate)
  EXPECT_GT(result[2].y(), result[0].bottom());

  // Fourth tab should be in the same row as third
  EXPECT_EQ(result[2].y(), result[3].y());

  // Check spacing between rows
  EXPECT_EQ(result[2].y(), result[0].bottom() + kVerticalTabsSpacing);
}

TEST(BraveTabStripLayoutHelperUnitTest,
     CalculatePinnedTabsBoundsInGrid_ExtraWidthDistribution) {
  std::vector<TabWidthConstraints> tabs;
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));

  // Width that creates extra pixels to distribute.
  // When there are three tabs, and width is 150,
  // available width would be 150 - kVeriticalTabMargins(4)*2 = 142
  // And we need to take away spacing between tabs (2 * 4) = 8
  // So effective available width = 134
  // Then all three tabs should be based on 134 / 3 = 44 pixels each,
  // with 2 extra pixels to distribute, so first two tabs should be 45 pixels
  std::vector<gfx::Rect> result;
  CalculatePinnedTabsBoundsInGrid(tabs, 150, &result);

  ASSERT_EQ(3u, result.size());

  // First two tabs should be 1 pixel wider than the third
  EXPECT_EQ(result[0].width(), result[1].width());
  EXPECT_EQ(45, result[0].width());
  EXPECT_EQ(result[0].width(), result[2].width() + 1);
}

TEST(BraveTabStripLayoutHelperUnitTest,
     CalculatePinnedTabsBoundsInGrid_ClosedTabsDoNotTakeSpace) {
  std::vector<TabWidthConstraints> tabs;
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned, TabOpen::kOpen));
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned, TabOpen::kClosed));
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned, TabOpen::kOpen));

  std::vector<gfx::Rect> result;
  CalculatePinnedTabsBoundsInGrid(tabs, 200, &result);

  ASSERT_EQ(3u, result.size());

  // All tabs get bounds, but closed tab shouldn't affect layout of next tab
  EXPECT_LT(result[0].right(), result[1].x());
  EXPECT_EQ(result[1].x(), result[2].x());
}

TEST(BraveTabStripLayoutHelperUnitTest,
     CalculatePinnedTabsBoundsInGrid_NoWidthProvided) {
  std::vector<TabWidthConstraints> tabs;
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));

  std::vector<gfx::Rect> result;
  CalculatePinnedTabsBoundsInGrid(tabs, std::nullopt, &result);

  ASSERT_EQ(2u, result.size());

  // Without width, should use minimum width
  EXPECT_EQ(kVerticalTabMinWidth, result[0].width());
  EXPECT_EQ(kVerticalTabMinWidth, result[1].width());
}

TEST(BraveTabStripLayoutHelperUnitTest,
     CalculatePinnedTabsBoundsInGrid_MarginsApplied) {
  std::vector<TabWidthConstraints> tabs;
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));

  std::vector<gfx::Rect> result;
  CalculatePinnedTabsBoundsInGrid(tabs, 100, &result);

  ASSERT_EQ(1u, result.size());

  // First tab should start at the margin
  EXPECT_EQ(kMarginForVerticalTabContainers, result[0].x());
  EXPECT_EQ(kMarginForVerticalTabContainers, result[0].y());
}

TEST(BraveTabStripLayoutHelperUnitTest,
     CalculatePinnedTabsBoundsInGrid_SpacingBetweenTabs) {
  std::vector<TabWidthConstraints> tabs;
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));

  std::vector<gfx::Rect> result;
  CalculatePinnedTabsBoundsInGrid(tabs, 200, &result);

  ASSERT_EQ(2u, result.size());

  // Check spacing between tabs in the same row
  EXPECT_EQ(kVerticalTabsSpacing, result[1].x() - result[0].right());
}

TEST(BraveTabStripLayoutHelperUnitTest,
     CalculateVerticalTabBounds_PinnedTabsNotInGrid_LayOutVertically) {
  std::vector<TabWidthConstraints> tabs;
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));
  tabs.push_back(MakeTabConstraints(TabPinned::kUnpinned));
  tabs.push_back(MakeTabConstraints(TabPinned::kUnpinned));

  constexpr int kAvailableWidth = 200;
  auto [bounds, _] = CalculateVerticalTabBounds(
      tabs, kAvailableWidth, /*should_layout_pinned_tabs_in_grid=*/false);

  ASSERT_EQ(5u, bounds.size());

  // When should_layout_pinned_tabs_in_grid is false, pinned tabs should be
  // laid out vertically (stacked on top of each other) not in a grid
  // All tabs should have the same x-coordinate (left edge)
  EXPECT_EQ(bounds[0].x(), bounds[1].x());
  EXPECT_EQ(bounds[1].x(), bounds[2].x());
  EXPECT_EQ(bounds[2].x(), bounds[3].x());
  EXPECT_EQ(bounds[3].x(), bounds[4].x());

  // Pinned tabs should be stacked vertically with proper spacing
  EXPECT_EQ(kMarginForVerticalTabContainers, bounds[0].y());
  EXPECT_EQ(bounds[0].bottom() + kVerticalTabsSpacing, bounds[1].y());
  EXPECT_EQ(bounds[1].bottom() + kVerticalTabsSpacing, bounds[2].y());

  // Unpinned tabs should continue the vertical layout after separator.
  EXPECT_EQ(bounds[2].bottom() + kVerticalTabsSpacing +
                kPinnedUnpinnedSeparatorHeight + kVerticalTabsSpacing,
            bounds[3].y());
  EXPECT_EQ(bounds[3].bottom() + kVerticalTabsSpacing, bounds[4].y());

  // All tabs should have the same width
  EXPECT_EQ(bounds[0].width(), bounds[1].width());
  EXPECT_EQ(bounds[1].width(), bounds[2].width());
  EXPECT_EQ(bounds[2].width(), bounds[3].width());
  EXPECT_EQ(bounds[3].width(), bounds[4].width());
}

TEST(BraveTabStripLayoutHelperUnitTest,
     CalculateVerticalTabBounds_PinnedTabsInGrid_LayOutInGrid) {
  std::vector<TabWidthConstraints> tabs;
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));
  tabs.push_back(MakeTabConstraints(TabPinned::kPinned));

  constexpr int kAvailableWidth = 200;
  auto [bounds, _] = CalculateVerticalTabBounds(
      tabs, kAvailableWidth, /*should_layout_pinned_tabs_in_grid=*/true);

  ASSERT_EQ(3u, bounds.size());

  // When should_layout_pinned_tabs_in_grid is true, pinned tabs should be
  // laid out in a grid (side by side when width allows)
  // First three tabs are pinned and should be in grid layout
  // With width of 200, all 3 pinned tabs should fit in one row
  EXPECT_EQ(bounds[0].y(), bounds[1].y());
  EXPECT_EQ(bounds[1].y(), bounds[2].y());

  // Tabs should be laid out left to right in the same row
  EXPECT_LT(bounds[0].right(), bounds[1].x());
  EXPECT_LT(bounds[1].right(), bounds[2].x());
}

}  // namespace tabs
