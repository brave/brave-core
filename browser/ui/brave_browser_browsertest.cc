/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"

using BraveBrowserBrowserTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveBrowserBrowserTest, NTPFaviconTest) {
  ui_test_utils::NavigateToURL(browser(), GURL("brave://newtab/"));

  auto* tab_model = browser()->tab_strip_model();
  EXPECT_FALSE(
      browser()->ShouldDisplayFavicon(tab_model->GetActiveWebContents()));
}
