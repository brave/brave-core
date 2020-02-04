/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/proxy_resolution/proxy_resolution_service.h"

#include <string>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "brave/net/proxy_resolution/proxy_config_service_tor.h"
#include "net/base/test_completion_callback.h"
#include "net/log/test_net_log.h"
#include "net/proxy_resolution/mock_proxy_resolver.h"
#include "net/test/gtest_util.h"
#include "net/test/test_with_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

using net::test::IsOk;

namespace net {

class ProxyResolutionServiceTest : public TestWithTaskEnvironment {
 public:
  ProxyResolutionServiceTest() = default;
  ~ProxyResolutionServiceTest() override = default;

  void SetUp() override {
    const std::string proxy_uri("socks5://127.0.0.1:5566");
    service_ = std::make_unique<ProxyResolutionService>(
        std::make_unique<ProxyConfigServiceTor>(proxy_uri),
        std::make_unique<MockAsyncProxyResolverFactory>(false),
        nullptr);
  }

  ProxyResolutionService* GetProxyResolutionService() { return service_.get(); }

 private:
  std::unique_ptr<ProxyResolutionService> service_;
  DISALLOW_COPY_AND_ASSIGN(ProxyResolutionServiceTest);
};

TEST_F(ProxyResolutionServiceTest, TorProxy) {
  ProxyResolutionService* service = GetProxyResolutionService();
  const GURL site_url("https://check.torproject.org/");
  const std::string isolation_key =
      ProxyConfigServiceTor::CircuitIsolationKey(site_url);

  ProxyInfo info;
  TestCompletionCallback callback;
  RecordingBoundTestNetLog log;
  std::unique_ptr<ProxyResolutionService::Request> request;
  int rv =
      service->ResolveProxy(site_url, std::string(), NetworkIsolationKey(),
                            &info, callback.callback(), &request, log.bound());
  EXPECT_THAT(rv, IsOk());

  ProxyServer server = info.proxy_server();
  HostPortPair host_port = server.host_port_pair();
  EXPECT_TRUE(server.scheme() == ProxyServer::SCHEME_SOCKS5);
  EXPECT_EQ(host_port.host(), "127.0.0.1");
  EXPECT_EQ(host_port.port(), 5566);
  EXPECT_EQ(host_port.username(), isolation_key);
  EXPECT_FALSE(host_port.password().empty());
}

}  // namespace net
