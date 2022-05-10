/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/bind.h"
#include "brave/components/de_amp/common/features.h"
#include "brave/components/de_amp/common/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/keep_alive/profile_keep_alive_types.h"
#include "chrome/browser/profiles/keep_alive/scoped_profile_keep_alive.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/reload_type.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/base/net_errors.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_MAC)
#include "content/public/common/content_features.h"
#endif

const char kTestHost[] = "a.test.com";
const char kTestAmpPage[] = "/test";
const char kTestRedirectingAmpPage1[] = "/redirecting_amp_page_1";
const char kTestRedirectingAmpPage2[] = "/redirecting_amp_page_2";
const char kTestSimpleNonAmpPage[] = "/simple";
const char kTestCanonicalPage[] = "/simple_canonical";
const char kTestAmpBody[] =
    R"(
    <html amp>
    <head>
    <link rel='canonical'
    href='https://%s:%s%s'>
    </head>
    </html>
    )";
const char kTestNonAmpBody[] =
    R"(
    <html>
    <head>
    </head>
    <body>Simple page</body>
    </html>
    )";
class DeAmpBrowserTest : public InProcessBrowserTest {
 public:
  DeAmpBrowserTest() {
    std::vector<base::Feature> disabled_features = {};
#if BUILDFLAG(IS_MAC)
    // On Mac, the DeAmpBrowserTest.AmpURLNotStoredInHistory test crashes
    // due to https://crbug.com/1284500: DCHECK in
    // blink::ContentToVisibleTimeReporter::TabWasShown when BFCache is
    // used. To get around the crash, disabling BFCache for these tests
    // until the upstream bug is fixed.
    disabled_features.push_back({features::kBackForwardCache});
#endif  // BUILDFLAG(IS_MAC)
    feature_list_.InitWithFeatures(
        /*enabled_features*/ {de_amp::features::kBraveDeAMP},
        disabled_features);
  }

  void SetUpOnMainThread() override {
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
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
  base::test::ScopedFeatureList feature_list_;

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  PrefService* prefs_;
};

std::unique_ptr<net::test_server::HttpResponse> BuildHttpResponseForAmpPage(
    const std::string& body,
    const std::string& canonical_link,
    const net::test_server::HttpRequest& request) {
  const auto port = request.GetURL().port();

  auto http_response = std::make_unique<net::test_server::BasicHttpResponse>();
  http_response->set_code(net::HTTP_OK);
  http_response->set_content_type("text/html");
  // ... rel="canonical" href="https://%s:%s%s" becomes
  // href="https://<test server base url>:<test port>/<canonical html page>"
  http_response->set_content(base::StringPrintf(
      body.c_str(), kTestHost, port.c_str(), canonical_link.c_str()));
  return http_response;
}

std::unique_ptr<net::test_server::HttpResponse> HandleRequestForRedirectTest(
    const std::string& body,
    const net::test_server::HttpRequest& request) {
  if (request.relative_url == kTestRedirectingAmpPage1) {
    return BuildHttpResponseForAmpPage(body, kTestRedirectingAmpPage2, request);
  } else if (request.relative_url == kTestRedirectingAmpPage2) {
    return BuildHttpResponseForAmpPage(body, kTestRedirectingAmpPage1, request);
  }
  return nullptr;
}

std::unique_ptr<net::test_server::HttpResponse> HandleServerRedirect(
    net::HttpStatusCode code,
    const std::string& source,
    const std::string& dest,
    const net::test_server::HttpRequest& request) {
  GURL request_url = request.GetURL();

  if (request_url.path() == source) {
    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->set_code(code);
    http_response->AddCustomHeader("Location", dest);
    http_response->set_content_type("text/html");
    http_response->set_content(base::StringPrintf(
        "<html><head></head><body>Redirecting to %s</body></html>",
        dest.c_str()));
    return http_response;
  } else {
    return nullptr;
  }
}

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const std::string& canonical_link,
    const std::string& body,
    const net::test_server::HttpRequest& request) {
  return BuildHttpResponseForAmpPage(body, canonical_link, request);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, SimpleDeAmp) {
  TogglePref(true);
  https_server_->RegisterRequestHandler(base::BindRepeating(
      HandleRequest, kTestSimpleNonAmpPage, kTestNonAmpBody));

  ASSERT_TRUE(https_server_->Start());

  // Go to any page
  const GURL simple = https_server_->GetURL(kTestHost, kTestSimpleNonAmpPage);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), simple));
  EXPECT_EQ(web_contents()->GetLastCommittedURL(), simple);

  // Now go to an AMP page
  https_server_.reset(new net::EmbeddedTestServer(
      net::test_server::EmbeddedTestServer::TYPE_HTTPS));
  https_server_->RegisterRequestHandler(
      base::BindRepeating(HandleRequest, kTestCanonicalPage, kTestAmpBody));
  ASSERT_TRUE(https_server_->Start());
  const GURL original_url = https_server_->GetURL(kTestHost, kTestAmpPage);
  const GURL landing_url = https_server_->GetURL(kTestHost, kTestCanonicalPage);
  NavigateToURLAndWaitForRedirects(original_url, landing_url);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, AmpPagesPointingAtEachOther) {
  TogglePref(true);
  https_server_->RegisterRequestHandler(
      base::BindRepeating(HandleRequestForRedirectTest, kTestAmpBody));
  ASSERT_TRUE(https_server_->Start());
  const GURL original_url =
      https_server_->GetURL(kTestHost, kTestRedirectingAmpPage1);
  const GURL landing_url =
      https_server_->GetURL(kTestHost, kTestRedirectingAmpPage2);
  NavigateToURLAndWaitForRedirects(original_url, landing_url);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, CanonicalRedirectsToAmp) {
  TogglePref(true);
  https_server_->RegisterRequestHandler(
      base::BindRepeating(HandleRequest, kTestCanonicalPage, kTestAmpBody));
  https_server_->RegisterRequestHandler(base::BindRepeating(
      HandleServerRedirect, net::HttpStatusCode::HTTP_PERMANENT_REDIRECT,
      kTestCanonicalPage, kTestAmpPage));
  ASSERT_TRUE(https_server_->Start());

  const GURL amp_url = https_server_->GetURL(kTestHost, kTestAmpPage);
  const GURL canonical_url =
      https_server_->GetURL(kTestHost, kTestCanonicalPage);

  NavigateToURLAndWaitForRedirects(amp_url, canonical_url);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, NonHttpScheme) {
  TogglePref(true);
  const char nonHttpSchemeBody[] =
      R"(
      <html amp>
      <head>
      <link rel='canonical' 
      href='brave://settings'>
      </head></html>
      )";
  https_server_->RegisterRequestHandler(base::BindRepeating(
      HandleRequest, kTestCanonicalPage, nonHttpSchemeBody));
  ASSERT_TRUE(https_server_->Start());
  const GURL original_url = https_server_->GetURL(kTestHost, kTestAmpPage);
  NavigateToURLAndWaitForRedirects(original_url, original_url);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, CanonicalLinkSameAsAmpPage) {
  TogglePref(true);
  https_server_->RegisterRequestHandler(
      base::BindRepeating(HandleRequest, kTestAmpPage, kTestAmpBody));
  ASSERT_TRUE(https_server_->Start());
  const GURL original_url = https_server_->GetURL(kTestHost, kTestAmpPage);
  NavigateToURLAndWaitForRedirects(original_url, original_url);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, PrefOff) {
  TogglePref(false);
  https_server_->RegisterRequestHandler(
      base::BindRepeating(HandleRequest, kTestCanonicalPage, kTestAmpBody));
  ASSERT_TRUE(https_server_->Start());
  const GURL original_url = https_server_->GetURL(kTestHost, kTestAmpPage);
  // Doesn't get De-AMPed
  NavigateToURLAndWaitForRedirects(original_url, original_url);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, RestoreTab) {
  TogglePref(true);
  https_server_->RegisterRequestHandler(
      base::BindRepeating(HandleRequest, kTestCanonicalPage, kTestAmpBody));
  ASSERT_TRUE(https_server_->Start());
  const GURL original_url = https_server_->GetURL(kTestHost, kTestAmpPage);
  const GURL landing_url = https_server_->GetURL(kTestHost, kTestCanonicalPage);
  NavigateToURLAndWaitForRedirects(original_url, landing_url);
  Profile* profile = browser()->profile();

  ScopedKeepAlive test_keep_alive(KeepAliveOrigin::PANEL_VIEW,
                                  KeepAliveRestartOption::DISABLED);
  ScopedProfileKeepAlive test_profile_keep_alive(
      profile, ProfileKeepAliveOrigin::kBrowserWindow);
  CloseBrowserSynchronously(browser());

  EXPECT_EQ(0u, BrowserList::GetInstance()->size());
  chrome::OpenWindowWithRestoredTabs(profile);
  EXPECT_EQ(1u, BrowserList::GetInstance()->size());
  SelectFirstBrowser();

  EXPECT_EQ(web_contents()->GetLastCommittedURL(), landing_url);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, AmpURLNotStoredInHistory) {
  TogglePref(true);
  https_server_->RegisterRequestHandler(base::BindRepeating(
      HandleRequest, kTestSimpleNonAmpPage, kTestNonAmpBody));

  ASSERT_TRUE(https_server_->Start());

  // Go to any page
  const GURL simple = https_server_->GetURL(kTestHost, kTestSimpleNonAmpPage);
  NavigateToURLAndWaitForRedirects(simple, simple);

  // Now go to an AMP page
  https_server_.reset(new net::EmbeddedTestServer(
      net::test_server::EmbeddedTestServer::TYPE_HTTPS));
  https_server_->RegisterRequestHandler(
      base::BindRepeating(HandleRequest, kTestCanonicalPage, kTestAmpBody));
  ASSERT_TRUE(https_server_->Start());

  const GURL original_url1 = https_server_->GetURL(kTestHost, kTestAmpPage);
  const GURL landing_url1 =
      https_server_->GetURL(kTestHost, kTestCanonicalPage);
  NavigateToURLAndWaitForRedirects(original_url1, landing_url1);

  // Go to another AMP page
  const std::string another_canonical_page = "/simple_canonical2.html";
  https_server_.reset(new net::EmbeddedTestServer(
      net::test_server::EmbeddedTestServer::TYPE_HTTPS));
  https_server_->RegisterRequestHandler(
      base::BindRepeating(HandleRequest, another_canonical_page, kTestAmpBody));
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

// Inspired by view-source test:
// chrome/browser/tab_contents/view_source_browsertest.cc
IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, NonDeAmpedPageSameAsOriginal) {
  TogglePref(true);
  https_server_->RegisterRequestHandler(base::BindRepeating(
      HandleRequest, kTestSimpleNonAmpPage, kTestNonAmpBody));
  ASSERT_TRUE(https_server_->Start());

  const GURL original_url =
      https_server_->GetURL(kTestHost, kTestSimpleNonAmpPage);
  NavigateToURLAndWaitForRedirects(original_url, original_url);
  content::RenderFrameHost* current_main_frame = web_contents()->GetMainFrame();
  // Open View Source for page
  content::WebContentsAddedObserver view_source_contents_observer;
  current_main_frame->ViewSource();
  content::WebContents* view_source_contents =
      view_source_contents_observer.GetWebContents();
  EXPECT_TRUE(WaitForLoadStop(view_source_contents));

  std::string source_text;
  // Get contents of view-source'd tab
  std::string view_source_extraction_script =
      R"(Array.from(document.querySelectorAll(".line-content")).reduce((prev, cur) => prev + cur.innerText + "\n", ""))";
  std::string actual_page_body =
      EvalJs(view_source_contents, view_source_extraction_script)
          .ExtractString();
  EXPECT_THAT(actual_page_body, testing::HasSubstr(kTestNonAmpBody));
}

class DeAmpBrowserTestBaseFeatureDisabled : public DeAmpBrowserTest {
 public:
  DeAmpBrowserTestBaseFeatureDisabled() {
    feature_list_.Reset();
    feature_list_.InitAndDisableFeature(de_amp::features::kBraveDeAMP);
  }
};

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTestBaseFeatureDisabled, DoesNotDeAmp) {
  https_server_->RegisterRequestHandler(
      base::BindRepeating(HandleRequest, kTestCanonicalPage, kTestAmpBody));
  ASSERT_TRUE(https_server_->Start());
  const GURL original_url = https_server_->GetURL(kTestHost, kTestAmpPage);
  // Doesn't get De-AMPed
  NavigateToURLAndWaitForRedirects(original_url, original_url);
}
