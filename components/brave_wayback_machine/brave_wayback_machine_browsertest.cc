/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/page_action/brave_page_action_icon_type.h"
#include "brave/browser/ui/views/page_action/wayback_machine_action_icon_view.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "net/http/http_status_code.h"

using BraveWaybackMachineTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveWaybackMachineTest, BubbleLaunchTest) {
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* button_provider = browser_view->toolbar_button_provider();

  auto* model = browser()->tab_strip_model();
  auto* contents = model->GetActiveWebContents();
  BraveWaybackMachineTabHelper* tab_helper =
      BraveWaybackMachineTabHelper::FromWebContents(contents);
  EXPECT_FALSE(tab_helper->ShouldCheckWaybackMachine(net::HTTP_OK));
  EXPECT_TRUE(tab_helper->ShouldCheckWaybackMachine(net::HTTP_NOT_FOUND));

  auto* icon = static_cast<WaybackMachineActionIconView*>(
      button_provider->GetPageActionIconView(
          brave::kWaybackMachineActionIconType));
  EXPECT_FALSE(icon->GetVisible());

  tab_helper->SetWaybackState(WaybackState::kNeedToCheck);
  EXPECT_TRUE(icon->GetVisible());

  // Check bubble is launched.
  icon->ExecuteCommandForTesting();
  EXPECT_TRUE(!!tab_helper->active_window());
}
