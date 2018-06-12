/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"

IN_PROC_BROWSER_TEST_F(InProcessBrowserTest, CanLoadChromeURL) {
  GURL brave_settings_url("brave://settings/");
  std::vector<GURL> urls {
    GURL("chrome://settings/"),
    GURL("about:settings/"),
    brave_settings_url
  };
  std::for_each(urls.begin(), urls.end(), [this, &brave_settings_url](const GURL& url) {
    content::WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();
    ui_test_utils::NavigateToURL(browser(), url);
    ASSERT_TRUE(WaitForLoadStop(contents));
    EXPECT_STREQ(contents->GetLastCommittedURL().spec().c_str(),
        brave_settings_url.spec().c_str());
  });
}
