// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/dispatch/tab_matcher_util.h"

#include "brave/browser/ui/gestures/dispatch/matched_tab_info.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"

namespace gestures {

class TabMatcherUtilBrowserTest : public PlatformBrowserTest {
 protected:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  Profile& profile() { return *chrome_test_utils::GetProfile(this); }
};

IN_PROC_BROWSER_TEST_F(TabMatcherUtilBrowserTest,
                       ReturnsEmptyWhenNoTabMatches) {
  EXPECT_THAT(FindMatchingTabs(profile(), "*nonexistent*"), testing::IsEmpty());
}

IN_PROC_BROWSER_TEST_F(TabMatcherUtilBrowserTest, MatchesTabByUrlPattern) {
  const GURL target_url = embedded_test_server()->GetURL("/title1.html");
  content::WebContents* const expected_web_contents =
      chrome::AddAndReturnTabAt(browser(), target_url, /*index=*/-1,
                                /*foreground=*/false);
  ASSERT_TRUE(content::WaitForLoadStop(expected_web_contents));

  const MatchedTabs matched_tabs = FindMatchingTabs(profile(), "*title1*");

  ASSERT_EQ(1U, matched_tabs.size());
  EXPECT_EQ(
      expected_web_contents,
      matched_tabs.front().browser_window->GetTabStripModel()->GetWebContentsAt(
          matched_tabs.front().index));
}

IN_PROC_BROWSER_TEST_F(TabMatcherUtilBrowserTest, MatchesTabByTitlePattern) {
  const GURL target_url = embedded_test_server()->GetURL("/title2.html");
  content::WebContents* const expected_web_contents =
      chrome::AddAndReturnTabAt(browser(), target_url, /*index=*/-1,
                                /*foreground=*/false);
  ASSERT_TRUE(content::WaitForLoadStop(expected_web_contents));

  EXPECT_THAT(FindMatchingTabs(profile(), "*Awesomeness*"),
              testing::SizeIs(1U));
}

IN_PROC_BROWSER_TEST_F(TabMatcherUtilBrowserTest, MatchesTabByRegexPattern) {
  const GURL target_url = embedded_test_server()->GetURL("/title1.html");
  content::WebContents* const expected_web_contents =
      chrome::AddAndReturnTabAt(browser(), target_url, /*index=*/-1,
                                /*foreground=*/false);
  ASSERT_TRUE(content::WaitForLoadStop(expected_web_contents));

  // `$` isn't valid in a glob, so this is detected and matched as regex.
  EXPECT_THAT(FindMatchingTabs(profile(), "title1\\.html$"),
              testing::SizeIs(1U));
}

IN_PROC_BROWSER_TEST_F(TabMatcherUtilBrowserTest,
                       ReturnsEveryMatchingTabInCreationOrder) {
  const GURL first_url = embedded_test_server()->GetURL("/title1.html");
  content::WebContents* const first_web_contents =
      chrome::AddAndReturnTabAt(browser(), first_url, /*index=*/-1,
                                /*foreground=*/false);
  ASSERT_TRUE(content::WaitForLoadStop(first_web_contents));
  const GURL second_url = embedded_test_server()->GetURL("/title1.html");
  content::WebContents* const second_web_contents =
      chrome::AddAndReturnTabAt(browser(), second_url, /*index=*/-1,
                                /*foreground=*/false);
  ASSERT_TRUE(content::WaitForLoadStop(second_web_contents));

  const MatchedTabs matched_tabs = FindMatchingTabs(profile(), "*title1*");

  ASSERT_EQ(2U, matched_tabs.size());
  EXPECT_EQ(
      first_web_contents,
      matched_tabs[0].browser_window->GetTabStripModel()->GetWebContentsAt(
          matched_tabs[0].index));
  EXPECT_EQ(
      second_web_contents,
      matched_tabs[1].browser_window->GetTabStripModel()->GetWebContentsAt(
          matched_tabs[1].index));
}

IN_PROC_BROWSER_TEST_F(TabMatcherUtilBrowserTest,
                       MatchesTabsAcrossMultipleWindowsOfTheSameProfile) {
  const GURL target_url = embedded_test_server()->GetURL("/title1.html");
  BrowserWindowInterface* const second_browser_window =
      CreateBrowser(&profile());
  content::WebContents* const expected_web_contents =
      chrome::AddAndReturnTabAt(second_browser_window, target_url,
                                /*index=*/-1, /*foreground=*/false);
  ASSERT_TRUE(content::WaitForLoadStop(expected_web_contents));

  const MatchedTabs matched_tabs = FindMatchingTabs(profile(), "*title1*");

  ASSERT_EQ(1U, matched_tabs.size());
  EXPECT_EQ(
      expected_web_contents,
      matched_tabs.front().browser_window->GetTabStripModel()->GetWebContentsAt(
          matched_tabs.front().index));
}

IN_PROC_BROWSER_TEST_F(TabMatcherUtilBrowserTest,
                       ExcludesTabsBelongingToAnIncognitoProfile) {
  const GURL target_url = embedded_test_server()->GetURL("/title1.html");
  BrowserWindowInterface* const incognito_browser_window =
      CreateIncognitoBrowser(&profile());
  content::WebContents* const incognito_web_contents =
      chrome::AddAndReturnTabAt(incognito_browser_window, target_url,
                                /*index=*/-1, /*foreground=*/false);
  ASSERT_TRUE(content::WaitForLoadStop(incognito_web_contents));

  EXPECT_THAT(FindMatchingTabs(profile(), "*title1*"), testing::IsEmpty());
}

}  // namespace gestures
