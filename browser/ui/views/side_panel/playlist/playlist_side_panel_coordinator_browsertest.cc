/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/components/playlist/common/features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/web_applications/web_app_browsertest_base.h"
#include "chrome/browser/web_applications/test/web_app_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Opens a new popup window from |web_contents| on |target_url| and returns
// the Browser it opened in.
Browser* OpenPopup(content::WebContents* web_contents, const GURL& target_url) {
  ui_test_utils::BrowserCreatedObserver browser_change_observer;
  content::TestNavigationObserver nav_observer(target_url);
  nav_observer.StartWatchingNewWebContents();

  std::string script = "window.open('" + target_url.spec() +
                       "', 'popup', 'width=400 height=400');";
  EXPECT_TRUE(content::ExecJs(web_contents, script));
  nav_observer.Wait();

  return browser_change_observer.Wait();
}

}  // namespace

class PlaylistCoordinatorBrowserTest : public web_app::WebAppBrowserTestBase {
 public:
  PlaylistCoordinatorBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(playlist::features::kPlaylist);
  }

  PlaylistCoordinatorBrowserTest(const PlaylistCoordinatorBrowserTest&) =
      delete;
  PlaylistCoordinatorBrowserTest& operator=(
      const PlaylistCoordinatorBrowserTest&) = delete;

  ~PlaylistCoordinatorBrowserTest() override = default;

 protected:
  Browser* InstallPWA(const GURL& start_url) {
    auto web_app_info =
        web_app::WebAppInstallInfo::CreateWithStartUrlForTesting(start_url);
    web_app_info->scope = start_url.GetWithoutFilename();
    web_app_info->user_display_mode =
        web_app::mojom::UserDisplayMode::kStandalone;

    webapps::AppId app_id = InstallWebApp(std::move(web_app_info));

    ui_test_utils::UrlLoadObserver url_observer(start_url);
    auto* app_browser = LaunchWebAppBrowser(app_id);
    url_observer.Wait();

    CHECK(app_browser);
    CHECK(app_browser != browser());

    CHECK(app_browser->app_controller());
    return app_browser;
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(PlaylistCoordinatorBrowserTest,
                       PlaylistSidePanelCoordinatorCreatedInNormalBrowser) {
  EXPECT_TRUE(browser()->GetFeatures().playlist_side_panel_coordinator());
  EXPECT_TRUE(sidebar::CanUseSidebar(browser()));
}

IN_PROC_BROWSER_TEST_F(PlaylistCoordinatorBrowserTest,
                       ControllerNotCreatedInPopup) {
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  Browser* popup = OpenPopup(browser_view->GetActiveWebContents(),
                             GURL("http://example.com"));
  EXPECT_TRUE(popup);

  BrowserView* popup_view = BrowserView::GetBrowserViewForBrowser(popup);

  // The popup should be in a new window.
  EXPECT_NE(browser_view, popup_view);

  // Popups are not the normal browser view.
  EXPECT_FALSE(popup_view->GetIsNormalType());
  EXPECT_TRUE(popup->is_type_popup());
  EXPECT_FALSE(sidebar::CanUseSidebar(popup));
  EXPECT_FALSE(popup->GetFeatures().playlist_side_panel_coordinator());
}

IN_PROC_BROWSER_TEST_F(PlaylistCoordinatorBrowserTest,
                       ControllerNotCreatedForDesktopPWA) {
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  ASSERT_TRUE(embedded_test_server()->Start());

  const GURL url = https_server()->GetURL("app.com", "/ssl/google.html");
  auto* app_browser = InstallPWA(url);

  EXPECT_TRUE(app_browser);

  BrowserView* app_view = BrowserView::GetBrowserViewForBrowser(app_browser);
  EXPECT_NE(app_view, browser_view);

  EXPECT_FALSE(app_view->GetIsNormalType());
  EXPECT_TRUE(app_browser->is_type_app());

  EXPECT_FALSE(app_view->GetIsNormalType());
  EXPECT_TRUE(app_browser->is_type_app());
  EXPECT_FALSE(sidebar::CanUseSidebar(app_browser));
  EXPECT_FALSE(app_browser->GetFeatures().playlist_side_panel_coordinator());
}
