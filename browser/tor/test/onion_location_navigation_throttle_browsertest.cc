/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "brave/browser/ui/views/location_bar/onion_location_view.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/tor/onion_location_tab_helper.h"
#include "brave/components/tor/pref_names.h"
#include "brave/components/tor/tor_navigation_throttle.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/net/proxy_resolution/proxy_config_service_tor.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/test/button_test_api.h"

namespace {

constexpr char kTestOnionPath[] = "/onion";
// URLs inside the Location or Onion-Location headers are allowed to
// include commas and it's not a special character.
constexpr char kTestOnionURL[] = "https://brave.onion/,https://brave2.onion";
constexpr char kTestOnionURL2[] = "https://brave3.onion/";
constexpr char kTestInvalidScheme[] = "/invalid_scheme";
constexpr char kTestInvalidSchemeURL[] = "brave://brave.onion";
constexpr char kTestNotOnion[] = "/not_onion";
constexpr char kTestNotOnionURL[] = "https://brave.com";
constexpr char kTestErrorPage[] = "/errorpage";
constexpr char kTestAttackerOnionURL[] = "https://attacker.onion";

std::unique_ptr<net::test_server::HttpResponse> HandleOnionLocation(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  http_response->set_code(net::HTTP_OK);
  http_response->set_content_type("text/html");
  http_response->set_content("<html><head></head></html>");
  if (request.GetURL().path_piece() == kTestOnionPath) {
    http_response->AddCustomHeader("onion-location", kTestOnionURL);
    // Subsequent headers should be ignored.
    http_response->AddCustomHeader("onion-location", kTestOnionURL2);
  } else if (request.GetURL().path_piece() == kTestInvalidScheme) {
    http_response->AddCustomHeader("onion-location", kTestInvalidSchemeURL);
  } else if (request.GetURL().path_piece() == kTestNotOnion) {
    http_response->AddCustomHeader("onion-location", kTestNotOnionURL);
  } else if (request.GetURL().path_piece() == kTestErrorPage) {
    http_response->AddCustomHeader("onion-location", kTestAttackerOnionURL);
    http_response->set_content(R"html(
        <html>
          <head>
            <script>
              // Going to the unreachable url.
              window.location.href="https://google.goom"
            </script>
          </head>
        </html>
      )html");
  }
  return std::move(http_response);
}

}  // namespace

class OnionLocationNavigationThrottleBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    test_https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    test_https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    test_https_server_->RegisterRequestHandler(
        base::BindRepeating(&HandleOnionLocation));
    ASSERT_TRUE(test_https_server_->Start());

    test_http_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTP);
    test_http_server_->RegisterRequestHandler(
        base::BindRepeating(&HandleOnionLocation));
    ASSERT_TRUE(test_http_server_->Start());

    net::ProxyConfigServiceTor::SetBypassTorProxyConfigForTesting(true);
    tor::TorNavigationThrottle::SetSkipWaitForTorConnectedForTesting(true);
  }

  net::EmbeddedTestServer* test_server() { return test_https_server_.get(); }
  net::EmbeddedTestServer* test_http_server() {
    return test_http_server_.get();
  }

  OnionLocationView* GetOnionLocationView(Browser* browser) {
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

  void CheckOnionLocationLabel(Browser* browser,
                               const GURL& url,
                               bool wait_for_tor_window = true) {
    bool is_tor = browser->profile()->IsTor();
    auto* onion_location_view = GetOnionLocationView(browser);
    ASSERT_TRUE(onion_location_view);
    auto* onion_button = onion_location_view->GetButton();
    ASSERT_TRUE(onion_button);
    EXPECT_TRUE(onion_button->GetVisible());
    EXPECT_EQ(onion_button->GetText(),
              is_tor ? brave_l10n::GetLocalizedResourceUTF16String(
                           IDS_LOCATION_BAR_ONION_AVAILABLE)
                     : u"");
    EXPECT_TRUE(
        onion_button->GetTooltipText().starts_with(l10n_util::GetStringFUTF16(
            is_tor ? IDS_LOCATION_BAR_ONION_AVAILABLE_TOOLTIP_TEXT
                   : IDS_LOCATION_BAR_OPEN_IN_TOR_TOOLTIP_TEXT,
            u"")));

    ui_test_utils::BrowserChangeObserver browser_creation_observer(
        nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);

    content::TestNavigationObserver navigation_observer(
        url, content::MessageLoopRunner::QuitMode::IMMEDIATE, false);
    navigation_observer.StartWatchingNewWebContents();
    ui::MouseEvent pressed(ui::EventType::kMousePressed, gfx::Point(),
                           gfx::Point(), ui::EventTimeForNow(),
                           ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON);
    ui::MouseEvent released(ui::EventType::kMouseReleased, gfx::Point(),
                            gfx::Point(), ui::EventTimeForNow(),
                            ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON);
    views::test::ButtonTestApi(onion_button).NotifyClick(pressed);
    views::test::ButtonTestApi(onion_button).NotifyClick(released);
    if (wait_for_tor_window) {
      browser_creation_observer.Wait();
    }
    BrowserList* browser_list = BrowserList::GetInstance();
    ASSERT_EQ(2U, browser_list->size());
    Browser* tor_browser = browser_list->get(1);
    ASSERT_TRUE(tor_browser->profile()->IsTor());
    content::WebContents* tor_web_contents =
        tor_browser->tab_strip_model()->GetActiveWebContents();
    navigation_observer.Wait();
    EXPECT_EQ(tor_web_contents->GetVisibleURL(), url);
    // We don't close the original tab
    EXPECT_EQ(browser->tab_strip_model()->count(), is_tor ? 2 : 1);
    // No new tab in Tor window
    EXPECT_EQ(tor_browser->tab_strip_model()->count(), is_tor ? 2 : 1);
  }

  Browser* OpenTorWindow() {
    return TorProfileManager::SwitchToTorProfile(browser()->profile());
  }

 private:
  std::unique_ptr<net::EmbeddedTestServer> test_https_server_;
  std::unique_ptr<net::EmbeddedTestServer> test_http_server_;
};

class OnionLocationHeaderNavigationThrottleBrowserTest
    : public OnionLocationNavigationThrottleBrowserTest,
      public testing::WithParamInterface<bool> {};

INSTANTIATE_TEST_SUITE_P(,
                         OnionLocationHeaderNavigationThrottleBrowserTest,
                         testing::Bool());

IN_PROC_BROWSER_TEST_P(OnionLocationHeaderNavigationThrottleBrowserTest,
                       OnionLocationHeader) {
  auto* browser = GetParam() ? OpenTorWindow() : this->browser();

  GURL url1 = test_server()->GetURL("/onion");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser, url1));
  content::WebContents* web_contents =
      browser->tab_strip_model()->GetActiveWebContents();
  tor::OnionLocationTabHelper* helper =
      tor::OnionLocationTabHelper::FromWebContents(web_contents);
  EXPECT_TRUE(helper->should_show_icon());
  EXPECT_EQ(helper->onion_location(), GURL(kTestOnionURL));
  CheckOnionLocationLabel(browser, GURL(kTestOnionURL), false);

  GURL url2 = test_server()->GetURL("/no_onion");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser, url2));
  web_contents = browser->tab_strip_model()->GetActiveWebContents();
  helper = tor::OnionLocationTabHelper::FromWebContents(web_contents);
  EXPECT_FALSE(helper->should_show_icon());
  EXPECT_TRUE(helper->onion_location().is_empty());
  auto* onion_location_view = GetOnionLocationView(browser);
  ASSERT_TRUE(onion_location_view);
  EXPECT_FALSE(onion_location_view->GetVisible());
}

IN_PROC_BROWSER_TEST_F(OnionLocationNavigationThrottleBrowserTest,
                       OnionDomain) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  content::TestNavigationObserver nav_observer(web_contents);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kTestOnionURL)));
  nav_observer.Wait();
  // Original request was blocked
  EXPECT_EQ(nav_observer.last_net_error_code(), net::ERR_BLOCKED_BY_CLIENT);
  tor::OnionLocationTabHelper* helper =
      tor::OnionLocationTabHelper::FromWebContents(web_contents);
  EXPECT_TRUE(helper->should_show_icon());
  EXPECT_EQ(helper->onion_location(), GURL(kTestOnionURL));
  CheckOnionLocationLabel(browser(), GURL(kTestOnionURL));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kTestNotOnionURL)));
  web_contents = browser()->tab_strip_model()->GetActiveWebContents();
  helper = tor::OnionLocationTabHelper::FromWebContents(web_contents);
  EXPECT_FALSE(helper->should_show_icon());
  EXPECT_TRUE(helper->onion_location().is_empty());
}

IN_PROC_BROWSER_TEST_F(OnionLocationNavigationThrottleBrowserTest,
                       OnionDomain_TorWindow) {
  auto* tor_browser = OpenTorWindow();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(tor_browser, GURL(kTestOnionURL)));
  auto* web_contents = tor_browser->tab_strip_model()->GetActiveWebContents();
  auto* helper = tor::OnionLocationTabHelper::FromWebContents(web_contents);
  EXPECT_FALSE(helper->should_show_icon());
  EXPECT_TRUE(helper->onion_location().is_empty());
}

IN_PROC_BROWSER_TEST_F(OnionLocationNavigationThrottleBrowserTest,
                       TorDisabled) {
  // Disable tor
  TorProfileServiceFactory::SetTorDisabled(true);

  // OnionLocationHeader_
  GURL url = test_server()->GetURL("/onion");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  tor::OnionLocationTabHelper* helper =
      tor::OnionLocationTabHelper::FromWebContents(web_contents);
  EXPECT_FALSE(helper->should_show_icon());
  EXPECT_TRUE(helper->onion_location().is_empty());

  // Onion Domain
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kTestOnionURL)));
  BrowserList* browser_list = BrowserList::GetInstance();
  EXPECT_EQ(1U, browser_list->size());

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(1U, browser_list->size());
}

IN_PROC_BROWSER_TEST_F(OnionLocationNavigationThrottleBrowserTest,
                       InvalidScheme) {
  GURL url = test_server()->GetURL("/invalid_scheme");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  tor::OnionLocationTabHelper* helper =
      tor::OnionLocationTabHelper::FromWebContents(web_contents);
  EXPECT_FALSE(helper->should_show_icon());
  EXPECT_TRUE(helper->onion_location().is_empty());

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  BrowserList* browser_list = BrowserList::GetInstance();
  EXPECT_EQ(1U, browser_list->size());
  ASSERT_FALSE(browser_list->get(0)->profile()->IsTor());

  web_contents =
      browser_list->get(0)->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(web_contents->GetVisibleURL(), url);
}

IN_PROC_BROWSER_TEST_F(OnionLocationNavigationThrottleBrowserTest, NotOnion) {
  GURL url = test_server()->GetURL("/not_onion");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  tor::OnionLocationTabHelper* helper =
      tor::OnionLocationTabHelper::FromWebContents(web_contents);
  EXPECT_FALSE(helper->should_show_icon());
  EXPECT_TRUE(helper->onion_location().is_empty());

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  BrowserList* browser_list = BrowserList::GetInstance();
  EXPECT_EQ(1U, browser_list->size());
  ASSERT_FALSE(browser_list->get(0)->profile()->IsTor());

  web_contents =
      browser_list->get(0)->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(web_contents->GetVisibleURL(), url);
}

IN_PROC_BROWSER_TEST_F(OnionLocationNavigationThrottleBrowserTest, HTTPHost) {
  GURL url = test_http_server()->GetURL("/onion");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  tor::OnionLocationTabHelper* helper =
      tor::OnionLocationTabHelper::FromWebContents(web_contents);
  EXPECT_FALSE(helper->should_show_icon());
  EXPECT_TRUE(helper->onion_location().is_empty());

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  BrowserList* browser_list = BrowserList::GetInstance();
  EXPECT_EQ(1U, browser_list->size());
  ASSERT_FALSE(browser_list->get(0)->profile()->IsTor());

  web_contents =
      browser_list->get(0)->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(web_contents->GetVisibleURL(), url);
}

IN_PROC_BROWSER_TEST_F(OnionLocationNavigationThrottleBrowserTest, ErrorPage) {
  auto* tor_browser = OpenTorWindow();
  const GURL url = test_server()->GetURL(kTestErrorPage);
  const GURL error_url("https://google.goom/");

  ui_test_utils::UrlLoadObserver observer(error_url);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(tor_browser, url));
  observer.Wait();

  content::WebContents* web_contents =
      tor_browser->tab_strip_model()->GetActiveWebContents();
  auto* helper = tor::OnionLocationTabHelper::FromWebContents(web_contents);

  EXPECT_EQ(error_url, web_contents->GetLastCommittedURL());
  EXPECT_TRUE(web_contents->GetPrimaryMainFrame()->IsErrorDocument());
  EXPECT_FALSE(helper->should_show_icon());
  EXPECT_TRUE(helper->onion_location().is_empty());
  EXPECT_FALSE(GetOnionLocationView(tor_browser)->GetVisible());
}
