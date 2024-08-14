/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/split_view/split_view_location_bar.h"

#include <memory>
#include <utility>

#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/timer/timer.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

class SplitViewLocationBarBrowserTest : public InProcessBrowserTest {
 public:
  SplitViewLocationBarBrowserTest()
      : scoped_features_(tabs::features::kBraveSplitView) {}
  ~SplitViewLocationBarBrowserTest() override = default;

  TabStripModel& tab_strip_model() { return *(browser()->tab_strip_model()); }

  SplitViewLocationBar& split_view_location_bar() {
    return *static_cast<BraveBrowserView*>(
                BrowserView::GetBrowserViewForBrowser(browser()))
                ->secondary_location_bar_;
  }

  // InProcessBrowserTest:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->Start());
    ASSERT_TRUE(embedded_https_test_server().Start());
  }

 private:
  base::test::ScopedFeatureList scoped_features_;
};

IN_PROC_BROWSER_TEST_F(SplitViewLocationBarBrowserTest,
                       VisibilityChangesWhenActiveTabChanges) {
  // Initially, the secondary location bar should be hidden
  EXPECT_FALSE(split_view_location_bar().GetWidget()->IsVisible());

  // When a new split view is created
  brave::NewSplitViewForTab(browser());
  ASSERT_EQ(1, tab_strip_model().GetIndexOfWebContents(
                   tab_strip_model().GetActiveWebContents()));

  // Then the secondary location bar should be visible
  EXPECT_TRUE(split_view_location_bar().GetWidget()->IsVisible());

  // When activating another tab that's not in a split view mode
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);

  // Then the secondary location bar should be hidden
  EXPECT_FALSE(split_view_location_bar().GetWidget()->IsVisible());

  // When switching back to one of tabs in split view mode
  tab_strip_model().ActivateTabAt(0);

  // Then the secondary location bar should be visible
  EXPECT_TRUE(split_view_location_bar().GetWidget()->IsVisible());

  // When breaking the split view
  brave::BreakTiles(browser());

  // Then the secondary location bar should be hidden
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(split_view_location_bar().GetWidget()->IsVisible());
}

IN_PROC_BROWSER_TEST_F(SplitViewLocationBarBrowserTest,
                       URLShouldBeUpdated_WhenActiveTabChanges) {
  brave::NewSplitViewForTab(browser());
  ASSERT_EQ(1, tab_strip_model().GetIndexOfWebContents(
                   tab_strip_model().GetActiveWebContents()));
  ASSERT_EQ("about:blank",
            tab_strip_model().GetWebContentsAt(0)->GetVisibleURL().spec());
  ASSERT_EQ(u"about:blank", split_view_location_bar().url_->GetText());

  // When activating another tab in split view,
  tab_strip_model().ActivateTabAt(0);

  // Then, the secondary location bar should be updated
  EXPECT_EQ("chrome://newtab/",
            tab_strip_model().GetWebContentsAt(1)->GetVisibleURL().spec());
  EXPECT_EQ(u"chrome://newtab", split_view_location_bar().url_->GetText());
}

IN_PROC_BROWSER_TEST_F(SplitViewLocationBarBrowserTest,
                       URLShouldBeUpdated_WhenNavigationHappens) {
  brave::NewSplitViewForTab(browser());
  ASSERT_EQ(1, tab_strip_model().GetIndexOfWebContents(
                   tab_strip_model().GetActiveWebContents()));
  ASSERT_EQ("about:blank",
            tab_strip_model().GetWebContentsAt(0)->GetVisibleURL().spec());
  ASSERT_EQ(u"about:blank", split_view_location_bar().url_->GetText());
  auto url_text = split_view_location_bar().url_->GetText();

  auto navigate_to_url = [&](content::WebContents* web_contents,
                             const GURL& new_url) {
    // Note that we're not using content::NavigateToURL() here because it
    // activates the contents. As split view location bar is for inactive tab,
    // the navigation shouldn't trigger activation.
    ASSERT_TRUE(
        content::ExecJs(tab_strip_model().GetWebContentsAt(0),
                        "window.location.href = '" + new_url.spec() + "';"));
  };

  // When navigating to a new URL: http
  navigate_to_url(tab_strip_model().GetWebContentsAt(0),
                  GURL("http://example.com"));
  ASSERT_EQ(1, tab_strip_model().GetIndexOfWebContents(
                   tab_strip_model().GetActiveWebContents()));

  // Then, the secondary location bar should be updated
  std::unique_ptr<base::RunLoop> run_loop;
  base::RepeatingTimer scheduler;
  auto wait_until_url_text_changes = [&]() {
    auto run_loop = std::make_unique<base::RunLoop>();
    scheduler.Start(
        FROM_HERE, base::Milliseconds(100), base::BindLambdaForTesting([&]() {
          if (url_text != split_view_location_bar().url_->GetText()) {
            run_loop->Quit();
            scheduler.Stop();
            url_text = split_view_location_bar().url_->GetText();
          }
        }));
    run_loop->Run();
  };
  wait_until_url_text_changes();

  EXPECT_EQ("http://example.com/",
            tab_strip_model().GetWebContentsAt(0)->GetVisibleURL().spec());
  EXPECT_EQ(u"example.com", url_text);

  // When navigating to another URL with https
  // Note that embedded_https_test_server() has registered valid cert for
  // a.com.
  navigate_to_url(tab_strip_model().GetWebContentsAt(0), GURL("https://a.com"));
  wait_until_url_text_changes();

  // Then, the secondary location bar should be updated
  EXPECT_EQ("https://a.com/",
            tab_strip_model().GetWebContentsAt(0)->GetVisibleURL().spec());
  EXPECT_EQ(u"a.com", url_text);

  // When navigating to another URL: bad cert
  net::EmbeddedTestServer https_server(net::EmbeddedTestServer::TYPE_HTTPS);
  net::SSLServerConfig ssl_config;
  ssl_config.client_cert_type =
      net::SSLServerConfig::ClientCertType::REQUIRE_CLIENT_CERT;
  https_server.SetSSLConfig(net::EmbeddedTestServer::CERT_BAD_VALIDITY,
                            ssl_config);
  https_server.ServeFilesFromSourceDirectory("chrome/test/data");
  ASSERT_TRUE(https_server.Start());
  GURL bad_url = https_server.GetURL("/");

  ASSERT_FALSE(split_view_location_bar().https_with_strike_->GetVisible());
  ASSERT_FALSE(split_view_location_bar().scheme_separator_->GetVisible());

  navigate_to_url(tab_strip_model().GetWebContentsAt(0), bad_url);
  wait_until_url_text_changes();

  // Then the https with strike should be visible
  EXPECT_TRUE(split_view_location_bar().https_with_strike_->GetVisible());
  EXPECT_TRUE(split_view_location_bar().scheme_separator_->GetVisible());

  // When Navigating to valid URL,
  navigate_to_url(tab_strip_model().GetWebContentsAt(0), GURL("http://a.com"));
  wait_until_url_text_changes();

  // Then the https with strike should be hidden
  EXPECT_FALSE(split_view_location_bar().https_with_strike_->GetVisible());
  EXPECT_FALSE(split_view_location_bar().scheme_separator_->GetVisible());
}
