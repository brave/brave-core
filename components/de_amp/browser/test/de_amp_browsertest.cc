/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "brave/components/de_amp/common/features.h"
#include "brave/components/de_amp/pref_names.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/testing_pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "third_party/re2/src/re2/re2.h"

const char kTestHost[] = "a.test.com";
const char kTestAmpPage[] = "/test.html";
const char kTestSimpleNonAmpPage[] = "/simple.html";
const char kTestCanonicalPage[] = "/simple_canonical.html";
const char kTestBody[] =
    R"(
      <html amp>
    <head>
    <link rel='canonical'
    href='https://%s:%s%s'>
    </head>
    </html>
    )";
class DeAmpBrowserTest : public InProcessBrowserTest {
 public:
  DeAmpBrowserTest() {
    feature_list_.InitAndEnableFeature(de_amp::features::kBraveDeAMP);
  }

  void SetUpOnMainThread() override {
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));

    // Serve test files
    base::FilePath path;
    base::PathService::Get(base::DIR_SRC_TEST_DATA_ROOT, &path);
    path = path.Append(FILE_PATH_LITERAL("brave"));
    path = path.Append(FILE_PATH_LITERAL("components"));
    path = path.Append(FILE_PATH_LITERAL("de_amp"));
    path = path.Append(FILE_PATH_LITERAL("browser"));
    path = path.Append(FILE_PATH_LITERAL("test"));
    path = path.Append(FILE_PATH_LITERAL("data"));
    https_server_->ServeFilesFromDirectory(path);

    prefs_ = browser()->profile()->GetPrefs();

    content::SetupCrossSiteRedirector(https_server_.get());
    InProcessBrowserTest::SetUpOnMainThread();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void TogglePref(const bool on) {
    prefs_->SetBoolean(de_amp::kDeAmpPrefEnabled, on);
    web_contents()->GetController().Reload(content::ReloadType::NORMAL, false);
  }

  void NavigateToURLAndWaitForRedirects(const GURL& original_url,
                                        const GURL& landing_url) {
    ui_test_utils::UrlLoadObserver load_complete(
        landing_url, content::NotificationService::AllSources());
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), original_url));
    load_complete.Wait();
    EXPECT_EQ(web_contents()->GetLastCommittedURL(), landing_url);
  }

  void GoBack(Browser* browser) {
    content::TestNavigationObserver observer(web_contents());
    chrome::GoBack(browser, WindowOpenDisposition::CURRENT_TAB);
    observer.Wait();
  }

  void GoForward(Browser* browser) {
    content::TestNavigationObserver observer(web_contents());
    chrome::GoForward(browser, WindowOpenDisposition::CURRENT_TAB);
    observer.Wait();
  }

 protected:
  std::unique_ptr<net::EmbeddedTestServer> https_server_;

 private:
  base::test::ScopedFeatureList feature_list_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  PrefService* prefs_;
};

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const std::string& base_url,
    const std::string& canonical_page,
    const std::string& body,
    const net::test_server::HttpRequest& request) {
  const auto port = request.GetURL().port();

  auto http_response = std::make_unique<net::test_server::BasicHttpResponse>();
  http_response->set_code(net::HTTP_OK);
  http_response->set_content_type("text/html");
  // ... rel="canonical" href="https://%s:%s%s" becomes
  // href="https://<test server base url>:<test port>/<canonical html page>"
  http_response->set_content(base::StringPrintf(
      body.c_str(), base_url.c_str(), port.c_str(), canonical_page.c_str()));
  return http_response;
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, SimpleDeAmp) {
  TogglePref(true);
  ASSERT_TRUE(https_server_->Start());

  // Go to any page
  const GURL simple = https_server_->GetURL(kTestHost, kTestSimpleNonAmpPage);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), simple));
  EXPECT_EQ(web_contents()->GetLastCommittedURL(), simple);

  // Now go to an AMP page
  https_server_.reset(new net::EmbeddedTestServer(
      net::test_server::EmbeddedTestServer::TYPE_HTTPS));
  https_server_->RegisterRequestHandler(base::BindRepeating(
      HandleRequest, kTestHost, kTestCanonicalPage, kTestBody));
  ASSERT_TRUE(https_server_->Start());
  const GURL original_url = https_server_->GetURL(kTestHost, kTestAmpPage);
  const GURL landing_url = https_server_->GetURL(kTestHost, kTestCanonicalPage);
  NavigateToURLAndWaitForRedirects(original_url, landing_url);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, NonHttpScheme) {
  TogglePref(true);
  const std::string nonHttpSchemeBody =
      R"(
        <html amp>\
      <head>\
      <link rel='canonical' 
      href='brave://settings'>\
      </head></html>
      )";
  https_server_->RegisterRequestHandler(base::BindRepeating(
      HandleRequest, kTestHost, kTestCanonicalPage, nonHttpSchemeBody));
  ASSERT_TRUE(https_server_->Start());
  const GURL original_url = https_server_->GetURL(kTestHost, kTestAmpPage);
  NavigateToURLAndWaitForRedirects(original_url, original_url);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, CanonicalLinkSameAsAmpPage) {
  TogglePref(true);
  https_server_->RegisterRequestHandler(
      base::BindRepeating(HandleRequest, kTestHost, kTestAmpPage, kTestBody));
  ASSERT_TRUE(https_server_->Start());
  const GURL original_url = https_server_->GetURL(kTestHost, kTestAmpPage);
  NavigateToURLAndWaitForRedirects(original_url, original_url);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, PrefOff) {
  TogglePref(false);
  https_server_->RegisterRequestHandler(base::BindRepeating(
      HandleRequest, kTestHost, kTestCanonicalPage, kTestBody));
  ASSERT_TRUE(https_server_->Start());
  const GURL original_url = https_server_->GetURL(kTestHost, kTestAmpPage);
  // Doesn't get De-AMPed
  NavigateToURLAndWaitForRedirects(original_url, original_url);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, RestorePage) {
  TogglePref(true);
  https_server_->RegisterRequestHandler(base::BindRepeating(
      HandleRequest, kTestHost, kTestCanonicalPage, kTestBody));
  ASSERT_TRUE(https_server_->Start());
  const GURL original_url = https_server_->GetURL(kTestHost, kTestAmpPage);
  const GURL landing_url = https_server_->GetURL(kTestHost, kTestCanonicalPage);
  NavigateToURLAndWaitForRedirects(original_url, landing_url);

  // Close & restore tab
  browser()->tab_strip_model()->CloseSelectedTabs();
  Profile* profile = browser()->profile();
  chrome::OpenWindowWithRestoredTabs(profile);

  EXPECT_EQ(web_contents()->GetLastCommittedURL(), landing_url);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, BackAndForwardNavigatesAlsoRedirect) {
  TogglePref(true);
  ASSERT_TRUE(https_server_->Start());

  // Go to any page
  const GURL simple = https_server_->GetURL(kTestHost, kTestSimpleNonAmpPage);
  NavigateToURLAndWaitForRedirects(simple, simple);

  // Now go to an AMP page
  https_server_.reset(new net::EmbeddedTestServer(
      net::test_server::EmbeddedTestServer::TYPE_HTTPS));
  https_server_->RegisterRequestHandler(base::BindRepeating(
      HandleRequest, kTestHost, kTestCanonicalPage, kTestBody));
  ASSERT_TRUE(https_server_->Start());

  const GURL original_url1 = https_server_->GetURL(kTestHost, kTestAmpPage);
  const GURL landing_url1 =
      https_server_->GetURL(kTestHost, kTestCanonicalPage);
  NavigateToURLAndWaitForRedirects(original_url1, landing_url1);

  // Go to another AMP page
  const std::string another_canonical_page = "/simple_canonical2.html";
  https_server_.reset(new net::EmbeddedTestServer(
      net::test_server::EmbeddedTestServer::TYPE_HTTPS));
  https_server_->RegisterRequestHandler(base::BindRepeating(
      HandleRequest, kTestHost, another_canonical_page, kTestBody));
  ASSERT_TRUE(https_server_->Start());

  const GURL original_url2 = https_server_->GetURL(kTestHost, kTestAmpPage);
  const GURL landing_url2 =
      https_server_->GetURL(kTestHost, another_canonical_page);
  NavigateToURLAndWaitForRedirects(original_url2, landing_url2);

  // Going back and forward in history should ignore the non-AMP page
  GoBack(browser());
  EXPECT_EQ(web_contents()->GetLastCommittedURL(), landing_url1);
  GoBack(browser());
  EXPECT_EQ(web_contents()->GetLastCommittedURL(), simple);

  GoForward(browser());
  EXPECT_EQ(web_contents()->GetLastCommittedURL(), landing_url1);
  GoForward(browser());
  EXPECT_EQ(web_contents()->GetLastCommittedURL(), landing_url2);
}

class DeAmpFeatureFlagOffBrowserTest : public DeAmpBrowserTest {
 public:
  DeAmpFeatureFlagOffBrowserTest() {
    feature_list_.InitAndDisableFeature(de_amp::features::kBraveDeAMP);
  }

  void SetUpOnMainThread() override { DeAmpBrowserTest::SetUpOnMainThread(); }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(DeAmpFeatureFlagOffBrowserTest, DoesNotDeAmp) {
  https_server_->RegisterRequestHandler(base::BindRepeating(
      HandleRequest, kTestHost, kTestCanonicalPage, kTestBody));
  ASSERT_TRUE(https_server_->Start());
  const GURL original_url = https_server_->GetURL(kTestHost, kTestAmpPage);
  // Doesn't get De-AMPed
  NavigateToURLAndWaitForRedirects(original_url, original_url);
}
