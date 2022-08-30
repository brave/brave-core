// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/bind.h"
#include "base/test/bind.h"

#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "brave/components/constants/brave_paths.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

constexpr base::TimeDelta k4YearsInDays = base::Days(1461);
// There might be a gap of a few milliseconds between setting the cookie and it
// getting stored. To prevent flapping tests, set this margin to be large
// (but we're still testing what we want to test)
// See: net/cookies/canonical_cookie_unittest.cc
constexpr base::TimeDelta kMarginForTesting = base::Seconds(5);

class CookieExpirationTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_->ServeFilesFromDirectory(test_data_dir);
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

  // Set a cookie with JavaScript.
  void JSDocumentCookieWriteCookie(Browser* browser, std::string age) {
    bool rv = content::ExecuteScript(
        browser->tab_strip_model()->GetActiveWebContents(),
        base::StringPrintf("document.cookie = 'name=Test; %s'", age.c_str()));
    ASSERT_TRUE(rv);
  }

  void JSCookieStoreWriteCookie(Browser* browser, std::string expires_in_ms) {
    content::EvalJsResult result = content::EvalJs(
        browser->tab_strip_model()->GetActiveWebContents(),
        base::StringPrintf("(async () => {"
                           "return await window.cookieStore.set("
                           "       { name: 'name',"
                           "         value: 'Good',"
                           "         expires: Date.now() + %s,"
                           "       });"
                           "})()",
                           expires_in_ms.c_str()));
  }

  std::vector<net::CanonicalCookie> GetAllCookiesDirect(Browser* browser) {
    base::RunLoop run_loop;
    std::vector<net::CanonicalCookie> cookies_out;
    browser->tab_strip_model()
        ->GetActiveWebContents()
        ->GetBrowserContext()
        ->GetDefaultStoragePartition()
        ->GetCookieManagerForBrowserProcess()
        ->GetAllCookies(base::BindLambdaForTesting(
            [&run_loop,
             &cookies_out](const std::vector<net::CanonicalCookie>& cookies) {
              cookies_out = cookies;
              run_loop.Quit();
            }));
    run_loop.Run();
    return cookies_out;
  }

 protected:
  std::unique_ptr<net::EmbeddedTestServer> https_server_;

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
};

IN_PROC_BROWSER_TEST_F(CookieExpirationTest,
                       CheckExpiryForDocumentCookieLessThanMax) {
  ASSERT_TRUE(https_server_->Start());
  auto less_than_max = base::Days(2);

  GURL url = https_server_->GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  JSDocumentCookieWriteCookie(
      browser(), "max-age=" + std::to_string(less_than_max.InSeconds()));
  std::vector<net::CanonicalCookie> all_cookies =
      GetAllCookiesDirect(browser());
  EXPECT_EQ(1u, all_cookies.size());
  for (const net::CanonicalCookie& cookie : all_cookies) {
    EXPECT_LE(
        (base::Time::Now() + less_than_max - cookie.ExpiryDate()).magnitude(),
        kMarginForTesting);
  }
}

IN_PROC_BROWSER_TEST_F(CookieExpirationTest,
                       CheckExpiryForDocumentCookieMoreThanMax) {
  ASSERT_TRUE(https_server_->Start());

  GURL url = https_server_->GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  JSDocumentCookieWriteCookie(
      browser(), "max-age=" + std::to_string(k4YearsInDays.InSeconds()));
  std::vector<net::CanonicalCookie> all_cookies =
      GetAllCookiesDirect(browser());
  EXPECT_EQ(1u, all_cookies.size());
  for (const net::CanonicalCookie& cookie : all_cookies) {
    EXPECT_EQ((cookie.ExpiryDate() - cookie.CreationDate()).InDays(), 7);
  }
}

IN_PROC_BROWSER_TEST_F(CookieExpirationTest,
                       CheckExpiryForCookieStoreLessThanMax) {
  ASSERT_TRUE(https_server_->Start());
  auto less_than_max = base::Days(2);
  GURL url = https_server_->GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  JSCookieStoreWriteCookie(browser(),
                           std::to_string(less_than_max.InMilliseconds()));

  std::vector<net::CanonicalCookie> all_cookies =
      GetAllCookiesDirect(browser());
  EXPECT_EQ(1u, all_cookies.size());
  for (const net::CanonicalCookie& cookie : all_cookies) {
    EXPECT_LE(
        (base::Time::Now() + less_than_max - cookie.ExpiryDate()).magnitude(),
        kMarginForTesting);
  }
}

IN_PROC_BROWSER_TEST_F(CookieExpirationTest,
                       CheckExpiryForCookieStoreMoreThanMax) {
  ASSERT_TRUE(https_server_->Start());

  GURL url = https_server_->GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  JSCookieStoreWriteCookie(browser(),
                           std::to_string(k4YearsInDays.InMilliseconds()));

  std::vector<net::CanonicalCookie> all_cookies =
      GetAllCookiesDirect(browser());
  EXPECT_EQ(1u, all_cookies.size());
  for (const net::CanonicalCookie& cookie : all_cookies) {
    EXPECT_EQ((cookie.ExpiryDate() - cookie.CreationDate()).InDays(), 7);
  }
}

IN_PROC_BROWSER_TEST_F(CookieExpirationTest,
                       CheckExpiryForHttpCookiesLessThanMax) {
  auto less_than_max = base::Days(30);
  std::string cookie_string =
      "test=http;max-age=" + std::to_string(less_than_max.InSeconds());
  https_server_->RegisterRequestHandler(base::BindLambdaForTesting(
      [&](const net::test_server::HttpRequest& request)
          -> std::unique_ptr<net::test_server::HttpResponse> {
        auto response = std::make_unique<net::test_server::BasicHttpResponse>();
        response->AddCustomHeader("Set-Cookie", cookie_string);
        return std::move(response);
      }));
  ASSERT_TRUE(https_server_->Start());

  GURL url = https_server_->GetURL("a.com", "/simple.html");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  std::vector<net::CanonicalCookie> all_cookies =
      GetAllCookiesDirect(browser());
  EXPECT_EQ(1u, all_cookies.size());
  for (const net::CanonicalCookie& cookie : all_cookies) {
    EXPECT_LE(
        (base::Time::Now() + less_than_max - cookie.ExpiryDate()).magnitude(),
        kMarginForTesting);
  }
}

IN_PROC_BROWSER_TEST_F(CookieExpirationTest,
                       CheckExpiryForHttpCookiesMoreThanMax) {
  std::string cookie_string =
      "test=http;max-age=" + std::to_string(k4YearsInDays.InSeconds());
  https_server_->RegisterRequestHandler(base::BindLambdaForTesting(
      [&](const net::test_server::HttpRequest& request)
          -> std::unique_ptr<net::test_server::HttpResponse> {
        auto response = std::make_unique<net::test_server::BasicHttpResponse>();
        response->AddCustomHeader("Set-Cookie", cookie_string);
        return std::move(response);
      }));
  ASSERT_TRUE(https_server_->Start());

  GURL url = https_server_->GetURL("a.com", "/simple.html");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  std::vector<net::CanonicalCookie> all_cookies =
      GetAllCookiesDirect(browser());
  EXPECT_EQ(1u, all_cookies.size());
  for (const net::CanonicalCookie& cookie : all_cookies) {
    EXPECT_EQ((cookie.ExpiryDate() - cookie.CreationDate()).InDays(), 180);
  }
}
