// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_actions/brave_shields_toolbar_button.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/web_apps/frame_toolbar/web_app_frame_toolbar_view.h"
#include "chrome/browser/ui/views/web_apps/frame_toolbar/web_app_toolbar_button_container.h"
#include "chrome/browser/ui/web_applications/app_browser_controller.h"
#include "chrome/browser/ui/web_applications/web_app_browsertest_base.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/view_utils.h"

class PwaShieldsBrowserTest : public web_app::WebAppBrowserTestBase {
 public:
  PwaShieldsBrowserTest() = default;
  PwaShieldsBrowserTest(const PwaShieldsBrowserTest&) = delete;
  PwaShieldsBrowserTest& operator=(const PwaShieldsBrowserTest&) = delete;
  ~PwaShieldsBrowserTest() override = default;
};

IN_PROC_BROWSER_TEST_F(PwaShieldsBrowserTest, ShieldsButtonInWebAppTitleBar) {
  const webapps::AppId app_id = InstallPWA(GetInstallableAppURL());
  Browser* app_browser = LaunchWebAppBrowser(app_id);
  ASSERT_TRUE(app_browser);
  EXPECT_TRUE(web_app::AppBrowserController::IsWebApp(app_browser));

  auto* browser_view = static_cast<BraveBrowserView*>(
      BrowserView::GetBrowserViewForBrowser(app_browser));
  ASSERT_TRUE(browser_view);
  ASSERT_TRUE(browser_view->web_app_frame_toolbar_for_testing());
  auto* right = browser_view->web_app_frame_toolbar_for_testing()
                    ->get_right_container_for_testing();
  ASSERT_TRUE(right);

  BraveShieldsToolbarButton* shields = nullptr;
  for (const auto& child : right->children()) {
    if (auto* b = views::AsViewClass<BraveShieldsToolbarButton>(child.get())) {
      shields = b;
      break;
    }
  }
  ASSERT_NE(shields, nullptr);
  EXPECT_TRUE(shields->GetVisible());
}
