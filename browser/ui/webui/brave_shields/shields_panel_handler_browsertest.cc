/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_shields/shields_panel_handler.h"

#include "base/test/run_until.h"
#include "brave/browser/ui/views/brave_actions/brave_shields_action_view.h"
#include "brave/browser/ui/views/brave_actions/brave_shields_toolbar_button.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/views/interaction/browser_elements_views.h"
#include "chrome/browser/ui/web_applications/web_app_browsertest_base.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "ui/views/controls/button/button_controller.h"
#include "ui/views/view_utils.h"
#include "ui/views/widget/widget.h"

class ShieldsPanelHandlerBrowserTest : public web_app::WebAppBrowserTestBase {
 protected:
  template <typename ShieldsView>
  ShieldsView* FindShieldsView(BrowserWindowInterface* browser) {
    ShieldsView* found = nullptr;
    EXPECT_TRUE(base::test::RunUntil([&] {
      for (views::View* view : BrowserElementsViews::From(browser)->GetAllViews(
               BraveShieldsActionView::kShieldsActionIcon)) {
        if ((found = views::AsViewClass<ShieldsView>(view))) {
          return true;
        }
      }
      return false;
    }));
    return found;
  }

  template <typename ShieldsView>
  void ExpectBubbleFoundAfterClicking(BrowserWindowInterface* browser) {
    ShieldsView* shields_view = FindShieldsView<ShieldsView>(browser);
    ASSERT_TRUE(shields_view);
    EXPECT_FALSE(ShieldsPanelHandler::GetShieldsBubbleWidget(*browser));

    // The press is handled by the views' menu button controller.
    shields_view->button_controller()->NotifyClick();
    ASSERT_TRUE(base::test::RunUntil(
        [&] { return shields_view->GetBubbleWidget() != nullptr; }));
    EXPECT_EQ(shields_view->GetBubbleWidget(),
              ShieldsPanelHandler::GetShieldsBubbleWidget(*browser));
  }
};

IN_PROC_BROWSER_TEST_F(ShieldsPanelHandlerBrowserTest,
                       FindsBubbleOfLocationBarIcon) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GetInstallableAppURL()));
  ExpectBubbleFoundAfterClicking<BraveShieldsActionView>(browser());
}

#if BUILDFLAG(IS_MAC)
// Web app windows are flaky on Mac CI, as in pwa_shields_browsertest.cc.
#define MAYBE_FindsBubbleOfWebAppTitleBarButton \
  DISABLED_FindsBubbleOfWebAppTitleBarButton
#else
#define MAYBE_FindsBubbleOfWebAppTitleBarButton \
  FindsBubbleOfWebAppTitleBarButton
#endif

// Web app windows also have the location bar icon, with the same identifier.
IN_PROC_BROWSER_TEST_F(ShieldsPanelHandlerBrowserTest,
                       MAYBE_FindsBubbleOfWebAppTitleBarButton) {
  const webapps::AppId app_id = InstallPWA(GetInstallableAppURL());
  BrowserWindowInterface* app_browser = LaunchWebAppBrowser(app_id);
  ASSERT_TRUE(app_browser);
  ExpectBubbleFoundAfterClicking<BraveShieldsToolbarButton>(app_browser);
}
