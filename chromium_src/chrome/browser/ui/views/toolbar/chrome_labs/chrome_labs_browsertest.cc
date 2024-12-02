/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/ui/views/toolbar/chrome_labs/chrome_labs_browsertest.cc"

using ChromeLabsModelBrowserTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(ChromeLabsModelBrowserTest, NoChromeLabsModel) {
  EXPECT_FALSE(BrowserView::GetBrowserViewForBrowser(browser())
                   ->toolbar()
                   ->chrome_labs_model());
}
