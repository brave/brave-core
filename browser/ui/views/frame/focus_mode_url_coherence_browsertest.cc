/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <string_view>

#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/focus_mode/focus_mode_controller.h"
#include "brave/browser/ui/focus_mode/focus_mode_features.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/focus_mode_title_bar_view.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/default_handlers.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/scoped_animation_duration_scale_mode.h"
#include "ui/views/controls/label.h"
#include "ui/views/test/views_test_utils.h"
#include "url/gurl.h"

// Verifies that the Focus Mode title bar's domain label stays coherent with the
// active page across navigation edge cases (client/server redirects and
// back/forward cache restore). The title bar subscribes to TabUIHelper change
// callbacks; these tests exercise the presentation layer
// (FocusModeTitleBarView::Update) rather than the data source alone, which is
// already covered by tab_ui_helper_browsertest.cc upstream.
class FocusModeUrlCoherenceBrowserTest : public InProcessBrowserTest {
 public:
  FocusModeUrlCoherenceBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitAndEnableFeature(features::kBraveFocusMode);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    // Registers /server-redirect?<url>, /client-redirect?<url>,
    // /cross-site/redir?<url> and friends on the HTTPS server.
    net::test_server::RegisterDefaultHandlers(&https_server_);
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    https_server_.ServeFilesFromSourceDirectory("chrome/test/data");
    ASSERT_TRUE(https_server_.Start());

    // Enable Focus Mode (the user-visible "setting") up front, so it is on
    // for the whole test window lifetime, not just partway through each test.
    // The kBraveFocusMode flag is enabled via |feature_list_| in the ctor.
    EnableFocusMode();
    ASSERT_TRUE(browser()->GetFeatures().focus_mode_controller()->IsEnabled());
  }

  BraveBrowserView* browser_view() {
    return BraveBrowserView::From(
        BrowserView::GetBrowserViewForBrowser(browser()));
  }

  FocusModeTitleBarView* title_bar() {
    return browser_view()->focus_mode_title_bar_for_testing();
  }

  std::u16string domain_text() {
    return std::u16string(title_bar()->domain_label_for_testing()->GetText());
  }

  bool WaitForDomainToContain(std::u16string_view host) {
    return base::test::RunUntil([&]() {
      return title_bar()->GetVisible() &&
             domain_text().find(host) != std::u16string::npos;
    });
  }

  content::WebContents* GetWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void EnableFocusMode() {
    browser()->GetFeatures().focus_mode_controller()->SetEnabled(true);
    views::test::RunScheduledLayout(browser_view());
  }

  net::EmbeddedTestServer https_server_;

 private:
  base::test::ScopedFeatureList feature_list_;
  gfx::ScopedAnimationDurationScaleMode zero_duration_mode_{
      gfx::ScopedAnimationDurationScaleMode::ZERO_DURATION};
};

// A client-side redirect (meta refresh / location.href) must update the domain
// label to the redirect destination. The initial page fires
// DidFinishNavigation, then the redirect target fires a second
// DidFinishNavigation; TabUIHelper notifies on both, and the title bar's
// subscription must re-render with the new host.
IN_PROC_BROWSER_TEST_F(FocusModeUrlCoherenceBrowserTest,
                       ClientRedirectUpdatesDomain) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server_.GetURL("a.test", "/empty.html")));
  ASSERT_TRUE(WaitForDomainToContain(u"a.test"));

  // /client-redirect?<url> returns an HTML page that meta-refreshes to <url>.
  const GURL redirect_url = https_server_.GetURL(
      "a.test", "/client-redirect?" +
                    https_server_.GetURL("b.test", "/empty.html").spec());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), redirect_url));

  // The redirect is client-initiated, so wait for the destination host to
  // appear in the label rather than asserting synchronously.
  EXPECT_TRUE(WaitForDomainToContain(u"b.test"));
}

// A server-side redirect (HTTP 301) must update the domain label to the
// redirect destination. NavigateToURL follows the chain and waits for the
// final load, so the label should reflect the destination host immediately.
IN_PROC_BROWSER_TEST_F(FocusModeUrlCoherenceBrowserTest,
                       ServerRedirectUpdatesDomain) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server_.GetURL("a.test", "/empty.html")));
  ASSERT_TRUE(WaitForDomainToContain(u"a.test"));

  // /server-redirect?<url> responds with HTTP MOVED PERMANENTLY to <url>.
  const GURL redirect_url = https_server_.GetURL(
      "a.test", "/server-redirect?" +
                    https_server_.GetURL("b.test", "/empty.html").spec());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), redirect_url));

  EXPECT_TRUE(WaitForDomainToContain(u"b.test"));
}

// Activating a prerendered page would be the fourth edge case, but Prerender2
// is disabled in Brave by default on two independent layers
// (blink::features::kPrerender2 in chromium_src/.../features.cc and the
// "Use a prediction service" pref in brave_profile_prefs.cc), and the content/
// prerender path additionally rejects the cross-site host transition
// (a.test -> b.test) that would make this test meaningful. Upstream
// tab_ui_helper_browsertest.cc already covers TabUIHelper's reaction
// (ShouldNotAffectTabUIHelperOnPrerendering), so the Brave presentation layer
// adds nothing testable here.

// Restoring a page from the back/forward cache must update the domain label
// back to the restored page's host. bfcache restore fires DidFinishNavigation;
// TabUIHelper notifies, and the title bar must re-render with the prior host.
IN_PROC_BROWSER_TEST_F(FocusModeUrlCoherenceBrowserTest,
                       BackForwardCacheRestoreUpdatesDomain) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server_.GetURL("a.test", "/empty.html")));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server_.GetURL("b.test", "/empty.html")));
  ASSERT_TRUE(WaitForDomainToContain(u"b.test"));

  // Go back to the first page; bfcache restore (or a normal back navigation
  // if bfcache is not available for this page) must update the label.
  content::WebContents* const web_contents = GetWebContents();
  web_contents->GetController().GoBack();
  ASSERT_TRUE(content::WaitForLoadStop(web_contents));

  EXPECT_TRUE(WaitForDomainToContain(u"a.test"));
}
