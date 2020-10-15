/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/tor/onion_location_tab_helper.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "brave/browser/ui/views/location_bar/onion_location_view.h"
#include "brave/components/tor/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/browser_test.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

namespace {

constexpr char kTestOnionPath[] = "/onion";
constexpr char kTestOnionURL[] = "https://brave.onion";

std::unique_ptr<net::test_server::HttpResponse> HandleOnionLocation(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  if (request.GetURL().path_piece() == kTestOnionPath) {
    http_response->set_code(net::HTTP_OK);
    http_response->set_content_type("text/html");
    http_response->set_content("<html><head></head></html>");
    http_response->AddCustomHeader("onion-location", kTestOnionURL);
  } else {
    http_response->set_code(net::HTTP_OK);
    http_response->set_content_type("text/html");
    http_response->set_content("<html><head></head></html>");
  }
  return std::move(http_response);
}
}  // namespace

class OnionLocationNavigationThrottleBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    test_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    test_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    test_server_->RegisterRequestHandler(base::Bind(&HandleOnionLocation));
    ASSERT_TRUE(test_server_->Start());
  }

  net::EmbeddedTestServer* test_server() { return test_server_.get(); }

  void CheckOnionLocationLabel(Browser* browser) {
    BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
    ASSERT_NE(browser_view, nullptr);
    BraveLocationBarView* brave_location_bar_view =
        static_cast<BraveLocationBarView*>(browser_view->GetLocationBarView());
    ASSERT_NE(brave_location_bar_view, nullptr);
    views::LabelButton* onion_button =
        brave_location_bar_view->GetOnionLocationView()->GetButton();
    EXPECT_TRUE(onion_button->GetVisible());
    EXPECT_EQ(onion_button->GetText(),
              l10n_util::GetStringUTF16((IDS_LOCATION_BAR_OPEN_IN_TOR)));
  }

 private:
  std::unique_ptr<net::EmbeddedTestServer> test_server_;
};

IN_PROC_BROWSER_TEST_F(OnionLocationNavigationThrottleBrowserTest,
                       OnionLocationHeader) {
  GURL url1 = test_server()->GetURL("/onion");
  ui_test_utils::NavigateToURL(browser(), url1);
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  tor::OnionLocationTabHelper* helper =
      tor::OnionLocationTabHelper::FromWebContents(web_contents);
  EXPECT_TRUE(helper->should_show_icon());
  EXPECT_EQ(helper->onion_location(), GURL(kTestOnionURL));
  CheckOnionLocationLabel(browser());

  GURL url2 = test_server()->GetURL("/no_onion");
  ui_test_utils::NavigateToURL(browser(), url2);
  web_contents = browser()->tab_strip_model()->GetActiveWebContents();
  helper = tor::OnionLocationTabHelper::FromWebContents(web_contents);
  EXPECT_FALSE(helper->should_show_icon());
  EXPECT_TRUE(helper->onion_location().is_empty());
}

IN_PROC_BROWSER_TEST_F(OnionLocationNavigationThrottleBrowserTest,
                       OnionDomain) {
  BrowserList* browser_list = BrowserList::GetInstance();
  ui_test_utils::NavigateToURL(browser(), GURL("https://brave.com"));
  EXPECT_EQ(1U, browser_list->size());
  ASSERT_FALSE(brave::IsTorProfile(browser_list->get(0)->profile()));

  content::WindowedNotificationObserver tor_browser_creation_observer(
      chrome::NOTIFICATION_BROWSER_OPENED,
      content::NotificationService::AllSources());
  ui_test_utils::NavigateToURL(browser(), GURL(kTestOnionURL));
  tor_browser_creation_observer.Wait();
  EXPECT_EQ(2U, browser_list->size());
  ASSERT_TRUE(brave::IsTorProfile(browser_list->get(1)->profile()));
  content::WebContents* web_contents =
      browser_list->get(1)->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(web_contents->GetURL(), GURL(kTestOnionURL));
}

IN_PROC_BROWSER_TEST_F(OnionLocationNavigationThrottleBrowserTest,
                       OnionDomain_TorWindow) {
  content::WindowedNotificationObserver tor_browser_creation_observer(
      chrome::NOTIFICATION_BROWSER_OPENED,
      content::NotificationService::AllSources());
  brave::NewOffTheRecordWindowTor(browser());
  tor_browser_creation_observer.Wait();

  BrowserList* browser_list = BrowserList::GetInstance();
  Browser* tor_browser = browser_list->get(1);
  ASSERT_TRUE(brave::IsTorProfile(tor_browser->profile()));
  EXPECT_EQ(2U, browser_list->size());

  ui_test_utils::NavigateToURL(browser(), GURL("https://brave.com"));
  ui_test_utils::NavigateToURL(browser(), GURL(kTestOnionURL));
  EXPECT_EQ(2U, browser_list->size());
  content::WebContents* web_contents =
      tor_browser->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(web_contents->GetURL(), GURL(kTestOnionURL));
  EXPECT_EQ(tor_browser->tab_strip_model()->count(), 2);
}

IN_PROC_BROWSER_TEST_F(OnionLocationNavigationThrottleBrowserTest,
                       AutoOnionLocationPref) {
  browser()->profile()->GetPrefs()->SetBoolean(tor::prefs::kAutoOnionLocation,
                                               true);
  content::WindowedNotificationObserver tor_browser_creation_observer(
      chrome::NOTIFICATION_BROWSER_OPENED,
      content::NotificationService::AllSources());

  GURL url = test_server()->GetURL("/onion");
  ui_test_utils::NavigateToURL(browser(), url);
  tor_browser_creation_observer.Wait();
  // Last tab will not be closed
  EXPECT_EQ(browser()->tab_strip_model()->count(), 1);
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  tor::OnionLocationTabHelper* helper =
      tor::OnionLocationTabHelper::FromWebContents(web_contents);
  EXPECT_FALSE(helper->should_show_icon());
  EXPECT_TRUE(helper->onion_location().is_empty());

  BrowserList* browser_list = BrowserList::GetInstance();
  EXPECT_EQ(2U, browser_list->size());
  Browser* tor_browser = browser_list->get(1);
  ASSERT_TRUE(brave::IsTorProfile(tor_browser->profile()));
  web_contents = tor_browser->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(web_contents->GetURL(), GURL(kTestOnionURL));

  // Open a new tab
  NavigateParams params(
      NavigateParams(browser(), url, ui::PAGE_TRANSITION_TYPED));
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  ui_test_utils::NavigateToURL(&params);

  EXPECT_EQ(browser()->tab_strip_model()->count(), 1);

  EXPECT_EQ(2U, browser_list->size());
  web_contents = tor_browser->tab_strip_model()->GetWebContentsAt(2);
  EXPECT_EQ(tor_browser->tab_strip_model()->count(), 3);
  EXPECT_EQ(web_contents->GetURL(), GURL(kTestOnionURL));
}

IN_PROC_BROWSER_TEST_F(OnionLocationNavigationThrottleBrowserTest,
                       TorDisabled) {
  // Disable tor
  TorProfileServiceFactory::SetTorDisabled(true);

  // OnionLocationHeader_
  GURL url = test_server()->GetURL("/onion");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  tor::OnionLocationTabHelper* helper =
      tor::OnionLocationTabHelper::FromWebContents(web_contents);
  EXPECT_FALSE(helper->should_show_icon());
  EXPECT_TRUE(helper->onion_location().is_empty());

  // Onion Domain
  ui_test_utils::NavigateToURL(browser(), GURL(kTestOnionURL));
  BrowserList* browser_list = BrowserList::GetInstance();
  EXPECT_EQ(1U, browser_list->size());

  // AutoOnionLocationPref
  browser()->profile()->GetPrefs()->SetBoolean(tor::prefs::kAutoOnionLocation,
                                               true);
  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_EQ(1U, browser_list->size());
}
