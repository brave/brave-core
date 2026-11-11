/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/psst/core/common/constants.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"

namespace {

// Script checks that PSST dialog appeared
constexpr char kPsstDialogCheckScript[] = R"(
    new Promise((resolve) => {
      const isRendered = () => !!document.querySelector('#root > *');
      if (isRendered()) {
        resolve(true);
        return;
      }
      const root = document.getElementById('root');
      const observer = new MutationObserver(() => {
        if (isRendered()) {
          observer.disconnect();
          resolve(true);
        }
      });
      observer.observe(root, { childList: true, subtree: true });
    })
  )";

}  // namespace

namespace psst {

class BravePsstDialogUIBrowserTest : public PlatformBrowserTest {
 public:
  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }
};

IN_PROC_BROWSER_TEST_F(BravePsstDialogUIBrowserTest,
                       DirectNavigationDoesNotCrash) {
  const GURL url(kBraveUIPsstURL);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  ASSERT_TRUE(content::WaitForLoadStop(web_contents()));

  EXPECT_EQ(true, content::EvalJs(web_contents(), kPsstDialogCheckScript));
  EXPECT_EQ(url, web_contents()->GetLastCommittedURL());
}

}  // namespace psst
