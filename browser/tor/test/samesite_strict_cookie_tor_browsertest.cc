/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/strings/string_split.h"
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "brave/browser/ui/views/location_bar/onion_location_view.h"
#include "brave/components/tor/onion_location_tab_helper.h"
#include "brave/components/tor/tor_navigation_throttle.h"
#include "brave/net/proxy_resolution/proxy_config_service_tor.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/default_handlers.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "ui/events/base_event_utils.h"
#include "ui/views/test/button_test_api.h"
#include "url/gurl.h"

namespace {

constexpr char kSiteA[] = "a.test";
constexpr char kSiteB[] = "b.test";
constexpr char kOnion[] = "c.onion";
constexpr char kSameSiteStrictCookie[] = "strict_cookie=1; SameSite=Strict";
constexpr char kOnionLinkPagePath[] = "/onion-link-page.html";

std::unique_ptr<net::test_server::HttpResponse> HandleOnionLinkPage(
    const GURL& onion_echo_url,
    const net::test_server::HttpRequest& request) {
  if (request.relative_url != kOnionLinkPagePath) {
    return nullptr;
  }
  auto response = std::make_unique<net::test_server::BasicHttpResponse>();
  response->set_code(net::HTTP_OK);
  response->set_content_type("text/html");
  response->set_content(absl::StrFormat(
      R"(<html><body><a id="onion" href="%s">link</a></body></html>)",
      onion_echo_url.spec().c_str()));
  return response;
}

std::unique_ptr<net::test_server::HttpResponse> HandleSetStrictCookie(
    const GURL& onion_location,
    const net::test_server::HttpRequest& request) {
  if (request.relative_url.find("/set-strict-cookie") != 0) {
    return nullptr;
  }
  auto response = std::make_unique<net::test_server::BasicHttpResponse>();
  response->set_code(net::HTTP_OK);
  response->set_content_type("text/html");
  response->set_content("<html><body>cookie set</body></html>");
  response->AddCustomHeader("Set-Cookie", kSameSiteStrictCookie);
  response->AddCustomHeader("onion-location", onion_location.spec());
  return response;
}

IconLabelBubbleView* GetOnionLocationView(Browser* browser) {
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  if (!browser_view) {
    return nullptr;
  }
  BraveLocationBarView* brave_location_bar_view =
      static_cast<BraveLocationBarView*>(browser_view->GetLocationBarView());
  if (!brave_location_bar_view) {
    return nullptr;
  }
  return brave_location_bar_view->GetOnionLocationView();
}

void ClickOnionLocationIcon(Browser* browser) {
  auto* onion_location_view = GetOnionLocationView(browser);
  ASSERT_TRUE(onion_location_view);
  ui::MouseEvent pressed(ui::EventType::kMousePressed, gfx::Point(),
                         gfx::Point(), ui::EventTimeForNow(),
                         ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON);
  ui::MouseEvent released(ui::EventType::kMouseReleased, gfx::Point(),
                          gfx::Point(), ui::EventTimeForNow(),
                          ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON);
  views::test::ButtonTestApi(onion_location_view).NotifyClick(pressed);
  views::test::ButtonTestApi(onion_location_view).NotifyClick(released);
}

std::vector<std::string> GetEchoedHeaders(content::WebContents* web_contents) {
  const std::string response_body =
      content::EvalJs(web_contents, "document.body.textContent")
          .ExtractString();
  return base::SplitString(response_body, "\n", base::TRIM_WHITESPACE,
                           base::SPLIT_WANT_NONEMPTY);
}

}  // namespace

class SameSiteStrictCookieTorBrowserTest : public InProcessBrowserTest {
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");

    onion_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::EmbeddedTestServer::TYPE_HTTP);
    onion_server_->RegisterRequestHandler(
        base::BindRepeating(&HandleSetStrictCookie, GURL::EmptyGURL()));
    net::test_server::RegisterDefaultHandlers(onion_server_.get());
    ASSERT_TRUE(onion_server_->Start());

    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    https_server_->RegisterRequestHandler(base::BindRepeating(
        &HandleSetStrictCookie,
        onion_server_->GetURL(kOnion, "/echoheader?Cookie&Sec-Fetch-Site")));
    https_server_->RegisterRequestHandler(base::BindRepeating(
        &HandleOnionLinkPage,
        onion_server_->GetURL(kOnion, "/echoheader?Cookie&Sec-Fetch-Site")));

    net::test_server::RegisterDefaultHandlers(https_server_.get());
    ASSERT_TRUE(https_server_->Start());

    net::ProxyConfigServiceTor::SetBypassTorProxyConfigForTesting(true);
    tor::TorNavigationThrottle::SetSkipWaitForTorConnectedForTesting(true);
  }

 protected:
  std::unique_ptr<net::test_server::EmbeddedTestServer> https_server_;
  std::unique_ptr<net::test_server::EmbeddedTestServer> onion_server_;
};

IN_PROC_BROWSER_TEST_F(SameSiteStrictCookieTorBrowserTest,
                       OpenLinkInTorDoesNotSendSameSiteStrictCookie) {
  Browser* tor_browser =
      TorProfileManager::SwitchToTorProfile(browser()->profile());

  const auto set_cookie_url =
      https_server_->GetURL(kSiteB, "/set-strict-cookie");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(tor_browser, set_cookie_url));

  const GURL view_cookies_url =
      https_server_->GetURL(kSiteB, "/echoheader?Cookie");

  auto exec_and_check_cookies = [&](const GURL& site_url) {
    content::ContextMenuParams params;
    params.frame_origin = url::Origin::Create(site_url);
    params.page_url = site_url;
    params.frame_url = site_url;
    params.link_url = view_cookies_url;

    TestRenderViewContextMenu menu(*browser()
                                        ->tab_strip_model()
                                        ->GetActiveWebContents()
                                        ->GetPrimaryMainFrame(),
                                   params);
    content::TestNavigationObserver observer(view_cookies_url);
    observer.StartWatchingNewWebContents();
    menu.ExecuteCommand(IDC_CONTENT_CONTEXT_OPENLINKTOR, 0);
    observer.Wait();

    auto* web_contents = tor_browser->tab_strip_model()->GetActiveWebContents();
    return content::EvalJs(web_contents, "document.body.textContent") == "None";
  };
  {
    // Cross-site case
    const auto cross_site_page_url =
        https_server_->GetURL(kSiteA, "/cross-site-page.html");
    EXPECT_TRUE(exec_and_check_cookies(cross_site_page_url));
  }
  {
    // Same-site case
    const auto same_site_page_url =
        https_server_->GetURL(kSiteB, "/cross-site-page.html");
    EXPECT_FALSE(exec_and_check_cookies(same_site_page_url));
  }
}

IN_PROC_BROWSER_TEST_F(SameSiteStrictCookieTorBrowserTest, OnionLocation) {
  Browser* tor_browser =
      TorProfileManager::SwitchToTorProfile(browser()->profile());

  const auto set_cookie_url =
      onion_server_->GetURL(kOnion, "/set-strict-cookie");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(tor_browser, set_cookie_url));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server_->GetURL(kSiteA, "/set-strict-cookie")));

  const GURL view_cookies_url =
      onion_server_->GetURL(kOnion, "/echoheader?Cookie&Sec-Fetch-Site");
  content::TestNavigationObserver observer(view_cookies_url);
  observer.StartWatchingNewWebContents();

  ClickOnionLocationIcon(browser());
  observer.Wait();

  auto* web_contents = tor_browser->tab_strip_model()->GetActiveWebContents();
  const std::vector<std::string> echoed_headers =
      GetEchoedHeaders(web_contents);
  ASSERT_EQ(echoed_headers.size(), 2u);
  EXPECT_EQ(echoed_headers[0], "None");
  EXPECT_EQ(echoed_headers[1], "cross-site");
}

IN_PROC_BROWSER_TEST_F(
    SameSiteStrictCookieTorBrowserTest,
    BlockedOnionLinkCrossSiteDoesNotSendSameSiteStrictCookie) {
  Browser* tor_browser =
      TorProfileManager::SwitchToTorProfile(browser()->profile());

  const GURL set_cookie_url =
      onion_server_->GetURL(kOnion, "/set-strict-cookie");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(tor_browser, set_cookie_url));

  const GURL onion_echo_url =
      onion_server_->GetURL(kOnion, "/echoheader?Cookie&Sec-Fetch-Site");
  const GURL cross_site_page_url =
      https_server_->GetURL(kSiteA, kOnionLinkPagePath);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), cross_site_page_url));

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  content::TestNavigationObserver nav_observer(web_contents);
  ASSERT_TRUE(content::ExecJs(web_contents,
                              "document.getElementById('onion').click()"));
  nav_observer.Wait();
  EXPECT_EQ(nav_observer.last_net_error_code(), net::ERR_BLOCKED_BY_CLIENT);
  EXPECT_TRUE(web_contents->GetPrimaryMainFrame()->IsErrorDocument());

  tor::OnionLocationTabHelper* helper =
      tor::OnionLocationTabHelper::FromWebContents(web_contents);
  ASSERT_TRUE(helper);
  EXPECT_TRUE(helper->should_show_icon());
  EXPECT_EQ(helper->onion_location(), onion_echo_url);

  content::TestNavigationObserver tor_observer(onion_echo_url);
  tor_observer.StartWatchingNewWebContents();
  ClickOnionLocationIcon(browser());
  tor_observer.Wait();

  content::WebContents* tor_web_contents =
      tor_browser->tab_strip_model()->GetActiveWebContents();
  const std::vector<std::string> echoed_headers =
      GetEchoedHeaders(tor_web_contents);
  ASSERT_EQ(echoed_headers.size(), 2u);
  EXPECT_EQ(echoed_headers[0], "None");
  EXPECT_EQ(echoed_headers[1], "cross-site");
}
