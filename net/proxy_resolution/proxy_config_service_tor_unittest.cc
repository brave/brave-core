/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/net/proxy_resolution/proxy_config_service_tor.h"

#include <memory>
#include <string>
#include <utility>

#include "base/location.h"
#include "base/task/single_thread_task_runner.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "net/base/proxy_server.h"
#include "net/base/schemeful_site.h"
#include "net/proxy_resolution/configured_proxy_resolution_service.h"
#include "net/proxy_resolution/mock_proxy_resolver.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/proxy_resolution/proxy_config_with_annotation.h"
#include "net/test/test_with_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace net {

class ProxyConfigServiceTorTest : public TestWithTaskEnvironment {
 public:
  ProxyConfigServiceTorTest()
      : TestWithTaskEnvironment(
            base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        proxy_uri_("socks5://127.0.0.1:5566") {}
  ProxyConfigServiceTorTest(const ProxyConfigServiceTorTest&) = delete;
  ProxyConfigServiceTorTest& operator=(const ProxyConfigServiceTorTest&) =
      delete;
  ~ProxyConfigServiceTorTest() override = default;

  void SetUp() override {
    auto config_service =
        net::ProxyConfigService::CreateSystemProxyConfigService(
            base::SingleThreadTaskRunner::GetCurrentDefault());

    service_ = std::make_unique<ConfiguredProxyResolutionService>(
        std::move(config_service),
        std::make_unique<MockAsyncProxyResolverFactory>(false), nullptr,
        /*quick_check_enabled=*/true);
  }

  ConfiguredProxyResolutionService* service() { return service_.get(); }

  void CheckProxyServer(const base::Location& location,
                        const net::ProxyServer& proxy_server,
                        const std::string& expected_username) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    ASSERT_TRUE(proxy_server.scheme() == ProxyServer::SCHEME_SOCKS5);
    ASSERT_EQ(proxy_server.host_port_pair().host(), "127.0.0.1");
    ASSERT_EQ(proxy_server.host_port_pair().port(), 5566);
    EXPECT_EQ(proxy_server.host_port_pair().username(), expected_username);
    EXPECT_TRUE(!proxy_server.host_port_pair().password().empty());
  }

  std::string proxy_uri() const { return proxy_uri_; }

 private:
  std::string proxy_uri_;
  std::unique_ptr<net::ConfiguredProxyResolutionService> service_;
};

TEST_F(ProxyConfigServiceTorTest, CircuitAnonymizationKey) {
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
    std::string actual_key =
        ProxyConfigServiceTor::CircuitAnonymizationKey(url);

    EXPECT_EQ(expected_key, actual_key);
  }
}

TEST_F(ProxyConfigServiceTorTest, SetNewTorCircuit) {
  const GURL site_url("https://check.torproject.org/");
  const std::string circuit_anonymization_key =
      ProxyConfigServiceTor::CircuitAnonymizationKey(site_url);

  ProxyConfigServiceTor proxy_config_service(proxy_uri());
  ProxyConfigWithAnnotation config;

  proxy_config_service.SetNewTorCircuit(site_url);
  proxy_config_service.GetLatestProxyConfig(&config);
  auto single_proxy =
      config.value().proxy_rules().single_proxies.First().GetProxyServer(
          /*chain_index=*/0);
  CheckProxyServer(FROM_HERE, single_proxy, circuit_anonymization_key);
}

TEST_F(ProxyConfigServiceTorTest, SetProxyAuthorization) {
  const GURL site_url("https://check.torproject.org/");
  const GURL site_url2("https://brave.com/");
  const std::string circuit_anonymization_key =
      ProxyConfigServiceTor::CircuitAnonymizationKey(site_url);
  const std::string circuit_anonymization_key2 =
      ProxyConfigServiceTor::CircuitAnonymizationKey(site_url2);
  const net::SchemefulSite site(site_url);
  const net::SchemefulSite site2(site_url2);
  const auto network_anonymization_key =
      net::NetworkAnonymizationKey::CreateFromFrameSite(site, site);
  const auto network_anonymization_key2 =
      net::NetworkAnonymizationKey::CreateFromFrameSite(site2, site2);

  ProxyConfigServiceTor proxy_config_service(proxy_uri());
  ProxyConfigWithAnnotation config;
  proxy_config_service.GetLatestProxyConfig(&config);

  ProxyInfo info;
  ProxyConfigServiceTor::SetProxyAuthorization(
      config, site_url, network_anonymization_key, service(), &info);
  auto proxy_server = info.proxy_chain().GetProxyServer(/*server_index=*/0);
  CheckProxyServer(FROM_HERE, proxy_server, circuit_anonymization_key);

  // everything should still be the same on subsequent calls
  std::string password = proxy_server.host_port_pair().password();
  ProxyConfigServiceTor::SetProxyAuthorization(
      config, site_url, network_anonymization_key, service(), &info);
  proxy_server = info.proxy_chain().GetProxyServer(/*server_index=*/0);
  CheckProxyServer(FROM_HERE, proxy_server, circuit_anonymization_key);

  // Test new tor circuit.
  proxy_config_service.SetNewTorCircuit(site_url);
  proxy_config_service.GetLatestProxyConfig(&config);
  ProxyConfigServiceTor::SetProxyAuthorization(
      config, site_url, network_anonymization_key, service(), &info);
  proxy_server = info.proxy_chain().GetProxyServer(/*server_index=*/0);
  CheckProxyServer(FROM_HERE, proxy_server, circuit_anonymization_key);

  EXPECT_NE(proxy_server.host_port_pair().password(), password);

  // everything should still be the same on subsequent calls
  password = proxy_server.host_port_pair().password();
  ProxyConfigServiceTor::SetProxyAuthorization(
      config, site_url, network_anonymization_key, service(), &info);
  proxy_server = info.proxy_chain().GetProxyServer(/*server_index=*/0);
  CheckProxyServer(FROM_HERE, proxy_server, circuit_anonymization_key);

  // SetNewTorCircuit should not affect other urls
  proxy_config_service.GetLatestProxyConfig(&config);
  ProxyConfigServiceTor::SetProxyAuthorization(
      config, site_url2, network_anonymization_key2, service(), &info);
  proxy_server = info.proxy_chain().GetProxyServer(/*server_index=*/0);
  CheckProxyServer(FROM_HERE, proxy_server, circuit_anonymization_key2);
  EXPECT_NE(proxy_server.host_port_pair().password(), password);

  const auto tag = ProxyConfigServiceTor::GetTorAnnotationTagForTesting();
  // Empty config
  const ProxyConfigWithAnnotation empty_config(ProxyConfig(), tag);
  info = ProxyInfo();
  ProxyConfigServiceTor::SetProxyAuthorization(
      empty_config, site_url, network_anonymization_key, service(), &info);
  EXPECT_TRUE(info.is_empty());

  // Empty proxy rules
  const ProxyConfigWithAnnotation empty_proxy_rules_config(
      ProxyConfig::CreateForTesting(ProxyList()), tag);
  info = ProxyInfo();
  ProxyConfigServiceTor::SetProxyAuthorization(
      empty_proxy_rules_config, site_url, network_anonymization_key, service(),
      &info);
  EXPECT_TRUE(info.is_empty());
}

TEST_F(ProxyConfigServiceTorTest, SetProxyAuthorization_Subresources) {
  const GURL site_url1("https://brave.com/");
  const GURL site_url2("https://bravesoftware.com/");  // subresource
  const GURL site_url3("https://brave.software.com/");
  const net::SchemefulSite site1(site_url1);
  const net::SchemefulSite site2(site_url2);
  const net::SchemefulSite site3(site_url3);
  const auto network_anonymization_key_1_2 =
      net::NetworkAnonymizationKey::CreateFromFrameSite(site1, site2);
  const std::string circuit_anonymization_key1 =
      ProxyConfigServiceTor::CircuitAnonymizationKey(site_url1);
  const auto network_anonymization_key_3_2 =
      net::NetworkAnonymizationKey::CreateFromFrameSite(site3, site2);
  const std::string circuit_anonymization_key3 =
      ProxyConfigServiceTor::CircuitAnonymizationKey(site_url3);

  ProxyConfigServiceTor proxy_config_service(proxy_uri());
  ProxyConfigWithAnnotation config;
  proxy_config_service.GetLatestProxyConfig(&config);

  ProxyInfo info;
  ProxyConfigServiceTor::SetProxyAuthorization(
      config, site_url2, network_anonymization_key_1_2, service(), &info);
  auto proxy_server = info.proxy_chain().GetProxyServer(/*server_index=*/0);
  CheckProxyServer(FROM_HERE, proxy_server, circuit_anonymization_key1);
  const auto password1 = proxy_server.host_port_pair().password();

  ProxyConfigServiceTor::SetProxyAuthorization(
      config, site_url2, network_anonymization_key_3_2, service(), &info);
  proxy_server = info.proxy_chain().GetProxyServer(/*server_index=*/0);
  CheckProxyServer(FROM_HERE, proxy_server, circuit_anonymization_key3);
  const auto password3 = proxy_server.host_port_pair().password();

  EXPECT_NE(password1, password3);
}

TEST_F(ProxyConfigServiceTorTest, CircuitTimeout) {
  const GURL site_url("https://brave.com/");
  const std::string circuit_anonymization_key =
      ProxyConfigServiceTor::CircuitAnonymizationKey(site_url);
  const net::SchemefulSite site(site_url);
  const auto network_anonymization_key =
      net::NetworkAnonymizationKey::CreateFromFrameSite(site, site);

  ProxyConfigServiceTor proxy_config_service(proxy_uri());
  ProxyConfigWithAnnotation config;
  proxy_config_service.GetLatestProxyConfig(&config);

  ProxyInfo info;
  ProxyConfigServiceTor::SetProxyAuthorization(
      config, site_url, network_anonymization_key, service(), &info);
  auto proxy_server = info.proxy_chain().GetProxyServer(/*server_index=*/0);
  CheckProxyServer(FROM_HERE, proxy_server, circuit_anonymization_key);

  auto password = proxy_server.host_port_pair().password();

  // password is still the same
  FastForwardBy(base::Minutes(9));
  ProxyConfigServiceTor::SetProxyAuthorization(
      config, site_url, network_anonymization_key, service(), &info);
  proxy_server = info.proxy_chain().GetProxyServer(/*server_index=*/0);
  CheckProxyServer(FROM_HERE, proxy_server, circuit_anonymization_key);
  EXPECT_EQ(proxy_server.host_port_pair().password(), password);

  // Exceeds 10 mins, new password is generated
  FastForwardBy(base::Minutes(2));
  ProxyConfigServiceTor::SetProxyAuthorization(
      config, site_url, network_anonymization_key, service(), &info);
  proxy_server = info.proxy_chain().GetProxyServer(/*server_index=*/0);
  CheckProxyServer(FROM_HERE, proxy_server, circuit_anonymization_key);
  EXPECT_NE(proxy_server.host_port_pair().password(), password);
  password = proxy_server.host_port_pair().password();

  // Another timeout
  FastForwardBy(base::Minutes(11));
  ProxyConfigServiceTor::SetProxyAuthorization(
      config, site_url, network_anonymization_key, service(), &info);
  proxy_server = info.proxy_chain().GetProxyServer(/*server_index=*/0);
  CheckProxyServer(FROM_HERE, proxy_server, circuit_anonymization_key);
  EXPECT_NE(proxy_server.host_port_pair().password(), password);
}
}  // namespace net
