/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/layout/brave_browser_view_tabbed_layout_impl.h"

#include <memory>

#include "build/build_config.h"
#include "chrome/browser/ui/views/frame/layout/browser_view_layout.h"
#include "chrome/browser/ui/views/frame/layout/browser_view_layout_delegate.h"
#include "chrome/browser/ui/views/frame/layout/browser_view_layout_params.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/insets.h"

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
  raw_ptr<testing::NiceMock<MockBrowserViewLayoutDelegateForMac>> mock_;
};

TEST_P(BraveBrowserViewTabbedLayoutImplMacTest, AddFrameBorderInsets) {
  const auto& p = GetParam();
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
        // Vertical tabs visible, not fullscreen: 1px border on left, right,
        // and bottom edges.
        AddFrameBorderInsetsCase{true, false, gfx::Insets(),
                                 gfx::Insets::TLBR(0, 1, 1, 1)}));
#endif  // BUILDFLAG(IS_MAC)
