/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/net/proxy_resolution/proxy_config_service_tor.h"

#include <string>
#include <memory>

#include "base/threading/thread_task_runner_handle.h"
#include "net/base/proxy_server.h"
#include "net/proxy_resolution/configured_proxy_resolution_service.h"
#include "net/proxy_resolution/mock_proxy_resolver.h"
#include "net/proxy_resolution/proxy_config_with_annotation.h"
#include "net/test/test_with_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace net {

class ProxyConfigServiceTorTest : public TestWithTaskEnvironment {
 public:
  ProxyConfigServiceTorTest() {}
  ProxyConfigServiceTorTest(const ProxyConfigServiceTorTest&) = delete;
  ProxyConfigServiceTorTest& operator=(const ProxyConfigServiceTorTest&) =
      delete;
  ~ProxyConfigServiceTorTest() override {}

 private:
};

TEST_F(ProxyConfigServiceTorTest, CircuitIsolationKey) {
  const struct {
    GURL url;
    std::string key;
  } cases[] = {
      {
          GURL("https://1.1.1.1/"),
          "1.1.1.1",
      },
      {
          GURL("https://1.1.1.1:53/"),
          "1.1.1.1",
      },
      {
          GURL("https://127.0.0.1/"),
          "127.0.0.1",
      },
      {
          GURL("https://127.0.0.53/"),
          "127.0.0.53",
      },
      {
          GURL("https://8.8.8.8/"),
          "8.8.8.8",
      },
      {
          GURL("https://8.8.8.8:80/"),
          "8.8.8.8",
      },
      {
          GURL("https://[::1]/"),
          "[::1]",
      },
      {
          GURL("https://check.torproject.org/"),
          "torproject.org",
      },
      {
          GURL("https://check.torproject.org/x"),
          "torproject.org",
      },
      {
          GURL("https://check.torproject.org/x?y"),
          "torproject.org",
      },
      {
          GURL("https://check.torproject.org/x?y#z"),
          "torproject.org",
      },
      {
          GURL("https://localhost/"),
          "localhost",
      },
      {
          GURL("https://localhost:8888/"),
          "localhost",
      },
      {
          GURL("https://user:pass@localhost:8888/"),
          "localhost",
      },
      {
          GURL("https://www.bbc.co.uk/"),
          "bbc.co.uk",
      },
  };

  for (auto& c : cases) {
    const GURL& url = c.url;
    const std::string& expected_key = c.key;
    std::string actual_key = ProxyConfigServiceTor::CircuitIsolationKey(url);

    EXPECT_EQ(expected_key, actual_key);
  }
}

TEST_F(ProxyConfigServiceTorTest, SetNewTorCircuit) {
  const std::string proxy_uri("socks5://127.0.0.1:5566");
  const GURL site_url("https://check.torproject.org/");
  const std::string isolation_key =
      ProxyConfigServiceTor::CircuitIsolationKey(site_url);

  ProxyConfigServiceTor proxy_config_service(proxy_uri);
  ProxyConfigWithAnnotation config;

  proxy_config_service.SetNewTorCircuit(site_url);
  proxy_config_service.GetLatestProxyConfig(&config);
  auto single_proxy = config.value().proxy_rules().single_proxies.Get();
  EXPECT_TRUE(!single_proxy.host_port_pair().password().empty());
  EXPECT_TRUE(single_proxy.scheme() == ProxyServer::SCHEME_SOCKS5);
  EXPECT_EQ(single_proxy.host_port_pair().username(), isolation_key);
  EXPECT_EQ(single_proxy.host_port_pair().host(), "127.0.0.1");
  EXPECT_EQ(single_proxy.host_port_pair().port(), 5566);
}

TEST_F(ProxyConfigServiceTorTest, SetProxyAuthorization) {
  const std::string proxy_uri("socks5://127.0.0.1:5566");
  const GURL site_url("https://check.torproject.org/");
  const GURL site_url2("https://brave.com/");
  const std::string isolation_key =
      ProxyConfigServiceTor::CircuitIsolationKey(site_url);
  const std::string isolation_key2 =
      ProxyConfigServiceTor::CircuitIsolationKey(site_url2);

  auto config_service =
      ConfiguredProxyResolutionService::CreateSystemProxyConfigService(
          base::ThreadTaskRunnerHandle::Get());

  auto* service = new ConfiguredProxyResolutionService(
      std::move(config_service),
      std::make_unique<MockAsyncProxyResolverFactory>(false), nullptr,
      /*quick_check_enabled=*/true);

  ProxyConfigServiceTor proxy_config_service(proxy_uri);
  ProxyConfigWithAnnotation config;
  proxy_config_service.GetLatestProxyConfig(&config);

  ProxyInfo info;
  ProxyConfigServiceTor::SetProxyAuthorization(
      config, site_url, service, &info);
  auto host_port_pair = info.proxy_server().host_port_pair();

  EXPECT_EQ(host_port_pair.username(), isolation_key);
  EXPECT_TRUE(info.proxy_server().scheme() == ProxyServer::SCHEME_SOCKS5);
  EXPECT_EQ(host_port_pair.host(), "127.0.0.1");
  EXPECT_EQ(host_port_pair.port(), 5566);

  // everything should still be the same on subsequent calls
  std::string password = host_port_pair.password();
  ProxyInfo info2;
  ProxyConfigServiceTor::SetProxyAuthorization(
      config, site_url, service, &info2);
  host_port_pair = info2.proxy_server().host_port_pair();

  EXPECT_EQ(host_port_pair.username(), isolation_key);
  EXPECT_EQ(host_port_pair.password(), password);
  EXPECT_TRUE(info2.proxy_server().scheme() == ProxyServer::SCHEME_SOCKS5);
  EXPECT_EQ(host_port_pair.host(), "127.0.0.1");
  EXPECT_EQ(host_port_pair.port(), 5566);

  // TODO(darkdh): Test persistent circuit isolation until timeout.

  // Test new tor circuit.
  proxy_config_service.SetNewTorCircuit(site_url);
  proxy_config_service.GetLatestProxyConfig(&config);
  ProxyInfo info3;
  ProxyConfigServiceTor::SetProxyAuthorization(
      config, site_url, service, &info3);
  host_port_pair = info3.proxy_server().host_port_pair();

  EXPECT_EQ(host_port_pair.username(), isolation_key);
  EXPECT_NE(host_port_pair.password(), password);
  EXPECT_TRUE(info3.proxy_server().scheme() == ProxyServer::SCHEME_SOCKS5);
  EXPECT_EQ(host_port_pair.host(), "127.0.0.1");
  EXPECT_EQ(host_port_pair.port(), 5566);

  // everything should still be the same on subsequent calls
  password = host_port_pair.password();
  ProxyInfo info4;
  ProxyConfigServiceTor::SetProxyAuthorization(
      config, site_url, service, &info4);
  host_port_pair = info4.proxy_server().host_port_pair();

  EXPECT_EQ(host_port_pair.username(), isolation_key);
  EXPECT_EQ(host_port_pair.password(), password);
  EXPECT_TRUE(info4.proxy_server().scheme() == ProxyServer::SCHEME_SOCKS5);
  EXPECT_EQ(host_port_pair.host(), "127.0.0.1");
  EXPECT_EQ(host_port_pair.port(), 5566);

  // SetNewTorCircuit should not affect other urls
  proxy_config_service.GetLatestProxyConfig(&config);
  ProxyInfo info5;
  ProxyConfigServiceTor::SetProxyAuthorization(
      config, site_url2, service, &info5);
  host_port_pair = info5.proxy_server().host_port_pair();
  EXPECT_EQ(host_port_pair.username(), isolation_key2);
  EXPECT_NE(host_port_pair.password(), password);
  EXPECT_TRUE(info5.proxy_server().scheme() == ProxyServer::SCHEME_SOCKS5);
  EXPECT_EQ(host_port_pair.host(), "127.0.0.1");
  EXPECT_EQ(host_port_pair.port(), 5566);
}

}  // namespace net
