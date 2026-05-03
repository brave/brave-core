// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_actions/brave_shields_action_view.h"
#include "brave/browser/ui/views/brave_actions/brave_shields_toolbar_button.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/interaction/browser_elements_views.h"
#include "chrome/browser/ui/web_applications/app_browser_controller.h"
#include "chrome/browser/ui/web_applications/web_app_browsertest_base.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

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
  ASSERT_TRUE(web_app::AppBrowserController::IsWebApp(app_browser));

  ASSERT_TRUE(BrowserView::GetBrowserViewForBrowser(app_browser));

  auto* elements = BrowserElementsViews::From(app_browser);
  ASSERT_TRUE(elements);
  auto* shields = elements->GetViewAs<BraveShieldsToolbarButton>(
      BraveShieldsActionView::kShieldsActionIcon, /*require_visible=*/true);
  ASSERT_NE(shields, nullptr);
  EXPECT_TRUE(shields->GetVisible());
}
