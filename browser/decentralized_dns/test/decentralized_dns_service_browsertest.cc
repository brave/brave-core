/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/decentralized_dns/constants.h"
#include "brave/components/decentralized_dns/features.h"
#include "brave/components/decentralized_dns/pref_names.h"
#include "brave/net/decentralized_dns/constants.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/secure_dns_config.h"
#include "chrome/browser/net/stub_resolver_config_reader.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "net/dns/public/secure_dns_mode.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace decentralized_dns {

class DecentralizedDnsServiceBrowserTest : public InProcessBrowserTest {
 public:
  DecentralizedDnsServiceBrowserTest() {
    feature_list_.InitAndEnableFeature(features::kDecentralizedDns);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    stub_config_reader_ =
        SystemNetworkContextManager::GetStubResolverConfigReader();
    ASSERT_TRUE(stub_config_reader_);
  }

  ~DecentralizedDnsServiceBrowserTest() override = default;

  PrefService* local_state() { return g_browser_process->local_state(); }

  SecureDnsConfig GetSecureDnsConfiguration(
      bool force_check_parental_controls_for_automatic_mode = false) {
    return stub_config_reader_->GetSecureDnsConfiguration(
        force_check_parental_controls_for_automatic_mode);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
  raw_ptr<StubResolverConfigReader> stub_config_reader_ = nullptr;
};

IN_PROC_BROWSER_TEST_F(DecentralizedDnsServiceBrowserTest,
                       UpdateConfigWhenPrefChanged) {
  // Initial state.
  EXPECT_EQ(local_state()->GetInteger(kUnstoppableDomainsResolveMethod),
            static_cast<int>(ResolveMethodTypes::ASK));
  SecureDnsConfig config = GetSecureDnsConfiguration();
  EXPECT_EQ(config.mode(), net::SecureDnsMode::kAutomatic);
  EXPECT_EQ(config.servers().size(), 0u);

  // Set resolve method to DoH should update the config.
  local_state()->SetInteger(
      kUnstoppableDomainsResolveMethod,
      static_cast<int>(ResolveMethodTypes::DNS_OVER_HTTPS));
  config = GetSecureDnsConfiguration();
  std::vector<net::DnsOverHttpsServerConfig> expected_doh_servers = {
      *net::DnsOverHttpsServerConfig::FromString(
          kUnstoppableDomainsDoHResolver)};
  EXPECT_EQ(config.servers(), expected_doh_servers);

  // Set custom DoH provider should still keep the resolver for UD.
  local_state()->SetString(prefs::kDnsOverHttpsTemplates, "https://test.com");
  config = GetSecureDnsConfiguration();
  expected_doh_servers = {
      *net::DnsOverHttpsServerConfig::FromString(
          kUnstoppableDomainsDoHResolver),
      *net::DnsOverHttpsServerConfig::FromString("https://test.com")};
  EXPECT_EQ(config.servers(), expected_doh_servers);

  // Set secure mode to off should return empty DoH servers.
  local_state()->SetString(
      prefs::kDnsOverHttpsMode,
      SecureDnsConfig::ModeToString(net::SecureDnsMode::kOff));
  config = GetSecureDnsConfiguration();
  EXPECT_EQ(config.servers().size(), 0u);

  // Turn on secure mode again should get the same result as before.
  local_state()->SetString(
      prefs::kDnsOverHttpsMode,
      SecureDnsConfig::ModeToString(net::SecureDnsMode::kSecure));
  config = GetSecureDnsConfiguration();
  EXPECT_EQ(config.servers(), expected_doh_servers);

  // Set resolve method of ENS to DoH should update the config.
  local_state()->SetInteger(
      kENSResolveMethod, static_cast<int>(ResolveMethodTypes::DNS_OVER_HTTPS));
  config = GetSecureDnsConfiguration();
  expected_doh_servers = {
      *net::DnsOverHttpsServerConfig::FromString(kENSDoHResolver),
      *net::DnsOverHttpsServerConfig::FromString(
          kUnstoppableDomainsDoHResolver),
      *net::DnsOverHttpsServerConfig::FromString("https://test.com")};
  EXPECT_EQ(config.servers(), expected_doh_servers);

  // Set resolve method to disabled should keep user's DoH setting.
  local_state()->SetInteger(kUnstoppableDomainsResolveMethod,
                            static_cast<int>(ResolveMethodTypes::DISABLED));
  local_state()->SetInteger(kENSResolveMethod,
                            static_cast<int>(ResolveMethodTypes::DISABLED));
  config = GetSecureDnsConfiguration();
  expected_doh_servers = {
      *net::DnsOverHttpsServerConfig::FromString("https://test.com")};
  EXPECT_EQ(config.servers(), expected_doh_servers);
}

IN_PROC_BROWSER_TEST_F(DecentralizedDnsServiceBrowserTest,
                       HideDecentralizedDnsResolvers) {
  // Initial state.
  EXPECT_EQ(local_state()->GetInteger(kUnstoppableDomainsResolveMethod),
            static_cast<int>(ResolveMethodTypes::ASK));
  SecureDnsConfig config = GetSecureDnsConfiguration();
  EXPECT_EQ(config.mode(), net::SecureDnsMode::kAutomatic);
  EXPECT_EQ(config.servers().size(), 0u);

  // Set resolve method to DoH should update the config.
  local_state()->SetInteger(
      kUnstoppableDomainsResolveMethod,
      static_cast<int>(ResolveMethodTypes::DNS_OVER_HTTPS));
  config = GetSecureDnsConfiguration();
  std::vector<net::DnsOverHttpsServerConfig> expected_doh_servers = {
      *net::DnsOverHttpsServerConfig::FromString(
          kUnstoppableDomainsDoHResolver)};
  EXPECT_EQ(config.servers(), expected_doh_servers);

  // Set custom DoH provider should still keep the resolver for UD.
  local_state()->SetString(prefs::kDnsOverHttpsTemplates, "https://test.com");
  config = GetSecureDnsConfiguration();
  expected_doh_servers = {
      *net::DnsOverHttpsServerConfig::FromString(
          kUnstoppableDomainsDoHResolver),
      *net::DnsOverHttpsServerConfig::FromString("https://test.com")};
  EXPECT_EQ(config.servers(), expected_doh_servers);

  // Set resolve method of ENS to DoH should update the config.
  local_state()->SetInteger(
      kENSResolveMethod, static_cast<int>(ResolveMethodTypes::DNS_OVER_HTTPS));
  config = GetSecureDnsConfiguration();
  expected_doh_servers = {
      *net::DnsOverHttpsServerConfig::FromString(kENSDoHResolver),
      *net::DnsOverHttpsServerConfig::FromString(
          kUnstoppableDomainsDoHResolver),
      *net::DnsOverHttpsServerConfig::FromString("https://test.com")};
  EXPECT_EQ(config.servers(), expected_doh_servers);

  // Should hide unstoppable domains resolver if
  // force_check_parental_controls_for_automatic_mode is true, used for hiding
  // the special resolver in settings.
  config = GetSecureDnsConfiguration(true);
  expected_doh_servers = {
      *net::DnsOverHttpsServerConfig::FromString("https://test.com")};
  EXPECT_EQ(config.servers(), expected_doh_servers);
}

}  // namespace decentralized_dns
