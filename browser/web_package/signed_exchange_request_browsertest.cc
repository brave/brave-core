/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <optional>
#include <string_view>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/lock.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/common/content_constants_internal.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace {

constexpr char kDefaultAcceptHeaderValue[] = "*/*";

}  // namespace

class SignedExchangeRequestBrowserTest : public InProcessBrowserTest {
 public:
  using self = SignedExchangeRequestBrowserTest;
  SignedExchangeRequestBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}
  ~SignedExchangeRequestBrowserTest() override = default;

 protected:
  void NavigateAndWaitForTitle(const GURL& url, std::string_view title) {
    std::u16string expected_title = base::ASCIIToUTF16(title);
    content::TitleWatcher title_watcher(
        browser()->tab_strip_model()->GetActiveWebContents(), expected_title);
    EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    EXPECT_EQ(expected_title, title_watcher.WaitAndGetTitle());
  }

  void SetUp() override {
    https_server_.ServeFilesFromSourceDirectory("content/test/data");
    https_server_.RegisterRequestHandler(
        base::BindRepeating(&self::RedirectResponseHandler));
    https_server_.RegisterRequestHandler(base::BindRepeating(
        &self::FallbackSxgResponseHandler, base::Unretained(this)));
    https_server_.RegisterRequestMonitor(
        base::BindRepeating(&self::MonitorRequest, base::Unretained(this)));
    ASSERT_TRUE(https_server_.Start());
    InProcessBrowserTest::SetUp();
  }

  static std::unique_ptr<net::test_server::HttpResponse>
  RedirectResponseHandler(const net::test_server::HttpRequest& request) {
    if (!base::StartsWith(request.relative_url, "/r?",
                          base::CompareCase::SENSITIVE)) {
      return nullptr;
    }
    std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
        new net::test_server::BasicHttpResponse);
    http_response->set_code(net::HTTP_MOVED_PERMANENTLY);
    http_response->AddCustomHeader("Location", request.relative_url.substr(3));
    http_response->AddCustomHeader("Cache-Control", "no-cache");
    return std::move(http_response);
  }

  // Responds with a prologue-only signed exchange that triggers a fallback
  // redirect.
  std::unique_ptr<net::test_server::HttpResponse> FallbackSxgResponseHandler(
      const net::test_server::HttpRequest& request) {
    const std::string prefix = "/fallback_sxg?";
    if (!base::StartsWith(request.relative_url, prefix,
                          base::CompareCase::SENSITIVE)) {
      return nullptr;
    }
    std::string fallback_url(request.relative_url.substr(prefix.length()));
    if (fallback_url.empty()) {
      // If fallback URL is not specified, fallback to itself.
      fallback_url = https_server_.GetURL(prefix).spec();
    }

    std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
        new net::test_server::BasicHttpResponse);
    http_response->set_code(net::HTTP_OK);
    http_response->set_content_type("application/signed-exchange;v=b3");

    std::string sxg("sxg1-b3", 8);
    sxg.push_back(fallback_url.length() >> 8);
    sxg.push_back(fallback_url.length() & 0xff);
    sxg += fallback_url;
    // FallbackUrlAndAfter() requires 6 more bytes for sizes of next fields.
    sxg.resize(sxg.length() + 6);

    http_response->set_content(sxg);
    return std::move(http_response);
  }

  void MonitorRequest(const net::test_server::HttpRequest& request) {
    const auto it = request.headers.find(net::HttpRequestHeaders::kAccept);
    if (it == request.headers.end())
      return;
    // Note this method is called on the EmbeddedTestServer's background thread.
    base::AutoLock lock(url_accept_header_map_lock_);
    url_accept_header_map_[request.base_url.Resolve(request.relative_url)] =
        it->second;
  }

  void CheckAcceptHeader(const GURL& url, bool is_navigation) {
    const auto accept_header = GetInterceptedAcceptHeader(url);
    ASSERT_TRUE(accept_header);
    EXPECT_EQ(*accept_header,
              is_navigation ? std::string(content::kFrameAcceptHeaderValue)
                            : std::string(kDefaultAcceptHeaderValue));
  }

  std::optional<std::string> GetInterceptedAcceptHeader(const GURL& url) const {
    base::AutoLock lock(url_accept_header_map_lock_);
    const auto it = url_accept_header_map_.find(url);
    if (it == url_accept_header_map_.end())
      return std::nullopt;
    return it->second;
  }

  void CheckNavigationAcceptHeader(const std::vector<GURL>& urls) {
    for (const auto& url : urls) {
      SCOPED_TRACE(url);
      CheckAcceptHeader(url, true /* is_navigation */);
    }
  }

  void CheckPrefetchAcceptHeader(const std::vector<GURL>& urls) {
    for (const auto& url : urls) {
      SCOPED_TRACE(url);
      CheckAcceptHeader(url, false /* is_navigation */);
    }
  }

  net::EmbeddedTestServer https_server_;

 private:
  // url_accept_header_map_ is accessed both on the main thread and on the
  // EmbeddedTestServer's background thread via MonitorRequest(), so it must be
  // locked.
  mutable base::Lock url_accept_header_map_lock_;
  std::map<GURL, std::string> url_accept_header_map_;
};

IN_PROC_BROWSER_TEST_F(SignedExchangeRequestBrowserTest, AlwaysDisabled) {
  const GURL test_url = https_server_.GetURL("/sxg/test.html");
  NavigateAndWaitForTitle(test_url, test_url.spec());
  CheckNavigationAcceptHeader({test_url});
}

IN_PROC_BROWSER_TEST_F(SignedExchangeRequestBrowserTest,
                       PrefetchAlwaysDisabled) {
  const GURL target = https_server_.GetURL("/sxg/hello.txt");
  const GURL page_url =
      https_server_.GetURL(std::string("/sxg/prefetch.html#") + target.spec());
  NavigateAndWaitForTitle(page_url, "OK");
  CheckPrefetchAcceptHeader({target});
}
