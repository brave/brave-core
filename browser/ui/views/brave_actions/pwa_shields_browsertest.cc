// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/test/run_until.h"
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

#if BUILDFLAG(IS_MAC)
// Flaky on Mac CI
#define MAYBE_ShieldsButtonInWebAppTitleBar \
  DISABLED_ShieldsButtonInWebAppTitleBar
#else
#define MAYBE_ShieldsButtonInWebAppTitleBar ShieldsButtonInWebAppTitleBar
#endif

IN_PROC_BROWSER_TEST_F(PwaShieldsBrowserTest,
                       MAYBE_ShieldsButtonInWebAppTitleBar) {
  const webapps::AppId app_id = InstallPWA(GetInstallableAppURL());
  Browser* app_browser = LaunchWebAppBrowser(app_id);
  ASSERT_TRUE(app_browser);
  ASSERT_TRUE(web_app::AppBrowserController::IsWebApp(app_browser));

  ASSERT_TRUE(BrowserView::GetBrowserViewForBrowser(app_browser));

  auto* elements = BrowserElementsViews::From(app_browser);
  ASSERT_TRUE(elements);
  views::View* shields = nullptr;
  ASSERT_TRUE(base::test::RunUntil([&] {
    shields = elements->GetView(BraveShieldsActionView::kShieldsActionIcon,
                                /*require_visible=*/true);
    return shields != nullptr;
  }));
  ASSERT_NE(shields, nullptr);
  EXPECT_TRUE(views::IsViewClass<BraveShieldsToolbarButton>(shields));
  EXPECT_TRUE(shields->GetVisible());
}

#if BUILDFLAG(IS_MAC)
// Flaky on Mac CI
#define MAYBE_ShieldsButtonNotDuplicatedWhenToolbarReparented \
  DISABLED_ShieldsButtonNotDuplicatedWhenToolbarReparented
#else
#define MAYBE_ShieldsButtonNotDuplicatedWhenToolbarReparented \
  ShieldsButtonNotDuplicatedWhenToolbarReparented
#endif

// Regression test for https://github.com/brave/brave-browser/issues/57349:
// macOS immersive fullscreen reparents the web app toolbar (moving it into a
// separate overlay widget), which re-triggers
// WebAppToolbarButtonContainer::AddedToWidget(). In this case, we should not
// add multiple PWA Shields toolbar buttons to the web app window. Note that
// we are using the same id for the Shield page action view and toolbar button,
// so the number of views retrieved by BrowserElementsViews::GetAllViews()
// should remain 2 (1 for the page action view, 1 for the toolbar button) even
// after the reparenting.
IN_PROC_BROWSER_TEST_F(PwaShieldsBrowserTest,
                       MAYBE_ShieldsButtonNotDuplicatedWhenToolbarReparented) {
  const webapps::AppId app_id = InstallPWA(GetInstallableAppURL());
  Browser* app_browser = LaunchWebAppBrowser(app_id);
  ASSERT_TRUE(app_browser);

  auto* elements = BrowserElementsViews::From(app_browser);
  ASSERT_TRUE(elements);
  views::View* shields = nullptr;
  ASSERT_TRUE(base::test::RunUntil([&] {
    shields = elements->GetView(BraveShieldsActionView::kShieldsActionIcon,
                                /*require_visible=*/true);
    return shields != nullptr;
  }));
  ASSERT_TRUE(views::IsViewClass<BraveShieldsToolbarButton>(shields));

  views::View* container = shields->parent();
  ASSERT_TRUE(container);
  views::View* container_parent = container->parent();
  ASSERT_TRUE(container_parent);

  // 1 for page action view, 1 for toolbar button
  ASSERT_EQ(2u, elements
                    ->GetAllViews(BraveShieldsActionView::kShieldsActionIcon,
                                  /*require_visible=*/false)
                    .size());

  // Simulate the transient hidden state and the widget reparenting that
  // immersive fullscreen performs on macOS - which triggers AddedToWidget
  shields->SetVisible(false);
  container_parent->RemoveChildView(container);
  container_parent->AddChildView(container);
  shields->SetVisible(true);

  // Shouldn't add another action icon.
  EXPECT_EQ(2u, elements
                    ->GetAllViews(BraveShieldsActionView::kShieldsActionIcon,
                                  /*require_visible=*/false)
                    .size());
}
