// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "brave/browser/net/url_context.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/page_type.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using brave::ResponseCallback;

class BraveTorrentRedirectNetworkDelegateHelperTest
    : public InProcessBrowserTest {
 public:
  BraveTorrentRedirectNetworkDelegateHelperTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_.RegisterRequestHandler(base::BindRepeating(
        &BraveTorrentRedirectNetworkDelegateHelperTest::HandleRequest,
        base::Unretained(this)));

    ASSERT_TRUE(https_server_.Start());
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    auto relative_url = request.relative_url;
    if (relative_url.ends_with(".torrent")) {
      auto response = std::make_unique<net::test_server::BasicHttpResponse>();
      response->set_code(net::HTTP_OK);
      response->set_content("a torrent file");
      response->set_content_type("application/x-bittorrent");
      return response;
    }
    return nullptr;
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
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  GURL torrent_url() {
    return https_server_.GetURL("webtorrent.io", "/sintel.torrent");
  }

  GURL torrent_extension_url() {
    return GURL(
        "chrome-extension://lgjmpdmojkpocjcopdikifhejkkjglho/extension/"
        "brave_webtorrent2.html?https://webtorrent.io/torrents/sintel.torrent");
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::test_server::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest,
                       TorrentFileIsRedirected) {
  auto* contents = browser()->tab_strip_model()->GetActiveWebContents();

  NavigateParams params(browser(), torrent_url(),
                        ui::PageTransition::PAGE_TRANSITION_LINK);

  Navigate(&params);
  content::WaitForLoadStop(contents);
  EXPECT_EQ("webtorrent:" + torrent_url().spec(), contents->GetVisibleURL());
}

IN_PROC_BROWSER_TEST_F(BraveTorrentRedirectNetworkDelegateHelperTest,
                       LinkToExtensionFails) {
  auto* contents = browser()->tab_strip_model()->GetActiveWebContents();

  NavigateParams params(browser(), torrent_extension_url(),
                        ui::PageTransition::PAGE_TRANSITION_LINK);

  Navigate(&params);
  content::WaitForLoadStop(contents);

  EXPECT_EQ(content::PageType::PAGE_TYPE_ERROR,
            contents->GetController().GetLastCommittedEntry()->GetPageType());
}
