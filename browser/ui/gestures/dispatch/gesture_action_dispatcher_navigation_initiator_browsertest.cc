// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/functional/callback_helpers.h"
#include "brave/browser/ui/gestures/dispatch/gesture_action_dispatcher.h"
#include "brave/browser/ui/gestures/dispatch/test/gesture_action_dispatcher_test_util.h"
#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "brave/components/gestures/core/common/actions/smart_gesture_open_in_command_param_types.h"
#include "brave/components/gestures/core/common/actions/smart_gesture_selected_text_search_command_param_types.h"
#include "brave/components/gestures/core/common/actions/test/gesture_action_test_util.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_content_info.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_content_type.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/cookies/cookie_options.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "third_party/blink/public/common/tokens/tokens.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace gestures {

class GestureActionDispatcherNavigationInitiatorBrowserTest
    : public PlatformBrowserTest {
 public:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");

    // "a.test"/"b.test" give these tests two real, distinct origins, since
    // the attribution bug only shows up across a genuine cross-site hop.
    cross_site_https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::EmbeddedTestServer::TYPE_HTTPS);
    cross_site_https_server_->SetSSLConfig(
        net::EmbeddedTestServer::CERT_TEST_NAMES);
    cross_site_https_server_->AddDefaultHandlers(GetChromeTestDataDir());
    ASSERT_TRUE(cross_site_https_server_->Start());
  }

  content::WebContents* active_web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  tabs::TabInterface& active_tab() {
    return *chrome_test_utils::GetActiveTab(this);
  }

 protected:
  std::unique_ptr<net::EmbeddedTestServer> cross_site_https_server_;
};

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherNavigationInitiatorBrowserTest,
                       ShouldAttributeOpenLinkNavigationToHitTestedChildFrame) {
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(
      active_web_contents(),
      cross_site_https_server_->GetURL("a.test", "/iframe.html")));
  ASSERT_TRUE(content::NavigateIframeToURL(
      active_web_contents(), "test",
      cross_site_https_server_->GetURL("b.test", "/title1.html")));
  content::RenderFrameHost* const child_render_frame_host =
      content::ChildFrameAt(active_web_contents(), 0);
  ASSERT_TRUE(child_render_frame_host);

  const GURL target_url =
      cross_site_https_server_->GetURL("a.test", "/title2.html");
  content::TestNavigationObserver navigation_observer(target_url);
  navigation_observer.StartWatchingNewWebContents();

  GestureActionDispatcher action_dispatcher(active_tab());
  test::RunDispatcherAndWait(
      action_dispatcher,
      {test::MakeGestureActionOpenInCommandParam(
          kOpenLinkIn, SmartGestureOpenInCommandParamType::kNewForegroundTab)},
      SmartGestureContentInfo{
          .type = SmartGestureContentType::kLink,
          .value = target_url.spec(),
          .origin = child_render_frame_host->GetLastCommittedOrigin(),
          .frame_token = child_render_frame_host->GetFrameToken(),
          .process_id =
              child_render_frame_host->GetProcess()->GetDeprecatedID()});
  navigation_observer.Wait();

  EXPECT_EQ(child_render_frame_host->GetLastCommittedOrigin(),
            navigation_observer.last_initiator_origin());
  EXPECT_EQ(child_render_frame_host->GetFrameToken(),
            navigation_observer.last_initiator_frame_token());
}

IN_PROC_BROWSER_TEST_F(
    GestureActionDispatcherNavigationInitiatorBrowserTest,
    ShouldIgnoreForgedInitiatorOriginEvenWithValidFrameToken) {
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(
      active_web_contents(),
      cross_site_https_server_->GetURL("a.test", "/iframe.html")));
  ASSERT_TRUE(content::NavigateIframeToURL(
      active_web_contents(), "test",
      cross_site_https_server_->GetURL("b.test", "/title1.html")));
  content::RenderFrameHost* const child_render_frame_host =
      content::ChildFrameAt(active_web_contents(), 0);
  ASSERT_TRUE(child_render_frame_host);

  const GURL target_url =
      cross_site_https_server_->GetURL("a.test", "/title2.html");
  content::TestNavigationObserver navigation_observer(target_url);
  navigation_observer.StartWatchingNewWebContents();

  GestureActionDispatcher action_dispatcher(active_tab());
  // `frame_token`/`process_id` genuinely identify the real child frame, but
  // `origin` is forged (a renderer compromised enough to control the mojo
  // reply could claim this); the dispatcher must still attribute the
  // navigation to the frame's own browser-known origin, not this one.
  test::RunDispatcherAndWait(
      action_dispatcher,
      {test::MakeGestureActionOpenInCommandParam(
          kOpenLinkIn, SmartGestureOpenInCommandParamType::kNewForegroundTab)},
      SmartGestureContentInfo{
          .type = SmartGestureContentType::kLink,
          .value = target_url.spec(),
          .origin = url::Origin::Create(GURL("https://forged-origin.test")),
          .frame_token = child_render_frame_host->GetFrameToken(),
          .process_id =
              child_render_frame_host->GetProcess()->GetDeprecatedID()});
  navigation_observer.Wait();

  EXPECT_EQ(child_render_frame_host->GetLastCommittedOrigin(),
            navigation_observer.last_initiator_origin());
}

IN_PROC_BROWSER_TEST_F(
    GestureActionDispatcherNavigationInitiatorBrowserTest,
    ShouldNotAttributeOpenLinkNavigationWhenNoFrameWasCaptured) {
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(
      active_web_contents(),
      cross_site_https_server_->GetURL("a.test", "/title1.html")));

  const GURL target_url =
      cross_site_https_server_->GetURL("b.test", "/title2.html");
  content::TestNavigationObserver navigation_observer(target_url);
  navigation_observer.StartWatchingNewWebContents();

  GestureActionDispatcher action_dispatcher(active_tab());
  // No frame was captured for this content; unlike a token that fails to
  // resolve, this isn't a failed check, so the navigation must still
  // proceed, just without any initiator attribution.
  test::RunDispatcherAndWait(
      action_dispatcher,
      {test::MakeGestureActionOpenInCommandParam(
          kOpenLinkIn, SmartGestureOpenInCommandParamType::kNewForegroundTab)},
      SmartGestureContentInfo{.type = SmartGestureContentType::kLink,
                              .value = target_url.spec()});
  navigation_observer.Wait();

  EXPECT_FALSE(navigation_observer.last_initiator_origin().has_value());
}

IN_PROC_BROWSER_TEST_F(
    GestureActionDispatcherNavigationInitiatorBrowserTest,
    ShouldRejectNavigationWhenFrameTokenBelongsToAnotherTab) {
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(
      active_web_contents(),
      cross_site_https_server_->GetURL("a.test", "/title1.html")));
  tabs::TabInterface& gesture_tab = active_tab();
  const int tab_count_before = browser()->tab_strip_model()->count();

  // A second, unrelated tab; its frame is real and resolvable, but it does
  // not belong to `gesture_tab`.
  chrome::AddTabAt(browser(),
                   cross_site_https_server_->GetURL("b.test", "/title1.html"),
                   /*index=*/-1, /*foreground=*/true);
  content::RenderFrameHost* const other_tab_render_frame_host =
      active_web_contents()->GetPrimaryMainFrame();
  ASSERT_TRUE(other_tab_render_frame_host);

  const GURL target_url =
      cross_site_https_server_->GetURL("a.test", "/title2.html");
  GestureActionDispatcher action_dispatcher(gesture_tab);
  // `frame_token`/`process_id` genuinely resolve, but to a frame in a tab
  // other than `gesture_tab`'s own; the dispatcher must refuse to navigate.
  test::RunDispatcherAndWait(
      action_dispatcher,
      {test::MakeGestureActionOpenInCommandParam(
          kOpenLinkIn, SmartGestureOpenInCommandParamType::kNewForegroundTab)},
      SmartGestureContentInfo{
          .type = SmartGestureContentType::kLink,
          .value = target_url.spec(),
          .origin = other_tab_render_frame_host->GetLastCommittedOrigin(),
          .frame_token = other_tab_render_frame_host->GetFrameToken(),
          .process_id =
              other_tab_render_frame_host->GetProcess()->GetDeprecatedID()});

  EXPECT_EQ(tab_count_before + 1, browser()->tab_strip_model()->count());
}

IN_PROC_BROWSER_TEST_F(
    GestureActionDispatcherNavigationInitiatorBrowserTest,
    ShouldRejectNavigationWhenFrameTokenDoesNotResolveToALiveFrame) {
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(
      active_web_contents(),
      cross_site_https_server_->GetURL("a.test", "/title1.html")));
  const int tab_count_before = browser()->tab_strip_model()->count();

  const GURL target_url =
      cross_site_https_server_->GetURL("a.test", "/title2.html");
  GestureActionDispatcher action_dispatcher(active_tab());
  // A `frame_token` that never existed; the dispatcher must refuse to
  // navigate rather than fall back to some other interpretation of it.
  test::RunDispatcherAndWait(
      action_dispatcher,
      {test::MakeGestureActionOpenInCommandParam(
          kOpenLinkIn, SmartGestureOpenInCommandParamType::kNewForegroundTab)},
      SmartGestureContentInfo{.type = SmartGestureContentType::kLink,
                              .value = target_url.spec(),
                              .origin = url::Origin::Create(target_url),
                              .frame_token = blink::LocalFrameToken(),
                              .process_id = active_web_contents()
                                                ->GetPrimaryMainFrame()
                                                ->GetProcess()
                                                ->GetDeprecatedID()});

  EXPECT_EQ(tab_count_before, browser()->tab_strip_model()->count());
}

IN_PROC_BROWSER_TEST_F(
    GestureActionDispatcherNavigationInitiatorBrowserTest,
    ShouldReportNavigationAsCrossSiteWhenInitiatedByDistinctOriginChildFrame) {
  // Same site as `target_url` below, unlike the child frame navigated to
  // next; the test only proves the fix if choosing the wrong initiator
  // actually flips the observed header.
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(
      active_web_contents(),
      cross_site_https_server_->GetURL("a.test", "/iframe.html")));
  ASSERT_TRUE(content::NavigateIframeToURL(
      active_web_contents(), "test",
      cross_site_https_server_->GetURL("b.test", "/title1.html")));
  content::RenderFrameHost* const child_render_frame_host =
      content::ChildFrameAt(active_web_contents(), 0);
  ASSERT_TRUE(child_render_frame_host);

  const GURL target_url =
      cross_site_https_server_->GetURL("a.test", "/echoheader?Sec-Fetch-Site");
  content::TestNavigationObserver navigation_observer(target_url);
  navigation_observer.StartWatchingNewWebContents();

  GestureActionDispatcher action_dispatcher(active_tab());
  test::RunDispatcherAndWait(
      action_dispatcher,
      {test::MakeGestureActionOpenInCommandParam(
          kOpenLinkIn, SmartGestureOpenInCommandParamType::kNewForegroundTab)},
      SmartGestureContentInfo{
          .type = SmartGestureContentType::kLink,
          .value = target_url.spec(),
          .origin = child_render_frame_host->GetLastCommittedOrigin(),
          .frame_token = child_render_frame_host->GetFrameToken(),
          .process_id =
              child_render_frame_host->GetProcess()->GetDeprecatedID()});
  navigation_observer.Wait();

  EXPECT_EQ(
      "cross-site",
      content::EvalJs(browser()->tab_strip_model()->GetActiveWebContents(),
                      "document.body.textContent")
          .ExtractString());
}

IN_PROC_BROWSER_TEST_F(
    GestureActionDispatcherNavigationInitiatorBrowserTest,
    ShouldWithholdSameSiteCookieWhenInitiatedByDistinctOriginChildFrame) {
  const GURL target_url =
      cross_site_https_server_->GetURL("a.test", "/echoheader?Cookie");
  ASSERT_TRUE(content::SetCookie(
      active_web_contents()->GetBrowserContext(), target_url,
      "strict_cookie=1;SameSite=Strict;Secure",
      net::CookieOptions::SameSiteCookieContext::MakeInclusive()));

  ASSERT_TRUE(chrome_test_utils::NavigateToURL(
      active_web_contents(),
      cross_site_https_server_->GetURL("a.test", "/iframe.html")));
  ASSERT_TRUE(content::NavigateIframeToURL(
      active_web_contents(), "test",
      cross_site_https_server_->GetURL("b.test", "/title1.html")));
  content::RenderFrameHost* const child_render_frame_host =
      content::ChildFrameAt(active_web_contents(), 0);
  ASSERT_TRUE(child_render_frame_host);

  content::TestNavigationObserver navigation_observer(target_url);
  navigation_observer.StartWatchingNewWebContents();

  GestureActionDispatcher action_dispatcher(active_tab());
  test::RunDispatcherAndWait(
      action_dispatcher,
      {test::MakeGestureActionOpenInCommandParam(
          kOpenLinkIn, SmartGestureOpenInCommandParamType::kNewForegroundTab)},
      SmartGestureContentInfo{
          .type = SmartGestureContentType::kLink,
          .value = target_url.spec(),
          .origin = child_render_frame_host->GetLastCommittedOrigin(),
          .frame_token = child_render_frame_host->GetFrameToken(),
          .process_id =
              child_render_frame_host->GetProcess()->GetDeprecatedID()});
  navigation_observer.Wait();

  EXPECT_EQ("None", content::EvalJs(
                        browser()->tab_strip_model()->GetActiveWebContents(),
                        "document.body.textContent")
                        .ExtractString());
}

IN_PROC_BROWSER_TEST_F(GestureActionDispatcherNavigationInitiatorBrowserTest,
                       ShouldNotAttributeConfiguredUrlNavigationToPageOrigin) {
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(
      active_web_contents(),
      cross_site_https_server_->GetURL("a.test", "/title1.html")));

  const GURL target_url =
      cross_site_https_server_->GetURL("b.test", "/title2.html");
  content::TestNavigationObserver navigation_observer(active_web_contents());

  GestureActionDispatcher action_dispatcher(active_tab());
  action_dispatcher.DispatchActionsAsync(
      {test::MakeNavigateToUrlGestureAction(target_url)},
      /*smart_gesture_content=*/std::nullopt, base::DoNothing());
  navigation_observer.Wait();

  EXPECT_EQ(target_url, active_web_contents()->GetLastCommittedURL());
  EXPECT_FALSE(navigation_observer.last_initiator_origin().has_value());
}

IN_PROC_BROWSER_TEST_F(
    GestureActionDispatcherNavigationInitiatorBrowserTest,
    ShouldNotAttributeSearchSelectedTextNavigationToPageOrigin) {
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(
      active_web_contents(),
      cross_site_https_server_->GetURL("a.test", "/title1.html")));

  content::TestNavigationObserver navigation_observer(active_web_contents());

  GestureActionDispatcher action_dispatcher(active_tab());
  action_dispatcher.DispatchActionsAsync(
      {test::MakeSearchSelectedTextGestureAction(
          SmartGestureSelectedTextSearchCommandParamType::kBraveSearch)},
      SmartGestureContentInfo{.type = SmartGestureContentType::kSelectedText,
                              .value = "captured text"},
      base::DoNothing());
  navigation_observer.Wait();

  EXPECT_FALSE(navigation_observer.last_initiator_origin().has_value());
}

}  // namespace gestures
