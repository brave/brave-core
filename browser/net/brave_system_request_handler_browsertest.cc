/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_system_request_handler.h"

#include "base/memory/scoped_refptr.h"
#include "brave/components/constants/network_constants.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/simple_url_loader_test_helper.h"
#include "content/test/io_thread_shared_url_loader_factory_owner.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

// Test to check if key is added for brave apis
class BraveSystemRequestHandlerBrowsertest : public PlatformBrowserTest {
 public:
  BraveSystemRequestHandlerBrowsertest()
      : https_server_(net::test_server::EmbeddedTestServer::TYPE_HTTPS) {}

  ~BraveSystemRequestHandlerBrowsertest() override = default;

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

  scoped_refptr<network::SharedURLLoaderFactory>
  url_loader_for_browser_process_factory() const {
    return chrome_test_utils::GetProfile(this)
        ->GetDefaultStoragePartition()
        ->GetURLLoaderFactoryForBrowserProcess();
  }

  content::IOThreadSharedURLLoaderFactoryOwner::
      IOThreadSharedURLLoaderFactoryOwnerPtr
      url_loader_for_browser_process_io_thread_factory() const {
    return content::IOThreadSharedURLLoaderFactoryOwner::Create(
        chrome_test_utils::GetProfile(this)
            ->GetDefaultStoragePartition()
            ->GetURLLoaderFactoryForBrowserProcessIOThread());
  }

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_OK);

    https_server_.RegisterRequestMonitor(base::BindRepeating(
        &BraveSystemRequestHandlerBrowsertest::MonitorResourceRequest,
        base::Unretained(this)));

    ASSERT_TRUE(https_server_.Start());

    loader_factory_ = g_browser_process->system_network_context_manager()
                          ->GetSharedURLLoaderFactory();
  }

  bool GetServiceKeyPresentAndResetValue() {
    EXPECT_TRUE(service_key_present_.has_value());
    bool value = service_key_present_.value();
    service_key_present_.reset();
    return value;
  }

  void LoadURLOnIOThread(std::string_view host) {
    url_loader_for_browser_process_io_thread_factory()
        ->LoadBasicRequestOnIOThread(https_server_.GetURL(host, "/"));
  }

  void LoadURL(std::string_view host,
               scoped_refptr<network::SharedURLLoaderFactory> factory) {
    auto request = std::make_unique<network::ResourceRequest>();
    request->url = https_server_.GetURL(host, "/");
    content::SimpleURLLoaderTestHelper simple_loader_helper;
    std::unique_ptr<network::SimpleURLLoader> simple_loader =
        network::SimpleURLLoader::Create(std::move(request),
                                         TRAFFIC_ANNOTATION_FOR_TESTS);

    simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
        factory.get(), simple_loader_helper.GetCallback());
    simple_loader_helper.WaitForCallback();
  }

 private:
  void MonitorResourceRequest(const net::test_server::HttpRequest& request) {
    service_key_present_ = request.headers.count(kBraveServicesKeyHeader) > 0;
  }

  std::optional<bool> service_key_present_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::test_server::EmbeddedTestServer https_server_;
  scoped_refptr<network::SharedURLLoaderFactory> loader_factory_ = nullptr;
};

IN_PROC_BROWSER_TEST_F(BraveSystemRequestHandlerBrowsertest,
                       CheckForBraveServiceKey) {
  struct TestCase {
    std::string_view url;
    bool service_key_should_be_set;
  } test_cases[] = {
      {"demo.brave.com", true},
      {"demo.bravesoftware.com", true},
      {"brave.demo.com", false},
      {"randomdomain.com", false},
  };
  for (const auto test_case : test_cases) {
    LoadURL(test_case.url, loader_factory());
    EXPECT_EQ(GetServiceKeyPresentAndResetValue(),
              test_case.service_key_should_be_set);

    LoadURL(test_case.url, url_loader_for_browser_process_factory());
    EXPECT_EQ(GetServiceKeyPresentAndResetValue(),
              test_case.service_key_should_be_set);

    LoadURLOnIOThread(test_case.url);
    EXPECT_EQ(GetServiceKeyPresentAndResetValue(),
              test_case.service_key_should_be_set);
  }
}
