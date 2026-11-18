/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <atomic>
#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/strcat.h"
#include "base/test/test_future.h"
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/components/tor/tor_profile_service_impl.h"
#include "chrome/browser/net/profile_network_context_service.h"
#include "chrome/browser/net/profile_network_context_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/browser_test.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"

class TorProfileProxyBrowserTest : public InProcessBrowserTest {
 protected:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    embedded_test_server()->RegisterRequestMonitor(base::BindRepeating(
        [](std::atomic<int>* count, const net::test_server::HttpRequest&) {
          ++*count;
        },
        base::Unretained(&direct_request_count_)));
    ASSERT_TRUE(embedded_test_server()->Start());
    TorProfileManager::SwitchToTorProfile(browser()->GetProfile());
  }

  Profile* tor_profile() {
    return TorProfileManager::GetInstance().GetTorProfile(
        browser()->GetProfile());
  }

  // Starts a request from the browser process on the Tor profile, and
  // returns once the network service has received it.
  std::unique_ptr<network::SimpleURLLoader> StartRequest(
      base::test::TestFuture<scoped_refptr<net::HttpResponseHeaders>>& future) {
    auto resource_request = std::make_unique<network::ResourceRequest>();
    resource_request->url = embedded_test_server()->GetURL("/echo");
    auto loader = network::SimpleURLLoader::Create(
        std::move(resource_request), TRAFFIC_ANNOTATION_FOR_TESTS);
    content::StoragePartition* partition =
        tor_profile()->GetDefaultStoragePartition();
    loader->DownloadHeadersOnly(
        partition->GetURLLoaderFactoryForBrowserProcess().get(),
        future.GetCallback());
    partition->FlushNetworkInterfaceForTesting();
    return loader;
  }

  // Points the Tor profile at a SOCKS proxy that refuses connections, so any
  // request sent through Tor fails with ERR_PROXY_CONNECTION_FAILED.
  void SetUpClosedTorProxy() {
    net::EmbeddedTestServer closed_server;
    ASSERT_TRUE(closed_server.Start());
    const std::string closed_host_port =
        closed_server.host_port_pair().ToString();
    ASSERT_TRUE(closed_server.ShutdownAndWaitUntilComplete());

    static_cast<tor::TorProfileServiceImpl*>(
        TorProfileServiceFactory::GetForContext(tor_profile()))
        ->OnTorNewProxyURI(base::StrCat({"socks5://", closed_host_port}));
    ProfileNetworkContextServiceFactory::GetForContext(tor_profile())
        ->FlushProxyConfigMonitorForTesting();
  }

  // Requests the test server received directly, bypassing Tor.
  std::atomic<int> direct_request_count_ = 0;
};

IN_PROC_BROWSER_TEST_F(TorProfileProxyBrowserTest,
                       RequestBeforeTorIsUpWaitsForTorProxy) {
  base::test::TestFuture<scoped_refptr<net::HttpResponseHeaders>> future;
  auto loader = StartRequest(future);
  EXPECT_FALSE(future.IsReady());

  SetUpClosedTorProxy();
  EXPECT_TRUE(future.Wait());
  EXPECT_EQ(net::ERR_PROXY_CONNECTION_FAILED, loader->NetError());
  EXPECT_EQ(0, direct_request_count_);
}

IN_PROC_BROWSER_TEST_F(TorProfileProxyBrowserTest,
                       RequestAfterTorIsUpGoesThroughTorProxy) {
  SetUpClosedTorProxy();

  base::test::TestFuture<scoped_refptr<net::HttpResponseHeaders>> future;
  auto loader = StartRequest(future);
  EXPECT_TRUE(future.Wait());
  EXPECT_EQ(net::ERR_PROXY_CONNECTION_FAILED, loader->NetError());
  EXPECT_EQ(0, direct_request_count_);
}
