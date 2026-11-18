/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view.h"

#include "base/i18n/test/scoped_rtl_for_testing.h"
#include "brave/browser/ui/views/brave_layout_provider.h"
#include "components/split_tabs/split_tab_visual_data.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/layout/layout_provider.h"

namespace {

// The whole-contents radii the layout hands down. A value far from any real
// metric makes it obvious which corners the function left alone.
constexpr float kOuter = 100.f;

}  // namespace

// Befriended by BraveMultiContentsView so the tests can reach its private
// static without constructing a view.
class BraveMultiContentsViewTest : public testing::Test {
 protected:
  static float BorderRadius() {
    return views::LayoutProvider::Get()->GetCornerRadiusMetric(
        ExtendedShapeContextTokens::kRoundedCornersBorderRadius);
  }

  static gfx::RoundedCornersF SplitRadii(
      split_tabs::SplitTabLayout layout,
      bool is_first,
      bool is_last,
      const gfx::RoundedCornersF& corner_radii = gfx::RoundedCornersF(kOuter)) {
    return BraveMultiContentsView::GetSplitContentsCornerRadii(
        corner_radii, layout, is_first, is_last);
  }

  BraveLayoutProvider layout_provider_;
};

// The layout passes empty radii when the contents should not be rounded at
// all; splitting must not introduce any.
TEST_F(BraveMultiContentsViewTest, SplitContentsCornerRadiiEmptyStaysEmpty) {
  EXPECT_EQ(
      gfx::RoundedCornersF(),
      SplitRadii(split_tabs::SplitTabLayout::kSideBySide,
                 /*is_first=*/true, /*is_last=*/false, gfx::RoundedCornersF()));
}

// kSideBySide: the divider is vertical, so only the inner vertical edge is
// re-rounded and the pane's outer edge keeps the window-facing radius.
TEST_F(BraveMultiContentsViewTest,
       SplitContentsCornerRadiiSideBySideFirstPane) {
  const float border = BorderRadius();

  // The first pane's right edge meets the divider.
  EXPECT_EQ(gfx::RoundedCornersF(kOuter, border, border, kOuter),
            SplitRadii(split_tabs::SplitTabLayout::kSideBySide,
                       /*is_first=*/true, /*is_last=*/false));
}

TEST_F(BraveMultiContentsViewTest, SplitContentsCornerRadiiSideBySideLastPane) {
  const float border = BorderRadius();

  // The last pane's left edge meets the divider.
  EXPECT_EQ(gfx::RoundedCornersF(border, kOuter, kOuter, border),
            SplitRadii(split_tabs::SplitTabLayout::kSideBySide,
                       /*is_first=*/false, /*is_last=*/true));
}

// In RTL the first pane renders on the right, so the visual sides swap while
// the caller's is_first/is_last stay in tab order.
TEST_F(BraveMultiContentsViewTest, SplitContentsCornerRadiiSideBySideInRtl) {
  base::i18n::ScopedRTLForTesting scoped_rtl(true);
  const float border = BorderRadius();

  // The first pane is on the right, so its *left* edge meets the divider -
  // the mirror image of the LTR case.
  EXPECT_EQ(gfx::RoundedCornersF(border, kOuter, kOuter, border),
            SplitRadii(split_tabs::SplitTabLayout::kSideBySide,
                       /*is_first=*/true, /*is_last=*/false));
  EXPECT_EQ(gfx::RoundedCornersF(kOuter, border, border, kOuter),
            SplitRadii(split_tabs::SplitTabLayout::kSideBySide,
                       /*is_first=*/false, /*is_last=*/true));
}

// kStacked: the divider is horizontal, so the inner horizontal edge is
// re-rounded instead. RTL does not reorder a vertical stack.
TEST_F(BraveMultiContentsViewTest, SplitContentsCornerRadiiStackedFirstPane) {
  const float border = BorderRadius();

  // The first pane is on top; its bottom edge meets the divider.
  EXPECT_EQ(gfx::RoundedCornersF(kOuter, kOuter, border, border),
            SplitRadii(split_tabs::SplitTabLayout::kStacked,
                       /*is_first=*/true, /*is_last=*/false));
}

TEST_F(BraveMultiContentsViewTest, SplitContentsCornerRadiiStackedLastPane) {
  const float border = BorderRadius();

  // The last pane is at the bottom; its top edge meets the divider.
  EXPECT_EQ(gfx::RoundedCornersF(border, border, kOuter, kOuter),
            SplitRadii(split_tabs::SplitTabLayout::kStacked,
                       /*is_first=*/false, /*is_last=*/true));
}
