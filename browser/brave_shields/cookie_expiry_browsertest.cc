/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/functional/bind.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/test_future.h"
#include "brave/components/constants/brave_paths.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/cookie_access_details.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/cookies/canonical_cookie.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/default_handlers.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace {

// Waits for JavaScript cookie operations to complete.
class CookieObserver : public content::WebContentsObserver {
 public:
  explicit CookieObserver(content::WebContents* web_contents, const GURL& url)
      : content::WebContentsObserver(web_contents), monitored_url_(url) {}

  [[nodiscard]] bool Wait() { return future_.Wait(); }

 private:
  void OnCookiesAccessed(content::RenderFrameHost* render_frame_host,
                         const content::CookieAccessDetails& details) override {
    if (details.type == content::CookieAccessDetails::Type::kChange &&
        details.url == monitored_url_) {
      future_.SetValue(details);
    }
  }

  GURL monitored_url_;
  base::test::TestFuture<content::CookieAccessDetails> future_;
};

constexpr base::TimeDelta k4YearsInDays = base::Days(1461);
// There might be a gap of a few milliseconds between setting the cookie and it
// getting stored. To prevent flapping tests, set this margin to be large
// (but we're still testing what we want to test)
// See: net/cookies/canonical_cookie_unittest.cc
constexpr base::TimeDelta kMarginForTesting = base::Seconds(5);

}  // namespace

class CookieExpirationTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    RegisterDefaultHandlers(https_server_.get());

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(https_server_->Start());
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
    std::string cookie_string =
        absl::StrFormat("document.cookie = 'name=Test; %s'", age);
    ASSERT_TRUE(content::ExecJs(
        browser->tab_strip_model()->GetActiveWebContents(), cookie_string));
  }

  void JSCookieStoreWriteCookie(Browser* browser, std::string expires_in_ms) {
    ASSERT_TRUE(
        content::ExecJs(browser->tab_strip_model()->GetActiveWebContents(),
                        absl::StrFormat("(async () => {"
                                        "return await window.cookieStore.set("
                                        "       { name: 'name',"
                                        "         value: 'Good',"
                                        "         expires: Date.now() + %s,"
                                        "       });"
                                        "})()",
                                        expires_in_ms)));
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
  auto less_than_max = base::Days(2);

  GURL url = https_server_->GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  CookieObserver observer(browser()->tab_strip_model()->GetActiveWebContents(),
                          url);
  JSDocumentCookieWriteCookie(
      browser(), "max-age=" + base::NumberToString(less_than_max.InSeconds()));
  ASSERT_TRUE(observer.Wait());

  std::vector<net::CanonicalCookie> all_cookies =
      GetAllCookiesDirect(browser());
  EXPECT_EQ(1u, all_cookies.size());
  for (const net::CanonicalCookie& cookie : all_cookies) {
    EXPECT_LE((base::Time::Now() + less_than_max - cookie.ExpiryDate()),
              kMarginForTesting);
  }
}

IN_PROC_BROWSER_TEST_F(CookieExpirationTest,
                       CheckExpiryForDocumentCookieMoreThanMax) {
  GURL url = https_server_->GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  CookieObserver observer(browser()->tab_strip_model()->GetActiveWebContents(),
                          url);
  JSDocumentCookieWriteCookie(
      browser(), "max-age=" + base::NumberToString(k4YearsInDays.InSeconds()));
  ASSERT_TRUE(observer.Wait());

  std::vector<net::CanonicalCookie> all_cookies =
      GetAllCookiesDirect(browser());
  EXPECT_EQ(1u, all_cookies.size());
  for (const net::CanonicalCookie& cookie : all_cookies) {
    const base::Time expected = cookie.CreationDate() + base::Days(180);
    // Ensure the cap is applied and timing is within margin.
    EXPECT_LE(cookie.ExpiryDate(), expected + kMarginForTesting);
    EXPECT_GE(cookie.ExpiryDate(), expected - kMarginForTesting);
  }
}

IN_PROC_BROWSER_TEST_F(CookieExpirationTest,
                       CheckExpiryForCookieStoreLessThanMax) {
  auto less_than_max = base::Days(2);
  GURL url = https_server_->GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  CookieObserver observer(browser()->tab_strip_model()->GetActiveWebContents(),
                          url);
  JSCookieStoreWriteCookie(
      browser(), base::NumberToString(less_than_max.InMilliseconds()));
  ASSERT_TRUE(observer.Wait());

  std::vector<net::CanonicalCookie> all_cookies =
      GetAllCookiesDirect(browser());
  EXPECT_EQ(1u, all_cookies.size());
  for (const net::CanonicalCookie& cookie : all_cookies) {
    EXPECT_LE((base::Time::Now() + less_than_max - cookie.ExpiryDate()),
              kMarginForTesting);
  }
}

IN_PROC_BROWSER_TEST_F(CookieExpirationTest,
                       CheckExpiryForCookieStoreMoreThanMax) {
  GURL url = https_server_->GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  CookieObserver observer(browser()->tab_strip_model()->GetActiveWebContents(),
                          url);
  JSCookieStoreWriteCookie(
      browser(), base::NumberToString(k4YearsInDays.InMilliseconds()));
  ASSERT_TRUE(observer.Wait());

  std::vector<net::CanonicalCookie> all_cookies =
      GetAllCookiesDirect(browser());
  EXPECT_EQ(1u, all_cookies.size());
  for (const net::CanonicalCookie& cookie : all_cookies) {
    const base::Time expected = cookie.CreationDate() + base::Days(180);
    // Ensure the cap is applied and timing is within margin.
    EXPECT_LE(cookie.ExpiryDate(), expected + kMarginForTesting);
    EXPECT_GE(cookie.ExpiryDate(), expected - kMarginForTesting);
  }
}

IN_PROC_BROWSER_TEST_F(CookieExpirationTest,
                       CheckExpiryForHttpCookiesLessThanMax) {
  auto less_than_max = base::Days(30);
  std::string cookie_string = "/set-cookie?test=http;max-age=" +
                              base::NumberToString(less_than_max.InSeconds());

  GURL url = https_server_->GetURL("a.com", cookie_string);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  std::vector<net::CanonicalCookie> all_cookies =
      GetAllCookiesDirect(browser());
  EXPECT_EQ(1u, all_cookies.size());
  for (const net::CanonicalCookie& cookie : all_cookies) {
    EXPECT_LE((base::Time::Now() + less_than_max - cookie.ExpiryDate()),
              kMarginForTesting);
  }
}

IN_PROC_BROWSER_TEST_F(CookieExpirationTest,
                       CheckExpiryForHttpCookiesMoreThanMax) {
  std::string cookie_string =
      "test=http;max-age=" + base::NumberToString(k4YearsInDays.InSeconds());
  GURL url = https_server_->GetURL("a.com", "/set-cookie?" + cookie_string);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  std::vector<net::CanonicalCookie> all_cookies =
      GetAllCookiesDirect(browser());
  EXPECT_EQ(1u, all_cookies.size());
  for (const net::CanonicalCookie& cookie : all_cookies) {
    const base::Time expected = cookie.CreationDate() + base::Days(180);
    // Ensure the cap is applied and timing is within margin.
    EXPECT_LE(cookie.ExpiryDate(), expected + kMarginForTesting);
    EXPECT_GE(cookie.ExpiryDate(), expected - kMarginForTesting);
  }
}
