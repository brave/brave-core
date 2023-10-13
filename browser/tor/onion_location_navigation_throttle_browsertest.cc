/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

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
#include "chrome/browser/auth_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/test/button_test_api.h"

namespace {

constexpr char kTestOnionPath[] = "/onion";
constexpr char kTestOnionURL[] = "https://brave.onion";
constexpr char kTestInvalidScheme[] = "/invalid_scheme";
constexpr char kTestInvalidSchemeURL[] = "brave://brave.onion";
constexpr char kTestNotOnion[] = "/not_onion";
constexpr char kTestNotOnionURL[] = "https://brave.com";

std::unique_ptr<net::test_server::HttpResponse> HandleOnionLocation(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  http_response->set_code(net::HTTP_OK);
  http_response->set_content_type("text/html");
  http_response->set_content("<html><head></head></html>");
  if (request.GetURL().path_piece() == kTestOnionPath) {
    http_response->AddCustomHeader("onion-location", kTestOnionURL);
  } else if (request.GetURL().path_piece() == kTestInvalidScheme) {
    http_response->AddCustomHeader("onion-location", kTestInvalidSchemeURL);
  } else if (request.GetURL().path_piece() == kTestNotOnion) {
    http_response->AddCustomHeader("onion-location", kTestNotOnionURL);
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
              brave_l10n::GetLocalizedResourceUTF16String(
                  (is_tor ? IDS_LOCATION_BAR_ONION_AVAILABLE
                          : IDS_LOCATION_BAR_OPEN_IN_TOR)));

    ui_test_utils::BrowserChangeObserver browser_creation_observer(
        nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);
    ui::MouseEvent pressed(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                           ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON,
                           ui::EF_LEFT_MOUSE_BUTTON);
    ui::MouseEvent released(ui::ET_MOUSE_RELEASED, gfx::Point(), gfx::Point(),
                            ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON,
                            ui::EF_LEFT_MOUSE_BUTTON);
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

IN_PROC_BROWSER_TEST_F(OnionLocationNavigationThrottleBrowserTest,
                       OnionLocationHeader) {
  auto* tor_browser = OpenTorWindow();
  for (auto* browser : {browser(), tor_browser}) {
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
                       OnionDomain_AutoOnionRedirect) {
  browser()->profile()->GetPrefs()->SetBoolean(tor::prefs::kAutoOnionRedirect,
                                               true);
  BrowserList* browser_list = BrowserList::GetInstance();
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("https://brave.com")));
  EXPECT_EQ(1U, browser_list->size());
  ASSERT_FALSE(browser_list->get(0)->profile()->IsTor());
  ASSERT_EQ(browser(), browser_list->get(0));

  ui_test_utils::BrowserChangeObserver browser_creation_observer(
      nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  content::TestNavigationObserver nav_observer(web_contents);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kTestOnionURL)));
  browser_creation_observer.Wait();
  nav_observer.Wait();
  // Original request was blocked
  EXPECT_EQ(nav_observer.last_net_error_code(), net::ERR_BLOCKED_BY_CLIENT);
  EXPECT_EQ(2U, browser_list->size());
  Browser* tor_browser = browser_list->get(1);
  ASSERT_TRUE(tor_browser->profile()->IsTor());
  content::WebContents* tor_web_contents =
      tor_browser->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(tor_web_contents->GetVisibleURL(), GURL(kTestOnionURL));
  // We don't close the original tab
  EXPECT_EQ(browser()->tab_strip_model()->count(), 1);
  // No new tab in Tor window
  EXPECT_EQ(tor_browser->tab_strip_model()->count(), 1);
}

IN_PROC_BROWSER_TEST_F(OnionLocationNavigationThrottleBrowserTest,
                       OnionDomain_AutoOnionRedirect_TorWindow) {
  browser()->profile()->GetPrefs()->SetBoolean(tor::prefs::kAutoOnionRedirect,
                                               true);
  auto* tor_browser = OpenTorWindow();
  content::WebContents* web_contents =
      tor_browser->tab_strip_model()->GetActiveWebContents();
  content::TestNavigationObserver nav_observer(web_contents);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(tor_browser, GURL(kTestOnionURL)));
  nav_observer.Wait();
  // It will load in the same Tor window with same tab
  EXPECT_EQ(nav_observer.last_net_error_code(), net::ERR_NAME_NOT_RESOLVED);
  EXPECT_EQ(tor_browser->tab_strip_model()->count(), 1);
}

IN_PROC_BROWSER_TEST_F(OnionLocationNavigationThrottleBrowserTest,
                       OnionDomain_AutoOnionRedirect_OffByDefault) {
  BrowserList* browser_list = BrowserList::GetInstance();
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("https://brave.com")));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kTestOnionURL)));
  EXPECT_EQ(1U, browser_list->size());
  ASSERT_FALSE(browser_list->get(0)->profile()->IsTor());

  content::WebContents* web_contents =
      browser_list->get(0)->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(web_contents->GetVisibleURL(), GURL(kTestOnionURL));
}

IN_PROC_BROWSER_TEST_F(OnionLocationNavigationThrottleBrowserTest,
                       OnionLocationHeader_AutoOnionRedirect) {
  browser()->profile()->GetPrefs()->SetBoolean(tor::prefs::kAutoOnionRedirect,
                                               true);
  ui_test_utils::BrowserChangeObserver browser_creation_observer(
      nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);

  GURL url = test_server()->GetURL("/onion");
  content::TestNavigationObserver nav_observer(
      browser()->tab_strip_model()->GetActiveWebContents());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  browser_creation_observer.Wait();
  // We don't close the original tab but stop loading
  EXPECT_EQ(browser()->tab_strip_model()->count(), 1);
  EXPECT_EQ(nav_observer.last_net_error_code(), net::ERR_BLOCKED_BY_RESPONSE);

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  tor::OnionLocationTabHelper* helper =
      tor::OnionLocationTabHelper::FromWebContents(web_contents);
  EXPECT_FALSE(helper->should_show_icon());
  EXPECT_TRUE(helper->onion_location().is_empty());

  BrowserList* browser_list = BrowserList::GetInstance();
  EXPECT_EQ(2U, browser_list->size());
  Browser* tor_browser = browser_list->get(1);
  ASSERT_TRUE(tor_browser->profile()->IsTor());
  web_contents = tor_browser->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(web_contents->GetVisibleURL(), GURL(kTestOnionURL));

  // Open a new tab and navigate to the url again
  NavigateParams params(
      NavigateParams(browser(), url, ui::PAGE_TRANSITION_TYPED));
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  ui_test_utils::NavigateToURL(&params);

  // We stll don't close the original tab
  EXPECT_EQ(browser()->tab_strip_model()->count(), 2);

  EXPECT_EQ(2U, browser_list->size());
  // No new tab in Tor window and unique one tab per site
  EXPECT_EQ(tor_browser->tab_strip_model()->count(), 1);
  web_contents = tor_browser->tab_strip_model()->GetWebContentsAt(0);
  EXPECT_EQ(web_contents->GetVisibleURL(), GURL(kTestOnionURL));
}

IN_PROC_BROWSER_TEST_F(OnionLocationNavigationThrottleBrowserTest,
                       OnionLocationHeader_AutoOnionRedirect_TorWindow) {
  browser()->profile()->GetPrefs()->SetBoolean(tor::prefs::kAutoOnionRedirect,
                                               true);

  auto* tor_browser = OpenTorWindow();
  GURL url = test_server()->GetURL("/onion");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(tor_browser, url));
  EXPECT_EQ(tor_browser->tab_strip_model()->count(), 2);
  EXPECT_EQ(tor_browser->tab_strip_model()->active_index(), 1);
  content::WebContents* web_contents =
      tor_browser->tab_strip_model()->GetWebContentsAt(0);
  tor::OnionLocationTabHelper* helper =
      tor::OnionLocationTabHelper::FromWebContents(web_contents);
  EXPECT_FALSE(helper->should_show_icon());
  EXPECT_TRUE(helper->onion_location().is_empty());

  // Open a new tab and navigate to the url again
  NavigateParams params(
      NavigateParams(tor_browser, url, ui::PAGE_TRANSITION_TYPED));
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  ui_test_utils::NavigateToURL(&params);

  // We stll don't close the original tab
  // No new tab in Tor window and unique one tab per site
  ASSERT_EQ(tor_browser->tab_strip_model()->count(), 3);
  EXPECT_EQ(
      tor_browser->tab_strip_model()->GetWebContentsAt(0)->GetVisibleURL(),
      url);
  EXPECT_EQ(
      tor_browser->tab_strip_model()->GetWebContentsAt(1)->GetVisibleURL(),
      GURL(kTestOnionURL));
  EXPECT_EQ(
      tor_browser->tab_strip_model()->GetWebContentsAt(2)->GetVisibleURL(),
      url);
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

  // AutoOnionLocationPref
  browser()->profile()->GetPrefs()->SetBoolean(tor::prefs::kAutoOnionRedirect,
                                               true);
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

  browser()->profile()->GetPrefs()->SetBoolean(tor::prefs::kAutoOnionRedirect,
                                               true);
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

  browser()->profile()->GetPrefs()->SetBoolean(tor::prefs::kAutoOnionRedirect,
                                               true);
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

  browser()->profile()->GetPrefs()->SetBoolean(tor::prefs::kAutoOnionRedirect,
                                               true);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  BrowserList* browser_list = BrowserList::GetInstance();
  EXPECT_EQ(1U, browser_list->size());
  ASSERT_FALSE(browser_list->get(0)->profile()->IsTor());

  web_contents =
      browser_list->get(0)->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(web_contents->GetVisibleURL(), url);
}

IN_PROC_BROWSER_TEST_F(OnionLocationNavigationThrottleBrowserTest,
                       RenderInitiatedNavigations) {
  browser()->profile()->GetPrefs()->SetBoolean(tor::prefs::kAutoOnionRedirect,
                                               true);
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("https://brave.com")));

  const char kScript[] = R"js(
    // Spam user.
    for (let i = 0; i < 5; i++) {
      document.location.href = 'http://spam' + i + '.onion'
    }
  )js";

  // Renderer initiated navigations.
  ui_test_utils::BrowserChangeObserver browser_creation_observer(
      nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);
  content::ExecJs(browser()->tab_strip_model()->GetActiveWebContents(),
                  kScript);
  browser_creation_observer.Wait();

  BrowserList* browser_list = BrowserList::GetInstance();
  EXPECT_EQ(2U, browser_list->size());
  EXPECT_TRUE(browser_list->get(1)->profile()->IsTor());
  EXPECT_EQ(1, browser_list->get(1)->tab_strip_model()->count());
  auto* tor_tab = browser_list->get(1)->tab_strip_model()->GetWebContentsAt(0);
  EXPECT_EQ(GURL("http://spam4.onion"), tor_tab->GetVisibleURL());

  // Browser initiated navigation.
  content::TestNavigationObserver nav_observer(
      browser()->tab_strip_model()->GetActiveWebContents());
  browser()->tab_strip_model()->GetActiveWebContents()->GetController().LoadURL(
      GURL("http://user.onion"), content::Referrer(), ui::PAGE_TRANSITION_TYPED,
      {});
  nav_observer.Wait();

  EXPECT_EQ(2U, browser_list->size());
  EXPECT_TRUE(browser_list->get(1)->profile()->IsTor());
  EXPECT_EQ(2, browser_list->get(1)->tab_strip_model()->count());
  EXPECT_EQ(GURL("http://spam4.onion"), browser_list->get(1)
                                            ->tab_strip_model()
                                            ->GetWebContentsAt(0)
                                            ->GetVisibleURL());
  EXPECT_EQ(GURL("http://user.onion"), browser_list->get(1)
                                           ->tab_strip_model()
                                           ->GetWebContentsAt(1)
                                           ->GetVisibleURL());
}
