/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/layout/brave_browser_view_tabbed_layout_impl.h"

#include <memory>
#include <optional>

#include "brave/browser/ui/views/brave_layout_provider.h"
#include "build/build_config.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/frame/layout/browser_view_layout_delegate.h"
#include "chrome/browser/ui/views/tabs/organizer/organizer_panel_host.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/view.h"

namespace {

// Minimal fake for BrowserViewLayoutDelegate methods that are not exercised by
// CalculateSeparatorInfo() on the "no top UI" path (Brave returns early).
class FakeBrowserViewLayoutDelegate : public BrowserViewLayoutDelegate {
 public:
  FakeBrowserViewLayoutDelegate() = default;
  ~FakeBrowserViewLayoutDelegate() override = default;

  TabStripType GetTabStripType() const override { return TabStripType::kNone; }
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
  BrowserAnimationController* GetAnimationController() const override {
    return nullptr;
  }
  ExclusiveAccessBubbleViews* GetExclusiveAccessBubble() const override {
    return nullptr;
  }
  OrganizerPanelLocation GetOrganizerPanelLocation() const override {
    return OrganizerPanelLocation::kNone;
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
  int GetRestoredWindowCornerRadius() const override { return 0; }
  base::CallbackListSubscription AddOnGlassModeChangedCallback(
      base::RepeatingCallback<void(bool)> callback,
      bool* current_state_out) override {
    if (current_state_out) {
      *current_state_out = false;
    }
    return base::CallbackListSubscription();
  }

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
  bool IsWebPanelVisible() const override { return false; }
  bool IsWebPanelOnLeft() const override { return false; }
  bool IsSidebarControlVisible() const override { return false; }
  bool IsSidePanelVisible() const override { return false; }
  bool IsSidebarOnLeft() const override { return false; }
  void UpdateContentsCornerRadii(const gfx::RoundedCornersF&,
                                 const gfx::RoundedCornersF&) override {}
  bool IsFullscreen() const override { return false; }
};

// Mocks the CalculateSeparatorInfo() and GetContentsMargins() inputs
// exercised by the tests below: IsToolbarVisible/IsBookmarkBarVisible drive
// the "no top UI" early return, IsInfobarVisible and
// ShouldUseBraveWebViewRoundedCornersForContents drive the mac fullscreen
// separator-suppression branch, and GetBrowserWindowState additionally drives
// the mac fullscreen-with-toolbar top-padding-suppression branch in
// GetContentsMargins().
class MockBrowserViewLayoutDelegate : public FakeBrowserViewLayoutDelegate {
 public:
  MOCK_METHOD(bool, IsToolbarVisible, (), (const, override));
  MOCK_METHOD(bool, IsBookmarkBarVisible, (), (const, override));
  MOCK_METHOD(bool, IsInfobarVisible, (), (const, override));
  MOCK_METHOD(bool,
              ShouldUseBraveWebViewRoundedCornersForContents,
              (),
              (const, override));
  MOCK_METHOD(bool, IsFullscreenForTab, (), (const, override));
  MOCK_METHOD(BrowserViewLayoutDelegate::TabStripType,
              GetTabStripType,
              (),
              (const, override));
  MOCK_METHOD(bool, ShouldShowVerticalTabs, (), (const, override));
  MOCK_METHOD(bool, IsVerticalTabOnRight, (), (const, override));
  MOCK_METHOD(WindowState, GetBrowserWindowState, (), (const, override));
  MOCK_METHOD(bool, IsWebPanelVisible, (), (const, override));
  MOCK_METHOD(bool, IsWebPanelOnLeft, (), (const, override));
  MOCK_METHOD(bool, IsSidebarControlVisible, (), (const, override));
  MOCK_METHOD(bool, IsSidePanelVisible, (), (const, override));
  MOCK_METHOD(bool, IsSidebarOnLeft, (), (const, override));
};

int GetWindowCornerRadius() {
  return views::LayoutProvider::Get()->GetCornerRadiusMetric(
      ExtendedShapeContextTokens::kRoundedCornersBorderRadiusAtWindowCorner);
}

// In production the two radii coincide everywhere except recent macOS, so a
// corner assertion against either one would also pass with the other metric,
// or with a radius applied to the wrong corner. These values force them apart.
class DistinctCornerRadiiLayoutProvider : public BraveLayoutProvider {
 public:
  static constexpr int kBorder = 6;
  static constexpr int kWindowCorner = 16;

  using BraveLayoutProvider::GetCornerRadiusMetric;
  int GetCornerRadiusMetric(
      views::ShapeContextToken token,
      const gfx::Size& size = gfx::Size()) const override {
    switch (token) {
      case kRoundedCornersBorderRadius:
        return kBorder;
      case kRoundedCornersBorderRadiusAtWindowCorner:
        return kWindowCorner;
      default:
        return BraveLayoutProvider::GetCornerRadiusMetric(token, size);
    }
  }
};

// A layout with rounded corners on, a web panel on the given side, and no
// other UI: the default `BrowserViewLayoutViews` has no sidebar container,
// side panel or vertical tab strip host, so the web panel is the only thing
// beside the contents and every corner it does not touch must stay at the
// window radius.
std::unique_ptr<BraveBrowserViewTabbedLayoutImpl> CreateLayoutWithWebPanel(
    bool panel_visible,
    bool panel_on_left) {
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  ON_CALL(*mock, ShouldUseBraveWebViewRoundedCornersForContents())
      .WillByDefault(::testing::Return(true));
  ON_CALL(*mock, IsWebPanelVisible())
      .WillByDefault(::testing::Return(panel_visible));
  ON_CALL(*mock, IsWebPanelOnLeft())
      .WillByDefault(::testing::Return(panel_on_left));
  // No infobar and no top container keeps the contents at the top edge, so the
  // top corners follow the same left/right rule as the bottom ones.
  ON_CALL(*mock, IsInfobarVisible()).WillByDefault(::testing::Return(false));

  return std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), BrowserViewLayoutViews());
}

// A layout with rounded corners on and the sidebar control view and/or the
// side panel beside the contents on the given side. The two are separate views
// sharing that edge and either can be visible without the other, so they are
// set independently. Pass a `vtab_host` with a non-zero preferred width to also
// place Brave's vertical tab strip beside the contents; it must outlive the
// returned layout.
std::unique_ptr<BraveBrowserViewTabbedLayoutImpl> CreateLayoutWithSidebar(
    bool sidebar_control_visible,
    bool side_panel_visible,
    bool sidebar_on_left,
    views::View* vtab_host = nullptr,
    bool vtab_on_right = false) {
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  ON_CALL(*mock, ShouldUseBraveWebViewRoundedCornersForContents())
      .WillByDefault(::testing::Return(true));
  ON_CALL(*mock, IsSidebarControlVisible())
      .WillByDefault(::testing::Return(sidebar_control_visible));
  ON_CALL(*mock, IsSidePanelVisible())
      .WillByDefault(::testing::Return(side_panel_visible));
  ON_CALL(*mock, IsSidebarOnLeft())
      .WillByDefault(::testing::Return(sidebar_on_left));
  ON_CALL(*mock, ShouldShowVerticalTabs())
      .WillByDefault(::testing::Return(vtab_host != nullptr));
  ON_CALL(*mock, IsVerticalTabOnRight())
      .WillByDefault(::testing::Return(vtab_on_right));
  // No infobar and no top container keeps the contents at the top edge, so the
  // top corners follow the same left/right rule as the bottom ones.
  ON_CALL(*mock, IsInfobarVisible()).WillByDefault(::testing::Return(false));

  BrowserViewLayoutViews views;
  views.vertical_tab_strip_host = vtab_host;
  return std::make_unique<BraveBrowserViewTabbedLayoutImpl>(std::move(mock),
                                                            std::move(views));
}

int GetBorderCornerRadius() {
  return views::LayoutProvider::Get()->GetCornerRadiusMetric(
      ExtendedShapeContextTokens::kRoundedCornersBorderRadius);
}

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

// ---------------------------------------------------------------------------
// ComputeAdjustedInfobarBounds
//
// Verifies that the infobar's horizontal span is reset to the full browser
// window regardless of any panel-driven narrowing or shifting, and that
// vertical tab insets are applied correctly on each side.
// ---------------------------------------------------------------------------

TEST(BraveBrowserViewTabbedLayoutImplInfobarTest,
     FullWidthWhenPanelNarrowsContents) {
  // Upstream narrowed the infobar to 700px (contents area with panel open).
  // Full browser window is 1000px wide — the infobar must be reset to that.
  gfx::Rect adjusted =
      BraveBrowserViewTabbedLayoutImpl::ComputeAdjustedInfobarBounds(
          gfx::Rect(0, 50, 700, 30),
          /*visual_client_area=*/gfx::Rect(0, 0, 1000, 800),
          /*vtab_insets=*/std::nullopt);
  EXPECT_EQ(gfx::Rect(0, 50, 1000, 30), adjusted);
}

TEST(BraveBrowserViewTabbedLayoutImplInfobarTest,
     FullWidthWhenLeadingPanelShiftsContents) {
  // With a left-aligned panel open, upstream both narrows the infobar and
  // moves its origin to the panel's trailing edge. Both must be undone, or the
  // infobar slides right along with the panel and overflows the window.
  gfx::Rect adjusted =
      BraveBrowserViewTabbedLayoutImpl::ComputeAdjustedInfobarBounds(
          gfx::Rect(300, 50, 700, 30),
          /*visual_client_area=*/gfx::Rect(0, 0, 1000, 800),
          /*vtab_insets=*/std::nullopt);
  EXPECT_EQ(gfx::Rect(0, 50, 1000, 30), adjusted);
}

TEST(BraveBrowserViewTabbedLayoutImplInfobarTest,
     UnchangedWhenAlreadyFullWidth) {
  // When no panel is open the infobar already spans the full width; the call
  // must be a no-op for width.
  gfx::Rect adjusted =
      BraveBrowserViewTabbedLayoutImpl::ComputeAdjustedInfobarBounds(
          gfx::Rect(0, 50, 1000, 30),
          /*visual_client_area=*/gfx::Rect(0, 0, 1000, 800),
          /*vtab_insets=*/std::nullopt);
  EXPECT_EQ(gfx::Rect(0, 50, 1000, 30), adjusted);
}

TEST(BraveBrowserViewTabbedLayoutImplInfobarTest, LeadingVtabInsetsFromLeft) {
  // Vertical tabs on the left (leading) with width 150 → left inset of 150.
  // Width is reset to 1000 first, then inset → x=150, width=850.
  gfx::Insets insets;
  insets.set_left(150);
  gfx::Rect adjusted =
      BraveBrowserViewTabbedLayoutImpl::ComputeAdjustedInfobarBounds(
          gfx::Rect(0, 50, 700, 30),
          /*visual_client_area=*/gfx::Rect(0, 0, 1000, 800),
          std::make_optional(insets));
  EXPECT_EQ(150, adjusted.x());
  EXPECT_EQ(850, adjusted.width());
}

TEST(BraveBrowserViewTabbedLayoutImplInfobarTest, TrailingVtabInsetsFromRight) {
  // Vertical tabs on the right (trailing) with width 150 → right inset of 150.
  // Width is reset to 1000 first, then inset → x=0, width=850.
  gfx::Insets insets;
  insets.set_right(150);
  gfx::Rect adjusted =
      BraveBrowserViewTabbedLayoutImpl::ComputeAdjustedInfobarBounds(
          gfx::Rect(0, 50, 700, 30),
          /*visual_client_area=*/gfx::Rect(0, 0, 1000, 800),
          std::make_optional(insets));
  EXPECT_EQ(0, adjusted.x());
  EXPECT_EQ(850, adjusted.width());
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
      std::move(mock), std::move(views));

  // With no visible top UI, Brave's override returns a default-constructed
  // SeparatorInfo (no separators, no shadow box) before consulting upstream -
  // which would otherwise crash because `layout_data_` has not been populated.
  EXPECT_FALSE(layout->GetTopContainerSeparatorForTesting());
  EXPECT_FALSE(layout->GetMultiContentsSeparatorForTesting());
  EXPECT_FALSE(layout->GetShadowBoxForTesting());
}

namespace {

// Builds a layout in the normal window state with a visible toolbar, as
// `CalculateSeparatorInfo()` suppresses every separator when no top UI shows.
std::unique_ptr<BraveBrowserViewTabbedLayoutImpl> CreateLayoutForSeparatorInfo(
    bool rounded_corners,
    bool infobar_visible) {
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  EXPECT_CALL(*mock, IsToolbarVisible())
      .WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(*mock, IsInfobarVisible())
      .WillRepeatedly(::testing::Return(infobar_visible));
  EXPECT_CALL(*mock, ShouldUseBraveWebViewRoundedCornersForContents())
      .WillRepeatedly(::testing::Return(rounded_corners));

  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), BrowserViewLayoutViews());
  layout->SetWindowStateForTesting(
      BrowserViewLayoutDelegate::WindowState::kNormal);
  return layout;
}

}  // namespace

TEST(BraveBrowserViewTabbedLayoutImplTest,
     CalculateSeparatorInfoNoneWithRoundedCornersAndInfobar) {
  auto layout = CreateLayoutForSeparatorInfo(/*rounded_corners=*/true,
                                             /*infobar_visible=*/true);

  // Upstream would draw a top-container separator above the infobar, but the
  // rounded-corners infobar is a card with its own outline, so the separator
  // would double up with it.
  EXPECT_FALSE(layout->GetTopContainerSeparatorForTesting());
  EXPECT_FALSE(layout->GetMultiContentsSeparatorForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     CalculateSeparatorInfoNoneWithRoundedCornersWithoutInfobar) {
  auto layout = CreateLayoutForSeparatorInfo(/*rounded_corners=*/true,
                                             /*infobar_visible=*/false);

  // Same for the rounded contents view, which upstream would divide with a
  // multi-contents separator instead.
  EXPECT_FALSE(layout->GetTopContainerSeparatorForTesting());
  EXPECT_FALSE(layout->GetMultiContentsSeparatorForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     CalculateSeparatorInfoTopContainerWithoutRoundedCornersAndInfobar) {
  auto layout = CreateLayoutForSeparatorInfo(/*rounded_corners=*/false,
                                             /*infobar_visible=*/true);

  EXPECT_TRUE(layout->GetTopContainerSeparatorForTesting());
  EXPECT_FALSE(layout->GetMultiContentsSeparatorForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     CalculateSeparatorInfoTopContainerWithoutRoundedCornersOrInfobar) {
  auto layout = CreateLayoutForSeparatorInfo(/*rounded_corners=*/false,
                                             /*infobar_visible=*/false);

  // Upstream's multi-contents separator is promoted to a full-width one at the
  // top container's edge.
  EXPECT_TRUE(layout->GetTopContainerSeparatorForTesting());
  EXPECT_FALSE(layout->GetMultiContentsSeparatorForTesting());
}

// ---------------------------------------------------------------------------
// GetUpstreamVerticalTabStripWidth
//
// Upstream's vertical tab strip reserves no width when Brave's own vertical
// tabs are in use, when the tab strip isn't vertical, or when the region view
// isn't parented to the browser view.
// ---------------------------------------------------------------------------

TEST(BraveBrowserViewTabbedLayoutImplTest,
     UpstreamVerticalTabStripWidthZeroWithBraveVerticalTabs) {
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  EXPECT_CALL(*mock, ShouldShowVerticalTabs())
      .WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(*mock, GetTabStripType())
      .WillRepeatedly(::testing::Return(
          BrowserViewLayoutDelegate::TabStripType::kVertical));

  BrowserViewLayoutViews views;
  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), std::move(views));

  EXPECT_EQ(0, layout->GetUpstreamVerticalTabStripWidthForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     UpstreamVerticalTabStripWidthZeroWhenTabStripNotVertical) {
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  EXPECT_CALL(*mock, ShouldShowVerticalTabs())
      .WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*mock, GetTabStripType())
      .WillRepeatedly(::testing::Return(
          BrowserViewLayoutDelegate::TabStripType::kHorizontal));

  BrowserViewLayoutViews views;
  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), std::move(views));

  EXPECT_EQ(0, layout->GetUpstreamVerticalTabStripWidthForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     UpstreamVerticalTabStripWidthZeroWithoutRegionView) {
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  EXPECT_CALL(*mock, ShouldShowVerticalTabs())
      .WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*mock, GetTabStripType())
      .WillRepeatedly(::testing::Return(
          BrowserViewLayoutDelegate::TabStripType::kVertical));

  BrowserViewLayoutViews views;
  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), std::move(views));

  EXPECT_EQ(0, layout->GetUpstreamVerticalTabStripWidthForTesting());
}

// ---------------------------------------------------------------------------
// CalculateContentsCornerRadii
//
// A corner that meets the window frame follows the window's own corner radius;
// one that meets other browser UI uses the smaller border radius. These tests
// cover the top corners, which depend on whether any top chrome is laid out
// within the browser view. Note that the two radii only differ on macOS 26 and
// later.
// ---------------------------------------------------------------------------

TEST(BraveBrowserViewTabbedLayoutImplTest,
     CalculateContentsCornerRadiiEmptyWithoutRoundedCorners) {
  BraveLayoutProvider layout_provider;
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  EXPECT_CALL(*mock, ShouldUseBraveWebViewRoundedCornersForContents())
      .WillRepeatedly(::testing::Return(false));

  BrowserViewLayoutViews views;
  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), std::move(views));

  EXPECT_EQ(gfx::RoundedCornersF(),
            layout->CalculateContentsCornerRadiiForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     CalculateContentsCornerRadiiEmptyInTabFullscreen) {
  BraveLayoutProvider layout_provider;
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  EXPECT_CALL(*mock, ShouldUseBraveWebViewRoundedCornersForContents())
      .WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(*mock, IsFullscreenForTab())
      .WillRepeatedly(::testing::Return(true));

  BrowserViewLayoutViews views;
  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), std::move(views));

  // The web contents occupies the whole window, so nothing should be rounded.
  EXPECT_EQ(gfx::RoundedCornersF(),
            layout->CalculateContentsCornerRadiiForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     CalculateContentsCornerRadiiWindowCornersAtTopEdge) {
  BraveLayoutProvider layout_provider;
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  EXPECT_CALL(*mock, ShouldUseBraveWebViewRoundedCornersForContents())
      .WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(*mock, IsInfobarVisible())
      .WillRepeatedly(::testing::Return(false));

  // No top container within the browser view, as in focus mode, where it is
  // reparented into the top overlay.
  BrowserViewLayoutViews views;
  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), std::move(views));

  const float window_radius = GetWindowCornerRadius();
  EXPECT_EQ(gfx::RoundedCornersF(window_radius),
            layout->CalculateContentsCornerRadiiForTesting());
}

// ---------------------------------------------------------------------------
// Web panel corner radii
//
// The contents and the open panel share one edge, which both round with the
// smaller border radius; their outer edges still meet the window frame and
// keep the window radius. These run against a layout provider whose two radii
// differ, so a radius landing on the wrong edge is detectable.
// ---------------------------------------------------------------------------

TEST(BraveBrowserViewTabbedLayoutImplTest,
     CalculateContentsCornerRadiiWithWebPanelOnLeft) {
  DistinctCornerRadiiLayoutProvider layout_provider;
  auto layout =
      CreateLayoutWithWebPanel(/*panel_visible=*/true, /*panel_on_left=*/true);

  constexpr float kBorder = DistinctCornerRadiiLayoutProvider::kBorder;
  constexpr float kWindow = DistinctCornerRadiiLayoutProvider::kWindowCorner;
  EXPECT_EQ(gfx::RoundedCornersF(kBorder, kWindow, kWindow, kBorder),
            layout->CalculateContentsCornerRadiiForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     CalculateContentsCornerRadiiWithWebPanelOnRight) {
  DistinctCornerRadiiLayoutProvider layout_provider;
  auto layout =
      CreateLayoutWithWebPanel(/*panel_visible=*/true, /*panel_on_left=*/false);

  constexpr float kBorder = DistinctCornerRadiiLayoutProvider::kBorder;
  constexpr float kWindow = DistinctCornerRadiiLayoutProvider::kWindowCorner;
  EXPECT_EQ(gfx::RoundedCornersF(kWindow, kBorder, kBorder, kWindow),
            layout->CalculateContentsCornerRadiiForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     CalculateWebPanelCornerRadiiWithPanelOnLeft) {
  DistinctCornerRadiiLayoutProvider layout_provider;
  auto layout =
      CreateLayoutWithWebPanel(/*panel_visible=*/true, /*panel_on_left=*/true);

  // The panel's right edge faces the contents; its left edge is the window's.
  constexpr float kBorder = DistinctCornerRadiiLayoutProvider::kBorder;
  constexpr float kWindow = DistinctCornerRadiiLayoutProvider::kWindowCorner;
  EXPECT_EQ(gfx::RoundedCornersF(kWindow, kBorder, kBorder, kWindow),
            layout->CalculateWebPanelCornerRadiiForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     CalculateWebPanelCornerRadiiWithPanelOnRight) {
  DistinctCornerRadiiLayoutProvider layout_provider;
  auto layout =
      CreateLayoutWithWebPanel(/*panel_visible=*/true, /*panel_on_left=*/false);

  constexpr float kBorder = DistinctCornerRadiiLayoutProvider::kBorder;
  constexpr float kWindow = DistinctCornerRadiiLayoutProvider::kWindowCorner;
  EXPECT_EQ(gfx::RoundedCornersF(kBorder, kWindow, kWindow, kBorder),
            layout->CalculateWebPanelCornerRadiiForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     CalculateWebPanelCornerRadiiIgnoresPanelVisibility) {
  DistinctCornerRadiiLayoutProvider layout_provider;
  auto layout =
      CreateLayoutWithWebPanel(/*panel_visible=*/false, /*panel_on_left=*/true);

  // BraveMultiContentsView::SetWebPanelContents() reads these before the panel
  // becomes visible, so they must already describe the panel's geometry. The
  // contents, in contrast, are unaffected while the panel is hidden.
  constexpr float kBorder = DistinctCornerRadiiLayoutProvider::kBorder;
  constexpr float kWindow = DistinctCornerRadiiLayoutProvider::kWindowCorner;
  EXPECT_EQ(gfx::RoundedCornersF(kWindow, kBorder, kBorder, kWindow),
            layout->CalculateWebPanelCornerRadiiForTesting());
  EXPECT_EQ(gfx::RoundedCornersF(kWindow),
            layout->CalculateContentsCornerRadiiForTesting());
}

// ---------------------------------------------------------------------------
// Sidebar / side panel corner radii
//
// The sidebar control view and the side panel share the contents' alignment
// edge, which rounds with the smaller border radius; the opposite edge still
// meets the window frame. These run against a layout provider whose two radii
// differ, so a radius landing on the wrong edge is detectable - unlike in
// production, where the two coincide outside recent macOS.
// ---------------------------------------------------------------------------

TEST(BraveBrowserViewTabbedLayoutImplTest,
     CalculateContentsCornerRadiiWithSidebarOnRight) {
  DistinctCornerRadiiLayoutProvider layout_provider;
  auto layout = CreateLayoutWithSidebar(/*sidebar_control_visible=*/true,
                                        /*side_panel_visible=*/false,
                                        /*sidebar_on_left=*/false);

  constexpr float kBorder = DistinctCornerRadiiLayoutProvider::kBorder;
  constexpr float kWindow = DistinctCornerRadiiLayoutProvider::kWindowCorner;
  EXPECT_EQ(gfx::RoundedCornersF(kWindow, kBorder, kBorder, kWindow),
            layout->CalculateContentsCornerRadiiForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     CalculateContentsCornerRadiiWithSidebarOnLeft) {
  DistinctCornerRadiiLayoutProvider layout_provider;
  auto layout = CreateLayoutWithSidebar(/*sidebar_control_visible=*/true,
                                        /*side_panel_visible=*/false,
                                        /*sidebar_on_left=*/true);

  constexpr float kBorder = DistinctCornerRadiiLayoutProvider::kBorder;
  constexpr float kWindow = DistinctCornerRadiiLayoutProvider::kWindowCorner;
  EXPECT_EQ(gfx::RoundedCornersF(kBorder, kWindow, kWindow, kBorder),
            layout->CalculateContentsCornerRadiiForTesting());
}

// Regression: the side panel can be visible while the sidebar control view is
// hidden, so the panel alone has to round the contents' adjacent corners.
TEST(BraveBrowserViewTabbedLayoutImplTest,
     CalculateContentsCornerRadiiWithSidePanelOnlyOnRight) {
  DistinctCornerRadiiLayoutProvider layout_provider;
  auto layout = CreateLayoutWithSidebar(/*sidebar_control_visible=*/false,
                                        /*side_panel_visible=*/true,
                                        /*sidebar_on_left=*/false);

  constexpr float kBorder = DistinctCornerRadiiLayoutProvider::kBorder;
  constexpr float kWindow = DistinctCornerRadiiLayoutProvider::kWindowCorner;
  EXPECT_EQ(gfx::RoundedCornersF(kWindow, kBorder, kBorder, kWindow),
            layout->CalculateContentsCornerRadiiForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     CalculateContentsCornerRadiiWithSidePanelOnlyOnLeft) {
  DistinctCornerRadiiLayoutProvider layout_provider;
  auto layout = CreateLayoutWithSidebar(/*sidebar_control_visible=*/false,
                                        /*side_panel_visible=*/true,
                                        /*sidebar_on_left=*/true);

  constexpr float kBorder = DistinctCornerRadiiLayoutProvider::kBorder;
  constexpr float kWindow = DistinctCornerRadiiLayoutProvider::kWindowCorner;
  EXPECT_EQ(gfx::RoundedCornersF(kBorder, kWindow, kWindow, kBorder),
            layout->CalculateContentsCornerRadiiForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     CalculateContentsCornerRadiiWithSidebarAwayFromContentsEdge) {
  DistinctCornerRadiiLayoutProvider layout_provider;
  // The alignment pref still says "left", but neither the sidebar control view
  // nor the side panel is beside the contents.
  auto layout = CreateLayoutWithSidebar(/*sidebar_control_visible=*/false,
                                        /*side_panel_visible=*/false,
                                        /*sidebar_on_left=*/true);

  EXPECT_EQ(
      gfx::RoundedCornersF(DistinctCornerRadiiLayoutProvider::kWindowCorner),
      layout->CalculateContentsCornerRadiiForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     CalculateContentsCornerRadiiWithSidebarAndVerticalTabsOnOppositeSides) {
  DistinctCornerRadiiLayoutProvider layout_provider;
  views::View vertical_tab_strip_host;
  vertical_tab_strip_host.SetPreferredSize(gfx::Size(50, 0));
  auto layout = CreateLayoutWithSidebar(/*sidebar_control_visible=*/true,
                                        /*side_panel_visible=*/false,
                                        /*sidebar_on_left=*/false,
                                        &vertical_tab_strip_host,
                                        /*vtab_on_right=*/false);

  // UI on both sides, so no corner reaches the window frame.
  EXPECT_EQ(gfx::RoundedCornersF(DistinctCornerRadiiLayoutProvider::kBorder),
            layout->CalculateContentsCornerRadiiForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     CalculateInfobarCornerRadiiEmptyWithoutRoundedCorners) {
  BraveLayoutProvider layout_provider;
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  EXPECT_CALL(*mock, ShouldUseBraveWebViewRoundedCornersForContents())
      .WillRepeatedly(::testing::Return(false));

  BrowserViewLayoutViews views;
  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), std::move(views));

  EXPECT_EQ(gfx::RoundedCornersF(),
            layout->CalculateInfobarCornerRadiiForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     CalculateInfobarCornerRadiiUsesBorderRadiusOnAllCorners) {
  BraveLayoutProvider layout_provider;
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  EXPECT_CALL(*mock, ShouldUseBraveWebViewRoundedCornersForContents())
      .WillRepeatedly(::testing::Return(true));

  BrowserViewLayoutViews views;
  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), std::move(views));

  const gfx::RoundedCornersF expected(GetBorderCornerRadius());
  EXPECT_EQ(expected, layout->CalculateInfobarCornerRadiiForTesting());
}

// ---------------------------------------------------------------------------
// GetInfobarMargins
//
// With rounded corners the infobar floats as its own card, inset on every side
// that faces a window edge. The sidebar and the side panel are laid out below
// the infobar, so only a vertical tab strip can abut it, and the strip pads
// itself from its neighbours - that side gets no margin.
// ---------------------------------------------------------------------------

TEST(BraveBrowserViewTabbedLayoutImplTest,
     GetInfobarMarginsEmptyWithoutRoundedCorners) {
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  EXPECT_CALL(*mock, ShouldUseBraveWebViewRoundedCornersForContents())
      .WillRepeatedly(::testing::Return(false));

  BrowserViewLayoutViews views;
  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), std::move(views));

  EXPECT_EQ(gfx::Insets(), layout->GetInfobarMarginsForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     GetInfobarMarginsAllSidesWithoutVerticalTabs) {
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  EXPECT_CALL(*mock, ShouldUseBraveWebViewRoundedCornersForContents())
      .WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(*mock, ShouldShowVerticalTabs())
      .WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*mock, GetTabStripType())
      .WillRepeatedly(::testing::Return(
          BrowserViewLayoutDelegate::TabStripType::kHorizontal));

  BrowserViewLayoutViews views;
  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), std::move(views));

  EXPECT_EQ(gfx::Insets(kRoundedCornersContentsViewMargin),
            layout->GetInfobarMarginsForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     GetInfobarMarginsSkipLeadingSideForLeadingVerticalTab) {
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  EXPECT_CALL(*mock, ShouldUseBraveWebViewRoundedCornersForContents())
      .WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(*mock, ShouldShowVerticalTabs())
      .WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(*mock, IsVerticalTabOnRight())
      .WillRepeatedly(::testing::Return(false));

  views::View vertical_tab_strip_host;
  vertical_tab_strip_host.SetPreferredSize(gfx::Size(50, 0));

  BrowserViewLayoutViews views;
  views.vertical_tab_strip_host = &vertical_tab_strip_host;
  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), std::move(views));

  gfx::Insets expected(kRoundedCornersContentsViewMargin);
  expected.set_left(0);
  EXPECT_EQ(expected, layout->GetInfobarMarginsForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     GetInfobarMarginsSkipTrailingSideForTrailingVerticalTab) {
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  EXPECT_CALL(*mock, ShouldUseBraveWebViewRoundedCornersForContents())
      .WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(*mock, ShouldShowVerticalTabs())
      .WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(*mock, IsVerticalTabOnRight())
      .WillRepeatedly(::testing::Return(true));

  views::View vertical_tab_strip_host;
  vertical_tab_strip_host.SetPreferredSize(gfx::Size(50, 0));

  BrowserViewLayoutViews views;
  views.vertical_tab_strip_host = &vertical_tab_strip_host;
  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), std::move(views));

  gfx::Insets expected(kRoundedCornersContentsViewMargin);
  expected.set_right(0);
  EXPECT_EQ(expected, layout->GetInfobarMarginsForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     GetInfobarMarginsAllSidesWhenVerticalTabStripHasNoWidth) {
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  EXPECT_CALL(*mock, ShouldUseBraveWebViewRoundedCornersForContents())
      .WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(*mock, ShouldShowVerticalTabs())
      .WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(*mock, IsVerticalTabOnRight())
      .WillRepeatedly(::testing::Return(false));

  // A strip that reports no width is completely hidden and floats above the
  // contents instead of sitting beside it, so it no longer owns the gap.
  views::View vertical_tab_strip_host;
  vertical_tab_strip_host.SetPreferredSize(gfx::Size(0, 0));

  BrowserViewLayoutViews views;
  views.vertical_tab_strip_host = &vertical_tab_strip_host;
  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), std::move(views));

  EXPECT_EQ(gfx::Insets(kRoundedCornersContentsViewMargin),
            layout->GetInfobarMarginsForTesting());
}

// ---------------------------------------------------------------------------
// GetContentsMargins
//
// The margins inset the contents from the window on every side so the rounded
// border is visible, and collapse to nothing when rounded corners are off or
// the tab is fullscreen.
// ---------------------------------------------------------------------------

TEST(BraveBrowserViewTabbedLayoutImplTest,
     GetContentsMarginsEmptyWithoutRoundedCorners) {
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  ON_CALL(*mock, ShouldUseBraveWebViewRoundedCornersForContents())
      .WillByDefault(::testing::Return(false));

  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), BrowserViewLayoutViews());

  EXPECT_EQ(gfx::Insets(), layout->GetContentsMarginsForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     GetContentsMarginsOnAllSidesWithRoundedCorners) {
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  ON_CALL(*mock, ShouldUseBraveWebViewRoundedCornersForContents())
      .WillByDefault(::testing::Return(true));
  // No infobar and no top container keeps the contents at the top edge, so the
  // top margin is not suppressed.
  ON_CALL(*mock, IsInfobarVisible()).WillByDefault(::testing::Return(false));

  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), BrowserViewLayoutViews());

  EXPECT_EQ(gfx::Insets(kRoundedCornersContentsViewMargin),
            layout->GetContentsMarginsForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     GetContentsMarginsEmptyInTabFullscreen) {
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  ON_CALL(*mock, ShouldUseBraveWebViewRoundedCornersForContents())
      .WillByDefault(::testing::Return(true));
  ON_CALL(*mock, IsFullscreenForTab()).WillByDefault(::testing::Return(true));

  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), BrowserViewLayoutViews());

  // The web contents occupies the whole window, so there is nothing to inset.
  EXPECT_EQ(gfx::Insets(), layout->GetContentsMarginsForTesting());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     GetContentsMarginsHidesTopPaddingBelowAnInfobar) {
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  ON_CALL(*mock, ShouldUseBraveWebViewRoundedCornersForContents())
      .WillByDefault(::testing::Return(true));
  ON_CALL(*mock, IsInfobarVisible()).WillByDefault(::testing::Return(true));

  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), BrowserViewLayoutViews());

  // The infobar already pads the contents, so only the top is dropped.
  gfx::Insets margins = layout->GetContentsMarginsForTesting();
  EXPECT_EQ(0, margins.top());
  EXPECT_EQ(kRoundedCornersContentsViewMargin, margins.left());
  EXPECT_EQ(kRoundedCornersContentsViewMargin, margins.right());
  EXPECT_EQ(kRoundedCornersContentsViewMargin, margins.bottom());
}

#if BUILDFLAG(IS_MAC)

// On macOS, when the window is in kFullscreenWithToolbar state, the top
// container lives in an overlay widget positioned above the contents, so
// the rounded-corners contents view no longer needs a top margin.
TEST(BraveBrowserViewTabbedLayoutImplTest,
     GetContentsMarginsMacFullscreenWithToolbarHidesTopPadding) {
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  EXPECT_CALL(*mock, ShouldUseBraveWebViewRoundedCornersForContents())
      .WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(*mock, IsInfobarVisible())
      .WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*mock, GetBrowserWindowState())
      .WillRepeatedly(::testing::Return(
          BrowserViewLayoutDelegate::WindowState::kFullscreenWithToolbar));

  BrowserViewLayoutViews views;
  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), std::move(views));

  // The top margin is suppressed, but the other sides still reserve room for
  // the rounded-corners shadow.
  gfx::Insets margins = layout->GetContentsMarginsForTesting();
  EXPECT_EQ(0, margins.top());
  EXPECT_EQ(kRoundedCornersContentsViewMargin, margins.left());
  EXPECT_EQ(kRoundedCornersContentsViewMargin, margins.right());
  EXPECT_EQ(kRoundedCornersContentsViewMargin, margins.bottom());
}

TEST(BraveBrowserViewTabbedLayoutImplTest,
     GetContentsMarginsMacRegularFullscreenKeepsTopPadding) {
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  EXPECT_CALL(*mock, ShouldUseBraveWebViewRoundedCornersForContents())
      .WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(*mock, IsInfobarVisible())
      .WillRepeatedly(::testing::Return(false));
  // Plain kFullscreen (toolbar auto-hidden), not kFullscreenWithToolbar.
  EXPECT_CALL(*mock, GetBrowserWindowState())
      .WillRepeatedly(::testing::Return(
          BrowserViewLayoutDelegate::WindowState::kFullscreen));

  BrowserViewLayoutViews views;
  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), std::move(views));

  // The mac-only branch is scoped to kFullscreenWithToolbar; plain kFullscreen
  // must keep the top margin.
  gfx::Insets margins = layout->GetContentsMarginsForTesting();
  EXPECT_EQ(kRoundedCornersContentsViewMargin, margins.top());
}

#endif  // BUILDFLAG(IS_MAC)
