/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/common/brave_paths.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/default_handlers.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

class BraveFaviconDatabaseBrowserTest : public InProcessBrowserTest {
 public:
  BraveFaviconDatabaseBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    content_client_.reset(new ChromeContentClient);
    content::SetContentClient(content_client_.get());
    browser_content_client_.reset(new BraveContentBrowserClient());
    content::SetBrowserClientForTesting(browser_content_client_.get());

    host_resolver()->AddRule("*", "127.0.0.1");

    brave::RegisterPathProvider();
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir_);
    https_server_.ServeFilesFromDirectory(test_data_dir_);
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    https_server_.RegisterRequestHandler(
        base::BindRepeating(&BraveFaviconDatabaseBrowserTest::HandleRequest,
                            base::Unretained(this)));
    https_server_.RegisterRequestMonitor(
        base::BindRepeating(&BraveFaviconDatabaseBrowserTest::SaveRequest,
                            base::Unretained(this)));

    ASSERT_TRUE(https_server_.Start());

    fav0_url_ = https_server_.GetURL("fav0.a.com", "/favicon.ico");
    fav1_url_ = https_server_.GetURL("fav1.a.com", "/favicon.ico");
    fav2_url_ = https_server_.GetURL("fav2.a.com", "/favicon.ico");
    fav3_url_ = https_server_.GetURL("fav3.a.com", "/favicon.ico");
    landing_url_ = https_server_.GetURL("a.com", "/simple.html");
    read_url_ = https_server_.GetURL("a.com", "/favicon_read.html");
    set_url_ = https_server_.GetURL("a.com", "/favicon_set.html");
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    // This is needed to load pages from "domain.com" without an interstitial.
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    std::string relative_path(request.GetURL().path());
    if (!base::EndsWith(relative_path, "/favicon.ico",
                        base::CompareCase::SENSITIVE)) {
      return nullptr;
    }

    std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
        new net::test_server::BasicHttpResponse());
    http_response->set_code(net::HTTP_OK);

    http_response->set_content_type("image/vnd.microsoft.icon");
    std::string file_contents;
    if (!base::ReadFileToString(test_data_dir_.AppendASCII("favicon.ico"),
                                &file_contents)) {
      return nullptr;
    }
    http_response->set_content(file_contents);

    return std::move(http_response);
  }

  void SaveRequest(const net::test_server::HttpRequest& request) {
    base::AutoLock auto_lock(save_request_lock_);

    GURL requested_host("https://" + request.headers.at("Host"));
    GURL::Replacements replace_host;
    std::string host_str = requested_host.host();
    replace_host.SetHostStr(host_str);

    requests_.push_back(request.GetURL().ReplaceComponents(replace_host));
  }

  void ClearRequests() {
    base::AutoLock auto_lock(save_request_lock_);
    requests_.clear();
  }

  GURL GetLastRequest() {
    base::AutoLock auto_lock(save_request_lock_);
    return requests_.back();
  }

  bool WasRequested(const GURL& url) {
    base::AutoLock auto_lock(save_request_lock_);
    return std::find(requests_.begin(), requests_.end(), url.spec()) !=
           requests_.end();
  }

  void TearDown() override {
    browser_content_client_.reset();
    content_client_.reset();
  }

  const net::EmbeddedTestServer& https_server() { return https_server_; }

  const GURL& fav0_url() { return fav0_url_; }
  const GURL& fav1_url() { return fav1_url_; }
  const GURL& fav2_url() { return fav2_url_; }
  const GURL& fav3_url() { return fav3_url_; }
  const GURL& landing_url() { return landing_url_; }

  GURL read_url(std::string uid) {
    GURL::Replacements replace_query;
    std::string uid_str = "uid=" + uid;
    replace_query.SetQueryStr(uid_str);
    return read_url_.ReplaceComponents(replace_query);
  }

  GURL set_url(std::string values) {
    GURL::Replacements replace_query;
    std::string values_str = "values=" + values;
    replace_query.SetQueryStr(values_str);
    return set_url_.ReplaceComponents(replace_query);
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void NavigateToURLAndWaitForRedirects(const GURL& url) {
    ui_test_utils::UrlLoadObserver load_complete(
        landing_url(), content::NotificationService::AllSources());
    ui_test_utils::NavigateToURL(browser(), url);
    EXPECT_EQ(contents()->GetMainFrame()->GetLastCommittedURL(), url);
    load_complete.Wait();

    EXPECT_EQ(contents()->GetLastCommittedURL(), landing_url());
    EXPECT_EQ(GetLastRequest().path(), landing_url().path());

    // Navigate again to make sure all of the favicons finished loading.
    ui_test_utils::NavigateToURL(browser(), GURL("about:blank"));
  }

 private:
  GURL fav0_url_;
  GURL fav1_url_;
  GURL fav2_url_;
  GURL fav3_url_;
  GURL landing_url_;
  GURL read_url_;
  GURL set_url_;
  base::FilePath test_data_dir_;
  std::unique_ptr<ChromeContentClient> content_client_;
  std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
  std::vector<GURL> requests_;
  mutable base::Lock save_request_lock_;

  base::ScopedTempDir temp_user_data_dir_;
  net::test_server::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(BraveFaviconDatabaseBrowserTest, SetRead1001) {
  ClearRequests();
  NavigateToURLAndWaitForRedirects(set_url("1001"));
  EXPECT_EQ(WasRequested(fav0_url()), true);
  EXPECT_EQ(WasRequested(fav1_url()), false);
  EXPECT_EQ(WasRequested(fav2_url()), false);
  EXPECT_EQ(WasRequested(fav3_url()), true);

  ClearRequests();
  NavigateToURLAndWaitForRedirects(read_url("read1001"));
  EXPECT_EQ(WasRequested(fav0_url()), true);
  EXPECT_EQ(WasRequested(fav1_url()), true);
  EXPECT_EQ(WasRequested(fav2_url()), true);
  EXPECT_EQ(WasRequested(fav3_url()), true);
}
