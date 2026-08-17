/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/focus_mode_title_bar_view.h"

#include <string>
#include <string_view>

#include "base/strings/utf_string_conversions.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/focus_mode/focus_mode_controller.h"
#include "brave/browser/ui/focus_mode/focus_mode_features.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/back_forward_cache_util.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/default_handlers.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/window_open_disposition.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/scoped_animation_duration_scale_mode.h"
#include "ui/views/controls/label.h"
#include "ui/views/test/views_test_utils.h"
#include "url/gurl.h"

// Exercises the BraveBrowserView wiring for the Focus Mode title bar. The title
// bar's own content formatting is covered by unit tests.
class FocusModeTitleBarViewBrowserTest : public InProcessBrowserTest {
 protected:
  FocusModeTitleBarViewBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitAndEnableFeatureWithParameters(
        features::kBraveFocusMode, {{"FocusModeUrlDisplay", "title-bar"}});
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    // Registers /server-redirect?<url>, /client-redirect?<url>,
    // /cross-site/redir?<url> and friends on the HTTPS server. Used by the
    // FocusModeUrlCoherenceBrowserTest subclass; harmless for TitleBarWiring.
    net::test_server::RegisterDefaultHandlers(&https_server_);
    // CERT_TEST_NAMES is valid for a.test/b.test, which are used below to give
    // each tab a distinct, secure host.
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    https_server_.ServeFilesFromSourceDirectory("chrome/test/data");
    ASSERT_TRUE(https_server_.Start());
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

  net::EmbeddedTestServer https_server_;

 private:
  base::test::ScopedFeatureList feature_list_;
  gfx::ScopedAnimationDurationScaleMode zero_duration_mode_{
      gfx::ScopedAnimationDurationScaleMode::ZERO_DURATION};
};

IN_PROC_BROWSER_TEST_F(FocusModeTitleBarViewBrowserTest, TitleBarWiring) {
  ASSERT_TRUE(title_bar());

  // The title bar is present but hidden until Focus Mode is enabled.
  EXPECT_FALSE(title_bar()->GetVisible());

  // Focus Mode stays disabled unless the active tab is secure.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server_.GetURL("a.test", "/empty.html")));

  // Enabling Focus Mode shows the title bar and points it at the active tab.
  browser()->GetFeatures().focus_mode_controller()->SetEnabled(true);
  views::test::RunScheduledLayout(browser_view());
  ASSERT_TRUE(title_bar()->GetVisible());
  ASSERT_TRUE(WaitForDomainToContain(u"a.test"));

  // The title bar occupies a strip at the top and the contents are pushed down
  // below it.
  const gfx::Rect title_bar_bounds = title_bar()->GetBoundsInScreen();
  const gfx::Rect contents_bounds =
      browser_view()->contents_container()->GetBoundsInScreen();
  EXPECT_EQ(title_bar()->height(), title_bar()->GetPreferredSize().height());
  EXPECT_GT(title_bar()->height(), 0);
  EXPECT_GT(title_bar_bounds.bottom(), title_bar_bounds.y());
  EXPECT_GE(contents_bounds.y(), title_bar_bounds.bottom());

  // Opening a new foreground tab updates the title bar to the new active tab.
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), https_server_.GetURL("b.test", "/empty.html"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_TAB |
          ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  EXPECT_TRUE(WaitForDomainToContain(u"b.test"));

  // Reactivating the first tab updates the title bar back.
  browser()->tab_strip_model()->ActivateTabAt(0);
  EXPECT_TRUE(WaitForDomainToContain(u"a.test"));

  // Disabling Focus Mode hides the title bar again.
  browser()->GetFeatures().focus_mode_controller()->SetEnabled(false);
  EXPECT_FALSE(title_bar()->GetVisible());
}

// Verifies that the Focus Mode title bar's domain label stays coherent with the
// active page across navigation edge cases (client/server redirects and
// back/forward cache restore). The title bar subscribes to TabUIHelper change
// callbacks; these tests exercise the presentation layer
// (FocusModeTitleBarView::Update) rather than the data source alone, which is
// already covered by tab_ui_helper_browsertest.cc upstream.
class FocusModeUrlCoherenceBrowserTest
    : public FocusModeTitleBarViewBrowserTest {
 public:
  FocusModeUrlCoherenceBrowserTest() {
    // Enable back/forward cache so the restore test exercises the cached-RFH
    // path rather than a normal back navigation. The redirect tests only do
    // forward navigations, so enabling bfcache does not affect them.
    bfcache_feature_list_.InitWithFeaturesAndParameters(
        content::GetDefaultEnabledBackForwardCacheFeaturesForTesting(),
        content::GetDefaultDisabledBackForwardCacheFeaturesForTesting());
  }

  void SetUpOnMainThread() override {
    FocusModeTitleBarViewBrowserTest::SetUpOnMainThread();
    // Enable Focus Mode (the user-visible "setting") up front, so it is on for
    // the whole test window lifetime, not just partway through each test.
    EnableFocusMode();
    ASSERT_TRUE(browser()->GetFeatures().focus_mode_controller()->IsEnabled());
  }

  content::WebContents* GetWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void EnableFocusMode() {
    browser()->GetFeatures().focus_mode_controller()->SetEnabled(true);
    views::test::RunScheduledLayout(browser_view());
  }

 private:
  base::test::ScopedFeatureList bfcache_feature_list_;
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
  ASSERT_TRUE(WaitForDomainToContain(u"a.test"))
      << base::UTF16ToUTF8(domain_text());

  // /client-redirect?<url> returns an HTML page that meta-refreshes to <url>.
  const GURL redirect_url = https_server_.GetURL(
      "a.test", "/client-redirect?" +
                    https_server_.GetURL("b.test", "/empty.html").spec());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), redirect_url));

  // The redirect is client-initiated, so wait for the destination host to
  // appear in the label rather than asserting synchronously.
  EXPECT_TRUE(WaitForDomainToContain(u"b.test"))
      << base::UTF16ToUTF8(domain_text());
}

// A server-side redirect (HTTP 301) must update the domain label to the
// redirect destination. NavigateToURL follows the chain and waits for the
// final load, so the label should reflect the destination host immediately.
IN_PROC_BROWSER_TEST_F(FocusModeUrlCoherenceBrowserTest,
                       ServerRedirectUpdatesDomain) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server_.GetURL("a.test", "/empty.html")));
  ASSERT_TRUE(WaitForDomainToContain(u"a.test"))
      << base::UTF16ToUTF8(domain_text());

  // /server-redirect?<url> responds with HTTP MOVED PERMANENTLY to <url>.
  const GURL redirect_url = https_server_.GetURL(
      "a.test", "/server-redirect?" +
                    https_server_.GetURL("b.test", "/empty.html").spec());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), redirect_url));

  EXPECT_TRUE(WaitForDomainToContain(u"b.test"))
      << base::UTF16ToUTF8(domain_text());
}

// Restoring a page from the back/forward cache must update the domain label
// back to the restored page's host. bfcache restore fires DidFinishNavigation;
// TabUIHelper notifies, and the title bar must re-render with the prior host.
IN_PROC_BROWSER_TEST_F(FocusModeUrlCoherenceBrowserTest,
                       BackForwardCacheRestoreUpdatesDomain) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server_.GetURL("a.test", "/empty.html")));
  ASSERT_TRUE(WaitForDomainToContain(u"a.test"))
      << base::UTF16ToUTF8(domain_text());

  // Capture the RFH that should enter bfcache when we navigate away.
  content::RenderFrameHost* const cached_rfh =
      GetWebContents()->GetPrimaryMainFrame();

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server_.GetURL("b.test", "/empty.html")));
  ASSERT_TRUE(WaitForDomainToContain(u"b.test"))
      << base::UTF16ToUTF8(domain_text());

  // Verify the previous page was actually cached, not evicted to a normal
  // back navigation. Without this assertion GoBack could pass via either path.
  ASSERT_EQ(cached_rfh->GetLifecycleState(),
            content::RenderFrameHost::LifecycleState::kInBackForwardCache);

  // Go back: bfcache restore fires DidFinishNavigation; the title bar must
  // re-render with the restored page's host.
  content::WebContents* const web_contents = GetWebContents();
  web_contents->GetController().GoBack();
  ASSERT_TRUE(content::WaitForLoadStop(web_contents));

  EXPECT_TRUE(WaitForDomainToContain(u"a.test"))
      << base::UTF16ToUTF8(domain_text());
}
