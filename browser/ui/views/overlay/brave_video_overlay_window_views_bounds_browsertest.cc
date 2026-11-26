// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <cstdlib>

#include "base/location.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/overlay/video_overlay_window_views.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/picture_in_picture_window_controller.h"
#include "content/public/browser/video_picture_in_picture_window_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/mojom/window_open_disposition.mojom.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size_conversions.h"

namespace {

constexpr char kPipPage[] = "/media/picture-in-picture/window-size.html";

// Restored sizes are re-fit to the video's aspect ratio, which can introduce
// rounding differences.
constexpr int kSizeTolerance = 2;

void ExpectSizeNear(
    const gfx::Size& expected,
    const gfx::Size& actual,
    const base::Location& location = base::Location::Current()) {
  SCOPED_TRACE(location.ToString());
  EXPECT_LE(std::abs(expected.width() - actual.width()), kSizeTolerance)
      << "expected " << expected.ToString() << ", got " << actual.ToString();
  EXPECT_LE(std::abs(expected.height() - actual.height()), kSizeTolerance)
      << "expected " << expected.ToString() << ", got " << actual.ToString();
}

}  // namespace

class BraveVideoOverlayWindowViewsBoundsBrowserTest
    : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    embedded_test_server()->ServeFilesFromSourceDirectory(
        GetChromeTestDataDir());
    ASSERT_TRUE(embedded_test_server()->Start());
  }

 protected:
  // Opens the test page in a new foreground tab of `browser`.
  content::WebContents* OpenTab(BrowserWindowInterface* browser) {
    if (!ui_test_utils::NavigateToURLWithDisposition(
            browser, embedded_test_server()->GetURL(kPipPage),
            ui::mojom::WindowOpenDisposition::kNewForegroundTab,
            ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP)) {
      return nullptr;
    }
    return browser->GetTabStripModel()->GetActiveWebContents();
  }

  void EnterPictureInPicture(content::WebContents* web_contents) {
    ASSERT_EQ(true, content::EvalJs(web_contents, "enterPictureInPicture();"));
    ASSERT_TRUE(GetOverlayWindow(web_contents)->IsVisible());
  }

  VideoOverlayWindowViews* GetOverlayWindow(
      content::WebContents* web_contents) {
    return static_cast<VideoOverlayWindowViews*>(
        content::PictureInPictureWindowController::
            GetOrCreateVideoPictureInPictureController(web_contents)
                ->GetWindowForTesting());
  }

  void ExitPictureInPicture(content::WebContents* web_contents) {
    content::TitleWatcher title_watcher(web_contents, u"leavepictureinpicture");
    ASSERT_TRUE(content::ExecJs(web_contents, "exitPictureInPicture();"));
    ASSERT_EQ(u"leavepictureinpicture", title_watcher.WaitAndGetTitle());
    ASSERT_FALSE(GetOverlayWindow(web_contents)->IsVisible());
  }

  static gfx::Rect GetWorkArea(VideoOverlayWindowViews* window) {
    return display::Screen::Get()
        ->GetDisplayNearestWindow(window->GetNativeWindow())
        .work_area();
  }

  // Moves `window` near the top-left corner of its work area and grows it,
  // keeping its aspect ratio so the size survives being re-fit to the video.
  static gfx::Rect MoveAndResize(VideoOverlayWindowViews* window) {
    const gfx::Rect work_area = GetWorkArea(window);
    const gfx::Size size =
        gfx::ScaleToRoundedSize(window->GetBounds().size(), 1.25f);
    const gfx::Rect bounds(work_area.x() + 40, work_area.y() + 60, size.width(),
                           size.height());
    window->SetBounds(bounds);
    return window->GetBounds();
  }
};

// Reopening Picture-in-Picture from a different tab reuses the bounds from the
// previous Picture-in-Picture window instead of resetting to the defaults.
IN_PROC_BROWSER_TEST_F(BraveVideoOverlayWindowViewsBoundsBrowserTest,
                       BoundsAreSharedAcrossTabs) {
  content::WebContents* first_tab = OpenTab(browser());
  ASSERT_TRUE(first_tab);
  ASSERT_NO_FATAL_FAILURE(EnterPictureInPicture(first_tab));
  VideoOverlayWindowViews* first_window = GetOverlayWindow(first_tab);
  const gfx::Rect default_bounds = first_window->GetBounds();

  const gfx::Rect custom_bounds = MoveAndResize(first_window);
  ASSERT_NE(default_bounds, custom_bounds);
  ASSERT_NO_FATAL_FAILURE(ExitPictureInPicture(first_tab));

  content::WebContents* second_tab = OpenTab(browser());
  ASSERT_TRUE(second_tab);
  ASSERT_NE(first_tab, second_tab);
  ASSERT_NO_FATAL_FAILURE(EnterPictureInPicture(second_tab));
  VideoOverlayWindowViews* second_window = GetOverlayWindow(second_tab);
  ASSERT_NE(first_window, second_window);

  const gfx::Rect restored_bounds = second_window->GetBounds();
  EXPECT_EQ(custom_bounds.origin(), restored_bounds.origin());
  ExpectSizeNear(custom_bounds.size(), restored_bounds.size());
}

// Closing the tab while Picture-in-Picture is showing still saves the bounds.
IN_PROC_BROWSER_TEST_F(BraveVideoOverlayWindowViewsBoundsBrowserTest,
                       BoundsAreSavedWhenTabIsClosed) {
  content::WebContents* first_tab = OpenTab(browser());
  ASSERT_TRUE(first_tab);
  ASSERT_NO_FATAL_FAILURE(EnterPictureInPicture(first_tab));
  const gfx::Rect custom_bounds = MoveAndResize(GetOverlayWindow(first_tab));

  content::WebContentsDestroyedWatcher destroyed_watcher(first_tab);
  browser()->GetTabStripModel()->CloseWebContents(first_tab,
                                                  TabCloseTypes::CLOSE_NONE);
  destroyed_watcher.Wait();

  content::WebContents* second_tab = OpenTab(browser());
  ASSERT_TRUE(second_tab);
  ASSERT_NO_FATAL_FAILURE(EnterPictureInPicture(second_tab));
  const gfx::Rect restored_bounds = GetOverlayWindow(second_tab)->GetBounds();
  EXPECT_EQ(custom_bounds.origin(), restored_bounds.origin());
  ExpectSizeNear(custom_bounds.size(), restored_bounds.size());
}

// Reopening Picture-in-Picture from the same tab still keeps its bounds.
IN_PROC_BROWSER_TEST_F(BraveVideoOverlayWindowViewsBoundsBrowserTest,
                       BoundsAreKeptInSameTab) {
  content::WebContents* tab = OpenTab(browser());
  ASSERT_TRUE(tab);
  ASSERT_NO_FATAL_FAILURE(EnterPictureInPicture(tab));
  const gfx::Rect custom_bounds = MoveAndResize(GetOverlayWindow(tab));
  ASSERT_NO_FATAL_FAILURE(ExitPictureInPicture(tab));

  ASSERT_NO_FATAL_FAILURE(EnterPictureInPicture(tab));
  const gfx::Rect restored_bounds = GetOverlayWindow(tab)->GetBounds();
  EXPECT_EQ(custom_bounds.origin(), restored_bounds.origin());
  ExpectSizeNear(custom_bounds.size(), restored_bounds.size());
}

// Private windows don't reuse bounds from regular windows, and vice versa.
IN_PROC_BROWSER_TEST_F(BraveVideoOverlayWindowViewsBoundsBrowserTest,
                       PrivateWindowsHaveSeparateBounds) {
  content::WebContents* regular_tab = OpenTab(browser());
  ASSERT_TRUE(regular_tab);
  ASSERT_NO_FATAL_FAILURE(EnterPictureInPicture(regular_tab));
  const gfx::Size default_size =
      GetOverlayWindow(regular_tab)->GetBounds().size();
  const gfx::Rect regular_bounds = MoveAndResize(GetOverlayWindow(regular_tab));
  ASSERT_NO_FATAL_FAILURE(ExitPictureInPicture(regular_tab));

  content::WebContents* private_tab = OpenTab(CreateIncognitoBrowser());
  ASSERT_TRUE(private_tab);
  ASSERT_NO_FATAL_FAILURE(EnterPictureInPicture(private_tab));
  VideoOverlayWindowViews* private_window = GetOverlayWindow(private_tab);
  EXPECT_NE(regular_bounds.origin(), private_window->GetBounds().origin());

  // Move the private window somewhere else and close it.
  const gfx::Rect work_area = GetWorkArea(private_window);
  private_window->SetBounds(gfx::Rect(work_area.x() + 100, work_area.y() + 120,
                                      default_size.width(),
                                      default_size.height()));
  ASSERT_NE(regular_bounds.origin(), private_window->GetBounds().origin());
  ASSERT_NO_FATAL_FAILURE(ExitPictureInPicture(private_tab));

  content::WebContents* second_regular_tab = OpenTab(browser());
  ASSERT_TRUE(second_regular_tab);
  ASSERT_NO_FATAL_FAILURE(EnterPictureInPicture(second_regular_tab));
  const gfx::Rect restored_bounds =
      GetOverlayWindow(second_regular_tab)->GetBounds();
  EXPECT_EQ(regular_bounds.origin(), restored_bounds.origin());
  ExpectSizeNear(regular_bounds.size(), restored_bounds.size());
}

// Bounds that ended up partially off-screen are brought back into the work
// area when restored.
IN_PROC_BROWSER_TEST_F(BraveVideoOverlayWindowViewsBoundsBrowserTest,
                       RestoredBoundsFitInWorkArea) {
  content::WebContents* first_tab = OpenTab(browser());
  ASSERT_TRUE(first_tab);
  ASSERT_NO_FATAL_FAILURE(EnterPictureInPicture(first_tab));
  VideoOverlayWindowViews* first_window = GetOverlayWindow(first_tab);
  const gfx::Rect work_area = GetWorkArea(first_window);
  const gfx::Size size = first_window->GetBounds().size();
  first_window->SetBounds(gfx::Rect(work_area.right() - size.width() / 2,
                                    work_area.bottom() - size.height() / 2,
                                    size.width(), size.height()));
  ASSERT_NO_FATAL_FAILURE(ExitPictureInPicture(first_tab));

  content::WebContents* second_tab = OpenTab(browser());
  ASSERT_TRUE(second_tab);
  ASSERT_NO_FATAL_FAILURE(EnterPictureInPicture(second_tab));
  const gfx::Rect restored_bounds = GetOverlayWindow(second_tab)->GetBounds();
  EXPECT_TRUE(work_area.Contains(restored_bounds))
      << "work area " << work_area.ToString() << ", window "
      << restored_bounds.ToString();
}
