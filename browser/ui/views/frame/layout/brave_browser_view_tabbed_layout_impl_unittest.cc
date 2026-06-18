/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/layout/brave_browser_view_tabbed_layout_impl.h"

#include <memory>

#include "chrome/browser/ui/views/frame/layout/browser_view_layout_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/rect.h"

namespace {

// Minimal fake for BrowserViewLayoutDelegate methods that are not exercised by
// CalculateSeparatorInfo() on the "no top UI" path (Brave returns early).
class FakeBrowserViewLayoutDelegate : public BrowserViewLayoutDelegate {
 public:
  FakeBrowserViewLayoutDelegate() = default;
  ~FakeBrowserViewLayoutDelegate() override = default;

  bool ShouldDrawTabStrip() const override { return false; }
  bool ShouldDrawVerticalTabStrip() const override { return false; }
  bool IsVerticalTabStripCollapsed() const override { return false; }
  bool ShouldDrawWebAppFrameToolbar() const override { return false; }
  bool GetUnframedModeEnabled() const override { return false; }
  BrowserLayoutParams GetBrowserLayoutParams(bool) const override {
    return BrowserLayoutParams();
  }
  WindowState GetBrowserWindowState() const override {
    return WindowState::kNormal;
  }
  views::LayoutAlignment GetWindowTitleAlignment() const override {
    return views::LayoutAlignment::kStart;
  }
  bool IsToolbarVisible() const override { return false; }
  bool IsBookmarkBarVisible() const override { return false; }
  bool IsInfobarVisible() const override { return false; }
  bool IsContentsSeparatorEnabled() const override { return false; }
  bool IsActiveTabSplit() const override { return false; }
  bool IsActiveTabAtLeadingWindowEdge() const override { return false; }
  const ImmersiveModeController* GetImmersiveModeController() const override {
    return nullptr;
  }
  ExclusiveAccessBubbleViews* GetExclusiveAccessBubble() const override {
    return nullptr;
  }
  bool IsTopControlsSlideBehaviorEnabled() const override { return false; }
  float GetTopControlsSlideBehaviorShownRatio() const override { return 0.0f; }
  gfx::NativeView GetHostViewForAnchoring() const override {
    return gfx::NativeView();
  }
  bool HasFindBarController() const override { return false; }
  void MoveWindowForFindBarIfNecessary() const override {}
  bool IsWindowControlsOverlayEnabled() const override { return false; }
  void UpdateWindowControlsOverlay(const gfx::Rect&) override {}
  bool ShouldLayoutTabStrip() const override { return false; }
  int GetExtraInfobarOffset() const override { return 0; }
  bool IsProjectsPanelVisible() const override { return false; }

  bool ShouldShowVerticalTabs() const override { return false; }
  bool ShouldShowWindowTitleForVerticalTabs() const override { return false; }
  bool IsVerticalTabOnRight() const override { return false; }
  bool ShouldUseBraveWebViewRoundedCornersForContents() const override {
    return false;
  }
  int GetRoundedCornersWebViewMargin() const override { return 0; }
  bool IsBookmarkBarOnByPref() const override { return false; }
  bool IsContentTypeSidePanelVisible() const override { return false; }
  bool IsFullscreenForBrowser() const override { return false; }
  bool IsFullscreenForTab() const override { return false; }
  bool IsFullscreen() const override { return false; }
};

// Only the two visibility hooks matter for the CalculateSeparatorInfo() "no
// top UI" early return; gmock makes the test spell that out with EXPECT_CALL.
class MockBrowserViewLayoutDelegate : public FakeBrowserViewLayoutDelegate {
 public:
  MOCK_METHOD(bool, IsToolbarVisible, (), (const, override));
  MOCK_METHOD(bool, IsBookmarkBarVisible, (), (const, override));
};

}  // namespace

// ---------------------------------------------------------------------------
// ComputeSidebarBounds
//
// All bounds are in stored (pre-paint-mirroring) coordinates;
// `sidebar_leading` selects the lowest-X edge, which renders on the visual
// left in LTR and the visual right in RTL.
// ---------------------------------------------------------------------------

TEST(BraveBrowserViewTabbedLayoutImplSidebarTest, ComputeSidebarBoundsOnRight) {
  // Browser width 1000, no vertical tab on the right side.
  // outer_right = 1000 (full right edge).
  gfx::Rect sidebar = BraveBrowserViewTabbedLayoutImpl::ComputeSidebarBounds(
      /*sidebar_leading=*/false, /*sidebar_width=*/200,
      /*outer_left=*/0, /*outer_right=*/1000,
      /*y=*/50, /*height=*/600);
  EXPECT_EQ(gfx::Rect(800, 50, 200, 600), sidebar);
}

TEST(BraveBrowserViewTabbedLayoutImplSidebarTest,
     ComputeSidebarBoundsOnRightWithVtabOnSameSide) {
  // Vertical tab on right, width 200 → outer_right = 1000 - 200 = 800.
  // Sidebar sits between contents and vertical tab.
  gfx::Rect sidebar = BraveBrowserViewTabbedLayoutImpl::ComputeSidebarBounds(
      /*sidebar_leading=*/false, /*sidebar_width=*/150,
      /*outer_left=*/0, /*outer_right=*/800,
      /*y=*/50, /*height=*/600);
  EXPECT_EQ(gfx::Rect(650, 50, 150, 600), sidebar);
}

TEST(BraveBrowserViewTabbedLayoutImplSidebarTest, ComputeSidebarBoundsOnLeft) {
  // Browser width 1000, no vertical tab on the left side.
  // outer_left = 0 (full left edge).
  gfx::Rect sidebar = BraveBrowserViewTabbedLayoutImpl::ComputeSidebarBounds(
      /*sidebar_leading=*/true, /*sidebar_width=*/200,
      /*outer_left=*/0, /*outer_right=*/1000,
      /*y=*/50, /*height=*/600);
  EXPECT_EQ(gfx::Rect(0, 50, 200, 600), sidebar);
}

TEST(BraveBrowserViewTabbedLayoutImplSidebarTest,
     ComputeSidebarBoundsOnLeftWithVtabOnSameSide) {
  // Vertical tab on left, width 200 → outer_left = 200.
  // Sidebar sits between vertical tab and contents.
  gfx::Rect sidebar = BraveBrowserViewTabbedLayoutImpl::ComputeSidebarBounds(
      /*sidebar_leading=*/true, /*sidebar_width=*/150,
      /*outer_left=*/200, /*outer_right=*/1000,
      /*y=*/50, /*height=*/600);
  EXPECT_EQ(gfx::Rect(200, 50, 150, 600), sidebar);
}

// ---------------------------------------------------------------------------
// ComputeAdjustedPanelBounds
//
// The function translates the upstream panel by a constant offset so its
// open/close slide plays out against the sidebar's inner edge. The upstream
// panel keeps full `target_width` and animates by varying its x via
// `visible_width` (0 → target); the function adds a constant shift, so it must
// preserve that x-driven slide and only land flush with the sidebar at full
// open. The "FullyOpen" cases pin the full-open geometry; the "Animating"
// cases lock in that a mid-animation panel still slides (is not flattened).
// `visual_client_area` is the upstream anchor rect, the same one upstream used
// to place the panel.
// ---------------------------------------------------------------------------

TEST(BraveBrowserViewTabbedLayoutImplSidebarTest,
     ComputeAdjustedPanelBoundsSidebarTrailingFullyOpen) {
  // Client 1000px wide. Trailing (right) sidebar at x=960, w=40 (flush, no VT).
  // Panel fully open: upstream x = client.right()-target = 1000-300 = 700.
  // Shift = sidebar.x()-client.right() = 960-1000 = -40 → x = 660, right = 960.
  gfx::Rect client(0, 0, 1000, 700);
  gfx::Rect sidebar(960, 50, 40, 600);
  gfx::Rect panel(700, 50, 300, 600);
  gfx::Rect adjusted =
      BraveBrowserViewTabbedLayoutImpl::ComputeAdjustedPanelBounds(
          /*sidebar_leading=*/false, sidebar, panel, client);
  EXPECT_EQ(gfx::Rect(660, 50, 300, 600), adjusted);
  EXPECT_EQ(sidebar.x(), adjusted.right());  // flush with sidebar's left edge
}

TEST(BraveBrowserViewTabbedLayoutImplSidebarTest,
     ComputeAdjustedPanelBoundsSidebarTrailingAnimating) {
  // Same client/sidebar. Panel 50% open: visible_width=150, so upstream
  // x = client.right()-visible = 1000-150 = 850.  Shift = -40 → x = 810.
  // The slide is preserved: 150px of panel is visible up to the sidebar edge.
  gfx::Rect client(0, 0, 1000, 700);
  gfx::Rect sidebar(960, 50, 40, 600);
  gfx::Rect panel(850, 50, 300, 600);
  gfx::Rect adjusted =
      BraveBrowserViewTabbedLayoutImpl::ComputeAdjustedPanelBounds(
          /*sidebar_leading=*/false, sidebar, panel, client);
  EXPECT_EQ(810, adjusted.x());
  EXPECT_EQ(150, sidebar.x() - adjusted.x());  // 150px visible to the user
}

TEST(BraveBrowserViewTabbedLayoutImplSidebarTest,
     ComputeAdjustedPanelBoundsSidebarAndVtabTrailingFullyOpen) {
  // The regression case: VT on the right (width 150) pushes the sidebar inward,
  // so sidebar.x()=810 (= 1000-150-40).  Panel fully open: upstream x=700.
  // Shift = sidebar.x()-client.right() = 810-1000 = -190 = -(sidebar 40 + VT
  // 150) → x = 510, right = 810.  A constant ±sidebar_width shift would have
  // left a 150px gap here.
  gfx::Rect client(0, 0, 1000, 700);
  gfx::Rect sidebar(810, 50, 40, 600);
  gfx::Rect panel(700, 50, 300, 600);
  gfx::Rect adjusted =
      BraveBrowserViewTabbedLayoutImpl::ComputeAdjustedPanelBounds(
          /*sidebar_leading=*/false, sidebar, panel, client);
  EXPECT_EQ(gfx::Rect(510, 50, 300, 600), adjusted);
  EXPECT_EQ(sidebar.x(), adjusted.right());  // flush with sidebar's left edge
}

TEST(BraveBrowserViewTabbedLayoutImplSidebarTest,
     ComputeAdjustedPanelBoundsSidebarLeadingFullyOpen) {
  // Client 1000px wide. Leading (left) sidebar at x=0, w=40 (flush, no VT).
  // Panel fully open: upstream x = client.x()-(target-visible) = 0.
  // Shift = sidebar.right()-client.x() = 40-0 = 40 → x = 40 = sidebar.right().
  gfx::Rect client(0, 0, 1000, 700);
  gfx::Rect sidebar(0, 50, 40, 600);
  gfx::Rect panel(0, 50, 300, 600);
  gfx::Rect adjusted =
      BraveBrowserViewTabbedLayoutImpl::ComputeAdjustedPanelBounds(
          /*sidebar_leading=*/true, sidebar, panel, client);
  EXPECT_EQ(gfx::Rect(40, 50, 300, 600), adjusted);
  EXPECT_EQ(sidebar.right(), adjusted.x());  // flush with sidebar's right edge
}

TEST(BraveBrowserViewTabbedLayoutImplSidebarTest,
     ComputeAdjustedPanelBoundsSidebarLeadingAnimating) {
  // Same client/sidebar. Panel 50% open: visible_width=150, so upstream
  // x = client.x()-(target-visible) = 0-(300-150) = -150.  Shift = 40 → x=-110.
  // The slide is preserved: 150px visible from the sidebar edge.
  gfx::Rect client(0, 0, 1000, 700);
  gfx::Rect sidebar(0, 50, 40, 600);
  gfx::Rect panel(-150, 50, 300, 600);
  gfx::Rect adjusted =
      BraveBrowserViewTabbedLayoutImpl::ComputeAdjustedPanelBounds(
          /*sidebar_leading=*/true, sidebar, panel, client);
  EXPECT_EQ(-110, adjusted.x());
  EXPECT_EQ(150, adjusted.right() - sidebar.right());  // 150px visible to user
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     CalculateSeparatorInfoNoneWhenNoVisibleTopUI) {
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  EXPECT_CALL(*mock, IsToolbarVisible())
      .WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*mock, IsBookmarkBarVisible())
      .WillRepeatedly(::testing::Return(false));

  BrowserViewLayoutViews views;
  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), nullptr, std::move(views));

  // With no visible top UI, Brave's override returns a default-constructed
  // SeparatorInfo (no separators, no shadow box) before consulting upstream -
  // which would otherwise crash because `layout_data_` has not been populated.
  EXPECT_FALSE(layout->GetTopContainerSeparatorForTesting());
  EXPECT_FALSE(layout->GetMultiContentsSeparatorForTesting());
  EXPECT_FALSE(layout->GetShadowBoxForTesting());
}
