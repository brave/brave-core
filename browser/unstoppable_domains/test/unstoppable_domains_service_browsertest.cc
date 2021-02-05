/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/components/unstoppable_domains/constants.h"
#include "brave/components/unstoppable_domains/features.h"
#include "brave/components/unstoppable_domains/pref_names.h"
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

namespace unstoppable_domains {

class UnstoppableDomainsServiceBrowserTest : public InProcessBrowserTest {
 public:
  UnstoppableDomainsServiceBrowserTest() {
    feature_list_.InitAndEnableFeature(features::kUnstoppableDomains);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    stub_config_reader_ =
        SystemNetworkContextManager::GetStubResolverConfigReader();
    ASSERT_TRUE(stub_config_reader_);
  }

  ~UnstoppableDomainsServiceBrowserTest() override = default;

  PrefService* local_state() { return g_browser_process->local_state(); }

  SecureDnsConfig GetSecureDnsConfiguration() {
    return stub_config_reader_->GetSecureDnsConfiguration(
        false /*force_check_parental_controls_for_automatic_mode */);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
  StubResolverConfigReader* stub_config_reader_;
};

IN_PROC_BROWSER_TEST_F(UnstoppableDomainsServiceBrowserTest,
                       UpdateConfigWhenPrefChanged) {
  // Initial state.
  EXPECT_EQ(local_state()->GetInteger(kResolveMethod),
            static_cast<int>(ResolveMethodTypes::ASK));
  SecureDnsConfig config = GetSecureDnsConfiguration();
  EXPECT_EQ(config.mode(), net::SecureDnsMode::kAutomatic);
  EXPECT_EQ(config.servers().size(), 0u);

  // Set resolve method to DoH should update the config.
  local_state()->SetInteger(
      kResolveMethod, static_cast<int>(ResolveMethodTypes::DNS_OVER_HTTPS));
  config = GetSecureDnsConfiguration();
  std::vector<net::DnsOverHttpsServerConfig> expected_doh_servers = {
      {kDoHResolver, true}};
  EXPECT_EQ(config.servers(), expected_doh_servers);

  // Set custom DoH provider should still keep the resolver for UD.
  local_state()->SetString(prefs::kDnsOverHttpsTemplates, "https://test.com");
  config = GetSecureDnsConfiguration();
  expected_doh_servers = {{kDoHResolver, true}, {"https://test.com", true}};
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

  // Set resolve method to disabled should keep user's DoH setting.
  local_state()->SetInteger(kResolveMethod,
                            static_cast<int>(ResolveMethodTypes::DISABLED));
  config = GetSecureDnsConfiguration();
  expected_doh_servers = {{"https://test.com", true}};
  EXPECT_EQ(config.servers(), expected_doh_servers);
}

}  // namespace unstoppable_domains
