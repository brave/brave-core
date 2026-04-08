/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/layout/brave_browser_view_tabbed_layout_impl.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/views/frame/layout/browser_view_layout.h"
#include "chrome/browser/ui/views/frame/layout/browser_view_layout_delegate.h"
#include "chrome/browser/ui/views/frame/layout/browser_view_layout_params.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"

namespace {

// Minimal fake for BrowserViewLayoutDelegate methods that are not exercised by
// GetTopSeparatorType() on the "no top UI" path (Brave returns early).
class FakeBrowserViewLayoutDelegate : public BrowserViewLayoutDelegate {
 public:
  FakeBrowserViewLayoutDelegate() = default;
  ~FakeBrowserViewLayoutDelegate() override = default;

  bool ShouldDrawTabStrip() const override { return false; }
  bool ShouldUseTouchableTabstrip() const override { return false; }
  bool ShouldDrawVerticalTabStrip() const override { return false; }
  bool IsVerticalTabStripCollapsed() const override { return false; }
  bool ShouldDrawWebAppFrameToolbar() const override { return false; }
  bool GetBorderlessModeEnabled() const override { return false; }
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

// Only the two visibility hooks matter for GetTopSeparatorType(); gmock makes
// the test spell that out with EXPECT_CALL.
class MockBrowserViewLayoutDelegate : public FakeBrowserViewLayoutDelegate {
 public:
  MOCK_METHOD(bool, IsToolbarVisible, (), (const, override));
  MOCK_METHOD(bool, IsBookmarkBarVisible, (), (const, override));
};

#if BUILDFLAG(IS_MAC)
// Mock for AddFrameBorderInsets tests: controls vertical tab and fullscreen
// state which are the only two conditions the method checks.
class MockBrowserViewLayoutDelegateForMac
    : public FakeBrowserViewLayoutDelegate {
 public:
  MOCK_METHOD(bool, ShouldShowVerticalTabs, (), (const, override));
  MOCK_METHOD(bool, IsFullscreen, (), (const, override));
};
#endif

}  // namespace

// ---------------------------------------------------------------------------
// AdjustParamsForSidebar tests
// ---------------------------------------------------------------------------

struct AdjustParamsCase {
  const char* name;
  // Inputs
  gfx::Rect visual_client_area;
  bool sidebar_on_left;
  int sidebar_width;
  // Expected output visual_client_area
  gfx::Rect expected_area;
};

class AdjustParamsForSidebarTest
    : public ::testing::TestWithParam<AdjustParamsCase> {};

TEST_P(AdjustParamsForSidebarTest, ReservesCorrectStrip) {
  const auto& p = GetParam();
  BrowserLayoutParams params;
  params.visual_client_area = p.visual_client_area;

  BrowserLayoutParams result =
      BraveBrowserViewTabbedLayoutImpl::AdjustParamsForSidebar(
          params, p.sidebar_on_left, p.sidebar_width);

  EXPECT_EQ(p.expected_area, result.visual_client_area) << p.name;
}

INSTANTIATE_TEST_SUITE_P(
    ,
    AdjustParamsForSidebarTest,
    ::testing::Values(
        // Sidebar on right: trims the right edge.
        AdjustParamsCase{"RightSide", gfx::Rect(0, 0, 1000, 600),
                         /*sidebar_on_left=*/false,
                         /*sidebar_width=*/52, gfx::Rect(0, 0, 948, 600)},

        // Sidebar on left: shifts the left edge.
        AdjustParamsCase{"LeftSide", gfx::Rect(0, 0, 1000, 600),
                         /*sidebar_on_left=*/true,
                         /*sidebar_width=*/52, gfx::Rect(52, 0, 948, 600)},

        // Zero width: params unchanged.
        AdjustParamsCase{"ZeroWidth", gfx::Rect(0, 0, 1000, 600),
                         /*sidebar_on_left=*/false,
                         /*sidebar_width=*/0, gfx::Rect(0, 0, 1000, 600)},

        // Non-zero origin: reservation is relative, not absolute.
        AdjustParamsCase{"NonZeroOriginRightSide", gfx::Rect(10, 20, 800, 500),
                         /*sidebar_on_left=*/false,
                         /*sidebar_width=*/52, gfx::Rect(10, 20, 748, 500)}),
    [](const ::testing::TestParamInfo<AdjustParamsCase>& info) {
      return info.param.name;
    });

// ---------------------------------------------------------------------------
// AdjustParamsForVerticalTabs tests
// ---------------------------------------------------------------------------

struct AdjustVTParamsCase {
  const char* name;
  gfx::Rect visual_client_area;
  bool vt_on_right;
  int vt_width;
  gfx::Rect expected_area;
};

class AdjustParamsForVerticalTabsTest
    : public ::testing::TestWithParam<AdjustVTParamsCase> {};

TEST_P(AdjustParamsForVerticalTabsTest, ReservesCorrectStrip) {
  const auto& p = GetParam();
  BrowserLayoutParams params;
  params.visual_client_area = p.visual_client_area;

  BrowserLayoutParams result =
      BraveBrowserViewTabbedLayoutImpl::AdjustParamsForVerticalTabs(
          params, p.vt_on_right, p.vt_width);

  EXPECT_EQ(p.expected_area, result.visual_client_area) << p.name;
}

INSTANTIATE_TEST_SUITE_P(
    ,
    AdjustParamsForVerticalTabsTest,
    ::testing::Values(
        // VT on right: trims the right edge.
        AdjustVTParamsCase{"VTOnRight", gfx::Rect(0, 0, 1000, 600),
                           /*vt_on_right=*/true, /*vt_width=*/200,
                           gfx::Rect(0, 0, 800, 600)},

        // VT on left: shifts the left edge.
        AdjustVTParamsCase{"VTOnLeft", gfx::Rect(0, 0, 1000, 600),
                           /*vt_on_right=*/false, /*vt_width=*/200,
                           gfx::Rect(200, 0, 800, 600)},

        // Zero width: params unchanged.
        AdjustVTParamsCase{"ZeroVTWidth", gfx::Rect(0, 0, 1000, 600),
                           /*vt_on_right=*/false, /*vt_width=*/0,
                           gfx::Rect(0, 0, 1000, 600)},

        // Non-zero origin: reservation is relative, not absolute.
        AdjustVTParamsCase{"NonZeroOriginVTRight", gfx::Rect(10, 20, 800, 500),
                           /*vt_on_right=*/true, /*vt_width=*/200,
                           gfx::Rect(10, 20, 600, 500)}),
    [](const ::testing::TestParamInfo<AdjustVTParamsCase>& info) {
      return info.param.name;
    });

// ---------------------------------------------------------------------------
// Combined VT + sidebar reservation tests
//
// These verify the two-step pre-reservation used in CalculateProposedLayout:
//   1. AdjustParamsForVerticalTabs  (VT at window edge)
//   2. AdjustParamsForSidebar        (sidebar adjacent to VT)
//
// Expected layouts for a 1000×600 window, vt_width=200, sidebar_width=52:
//   Both right         : [content(0..748)]   [sidebar(748..800)]
//   [VT(800..1000)] Both left          : [VT(0..200)] [sidebar(200..252)]
//   [content(252..1000)] VT left/sidebar right : [VT(0..200)]
//   [content(200..948)] [sidebar(948..1000)] VT right/sidebar left :
//   [sidebar(0..52)] [content(52..800)] [VT(800..1000)]
// ---------------------------------------------------------------------------

struct CombinedReservationCase {
  const char* name;
  bool vt_on_right;
  int vt_width;
  bool sidebar_on_left;
  int sidebar_width;
  gfx::Rect expected_content_area;  // visual_client_area after both adjustments
};

class CombinedReservationTest
    : public ::testing::TestWithParam<CombinedReservationCase> {};

TEST_P(CombinedReservationTest, ContentAreaCorrect) {
  const auto& p = GetParam();
  BrowserLayoutParams params;
  params.visual_client_area = gfx::Rect(0, 0, 1000, 600);

  // Step 1: reserve VT space at the window edge.
  const BrowserLayoutParams vt_adjusted =
      BraveBrowserViewTabbedLayoutImpl::AdjustParamsForVerticalTabs(
          params, p.vt_on_right, p.vt_width);

  // Step 2: reserve sidebar space adjacent to VT.
  const BrowserLayoutParams result =
      BraveBrowserViewTabbedLayoutImpl::AdjustParamsForSidebar(
          vt_adjusted, p.sidebar_on_left, p.sidebar_width);

  EXPECT_EQ(p.expected_content_area, result.visual_client_area) << p.name;
}

INSTANTIATE_TEST_SUITE_P(
    ,
    CombinedReservationTest,
    ::testing::Values(
        // Both on right: content on the left, sidebar then VT on the right.
        CombinedReservationCase{"BothOnRight",
                                /*vt_on_right=*/true, /*vt_width=*/200,
                                /*sidebar_on_left=*/false, /*sidebar_width=*/52,
                                gfx::Rect(0, 0, 748, 600)},

        // Both on left: VT then sidebar, content on the right.
        CombinedReservationCase{"BothOnLeft",
                                /*vt_on_right=*/false, /*vt_width=*/200,
                                /*sidebar_on_left=*/true, /*sidebar_width=*/52,
                                gfx::Rect(252, 0, 748, 600)},

        // VT on left, sidebar on right: VT reserves leading edge, sidebar
        // reserves trailing edge.
        CombinedReservationCase{"VTLeftSidebarRight",
                                /*vt_on_right=*/false, /*vt_width=*/200,
                                /*sidebar_on_left=*/false, /*sidebar_width=*/52,
                                gfx::Rect(200, 0, 748, 600)},

        // VT on right, sidebar on left: sidebar reserves leading edge, VT
        // reserves trailing edge.
        CombinedReservationCase{"VTRightSidebarLeft",
                                /*vt_on_right=*/true, /*vt_width=*/200,
                                /*sidebar_on_left=*/true, /*sidebar_width=*/52,
                                gfx::Rect(52, 0, 748, 600)}),
    [](const ::testing::TestParamInfo<CombinedReservationCase>& info) {
      return info.param.name;
    });

// ---------------------------------------------------------------------------
// RestoreInfobarBoundsForSidebar tests
//
// The infobar must span content + sidebar but not the VT strip. The upstream
// layout places it using adjusted_params (narrowed by both VT and sidebar).
// RestoreInfobarBoundsForSidebar() expands x/width to vt_adjusted_client_area
// so the sidebar is included while the VT strip is still excluded.
//
// y and height are always preserved from the original bounds.
// ---------------------------------------------------------------------------

struct InfobarExpansionCase {
  const char* name;
  gfx::Rect visual_client_area;
  bool sidebar_on_left;
  int sidebar_width;
  bool vt_on_right;
  int vt_width;
  // Infobar vertical extent (unchanged by expansion).
  int infobar_y = 50;
  int infobar_height = 30;
  gfx::Rect expected;
};

class InfobarExpansionTest
    : public ::testing::TestWithParam<InfobarExpansionCase> {};

TEST_P(InfobarExpansionTest, CoversContentPlusSidebar) {
  const auto& p = GetParam();
  BrowserLayoutParams params;
  params.visual_client_area = p.visual_client_area;

  const BrowserLayoutParams vt_adjusted =
      BraveBrowserViewTabbedLayoutImpl::AdjustParamsForVerticalTabs(
          params, p.vt_on_right, p.vt_width);
  const BrowserLayoutParams adjusted_params =
      BraveBrowserViewTabbedLayoutImpl::AdjustParamsForSidebar(
          vt_adjusted, p.sidebar_on_left, p.sidebar_width);

  // Simulate the infobar bounds placed by the upstream layout using
  // adjusted_params (content area only, narrowed by both VT and sidebar).
  const gfx::Rect upstream_infobar(
      adjusted_params.visual_client_area.x(), p.infobar_y,
      adjusted_params.visual_client_area.width(), p.infobar_height);

  const gfx::Rect result =
      BraveBrowserViewTabbedLayoutImpl::RestoreInfobarBoundsForSidebar(
          upstream_infobar, vt_adjusted.visual_client_area);

  EXPECT_EQ(p.expected, result) << p.name;
}

INSTANTIATE_TEST_SUITE_P(
    ,
    InfobarExpansionTest,
    ::testing::Values(
        // No VT, sidebar on right: expands to full width.
        InfobarExpansionCase{"RightSidebarNoVT", gfx::Rect(0, 0, 1000, 600),
                             /*sidebar_on_left=*/false, /*sidebar_width=*/52,
                             /*vt_on_right=*/false, /*vt_width=*/0,
                             /*infobar_y=*/50, /*infobar_height=*/30,
                             gfx::Rect(0, 50, 1000, 30)},

        // No VT, sidebar on left: expands to full width.
        InfobarExpansionCase{"LeftSidebarNoVT", gfx::Rect(0, 0, 1000, 600),
                             /*sidebar_on_left=*/true, /*sidebar_width=*/52,
                             /*vt_on_right=*/false, /*vt_width=*/0,
                             /*infobar_y=*/50, /*infobar_height=*/30,
                             gfx::Rect(0, 50, 1000, 30)},

        // VT on right, sidebar on right: covers [0..800], not VT [800..1000].
        InfobarExpansionCase{"VTRightSidebarRight", gfx::Rect(0, 0, 1000, 600),
                             /*sidebar_on_left=*/false, /*sidebar_width=*/52,
                             /*vt_on_right=*/true, /*vt_width=*/200,
                             /*infobar_y=*/50, /*infobar_height=*/30,
                             gfx::Rect(0, 50, 800, 30)},

        // VT on left, sidebar on left: covers [200..1000], not VT [0..200].
        InfobarExpansionCase{"VTLeftSidebarLeft", gfx::Rect(0, 0, 1000, 600),
                             /*sidebar_on_left=*/true, /*sidebar_width=*/52,
                             /*vt_on_right=*/false, /*vt_width=*/200,
                             /*infobar_y=*/50, /*infobar_height=*/30,
                             gfx::Rect(200, 50, 800, 30)},

        // VT on left, sidebar on right: covers [200..1000].
        InfobarExpansionCase{"VTLeftSidebarRight", gfx::Rect(0, 0, 1000, 600),
                             /*sidebar_on_left=*/false, /*sidebar_width=*/52,
                             /*vt_on_right=*/false, /*vt_width=*/200,
                             /*infobar_y=*/50, /*infobar_height=*/30,
                             gfx::Rect(200, 50, 800, 30)},

        // VT on right, sidebar on left: covers [0..800].
        InfobarExpansionCase{"VTRightSidebarLeft", gfx::Rect(0, 0, 1000, 600),
                             /*sidebar_on_left=*/true, /*sidebar_width=*/52,
                             /*vt_on_right=*/true, /*vt_width=*/200,
                             /*infobar_y=*/50, /*infobar_height=*/30,
                             gfx::Rect(0, 50, 800, 30)},

        // No VT, no sidebar: bounds unchanged.
        InfobarExpansionCase{"NoVTNoSidebar", gfx::Rect(0, 0, 1000, 600),
                             /*sidebar_on_left=*/false, /*sidebar_width=*/0,
                             /*vt_on_right=*/false, /*vt_width=*/0,
                             /*infobar_y=*/50, /*infobar_height=*/30,
                             gfx::Rect(0, 50, 1000, 30)}),
    [](const ::testing::TestParamInfo<InfobarExpansionCase>& info) {
      return info.param.name;
    });

// ---------------------------------------------------------------------------
// ContentsBackgroundBounds tests
//
// The contents background must span the full original visual_client_area
// horizontally (covering sidebar + VT columns) while using the vertical
// extent of the contents_container bounds (i.e. the area below the top
// container).  This is the complement of InsetContentsContainerBounds:
// the web view gets the narrowed bounds, the background gets the full width.
//
// For a 1000×600 window (top_container_height=50):
//   No sidebar/VT    → background == contents_container == (0,50,1000,550)
//   Sidebar right=52 → contents_container=(0,50,948,550); bg=(0,50,1000,550)
//   Sidebar left=52  → contents_container=(52,50,948,550); bg=(0,50,1000,550)
//   VT right+sidebar right (200+52=252) narrowing from right:
//                    → contents_container=(0,50,748,550); bg=(0,50,1000,550)
//   VT left+sidebar left (200+52=252) shift from left:
//                    → contents_container=(252,50,748,550); bg=(0,50,1000,550)
// ---------------------------------------------------------------------------

struct ContentsBackgroundBoundsCase {
  const char* name;
  gfx::Rect original_visual_client_area;
  // Simulate what the upstream layout produces for the contents_container
  // after params have been narrowed by VT+sidebar reservations.
  gfx::Rect contents_container_bounds;
  gfx::Rect expected;
};

class ContentsBackgroundBoundsTest
    : public ::testing::TestWithParam<ContentsBackgroundBoundsCase> {};

TEST_P(ContentsBackgroundBoundsTest, SpansFullOriginalWidth) {
  const auto& p = GetParam();
  const gfx::Rect result =
      BraveBrowserViewTabbedLayoutImpl::ComputeContentsBackgroundBounds(
          p.original_visual_client_area, p.contents_container_bounds);
  EXPECT_EQ(p.expected, result) << p.name;
}

INSTANTIATE_TEST_SUITE_P(
    ,
    ContentsBackgroundBoundsTest,
    ::testing::Values(
        // No sidebar, no VT: background equals the contents container exactly.
        ContentsBackgroundBoundsCase{
            "NoSidebarNoVT", gfx::Rect(0, 0, 1000, 600),
            gfx::Rect(0, 50, 1000, 550), gfx::Rect(0, 50, 1000, 550)},

        // Sidebar on right (width=52): contents_container is narrowed on the
        // right, but background must still cover the full width.
        ContentsBackgroundBoundsCase{"SidebarRight", gfx::Rect(0, 0, 1000, 600),
                                     gfx::Rect(0, 50, 948, 550),
                                     gfx::Rect(0, 50, 1000, 550)},

        // Sidebar on left (width=52): contents_container is shifted right,
        // but background must start at the original x=0.
        ContentsBackgroundBoundsCase{"SidebarLeft", gfx::Rect(0, 0, 1000, 600),
                                     gfx::Rect(52, 50, 948, 550),
                                     gfx::Rect(0, 50, 1000, 550)},

        // VT on right (200) + sidebar on right (52): contents_container is
        // doubly narrowed; background still spans full original width.
        ContentsBackgroundBoundsCase{
            "VTRightSidebarRight", gfx::Rect(0, 0, 1000, 600),
            gfx::Rect(0, 50, 748, 550), gfx::Rect(0, 50, 1000, 550)},

        // VT on left (200) + sidebar on left (52): contents_container is
        // shifted 252px; background starts at original x=0.
        ContentsBackgroundBoundsCase{
            "VTLeftSidebarLeft", gfx::Rect(0, 0, 1000, 600),
            gfx::Rect(252, 50, 748, 550), gfx::Rect(0, 50, 1000, 550)},

        // VT on left (200) + sidebar on right (52): VT shifts the left edge,
        // sidebar narrows from right; background still starts at x=0, w=1000.
        ContentsBackgroundBoundsCase{
            "VTLeftSidebarRight", gfx::Rect(0, 0, 1000, 600),
            gfx::Rect(200, 50, 748, 550), gfx::Rect(0, 50, 1000, 550)},

        // VT on right (200) + sidebar on left (52): sidebar shifts left edge,
        // VT narrows from right; background starts at x=0, w=1000.
        ContentsBackgroundBoundsCase{
            "VTRightSidebarLeft", gfx::Rect(0, 0, 1000, 600),
            gfx::Rect(52, 50, 748, 550), gfx::Rect(0, 50, 1000, 550)},

        // Non-zero origin: x/width are taken from original_visual_client_area,
        // not from the (shifted) contents_container.
        ContentsBackgroundBoundsCase{
            "NonZeroOriginSidebarLeft", gfx::Rect(10, 20, 800, 500),
            gfx::Rect(62, 70, 748, 430), gfx::Rect(10, 70, 800, 430)}),
    [](const ::testing::TestParamInfo<ContentsBackgroundBoundsCase>& info) {
      return info.param.name;
    });

// ---------------------------------------------------------------------------

TEST(BraveBrowserViewTabbedLayoutImplTest,
     GetTopSeparatorTypeNoneWhenNoVisibleTopUI) {
  auto mock =
      std::make_unique<testing::NiceMock<MockBrowserViewLayoutDelegate>>();
  EXPECT_CALL(*mock, IsToolbarVisible()).WillOnce(::testing::Return(false));
  EXPECT_CALL(*mock, IsBookmarkBarVisible()).WillOnce(::testing::Return(false));

  BrowserViewLayoutViews views;
  auto layout = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
      std::move(mock), nullptr, std::move(views));

  // TopSeparatorType::kNone (0) - no top separator when top UI is hidden.
  // TopSeparatorType is private member.
  EXPECT_EQ(0, static_cast<int>(layout->GetTopSeparatorType()));
}

#if BUILDFLAG(IS_MAC)
struct AddFrameBorderInsetsCase {
  bool show_vertical_tabs;
  bool is_fullscreen;
  gfx::Insets input;
  gfx::Insets expected;
  bool enable_embedded_vertical_tab_strip_feature = true;
};

// Test fixture that grants access to the private AddFrameBorderInsets method
// via the friend declaration in BraveBrowserViewTabbedLayoutImpl.
class BraveBrowserViewTabbedLayoutImplMacTest
    : public ::testing::TestWithParam<AddFrameBorderInsetsCase> {
 protected:
  void SetUp() override {
    auto mock = std::make_unique<
        testing::NiceMock<MockBrowserViewLayoutDelegateForMac>>();
    mock_ = mock.get();
    BrowserViewLayoutViews views;
    layout_ = std::make_unique<BraveBrowserViewTabbedLayoutImpl>(
        std::move(mock), nullptr, std::move(views));
  }

  gfx::Insets CallAddFrameBorderInsets(const gfx::Insets& insets) const {
    return layout_->AddFrameBorderInsets(insets);
  }

  std::unique_ptr<BraveBrowserViewTabbedLayoutImpl> layout_;

  // not owned
  raw_ptr<testing::NiceMock<MockBrowserViewLayoutDelegateForMac>> mock_;
};

TEST_P(BraveBrowserViewTabbedLayoutImplMacTest, AddFrameBorderInsets) {
  const auto& p = GetParam();
  base::test::ScopedFeatureList scoped_feature_list;

  // Nothing to do as it's enabled by default.
  if (!p.enable_embedded_vertical_tab_strip_feature) {
    scoped_feature_list.InitAndDisableFeature(
        tabs::kBraveVerticalTabStripEmbedded);
  }
  ON_CALL(*mock_, ShouldShowVerticalTabs())
      .WillByDefault(::testing::Return(p.show_vertical_tabs));
  ON_CALL(*mock_, IsFullscreen())
      .WillByDefault(::testing::Return(p.is_fullscreen));
  EXPECT_EQ(p.expected, CallAddFrameBorderInsets(p.input));
}

INSTANTIATE_TEST_SUITE_P(
    ,
    BraveBrowserViewTabbedLayoutImplMacTest,
    ::testing::Values(
        // Vertical tabs off: insets pass through unchanged regardless of
        // fullscreen state.
        AddFrameBorderInsetsCase{false, false, gfx::Insets(), gfx::Insets()},
        // Fullscreen (browser or tab): frame border is not drawn, so insets
        // pass through unchanged. IsFullscreen() covers both cases.
        AddFrameBorderInsetsCase{true, true, gfx::Insets(), gfx::Insets()},
        // Vertical tabs visible, not fullscreen: disable
        // embedded feature to get 1px border on left, right, and bottom edges.
        AddFrameBorderInsetsCase{true, false, gfx::Insets(),
                                 gfx::Insets::TLBR(0, 1, 1, 1), false},
        // Same delegate state with default embedded vertical strip on:
        // AddFrameBorderInsets returns input unchanged.
        AddFrameBorderInsetsCase{true, false, gfx::Insets(), gfx::Insets()}));
#endif  // BUILDFLAG(IS_MAC)
