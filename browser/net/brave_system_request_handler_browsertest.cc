/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_system_request_handler.h"

#include <string>

#include "brave/components/constants/network_constants.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/simple_url_loader_test_helper.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

// Test to check if key is added for brave apis
class SystemNetworkContextManagerBrowsertest : public PlatformBrowserTest {
 public:
  SystemNetworkContextManagerBrowsertest()
      : https_server_(net::test_server::EmbeddedTestServer::TYPE_HTTPS) {
    https_server_.RegisterRequestMonitor(base::BindRepeating(
        &SystemNetworkContextManagerBrowsertest::MonitorResourceRequest,
        base::Unretained(this)));
  }

  ~SystemNetworkContextManagerBrowsertest() override = default;

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

  scoped_refptr<network::SharedURLLoaderFactory> loader_factory() const {
    return loader_factory_;
  }

  void SetUpOnMainThread() override {
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(https_server_.Start());

    loader_factory_ = g_browser_process->system_network_context_manager()
                          ->GetSharedURLLoaderFactory();
  }

  bool LoadURL(const std::string& host) {
    auto request = std::make_unique<network::ResourceRequest>();
    request->url = https_server_.GetURL(host, "/");
    content::SimpleURLLoaderTestHelper simple_loader_helper;
    std::unique_ptr<network::SimpleURLLoader> simple_loader =
        network::SimpleURLLoader::Create(std::move(request),
                                         TRAFFIC_ANNOTATION_FOR_TESTS);

    simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
        loader_factory().get(), simple_loader_helper.GetCallback());
    simple_loader_helper.WaitForCallback();

    return service_key_present_;
  }

 private:
  void MonitorResourceRequest(const net::test_server::HttpRequest& request) {
    service_key_present_ = request.headers.count(kBraveServicesKeyHeader) > 0;
  }

  bool service_key_present_ = false;
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::test_server::EmbeddedTestServer https_server_;
  scoped_refptr<network::SharedURLLoaderFactory> loader_factory_ = nullptr;
};

IN_PROC_BROWSER_TEST_F(SystemNetworkContextManagerBrowsertest,
                       CheckForBraveServiceKey) {
  EXPECT_TRUE(LoadURL("demo.brave.com"));
  EXPECT_TRUE(LoadURL("demo.bravesoftware.com"));
  EXPECT_FALSE(LoadURL("brave.demo.com"));
  EXPECT_FALSE(LoadURL("randomdomain.com"));
}
