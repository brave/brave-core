/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/net/proxy_resolution/proxy_config_service_tor.h"

#include <string>

#include "base/macros.h"
#include "net/proxy_resolution/proxy_config_with_annotation.h"
#include "net/test/test_with_scoped_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace net {

class ProxyConfigServiceTorTest : public TestWithScopedTaskEnvironment {
 public:
  ProxyConfigServiceTorTest() {}
  ~ProxyConfigServiceTorTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ProxyConfigServiceTorTest);
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

TEST_F(ProxyConfigServiceTorTest, SetUsername) {
  std::string proxy_uri("socks5://127.0.0.1:5566");
  GURL site_url("https://check.torproject.org/");
  std::string isolation_key =
      ProxyConfigServiceTor::CircuitIsolationKey(site_url);
  ProxyConfigServiceTor proxy_config_service(proxy_uri);
  proxy_config_service.SetUsername(isolation_key, GetTorProxyMap());

  ProxyConfigWithAnnotation config;
  proxy_config_service.GetLatestProxyConfig(&config);
  ASSERT_FALSE(config.value().proxy_rules().single_proxies.IsEmpty());
  ProxyServer server = config.value().proxy_rules().single_proxies.Get();
  HostPortPair host_port = server.host_port_pair();
  EXPECT_TRUE(server.scheme() == ProxyServer::SCHEME_SOCKS5);
  EXPECT_EQ(host_port.host(), "127.0.0.1");
  EXPECT_EQ(host_port.port(), 5566);
  EXPECT_EQ(host_port.username(), isolation_key);

  // Test persistent circuit isolation until timeout.
  std::string password = host_port.password();
  EXPECT_FALSE(host_port.password().empty());
  proxy_config_service.SetUsername(isolation_key, GetTorProxyMap());
  proxy_config_service.GetLatestProxyConfig(&config);
  ASSERT_FALSE(config.value().proxy_rules().single_proxies.IsEmpty());
  server = config.value().proxy_rules().single_proxies.Get();
  host_port = server.host_port_pair();
  EXPECT_EQ(host_port.password(), password);

  // Test new identity.
  GetTorProxyMap()->Erase(isolation_key);
  proxy_config_service.SetUsername(isolation_key, GetTorProxyMap());
  proxy_config_service.GetLatestProxyConfig(&config);
  ASSERT_FALSE(config.value().proxy_rules().single_proxies.IsEmpty());
  server = config.value().proxy_rules().single_proxies.Get();
  host_port = server.host_port_pair();
  EXPECT_NE(host_port.password(), password);
}

}  // namespace net
