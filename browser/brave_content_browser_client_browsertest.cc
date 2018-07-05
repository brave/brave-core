/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"

using BraveContentBrowserClientTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest, CanLoadChromeURL) {
  GURL chrome_settings_url("chrome://settings/");
  std::vector<GURL> urls {
    chrome_settings_url,
    GURL("about:settings/"),
    GURL("brave://settings/")
  };
  std::for_each(urls.begin(), urls.end(), [this, &chrome_settings_url](const GURL& url) {
    content::WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();
    ui_test_utils::NavigateToURL(browser(), url);
    ASSERT_TRUE(WaitForLoadStop(contents));
    EXPECT_STREQ(contents->GetLastCommittedURL().spec().c_str(),
        chrome_settings_url.spec().c_str());
  });
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest, CanLoadCustomBravePages) {
  std::vector<GURL> urls {
    GURL("chrome://adblock/"),
    GURL("chrome://rewards/"),
    GURL("chrome://welcome/")
  };
  std::for_each(urls.begin(), urls.end(), [this](const GURL& url) {
    content::WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();
    ui_test_utils::NavigateToURL(browser(), url);
    ASSERT_TRUE(WaitForLoadStop(contents));
    EXPECT_STREQ(contents->GetLastCommittedURL().spec().c_str(),
        url.spec().c_str());
  });
}
