// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>
#include <vector>

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/overlay/video_overlay_window_views.h"
#include "chrome/browser/ui/web_applications/test/web_app_browsertest_util.h"
#include "chrome/browser/web_applications/test/os_integration_test_override_impl.h"
#include "chrome/browser/web_applications/test/web_app_install_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/picture_in_picture_window_controller.h"
#include "content/public/browser/video_picture_in_picture_window_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/gfx/x/connection.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/views/widget/widget.h"

namespace {

// The window's WM_CLASS property: res_name and res_class, each NUL-terminated.
std::string GetWmClass(gfx::NativeWindow native_window) {
  const auto window = static_cast<x11::Window>(
      native_window->GetHost()->GetAcceleratedWidget());
  std::vector<char> wm_class;
  EXPECT_TRUE(x11::Connection::Get()->GetArrayProperty(
      window, x11::Atom::WM_CLASS, &wm_class));
  return std::string(wm_class.begin(), wm_class.end());
}

}  // namespace

class BraveVideoOverlayWindowViewsBrowserTest : public InProcessBrowserTest {
 private:
  web_app::OsIntegrationTestOverrideBlockingRegistration faked_os_integration_;
};

// A web app's PiP window must carry the app window's WM_CLASS, so the window
// manager groups the two under the app's .desktop entry.
IN_PROC_BROWSER_TEST_F(BraveVideoOverlayWindowViewsBrowserTest,
                       WebAppPipWindowSharesAppWmClass) {
  if (ui::OzonePlatform::GetPlatformNameForTest() != "x11") {
    GTEST_SKIP() << "WM_CLASS is an X11 property";
  }

  embedded_test_server()->ServeFilesFromSourceDirectory(GetChromeTestDataDir());
  ASSERT_TRUE(embedded_test_server()->Start());
  const webapps::AppId app_id = web_app::test::InstallDummyWebApp(
      browser()->GetProfile(), "PiP app",
      embedded_test_server()->GetURL(
          "/media/picture-in-picture/window-size.html"));
  BrowserWindowInterface* app_browser =
      web_app::LaunchWebAppBrowserAndWait(browser()->GetProfile(), app_id);
  ASSERT_TRUE(app_browser);

  content::WebContents* web_contents =
      app_browser->GetTabStripModel()->GetActiveWebContents();
  content::VideoPictureInPictureWindowController* pip_controller =
      content::PictureInPictureWindowController::
          GetOrCreateVideoPictureInPictureController(web_contents);
  ASSERT_EQ(true, content::EvalJs(web_contents, "enterPictureInPicture();"));
  ASSERT_TRUE(pip_controller->GetWindowForTesting());

  const std::string app_wm_class =
      GetWmClass(BrowserView::GetBrowserViewForBrowser(app_browser)
                     ->GetWidget()
                     ->GetNativeWindow());
  ASSERT_FALSE(app_wm_class.empty());
  EXPECT_EQ(app_wm_class, GetWmClass(static_cast<VideoOverlayWindowViews*>(
                                         pip_controller->GetWindowForTesting())
                                         ->GetNativeWindow()));
}
