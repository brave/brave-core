// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_action_dispatcher.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/run_loop.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/actions/test/gesture_action_test_util.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

namespace gestures {

namespace {

// `DispatchActionsAsync` posts a single task for a one-action list (every
// test here passes a single action); posting a task after it and waiting
// for that instead guarantees the dispatched action already ran, without
// relying on `RunUntilIdle` to drain the whole queue.
void RunDispatcherAndWait(GestureActionDispatcher& action_dispatcher,
                          std::vector<GestureActionInfo> actions) {
  action_dispatcher.DispatchActionsAsync(std::move(actions));
  base::RunLoop run_loop;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, run_loop.QuitClosure());
  run_loop.Run();
}

}  // namespace

class GestureActionDispatcherBrowserTest : public PlatformBrowserTest {
 public:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->Start());

    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->AddDefaultHandlers(GetChromeTestDataDir());
    ASSERT_TRUE(https_server_->Start());
  }

  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

 protected:
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       BackDoesNothingWithNoBackHistory) {
  const GURL url = embedded_test_server()->GetURL("/title1.html");
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(web_contents(), url));

  GestureActionDispatcher action_dispatcher(
      *chrome_test_utils::GetActiveTab(this));
  RunDispatcherAndWait(action_dispatcher, {MakeGestureAction(IDC_BACK)});

  EXPECT_EQ(url, web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ForwardDoesNothingWithNoForwardHistory) {
  const GURL url = embedded_test_server()->GetURL("/title1.html");
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(web_contents(), url));

  GestureActionDispatcher action_dispatcher(
      *chrome_test_utils::GetActiveTab(this));
  RunDispatcherAndWait(action_dispatcher, {MakeGestureAction(IDC_FORWARD)});

  EXPECT_EQ(url, web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       BackNavigatesWhenHistoryExists) {
  const GURL first_url = embedded_test_server()->GetURL("/title1.html");
  const GURL second_url = embedded_test_server()->GetURL("/title2.html");
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(web_contents(), first_url));
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(web_contents(), second_url));

  GestureActionDispatcher action_dispatcher(
      *chrome_test_utils::GetActiveTab(this));
  content::TestNavigationObserver navigation_observer(web_contents());
  action_dispatcher.DispatchActionsAsync({MakeGestureAction(IDC_BACK)});
  navigation_observer.Wait();

  EXPECT_EQ(first_url, web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ForwardNavigatesWhenHistoryExists) {
  const GURL first_url = embedded_test_server()->GetURL("/title1.html");
  const GURL second_url = embedded_test_server()->GetURL("/title2.html");
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(web_contents(), first_url));
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(web_contents(), second_url));
  content::TestNavigationObserver back_observer(web_contents());
  web_contents()->GetController().GoBack();
  back_observer.Wait();
  ASSERT_EQ(first_url, web_contents()->GetLastCommittedURL());

  GestureActionDispatcher action_dispatcher(
      *chrome_test_utils::GetActiveTab(this));
  content::TestNavigationObserver navigation_observer(web_contents());
  action_dispatcher.DispatchActionsAsync({MakeGestureAction(IDC_FORWARD)});
  navigation_observer.Wait();

  EXPECT_EQ(second_url, web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       GenericBrowserCommandRunsMappedIdcCommand) {
  const int tab_count_before = browser()->tab_strip_model()->count();

  GestureActionDispatcher action_dispatcher(
      *chrome_test_utils::GetActiveTab(this));
  RunDispatcherAndWait(action_dispatcher, {MakeGestureAction(IDC_NEW_TAB)});

  EXPECT_EQ(tab_count_before + 1, browser()->tab_strip_model()->count());
}

// `ShowSettingsSubPage` may reuse the current tab, an already-open settings
// tab, or open a brand new one, so the observer below watches for the
// target URL across every tab rather than binding to a specific
// `WebContents` captured before the dispatch.
IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       OpenSettingsNavigatesToTheRequestedSubPage) {
  GestureActionDispatcher action_dispatcher(
      *chrome_test_utils::GetActiveTab(this));
  content::TestNavigationObserver navigation_observer(
      GURL("chrome://settings/appearance"));
  navigation_observer.WatchExistingWebContents();
  navigation_observer.StartWatchingNewWebContents();
  action_dispatcher.DispatchActionsAsync(
      {test::MakeOpenSettingsGestureAction(OpenSettingsPage::kAppearance)});
  navigation_observer.Wait();
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       OpenSettingsWithBasicPageNavigatesToTheTopLevelPage) {
  GestureActionDispatcher action_dispatcher(
      *chrome_test_utils::GetActiveTab(this));
  content::TestNavigationObserver navigation_observer(
      GURL("chrome://settings/"));
  navigation_observer.WatchExistingWebContents();
  navigation_observer.StartWatchingNewWebContents();
  action_dispatcher.DispatchActionsAsync(
      {test::MakeOpenSettingsGestureAction(OpenSettingsPage::kBasic)});
  navigation_observer.Wait();
}

class GestureActionDispatcherDisallowedSchemeBrowserTest
    : public GestureActionDispatcherBrowserTest,
      public testing::WithParamInterface<std::pair<std::string, std::string>> {
};

IN_PROC_BROWSER_TEST_P(GestureActionDispatcherDisallowedSchemeBrowserTest,
                       NavigateToUrlIgnoresDisallowedScheme) {
  const GURL start_url = embedded_test_server()->GetURL("/title1.html");
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(web_contents(), start_url));

  GestureActionDispatcher action_dispatcher(
      *chrome_test_utils::GetActiveTab(this));
  RunDispatcherAndWait(
      action_dispatcher,
      {test::MakeNavigateToUrlGestureAction(GetParam().second)});

  EXPECT_EQ(start_url, web_contents()->GetLastCommittedURL());
}

INSTANTIATE_TEST_SUITE_P(
    DisallowedSchemes,
    GestureActionDispatcherDisallowedSchemeBrowserTest,
    testing::Values(std::make_pair("Javascript", "javascript:void(0)"),
                    std::make_pair("File", "file:///etc/passwd"),
                    std::make_pair("Data", "data:text/html,<script>1</script>"),
                    std::make_pair("Ftp", "ftp://example.com/"),
                    std::make_pair("Chrome", "chrome://settings"),
                    std::make_pair("About", "about:blank"),
                    std::make_pair("Blob", "blob:https://example.com/x")),
    [](const testing::TestParamInfo<std::pair<std::string, std::string>>&
           info) { return info.param.first; });

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest, NavigateToHttpUrl) {
  const GURL start_url = embedded_test_server()->GetURL("/title1.html");
  const GURL target_url = embedded_test_server()->GetURL("/title2.html");
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(web_contents(), start_url));

  GestureActionDispatcher action_dispatcher(
      *chrome_test_utils::GetActiveTab(this));
  content::TestNavigationObserver navigation_observer(web_contents());
  action_dispatcher.DispatchActionsAsync(
      {test::MakeNavigateToUrlGestureAction(target_url.spec())});
  navigation_observer.Wait();

  EXPECT_EQ(target_url, web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest, NavigateToHttpsUrl) {
  const GURL start_url = embedded_test_server()->GetURL("/title1.html");
  const GURL target_url = https_server_->GetURL("/title2.html");
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(web_contents(), start_url));

  GestureActionDispatcher action_dispatcher(
      *chrome_test_utils::GetActiveTab(this));
  content::TestNavigationObserver navigation_observer(web_contents());
  action_dispatcher.DispatchActionsAsync(
      {test::MakeNavigateToUrlGestureAction(target_url.spec())});
  navigation_observer.Wait();

  EXPECT_EQ(target_url, web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       NavigateToInternalBraveUrl) {
  const GURL start_url = embedded_test_server()->GetURL("/title1.html");
  const GURL requested_url("brave://settings");
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(web_contents(), start_url));

  GestureActionDispatcher action_dispatcher(
      *chrome_test_utils::GetActiveTab(this));
  content::TestNavigationObserver navigation_observer(web_contents());
  action_dispatcher.DispatchActionsAsync(
      {test::MakeNavigateToUrlGestureAction(requested_url.spec())});
  navigation_observer.Wait();

  // `brave://settings` is just an alias; the settings WebUI itself commits
  // under the `chrome:` scheme.
  EXPECT_EQ(GURL("chrome://settings/"), web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       SwitchToTabActivatesTabMatchingUrlPattern) {
  const GURL target_url = embedded_test_server()->GetURL("/title2.html");
  content::WebContents* const expected_web_contents =
      chrome::AddAndReturnTabAt(browser(), target_url, /*index=*/-1,
                                /*foreground=*/false);
  ASSERT_TRUE(content::WaitForLoadStop(expected_web_contents));
  // A second, non-matching tab is activated so the dispatch below actually
  // has to move the active tab somewhere.
  chrome::AddTabAt(browser(), embedded_test_server()->GetURL("/title3.html"),
                   /*index=*/-1, /*foreground=*/true);

  GestureActionDispatcher action_dispatcher(
      *chrome_test_utils::GetActiveTab(this));
  RunDispatcherAndWait(action_dispatcher,
                       {test::MakeSwitchToTabGestureAction("*title2*")});

  EXPECT_EQ(expected_web_contents, web_contents());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       SwitchToTabActivatesTabMatchingRegexPattern) {
  const GURL target_url = embedded_test_server()->GetURL("/title2.html");
  content::WebContents* const expected_web_contents =
      chrome::AddAndReturnTabAt(browser(), target_url, /*index=*/-1,
                                /*foreground=*/false);
  ASSERT_TRUE(content::WaitForLoadStop(expected_web_contents));
  chrome::AddTabAt(browser(), embedded_test_server()->GetURL("/title3.html"),
                   /*index=*/-1, /*foreground=*/true);

  GestureActionDispatcher action_dispatcher(
      *chrome_test_utils::GetActiveTab(this));
  // `$` isn't valid in a glob, so this is detected and matched as regex.
  RunDispatcherAndWait(action_dispatcher,
                       {test::MakeSwitchToTabGestureAction("title2\\.html$")});

  EXPECT_EQ(expected_web_contents, web_contents());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       SwitchToTabDoesNothingWithNoMatch) {
  const GURL url = embedded_test_server()->GetURL("/title1.html");
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(web_contents(), url));
  content::WebContents* const active_web_contents = web_contents();

  GestureActionDispatcher action_dispatcher(
      *chrome_test_utils::GetActiveTab(this));
  RunDispatcherAndWait(action_dispatcher,
                       {test::MakeSwitchToTabGestureAction("*nonexistent*")});

  EXPECT_EQ(active_web_contents, web_contents());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       SwitchToTabCyclesThroughMatchingTabs) {
  const GURL target_url = embedded_test_server()->GetURL("/title2.html");
  content::WebContents* const first_match_web_contents =
      chrome::AddAndReturnTabAt(browser(), target_url, /*index=*/-1,
                                /*foreground=*/false);
  content::WebContents* const second_match_web_contents =
      chrome::AddAndReturnTabAt(browser(), target_url, /*index=*/-1,
                                /*foreground=*/false);
  ASSERT_TRUE(content::WaitForLoadStop(first_match_web_contents));
  ASSERT_TRUE(content::WaitForLoadStop(second_match_web_contents));

  GestureActionDispatcher action_dispatcher(
      *chrome_test_utils::GetActiveTab(this));
  auto switch_to_tab = []() -> std::vector<GestureActionInfo> {
    return {test::MakeSwitchToTabGestureAction("*title2*")};
  };

  RunDispatcherAndWait(action_dispatcher, switch_to_tab());
  EXPECT_EQ(first_match_web_contents, web_contents());

  RunDispatcherAndWait(action_dispatcher, switch_to_tab());
  EXPECT_EQ(second_match_web_contents, web_contents());

  RunDispatcherAndWait(action_dispatcher, switch_to_tab());
  EXPECT_EQ(first_match_web_contents, web_contents());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       SwitchToTabActivatesMatchInAnotherWindow) {
  const GURL target_url = embedded_test_server()->GetURL("/title2.html");
  BrowserWindowInterface* const second_browser_window =
      CreateBrowser(chrome_test_utils::GetProfile(this));
  content::WebContents* const expected_web_contents =
      chrome::AddAndReturnTabAt(second_browser_window, target_url,
                                /*index=*/-1, /*foreground=*/false);
  ASSERT_TRUE(content::WaitForLoadStop(expected_web_contents));

  GestureActionDispatcher action_dispatcher(
      *chrome_test_utils::GetActiveTab(this));
  RunDispatcherAndWait(action_dispatcher,
                       {test::MakeSwitchToTabGestureAction("*title2*")});

  EXPECT_EQ(expected_web_contents,
            second_browser_window->GetTabStripModel()->GetActiveWebContents());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       SwitchToTabIgnoresMatchInIncognitoWindow) {
  const GURL target_url = embedded_test_server()->GetURL("/title2.html");
  BrowserWindowInterface* const incognito_browser_window =
      CreateIncognitoBrowser(chrome_test_utils::GetProfile(this));
  content::WebContents* const incognito_web_contents =
      chrome::AddAndReturnTabAt(incognito_browser_window, target_url,
                                /*index=*/-1, /*foreground=*/false);
  ASSERT_TRUE(content::WaitForLoadStop(incognito_web_contents));

  const GURL start_url = embedded_test_server()->GetURL("/title1.html");
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(web_contents(), start_url));
  content::WebContents* const active_web_contents = web_contents();

  GestureActionDispatcher action_dispatcher(
      *chrome_test_utils::GetActiveTab(this));
  RunDispatcherAndWait(action_dispatcher,
                       {test::MakeSwitchToTabGestureAction("*title2*")});

  // The incognito tab matches the pattern, but a gesture in the regular
  // profile must never surface, activate, or otherwise reveal the existence
  // of a tab in an unrelated Incognito window.
  EXPECT_EQ(active_web_contents, web_contents());
  EXPECT_NE(
      incognito_web_contents,
      incognito_browser_window->GetTabStripModel()->GetActiveWebContents());
}

}  // namespace gestures
