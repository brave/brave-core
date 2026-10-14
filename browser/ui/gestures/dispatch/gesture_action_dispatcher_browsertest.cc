// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/dispatch/gesture_action_dispatcher.h"

#include <memory>

#include "base/test/run_until.h"
#include "brave/browser/ui/gestures/test/gesture_action_dispatcher_test_util.h"
#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/actions/smart_gesture_selected_text_search_command_param_types.h"
#include "brave/components/gestures/core/common/actions/test/gesture_action_test_util.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
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

  content::WebContents* active_web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  tabs::TabInterface& active_tab() {
    return *chrome_test_utils::GetActiveTab(this);
  }

 protected:
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ShouldNotNavigateBackWhenNoBackHistoryExists) {
  const GURL url = embedded_test_server()->GetURL("/title1.html");
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(active_web_contents(), url));

  GestureActionDispatcher action_dispatcher(active_tab());
  test::RunDispatcherAndWait(action_dispatcher, {MakeGestureAction(IDC_BACK)});

  EXPECT_EQ(url, active_web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ShouldNotNavigateForwardWhenNoForwardHistoryExists) {
  const GURL url = embedded_test_server()->GetURL("/title1.html");
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(active_web_contents(), url));

  GestureActionDispatcher action_dispatcher(active_tab());
  test::RunDispatcherAndWait(action_dispatcher,
                             {MakeGestureAction(IDC_FORWARD)});

  EXPECT_EQ(url, active_web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ShouldNavigateBackWhenHistoryExists) {
  const GURL first_url = embedded_test_server()->GetURL("/title1.html");
  ASSERT_TRUE(
      chrome_test_utils::NavigateToURL(active_web_contents(), first_url));
  const GURL second_url = embedded_test_server()->GetURL("/title2.html");
  ASSERT_TRUE(
      chrome_test_utils::NavigateToURL(active_web_contents(), second_url));

  GestureActionDispatcher action_dispatcher(active_tab());
  content::TestNavigationObserver navigation_observer(active_web_contents());
  action_dispatcher.DispatchActionsAsync(
      {MakeGestureAction(IDC_BACK)},
      /*smart_gesture_content=*/std::nullopt);
  navigation_observer.Wait();

  EXPECT_EQ(first_url, active_web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ShouldNavigateForwardWhenHistoryExists) {
  const GURL first_url = embedded_test_server()->GetURL("/title1.html");
  ASSERT_TRUE(
      chrome_test_utils::NavigateToURL(active_web_contents(), first_url));
  const GURL second_url = embedded_test_server()->GetURL("/title2.html");
  ASSERT_TRUE(
      chrome_test_utils::NavigateToURL(active_web_contents(), second_url));
  content::TestNavigationObserver back_observer(active_web_contents());
  active_web_contents()->GetController().GoBack();
  back_observer.Wait();
  ASSERT_EQ(first_url, active_web_contents()->GetLastCommittedURL());

  GestureActionDispatcher action_dispatcher(active_tab());
  content::TestNavigationObserver navigation_observer(active_web_contents());
  action_dispatcher.DispatchActionsAsync(
      {MakeGestureAction(IDC_FORWARD)},
      /*smart_gesture_content=*/std::nullopt);
  navigation_observer.Wait();

  EXPECT_EQ(second_url, active_web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ShouldExecuteMappedBrowserCommand) {
  const int tab_count_before = browser()->tab_strip_model()->count();

  GestureActionDispatcher action_dispatcher(active_tab());
  test::RunDispatcherAndWait(action_dispatcher,
                             {MakeGestureAction(IDC_NEW_TAB)});

  EXPECT_EQ(tab_count_before + 1, browser()->tab_strip_model()->count());
}

// `ShowSettingsSubPage` may reuse, reactivate, or open a new tab, so this
// watches for the URL across every tab rather than one captured up front.
IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ShouldOpenRequestedSettingsSubpage) {
  GestureActionDispatcher action_dispatcher(active_tab());
  content::TestNavigationObserver navigation_observer(
      GURL("chrome://settings/appearance"));
  navigation_observer.WatchExistingWebContents();
  navigation_observer.StartWatchingNewWebContents();
  action_dispatcher.DispatchActionsAsync(
      {test::MakeOpenSettingsPageGestureAction(
          OpenSettingsPageType::kAppearance)},
      /*smart_gesture_content=*/std::nullopt);
  navigation_observer.Wait();
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ShouldOpenMainSettingsPageWhenBasicPageIsRequested) {
  GestureActionDispatcher action_dispatcher(active_tab());
  content::TestNavigationObserver navigation_observer(
      GURL("chrome://settings/"));
  navigation_observer.WatchExistingWebContents();
  navigation_observer.StartWatchingNewWebContents();
  action_dispatcher.DispatchActionsAsync(
      {test::MakeOpenSettingsPageGestureAction(OpenSettingsPageType::kBasic)},
      /*smart_gesture_content=*/std::nullopt);
  navigation_observer.Wait();
}

struct DisallowedSchemeTestCaseInfo {
  std::string name;
  std::string url;
};

class GestureActionDispatcherDisallowedSchemeBrowserTest
    : public GestureActionDispatcherBrowserTest,
      public testing::WithParamInterface<DisallowedSchemeTestCaseInfo> {};

IN_PROC_BROWSER_TEST_P(GestureActionDispatcherDisallowedSchemeBrowserTest,
                       ShouldNotNavigateToUrl) {
  const GURL start_url = embedded_test_server()->GetURL("/title1.html");
  ASSERT_TRUE(
      chrome_test_utils::NavigateToURL(active_web_contents(), start_url));

  GestureActionDispatcher action_dispatcher(active_tab());
  test::RunDispatcherAndWait(
      action_dispatcher,
      {test::MakeNavigateToUrlGestureAction(GetParam().url)});

  EXPECT_EQ(start_url, active_web_contents()->GetLastCommittedURL());
}

INSTANTIATE_TEST_SUITE_P(
    DisallowedSchemes,
    GestureActionDispatcherDisallowedSchemeBrowserTest,
    testing::Values(
        DisallowedSchemeTestCaseInfo{"Javascript", "javascript:void(0)"},
        DisallowedSchemeTestCaseInfo{"File", "file:///etc/passwd"},
        DisallowedSchemeTestCaseInfo{"Data",
                                     "data:text/html,<script>1</script>"},
        DisallowedSchemeTestCaseInfo{"Ftp", "ftp://example.com/"},
        DisallowedSchemeTestCaseInfo{"Chrome", "chrome://settings"},
        DisallowedSchemeTestCaseInfo{"About", "about:blank"},
        DisallowedSchemeTestCaseInfo{"Blob", "blob:https://example.com/x"}),
    [](const testing::TestParamInfo<DisallowedSchemeTestCaseInfo>& info) {
      return info.param.name;
    });

IN_PROC_BROWSER_TEST_F(
    GestureActionDispatcherBrowserTest,
    ShouldWaitForNavigationToFinishBeforeRunningNextChainedAction) {
  // A second, inactive tab so the browser survives the close-tab action
  // below.
  chrome::AddTabAt(browser(), GURL("about:blank"), /*index=*/0,
                   /*foreground=*/false);
  const GURL target_url = embedded_test_server()->GetURL("/title2.html");
  content::TestNavigationManager navigation_manager(active_web_contents(),
                                                    target_url);

  GestureActionDispatcher action_dispatcher(active_tab());
  action_dispatcher.DispatchActionsAsync(
      {test::MakeNavigateToUrlGestureAction(target_url.spec()),
       MakeGestureAction(IDC_CLOSE_TAB)},
      /*smart_gesture_content=*/std::nullopt);
  ASSERT_TRUE(navigation_manager.WaitForRequestStart());

  // The close-tab action is chained after navigate-to-url; while this test
  // is deliberately holding that navigation from finishing, the close must
  // not have run yet.
  EXPECT_EQ(2, browser()->tab_strip_model()->count());

  ASSERT_TRUE(navigation_manager.WaitForNavigationFinished());

  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return browser()->tab_strip_model()->count() == 1; }));
}

IN_PROC_BROWSER_TEST_F(
    GestureActionDispatcherBrowserTest,
    ShouldNotRunRemainingActionsWhenActiveTabClosesWhileNavigationIsPending) {
  // A second, inactive tab so the browser survives closing the active one.
  chrome::AddTabAt(browser(), GURL("about:blank"), /*index=*/0,
                   /*foreground=*/false);
  const GURL target_url = embedded_test_server()->GetURL("/title2.html");
  content::TestNavigationManager navigation_manager(active_web_contents(),
                                                    target_url);

  GestureActionDispatcher action_dispatcher(active_tab());
  action_dispatcher.DispatchActionsAsync(
      {test::MakeNavigateToUrlGestureAction(target_url.spec()),
       MakeGestureAction(IDC_CLOSE_TAB)},
      /*smart_gesture_content=*/std::nullopt);
  ASSERT_TRUE(navigation_manager.WaitForRequestStart());

  // Simulates the user manually closing the tab a chained action is still
  // navigating, before that navigation has finished.
  content::WebContentsDestroyedWatcher destroyed_watcher(active_web_contents());
  chrome::CloseTab(browser());
  destroyed_watcher.Wait();
  ASSERT_EQ(1, browser()->tab_strip_model()->count());

  // The chain's own close-tab action, still pending when the tab it was
  // chained against was destroyed, must never run against whichever tab
  // happens to be active afterward.
  base::RunLoop run_loop;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, run_loop.QuitClosure());
  run_loop.Run();

  EXPECT_EQ(1, browser()->tab_strip_model()->count());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ShouldNavigateToHttpUrl) {
  const GURL start_url = embedded_test_server()->GetURL("/title1.html");
  const GURL target_url = embedded_test_server()->GetURL("/title2.html");
  ASSERT_TRUE(
      chrome_test_utils::NavigateToURL(active_web_contents(), start_url));

  GestureActionDispatcher action_dispatcher(active_tab());
  content::TestNavigationObserver navigation_observer(active_web_contents());
  action_dispatcher.DispatchActionsAsync(
      {test::MakeNavigateToUrlGestureAction(target_url.spec())},
      /*smart_gesture_content=*/std::nullopt);
  navigation_observer.Wait();

  EXPECT_EQ(target_url, active_web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ShouldNavigateToHttpsUrl) {
  const GURL start_url = embedded_test_server()->GetURL("/title1.html");
  const GURL target_url = https_server_->GetURL("/title2.html");
  ASSERT_TRUE(
      chrome_test_utils::NavigateToURL(active_web_contents(), start_url));

  GestureActionDispatcher action_dispatcher(active_tab());
  content::TestNavigationObserver navigation_observer(active_web_contents());
  action_dispatcher.DispatchActionsAsync(
      {test::MakeNavigateToUrlGestureAction(target_url.spec())},
      /*smart_gesture_content=*/std::nullopt);
  navigation_observer.Wait();

  EXPECT_EQ(target_url, active_web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ShouldNavigateToInternalBraveUrl) {
  const GURL start_url = embedded_test_server()->GetURL("/title1.html");
  const GURL requested_url("brave://settings");
  ASSERT_TRUE(
      chrome_test_utils::NavigateToURL(active_web_contents(), start_url));

  GestureActionDispatcher action_dispatcher(active_tab());
  content::TestNavigationObserver navigation_observer(active_web_contents());
  action_dispatcher.DispatchActionsAsync(
      {test::MakeNavigateToUrlGestureAction(requested_url.spec())},
      /*smart_gesture_content=*/std::nullopt);
  navigation_observer.Wait();

  // `brave://settings` is just an alias; the settings WebUI itself commits
  // under the `chrome:` scheme.
  EXPECT_EQ(GURL("chrome://settings/"),
            active_web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ShouldActivateTabMatchingUrlPattern) {
  const GURL target_url = embedded_test_server()->GetURL("/title2.html");
  content::WebContents* const expected_web_contents =
      chrome::AddAndReturnTabAt(browser(), target_url, /*index=*/-1,
                                /*foreground=*/false);
  ASSERT_TRUE(content::WaitForLoadStop(expected_web_contents));
  // A second, non-matching tab is activated so the dispatch below actually
  // has to move the active tab somewhere.
  chrome::AddTabAt(browser(), embedded_test_server()->GetURL("/title3.html"),
                   /*index=*/-1, /*foreground=*/true);

  GestureActionDispatcher action_dispatcher(active_tab());
  test::RunDispatcherAndWait(action_dispatcher,
                             {test::MakeSwitchToTabGestureAction("*title2*")});

  EXPECT_EQ(expected_web_contents, active_web_contents());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ShouldActivateTabMatchingRegexPattern) {
  const GURL target_url = embedded_test_server()->GetURL("/title2.html");
  content::WebContents* const expected_web_contents =
      chrome::AddAndReturnTabAt(browser(), target_url, /*index=*/-1,
                                /*foreground=*/false);
  ASSERT_TRUE(content::WaitForLoadStop(expected_web_contents));
  chrome::AddTabAt(browser(), embedded_test_server()->GetURL("/title3.html"),
                   /*index=*/-1, /*foreground=*/true);

  GestureActionDispatcher action_dispatcher(active_tab());
  // `$` isn't valid in a glob, so this is detected and matched as regex.
  test::RunDispatcherAndWait(
      action_dispatcher,
      {test::MakeSwitchToTabGestureAction("title2\\.html$")});

  EXPECT_EQ(expected_web_contents, active_web_contents());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ShouldNotSwitchTabsForInvalidRegexPattern) {
  const GURL target_url = embedded_test_server()->GetURL("/title2.html");
  chrome::AddAndReturnTabAt(browser(), target_url, /*index=*/-1,
                            /*foreground=*/false);
  content::WebContents* const web_contents_before_dispatch =
      active_web_contents();

  GestureActionDispatcher action_dispatcher(active_tab());
  // The unbalanced "(" makes this look like a regex (see `LooksLikeRegex`
  // in `tab_matcher_util.cc`), but it fails to compile; that must be
  // treated as no match, not a crash or an accidental match-everything.
  test::RunDispatcherAndWait(action_dispatcher,
                             {test::MakeSwitchToTabGestureAction("title2(")});

  EXPECT_EQ(web_contents_before_dispatch, active_web_contents());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ShouldNotNavigateToDisallowedScheme) {
  const GURL start_url = embedded_test_server()->GetURL("/title1.html");
  ASSERT_TRUE(
      chrome_test_utils::NavigateToURL(active_web_contents(), start_url));

  GestureActionDispatcher action_dispatcher(active_tab());
  test::RunDispatcherAndWait(
      action_dispatcher,
      {test::MakeNavigateToUrlGestureAction("javascript:alert(1)")});

  EXPECT_EQ(start_url, active_web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ShouldKeepCurrentTabWhenNoTabMatches) {
  const GURL url = embedded_test_server()->GetURL("/title1.html");
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(active_web_contents(), url));
  content::WebContents* const web_contents_before_dispatch =
      active_web_contents();

  GestureActionDispatcher action_dispatcher(active_tab());
  test::RunDispatcherAndWait(
      action_dispatcher, {test::MakeSwitchToTabGestureAction("*nonexistent*")});

  EXPECT_EQ(web_contents_before_dispatch, active_web_contents());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ShouldCycleThroughMatchingTabs) {
  const GURL target_url = embedded_test_server()->GetURL("/title2.html");
  content::WebContents* const first_match_web_contents =
      chrome::AddAndReturnTabAt(browser(), target_url, /*index=*/-1,
                                /*foreground=*/false);
  content::WebContents* const second_match_web_contents =
      chrome::AddAndReturnTabAt(browser(), target_url, /*index=*/-1,
                                /*foreground=*/false);
  ASSERT_TRUE(content::WaitForLoadStop(first_match_web_contents));
  ASSERT_TRUE(content::WaitForLoadStop(second_match_web_contents));

  GestureActionDispatcher action_dispatcher(active_tab());
  auto switch_to_tab = []() -> GestureActions {
    return {test::MakeSwitchToTabGestureAction("*title2*")};
  };

  test::RunDispatcherAndWait(action_dispatcher, switch_to_tab());
  EXPECT_EQ(first_match_web_contents, active_web_contents());

  test::RunDispatcherAndWait(action_dispatcher, switch_to_tab());
  EXPECT_EQ(second_match_web_contents, active_web_contents());

  test::RunDispatcherAndWait(action_dispatcher, switch_to_tab());
  EXPECT_EQ(first_match_web_contents, active_web_contents());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ShouldActivateMatchingTabInAnotherWindow) {
  const GURL target_url = embedded_test_server()->GetURL("/title2.html");
  BrowserWindowInterface* const second_browser_window =
      CreateBrowser(chrome_test_utils::GetProfile(this));
  content::WebContents* const expected_web_contents =
      chrome::AddAndReturnTabAt(second_browser_window, target_url,
                                /*index=*/-1, /*foreground=*/false);
  ASSERT_TRUE(content::WaitForLoadStop(expected_web_contents));

  GestureActionDispatcher action_dispatcher(active_tab());
  test::RunDispatcherAndWait(action_dispatcher,
                             {test::MakeSwitchToTabGestureAction("*title2*")});

  EXPECT_EQ(expected_web_contents,
            second_browser_window->GetTabStripModel()->GetActiveWebContents());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ShouldNotActivateMatchingTabInIncognitoWindow) {
  const GURL target_url = embedded_test_server()->GetURL("/title2.html");
  BrowserWindowInterface* const incognito_browser_window =
      CreateIncognitoBrowser(chrome_test_utils::GetProfile(this));
  content::WebContents* const incognito_web_contents =
      chrome::AddAndReturnTabAt(incognito_browser_window, target_url,
                                /*index=*/-1, /*foreground=*/false);
  ASSERT_TRUE(content::WaitForLoadStop(incognito_web_contents));

  const GURL start_url = embedded_test_server()->GetURL("/title1.html");
  ASSERT_TRUE(
      chrome_test_utils::NavigateToURL(active_web_contents(), start_url));
  content::WebContents* const web_contents_before_dispatch =
      active_web_contents();

  GestureActionDispatcher action_dispatcher(active_tab());
  test::RunDispatcherAndWait(action_dispatcher,
                             {test::MakeSwitchToTabGestureAction("*title2*")});

  // The incognito tab matches, but a regular-profile gesture must never
  // reveal or activate a tab in an unrelated Incognito window.
  EXPECT_EQ(web_contents_before_dispatch, active_web_contents());
  EXPECT_NE(
      incognito_web_contents,
      incognito_browser_window->GetTabStripModel()->GetActiveWebContents());
}

// Selects all of the active tab's page text, replacing whatever's there
// with a single deterministic word first, so the resulting search query is
// predictable regardless of the test page's real content.
void SelectAllPageText(content::WebContents* web_contents) {
  ASSERT_TRUE(content::ExecJs(web_contents,
                              "document.body.textContent = "
                              "'gesturesearchtext';"
                              "document.execCommand('selectAll')"));
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ShouldSearchSelectedTextWithBraveSearchByDefault) {
  const GURL start_url = embedded_test_server()->GetURL("/title1.html");
  ASSERT_TRUE(
      chrome_test_utils::NavigateToURL(active_web_contents(), start_url));
  SelectAllPageText(active_web_contents());

  GestureActionDispatcher action_dispatcher(active_tab());
  content::TestNavigationObserver navigation_observer(
      GURL("https://search.brave.com/search?q=gesturesearchtext"));
  navigation_observer.WatchExistingWebContents();
  action_dispatcher.DispatchActionsAsync(
      {test::MakeSearchSelectedTextGestureAction(
          SmartGestureSelectedTextSearchCommandParamType::kBraveSearch)},
      /*smart_gesture_content=*/std::nullopt);
  navigation_observer.Wait();

  EXPECT_EQ(GURL("https://search.brave.com/search?q=gesturesearchtext"),
            active_web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ShouldOpenBraveAskWithSelectedTextWhenRequested) {
  const GURL start_url = embedded_test_server()->GetURL("/title1.html");
  ASSERT_TRUE(
      chrome_test_utils::NavigateToURL(active_web_contents(), start_url));
  SelectAllPageText(active_web_contents());

  GestureActionDispatcher action_dispatcher(active_tab());
  content::TestNavigationObserver navigation_observer(
      GURL("https://ask.brave.com/?q=gesturesearchtext"));
  navigation_observer.WatchExistingWebContents();
  action_dispatcher.DispatchActionsAsync(
      {test::MakeSearchSelectedTextGestureAction(
          SmartGestureSelectedTextSearchCommandParamType::kBraveAsk)},
      /*smart_gesture_content=*/std::nullopt);
  navigation_observer.Wait();

  EXPECT_EQ(GURL("https://ask.brave.com/?q=gesturesearchtext"),
            active_web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherBrowserTest,
                       ShouldNotNavigateWhenNothingIsSelected) {
  const GURL start_url = embedded_test_server()->GetURL("/title1.html");
  ASSERT_TRUE(
      chrome_test_utils::NavigateToURL(active_web_contents(), start_url));

  GestureActionDispatcher action_dispatcher(active_tab());
  test::RunDispatcherAndWait(
      action_dispatcher,
      {test::MakeSearchSelectedTextGestureAction(
          SmartGestureSelectedTextSearchCommandParamType::kBraveSearch)});

  EXPECT_EQ(start_url, active_web_contents()->GetLastCommittedURL());
}

}  // namespace gestures
