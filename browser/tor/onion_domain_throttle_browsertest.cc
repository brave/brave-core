/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/base64.h"
#include "base/test/bind.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

class OnionDomainThrottleBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    auto request_handler = [](const net::test_server::HttpRequest& request)
        -> std::unique_ptr<net::test_server::HttpResponse> {
      auto http_response =
          std::make_unique<net::test_server::BasicHttpResponse>();
      http_response->set_content_type("image/png");
      std::string image;
      std::string base64_image =
          "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVQYV2NIbbj6HwAF"
          "wgK6ho3LlwAAAABJRU5ErkJggg==";
      base::Base64Decode(base64_image, &image);
      http_response->set_content(image);
      return http_response;
    };

    https_server_->RegisterDefaultHandler(base::BindRepeating(request_handler));
    ASSERT_TRUE(https_server_->Start());
  }

  net::EmbeddedTestServer* test_server() { return https_server_.get(); }

  std::string image_script(const std::string& src) {
    return base::StringPrintf(R"(
        new Promise(resolve => {
          let img = document.createElement('img');
          img.src = '%s';
          img.onload = function () {
            resolve(true);
          };
          img.onerror = function() {
            resolve(false);
          };
        });
    )",
                              src.c_str());
  }

 protected:
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};

// TODO(darkdh): We need modify proxy config in Tor window for test in order to
// to access test_server so that we can test SubresourceRequests for Tor
// window
IN_PROC_BROWSER_TEST_F(OnionDomainThrottleBrowserTest, SubresourceRequests) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server_->GetURL("a.test", "/simple.html")));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  struct {
    std::string src;
    bool result;
  } cases[] = {
      {"https://dns4torpnlfs2ifuz2s2yf3fc7rdmsbhm6rw75euj35pac6ap25zgqad.onion/"
       "favicon.ico",
       false},
      {"https://1.1.1.1/favicon.ico", true},
  };
  for (const auto& test_case : cases) {
    auto loaded = EvalJs(contents, image_script(test_case.src));
    ASSERT_TRUE(loaded.error.empty());
    EXPECT_EQ(base::Value(test_case.result), loaded.value);
  }
}
