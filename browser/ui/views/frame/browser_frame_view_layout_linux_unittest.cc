/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/test/bind.h"
#include "chrome/browser/ui/views/frame/browser_frame_view_layout_linux_native.h"
#include "chrome/browser/ui/views/frame/opaque_browser_frame_view_layout_delegate.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/linux/nav_button_provider.h"
#include "ui/linux/window_frame_provider.h"

namespace {

class MockOpaqueLayoutDelegate : public OpaqueBrowserFrameViewLayoutDelegate {
 public:
  MOCK_METHOD(bool, ShouldShowWindowIcon, (), (const, override));
  MOCK_METHOD(bool, ShouldShowWindowTitle, (), (const, override));
  MOCK_METHOD(std::u16string, GetWindowTitle, (), (const, override));
  MOCK_METHOD(int, GetIconSize, (), (const, override));
  MOCK_METHOD(gfx::Size, GetBrowserViewMinimumSize, (), (const, override));
  MOCK_METHOD(bool, ShouldShowCaptionButtons, (), (const, override));
  MOCK_METHOD(bool, IsRegularOrGuestSession, (), (const, override));
  MOCK_METHOD(bool, CanMaximize, (), (const, override));
  MOCK_METHOD(bool, CanMinimize, (), (const, override));
  MOCK_METHOD(bool, IsMaximized, (), (const, override));
  MOCK_METHOD(bool, IsMinimized, (), (const, override));
  MOCK_METHOD(bool, IsFullscreen, (), (const, override));
  MOCK_METHOD(bool, GetUnframedModeEnabled, (), (const, override));
  MOCK_METHOD(bool, IsTabStripVisible, (), (const, override));
  MOCK_METHOD(bool, IsToolbarVisible, (), (const, override));
  MOCK_METHOD(int, GetTopAreaHeight, (), (const, override));
  MOCK_METHOD(bool, UseCustomFrame, (), (const, override));
  MOCK_METHOD(bool, IsFrameCondensed, (), (const, override));
  MOCK_METHOD(void,
              UpdateWindowControlsOverlay,
              (const gfx::Rect&),
              (override));
  MOCK_METHOD(bool, ShouldDrawRestoredFrameShadow, (), (const, override));
  MOCK_METHOD(bool, IsTiled, (), (const, override));
  MOCK_METHOD(int, WebAppButtonHeight, (), (const, override));
  MOCK_METHOD(bool, ShouldShowBraveVerticalTabs, (), (const, override));
};

// Exposes the protected NonClientExtraTopThickness() for testing.
class TestBrowserFrameViewLayoutLinuxNative
    : public BrowserFrameViewLayoutLinuxNative {
 public:
  using BrowserFrameViewLayoutLinuxNative::BrowserFrameViewLayoutLinuxNative;
  using BrowserFrameViewLayoutLinuxNative::NonClientExtraTopThickness;
};

class FakeNavButtonProvider : public ui::NavButtonProvider {
 public:
  void RedrawImages(int, bool, bool) override {}
  gfx::ImageSkia GetImage(FrameButtonDisplayType, ButtonState) const override {
    return {};
  }
  gfx::Insets GetNavButtonMargin(FrameButtonDisplayType) const override {
    return {};
  }
  gfx::Insets GetTopAreaSpacing() const override { return {}; }
  int GetInterNavButtonSpacing() const override { return 0; }
  int GetNavButtonHeight(bool maximized) const override { return 0; }
};

// The headerbar is tall enough to tell apart from the regular title bar.
constexpr int kHeaderBarHeight = 1000;

class FakeWindowFrameProvider : public ui::WindowFrameProvider {
 public:
  int GetTopCornerRadiusDip() override { return 0; }
  bool IsTopFrameTranslucent() override { return false; }
  gfx::Insets GetFrameThicknessDip() override { return {}; }
  int GetTopAreaMinHeightDip() override { return kHeaderBarHeight; }
  gfx::Insets GetTopAreaPaddingDip() override { return {}; }
  gfx::Insets GetTopAreaBorderDip() override { return {}; }
  void PaintWindowFrame(gfx::Canvas*,
                        const gfx::Rect&,
                        int,
                        bool,
                        const gfx::Insets&) override {}
};

// Needs the layout and typography providers that NonClientTopHeight() reads.
using BrowserFrameViewLayoutLinuxHeaderBarTest = ChromeViewsTestBase;

}  // namespace

// Helper that returns NonClientTopHeight() for a window that shows its title.
static int ComputeTopHeightWithTitle(
    testing::NiceMock<MockOpaqueLayoutDelegate>& mock) {
  FakeNavButtonProvider nav_button_provider;
  FakeWindowFrameProvider frame_provider;
  auto layout = std::make_unique<TestBrowserFrameViewLayoutLinuxNative>(
      &nav_button_provider,
      base::BindLambdaForTesting([&](bool, bool) -> ui::WindowFrameProvider* {
        return &frame_provider;
      }));
  layout->set_delegate(&mock);
  ON_CALL(mock, ShouldShowWindowTitle()).WillByDefault(testing::Return(true));
  return layout->NonClientTopHeight(/*restored=*/false);
}

// With Brave vertical tabs, IsTabStripVisible()=false in a tabbed window. Only
// popup and app windows get the GTK headerbar, so the title bar keeps its
// regular metrics.
TEST_F(BrowserFrameViewLayoutLinuxHeaderBarTest,
       NonClientTopHeightIsNotHeaderBarWithVerticalTabs) {
  testing::NiceMock<MockOpaqueLayoutDelegate> mock;
  ON_CALL(mock, IsTabStripVisible()).WillByDefault(testing::Return(false));
  ON_CALL(mock, IsToolbarVisible()).WillByDefault(testing::Return(true));
  ON_CALL(mock, ShouldShowBraveVerticalTabs())
      .WillByDefault(testing::Return(true));
  EXPECT_LT(ComputeTopHeightWithTitle(mock), kHeaderBarHeight);
}

// Popup windows have no tab strip, but their location bar makes the toolbar
// visible. They use the headerbar.
TEST_F(BrowserFrameViewLayoutLinuxHeaderBarTest,
       NonClientTopHeightIsHeaderBarForPopup) {
  testing::NiceMock<MockOpaqueLayoutDelegate> mock;
  ON_CALL(mock, IsTabStripVisible()).WillByDefault(testing::Return(false));
  ON_CALL(mock, IsToolbarVisible()).WillByDefault(testing::Return(true));
  EXPECT_GE(ComputeTopHeightWithTitle(mock), kHeaderBarHeight);
}

// Helper that creates a layout with a NiceMock delegate and returns
// NonClientExtraTopThickness().
static int ComputeExtraTopThickness(
    testing::NiceMock<MockOpaqueLayoutDelegate>& mock) {
  auto layout = std::make_unique<TestBrowserFrameViewLayoutLinuxNative>(
      /*nav_button_provider=*/nullptr,
      BrowserFrameViewLayoutLinuxNative::FrameProviderGetter());
  layout->set_delegate(&mock);
  return layout->NonClientExtraTopThickness();
}

// With Brave vertical tabs: IsTabStripVisible()=false but
// IsToolbarVisible()=true. The 3px extra border must be suppressed so no
// spurious strip appears above the toolbar.
TEST(BrowserFrameViewLayoutLinuxTest,
     NonClientExtraTopThicknessZeroWhenToolbarVisible) {
  testing::NiceMock<MockOpaqueLayoutDelegate> mock;
  ON_CALL(mock, IsTabStripVisible()).WillByDefault(testing::Return(false));
  ON_CALL(mock, IsToolbarVisible()).WillByDefault(testing::Return(true));
  EXPECT_EQ(0, ComputeExtraTopThickness(mock));
}

// Normal browser window: both tab strip and toolbar visible. Upstream already
// returns 0 for this case; the brave override must not regress it.
TEST(BrowserFrameViewLayoutLinuxTest,
     NonClientExtraTopThicknessZeroWithTabStripAndToolbar) {
  testing::NiceMock<MockOpaqueLayoutDelegate> mock;
  ON_CALL(mock, IsTabStripVisible()).WillByDefault(testing::Return(true));
  ON_CALL(mock, IsToolbarVisible()).WillByDefault(testing::Return(true));
  EXPECT_EQ(0, ComputeExtraTopThickness(mock));
}

// Popup windows have neither a tab strip nor a toolbar.  Upstream returns
// kExtraTopBorder (3) to provide a thin resize border; our override must
// preserve that.
TEST(BrowserFrameViewLayoutLinuxTest,
     NonClientExtraTopThicknessThreeForPopupWindow) {
  testing::NiceMock<MockOpaqueLayoutDelegate> mock;
  ON_CALL(mock, IsTabStripVisible()).WillByDefault(testing::Return(false));
  ON_CALL(mock, IsToolbarVisible()).WillByDefault(testing::Return(false));
  EXPECT_EQ(3, ComputeExtraTopThickness(mock));
}
