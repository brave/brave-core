/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/path_service.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "url/gurl.h"

namespace {

const char kEmbeddedTestServerDirectory[] = "window_name";
const char kWindowNameScript[] = "window.name";
const char kLinkID[] = "clickme";

}  // namespace

class BraveWindowNameBrowserTest : public InProcessBrowserTest {
 public:
  BraveWindowNameBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    test_data_dir = test_data_dir.AppendASCII(kEmbeddedTestServerDirectory);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    EXPECT_TRUE(https_server_.Start());
  }

  BraveWindowNameBrowserTest(const BraveWindowNameBrowserTest&) = delete;
  BraveWindowNameBrowserTest& operator=(const BraveWindowNameBrowserTest&) =
      delete;

  ~BraveWindowNameBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    content_client_ = std::make_unique<ChromeContentClient>();
    content::SetContentClient(content_client_.get());
    browser_content_client_ = std::make_unique<BraveContentBrowserClient>();
    content::SetBrowserClientForTesting(browser_content_client_.get());
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  void TearDown() override {
    browser_content_client_.reset();
    content_client_.reset();
  }

 protected:
  net::EmbeddedTestServer https_server_;

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void SetHref(const std::string& id, const std::string& href) {
    content::RenderFrameHost* frame = web_contents()->GetPrimaryMainFrame();
    frame->ExecuteJavaScriptForTests(
        base::ASCIIToUTF16("document.getElementById('" + id + "').href='" +
                           href + "';\n"),
        base::NullCallback());
  }

  void Click(const std::string& id) {
    content::TestNavigationObserver observer(web_contents());
    content::RenderFrameHost* frame = web_contents()->GetPrimaryMainFrame();
    frame->ExecuteJavaScriptForTests(
        base::ASCIIToUTF16("document.getElementById('" + id + "').click();\n"),
        base::NullCallback());
    observer.WaitForNavigationFinished();
  }

 private:
  std::unique_ptr<ChromeContentClient> content_client_;
  std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
};

IN_PROC_BROWSER_TEST_F(BraveWindowNameBrowserTest, SameOrigin) {
  GURL url1 =
      https_server_.GetURL("a.test", "/set_window_name_same_origin.html");
  EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url1));
  // Navigating to url1 automatically redirects to "get_window_name.html". Since
  // the original and final URLs are in the same origin, window.name should
  // persist across navigation.
  EXPECT_EQ("foo", EvalJs(web_contents(), kWindowNameScript));
}

IN_PROC_BROWSER_TEST_F(BraveWindowNameBrowserTest, CrossOrigin) {
  GURL url1 = https_server_.GetURL("a.test", "/set_window_name.html");
  GURL url2 = https_server_.GetURL("b.test", "/get_window_name.html");

  EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url1));
  EXPECT_EQ("foo", EvalJs(web_contents(), kWindowNameScript));
  SetHref(kLinkID, url2.spec());
  Click(kLinkID);
  // Since these URLs are in different origins, window.name should be cleared
  // during navigation.
  EXPECT_EQ("", EvalJs(web_contents(), kWindowNameScript));
}

IN_PROC_BROWSER_TEST_F(BraveWindowNameBrowserTest, CrossOriginAndBack) {
  GURL url1 = https_server_.GetURL("a.test", "/set_window_name.html");
  GURL url2 = https_server_.GetURL("b.test", "/get_window_name.html");

  EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url1));
  EXPECT_EQ("foo", EvalJs(web_contents(), kWindowNameScript));
  SetHref(kLinkID, url2.spec());
  Click(kLinkID);
  // Since these URLs are in different origins, window.name should be cleared
  // during navigation.
  EXPECT_EQ("", EvalJs(web_contents(), kWindowNameScript));
  web_contents()->GetController().GoBack();
  EXPECT_TRUE(WaitForLoadStop(web_contents()));
  EXPECT_EQ("foo", EvalJs(web_contents(), kWindowNameScript));
}
