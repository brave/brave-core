/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "url/gurl.h"

class FencedFrameDisabledTest : public InProcessBrowserTest {};

IN_PROC_BROWSER_TEST_F(FencedFrameDisabledTest,
                       CreatingFencedFrameDoesNotKillTab) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), GURL("data:text/html,<body></body>")));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));

  EXPECT_EQ(true, content::EvalJs(
                      contents,
                      "new Promise(resolve => {"
                      "  document.body.appendChild("
                      "      document.createElement('fencedframe'));"
                      "  setTimeout(() => resolve("
                      "      !!document.querySelector('fencedframe')), 1000);"
                      "})"));

  EXPECT_FALSE(contents->IsCrashed());
  EXPECT_EQ(4, content::EvalJs(contents, "2 + 2"));
}
