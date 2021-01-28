/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/common/brave_paths.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/embedder_support/switches.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/url_loader_interceptor.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

class NativeFileSystemAPIBrowserTest : public InProcessBrowserTest {
 public:
  NativeFileSystemAPIBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_.ServeFilesFromDirectory(test_data_dir);
  }

  ~NativeFileSystemAPIBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    EXPECT_TRUE(https_server_.Start());
    // Map all hosts to localhost.
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderFrameHost* main_frame() {
    return web_contents()->GetMainFrame();
  }

 protected:
  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(NativeFileSystemAPIBrowserTest, FilePicker) {
  const GURL url = https_server_.GetURL("/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  auto result = content::EvalJs(main_frame(), "self.showOpenFilePicker()");
  EXPECT_TRUE(result.error.find("self.showOpenFilePicker is not a function") !=
              std::string::npos)
      << result.error;
}

namespace {

constexpr char kOriginTrialTestPublicKey[] =
    "dRCs+TocuKkocNKa0AtZ4awrt9XKH2SQCI6o4FY6BNA=";

const char kTestHeaders[] = "HTTP/1.1 200 OK\nContent-type: text/html\n\n";

constexpr char kOriginTrialTestHostname[] = "https://localhost";
constexpr char kOriginTrialPage[] = "page.html";

// tools/origin_trials/generate_token.py \
//    --expire-days 3650 https://localhost NativeFileSystem2
constexpr char kOriginTrialToken[] =
    "AzOJFCOVN9n5+fKf7X2W8DpbQzs54hnLqPxDGPpm/XyfBZTgOybwDGNWhKMUVPf1qn3t7LTZA3"
    "LlRBlFPbMn9AIAAABZeyJvcmlnaW4iOiAiaHR0cHM6Ly9sb2NhbGhvc3Q6NDQzIiwgImZlYXR1"
    "cmUiOiAiTmF0aXZlRmlsZVN5c3RlbTIiLCAiZXhwaXJ5IjogMTkyMDkyMzIxOX0=";

constexpr char kOriginTrialTestResponseTemplate[] = R"(
<html>
<head>
  <title>Native File System Origin Trial Test</title>
  META_TAG
</head>
</html>
)";

std::string GetContentForURL(const std::string& url) {
  if (!base::EndsWith(url, kOriginTrialPage, base::CompareCase::SENSITIVE))
    return std::string();

  std::string response = kOriginTrialTestResponseTemplate;
  std::string meta_tag =
      base::StrCat({R"(<meta http-equiv="origin-trial" content=")",
                    kOriginTrialToken, R"(">)"});
  base::ReplaceFirstSubstringAfterOffset(&response, 0, "META_TAG", meta_tag);
  return response;
}

bool URLLoaderInterceptorCallback(
    content::URLLoaderInterceptor::RequestParams* params) {
  content::URLLoaderInterceptor::WriteResponse(
      kTestHeaders, GetContentForURL(params->url_request.url.path()),
      params->client.get());
  return true;
}

}  // namespace

class NativeFileSystemOriginTrialBrowserTest : public InProcessBrowserTest {
 public:
  NativeFileSystemOriginTrialBrowserTest() = default;
  ~NativeFileSystemOriginTrialBrowserTest() override = default;

  void SetUpDefaultCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpDefaultCommandLine(command_line);
    command_line->AppendSwitchASCII(embedder_support::kOriginTrialPublicKey,
                                    kOriginTrialTestPublicKey);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    // We use a URLLoaderInterceptor, rather than the EmbeddedTestServer, since
    // the origin trial token in the response is associated with a fixed
    // origin, whereas EmbeddedTestServer serves content on a random port.
    url_loader_interceptor_ = std::make_unique<content::URLLoaderInterceptor>(
        base::BindRepeating(&URLLoaderInterceptorCallback));
  }

  void TearDownOnMainThread() override {
    url_loader_interceptor_.reset();
    InProcessBrowserTest::TearDownOnMainThread();
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderFrameHost* main_frame() {
    return web_contents()->GetMainFrame();
  }

 protected:
  std::unique_ptr<content::URLLoaderInterceptor> url_loader_interceptor_;
};

IN_PROC_BROWSER_TEST_F(NativeFileSystemOriginTrialBrowserTest, OriginTrial) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), GURL(base::JoinString(
                     {kOriginTrialTestHostname, kOriginTrialPage}, "/"))));

  auto result = content::EvalJs(main_frame(), "self.showOpenFilePicker()");
  EXPECT_TRUE(result.error.find("self.showOpenFilePicker is not a function") !=
              std::string::npos)
      << result.error;
}
