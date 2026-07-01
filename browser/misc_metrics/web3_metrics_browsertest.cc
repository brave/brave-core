/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/web3_metrics.h"

#include "base/path_service.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/run_until.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

namespace misc_metrics {

class Web3MetricsBrowserTest : public PlatformBrowserTest {
 public:
  Web3MetricsBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}
  ~Web3MetricsBrowserTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    PlatformBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    PlatformBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(https_server_.Start());
  }

  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  content::RenderFrameHost* primary_main_frame() {
    return web_contents()->GetPrimaryMainFrame();
  }

  void WaitForDappVisits(int count) {
    ASSERT_TRUE(base::test::RunUntil([this, count]() {
      return histogram_tester_.GetBucketCount(kWeb3DappVisitHistogramName, 1) ==
             count;
    }));
  }

 protected:
  base::HistogramTester histogram_tester_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::EmbeddedTestServer https_server_;
};

// Dispatching an EIP-6963 provider discovery request is reported.
IN_PROC_BROWSER_TEST_F(Web3MetricsBrowserTest, RequestProviderReported) {
  ASSERT_TRUE(content::NavigateToURL(web_contents(),
                                     https_server_.GetURL("/simple.html")));

  histogram_tester_.ExpectTotalCount(kWeb3DappVisitHistogramName, 0);

  ASSERT_TRUE(content::ExecJs(
      primary_main_frame(),
      "window.dispatchEvent(new Event('eip6963:requestProvider'));"));

  WaitForDappVisits(2);
}

// Accessing a provider that the page itself installed is reported.
IN_PROC_BROWSER_TEST_F(Web3MetricsBrowserTest, ProviderAccessReported) {
  ASSERT_TRUE(content::NavigateToURL(web_contents(),
                                     https_server_.GetURL("/simple.html")));

  histogram_tester_.ExpectTotalCount(kWeb3DappVisitHistogramName, 0);

  // Assign a provider (routes through the injected setter) and then call a
  // method on it. Reading `.request` fires the proxy `get` trap (1) and
  // invoking it fires the `apply` trap (2).
  ASSERT_TRUE(content::ExecJs(primary_main_frame(),
                              "window.ethereum = { request: () => 42 };"
                              "window.ethereum.request();"));

  WaitForDappVisits(2);
}

// Pages that never touch a web3 provider record nothing.
IN_PROC_BROWSER_TEST_F(Web3MetricsBrowserTest, NoProviderUsageNotReported) {
  ASSERT_TRUE(content::NavigateToURL(web_contents(),
                                     https_server_.GetURL("/simple.html")));

  // Run benign script that should never trigger the proxy.
  ASSERT_TRUE(content::ExecJs(primary_main_frame(), "1 + 1;"));

  // Then trigger a known interaction as a fence. Because both messages travel
  // on the same Web3Metrics pipe (same frame), FIFO ordering guarantees that
  // any erroneous sample from the benign script would already be counted by the
  // time this fence sample lands. Waiting for exactly the fence's sample count
  // proves the benign script contributed none.
  ASSERT_TRUE(content::ExecJs(
      primary_main_frame(),
      "window.dispatchEvent(new Event('eip6963:requestProvider'));"));
  WaitForDappVisits(2);
}

}  // namespace misc_metrics
