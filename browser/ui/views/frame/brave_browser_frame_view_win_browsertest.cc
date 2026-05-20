/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/views/frame/browser_caption_button_container_win.h"
#include "chrome/browser/ui/views/frame/browser_frame_view_win.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/top_container_view.h"
#include "chrome/browser/win/titlebar_config.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "ui/views/view_utils.h"

// Regression tests for https://github.com/brave/brave-browser/issues/55406.
//
// `BrowserFrameViewWin::LayoutCaptionButtons()` positions the caption button
// container so its bottom aligns with `top_container` top and its top sits at
// `WindowTopY()` below the window top. Before the fix,
// `BraveBrowserFrameViewWin::LayoutCaptionButtons()` applied an extra
// `tabs::GetHorizontalTabControlsDelta()` Y-offset (negative in every mode)
// that shifted the container upward and broke both alignments.

namespace {

class BraveBrowserFrameViewWinTest : public InProcessBrowserTest {
 public:
  BraveBrowserFrameViewWinTest() = default;

  BrowserView* browser_view() {
    return BrowserView::GetBrowserViewForBrowser(browser());
  }

  BrowserFrameViewWin* win_frame_view() {
    return views::AsViewClass<BrowserFrameViewWin>(
        browser_view()->browser_widget()->GetFrameView());
  }
};

// Parameterized fixture: bool param = compact horizontal tabs enabled.
class BraveBrowserFrameViewWinLayoutTest
    : public BraveBrowserFrameViewWinTest,
      public testing::WithParamInterface<bool> {
 public:
  BraveBrowserFrameViewWinLayoutTest() {
    if (GetParam()) {
      scoped_feature_list_.InitAndEnableFeature(
          tabs::kBraveCompactHorizontalTabs);
    }
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Verifies that after layout the caption button container:
//   1. Has its bottom aligned with the top of `top_container` (tab/toolbar row
//      top). The pre-fix code added a negative delta to the container's Y,
//      moving it upward so its bottom fell short of the tab strip top.
//   2. Has its top at window-top + WindowTopY() (the non-client border
//      thickness for the current window state).
IN_PROC_BROWSER_TEST_P(BraveBrowserFrameViewWinLayoutTest,
                       CaptionButtonContainerAlignedWithTopContainerAndBorder) {
  auto* frame = win_frame_view();
  ASSERT_TRUE(frame) << "Expected BrowserFrameViewWin; wrong frame type";

  if (!ShouldBrowserCustomDrawTitlebar(browser_view())) {
    GTEST_SKIP() << "Custom titlebar not drawn (e.g. Mica enabled); "
                    "caption button position is managed by the OS";
  }

  ASSERT_FALSE(browser_view()->GetWidget()->IsMaximized())
      << "Test requires a restored (non-maximized) window; "
         "border thickness is 0 when maximized";

  RunScheduledLayouts();

  const auto* container = frame->caption_button_container_for_testing();
  ASSERT_TRUE(container);

  const gfx::Rect container_screen = container->GetBoundsInScreen();
  const gfx::Rect top_container_screen =
      browser_view()->top_container()->GetBoundsInScreen();
  const gfx::Rect window_screen = frame->GetWidget()->GetWindowBoundsInScreen();

  // 1. Container bottom must touch the tab/toolbar row top.
  EXPECT_EQ(container_screen.bottom(), top_container_screen.y())
      << "Caption button container bottom (" << container_screen.bottom()
      << ") must equal top_container top (" << top_container_screen.y()
      << ") in " << (GetParam() ? "compact" : "default") << " mode";

  // 2. Container top must be at window top + WindowTopY().
  EXPECT_EQ(container_screen.y(), window_screen.y() + frame->WindowTopY())
      << "Caption button container top (" << container_screen.y()
      << ") must be window top (" << window_screen.y() << ") + WindowTopY ("
      << frame->WindowTopY() << ") in " << (GetParam() ? "compact" : "default")
      << " mode";
}

INSTANTIATE_TEST_SUITE_P(,
                         BraveBrowserFrameViewWinLayoutTest,
                         testing::Bool(),
                         [](const testing::TestParamInfo<bool>& info) {
                           return info.param ? "Compact" : "Default";
                         });

}  // namespace
