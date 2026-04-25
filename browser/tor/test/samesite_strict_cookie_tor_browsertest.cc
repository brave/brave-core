/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "brave/browser/ui/views/location_bar/onion_location_view.h"
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
#include "ui/events/base_event_utils.h"
#include "ui/views/test/button_test_api.h"
#include "url/gurl.h"

namespace {

constexpr char kSiteA[] = "a.test";
constexpr char kSiteB[] = "b.test";
constexpr char kOnion[] = "c.onion";
constexpr char kSameSiteStrictCookie[] = "strict_cookie=1; SameSite=Strict";

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
        onion_server_->GetURL(kOnion, "/echoheader?Cookie")));

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

  auto* onion_location_view = GetOnionLocationView(browser());

  const GURL view_cookies_url =
      onion_server_->GetURL(kOnion, "/echoheader?Cookie");
  content::TestNavigationObserver observer(view_cookies_url);
  observer.StartWatchingNewWebContents();

  ui::MouseEvent pressed(ui::EventType::kMousePressed, gfx::Point(),
                         gfx::Point(), ui::EventTimeForNow(),
                         ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON);
  ui::MouseEvent released(ui::EventType::kMouseReleased, gfx::Point(),
                          gfx::Point(), ui::EventTimeForNow(),
                          ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON);
  views::test::ButtonTestApi(onion_location_view).NotifyClick(pressed);
  views::test::ButtonTestApi(onion_location_view).NotifyClick(released);
  observer.Wait();

  auto* web_contents = tor_browser->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(content::EvalJs(web_contents, "document.body.textContent"), "None");
}
