/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "net/http/http_status_code.h"

using BraveWaybackMachineTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveWaybackMachineTest, DialogLaunchTest) {
  auto* model = browser()->tab_strip_model();
  auto* contents = model->GetActiveWebContents();
  BraveWaybackMachineTabHelper* tab_helper =
      BraveWaybackMachineTabHelper::FromWebContents(contents);
  EXPECT_FALSE(tab_helper->ShouldShowWaybackMachineDialog(net::HTTP_OK));
  EXPECT_TRUE(tab_helper->ShouldShowWaybackMachineDialog(net::HTTP_NOT_FOUND));
  tab_helper->ShowWaybackMachineDialog();

  // Check dialog is launched.
  EXPECT_TRUE(!!tab_helper->active_dialog());
}
