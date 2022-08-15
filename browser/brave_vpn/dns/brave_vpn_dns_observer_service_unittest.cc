/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/dns/brave_vpn_dns_observer_service.h"

#include <unordered_map>

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#include "brave/components/brave_vpn/pref_names.h"
#include "chrome/browser/net/secure_dns_config.h"
#include "chrome/browser/net/secure_dns_util.h"
#include "chrome/browser/net/stub_resolver_config_reader.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/country_codes/country_codes.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "net/dns/public/doh_provider_entry.h"
#include "net/dns/public/secure_dns_mode.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn {
namespace {
std::string GetDefaultDNSProvidersForCountry() {
  namespace secure_dns = chrome_browser_net::secure_dns;
  // Use default hardcoded servers for current country.
  auto providers = secure_dns::ProvidersForCountry(
      secure_dns::SelectEnabledProviders(net::DohProviderEntry::GetList()),
      country_codes::GetCurrentCountryID());
  for (const net::DohProviderEntry* entry : net::DohProviderEntry::GetList()) {
    if (entry->provider != "Cloudflare")
      continue;
    net::DnsOverHttpsConfig doh_config({entry->doh_server_config});
    return doh_config.ToString();
  }
  NOTREACHED() << "Should not be reached as we expect cloudflare is available "
                  "in default list.";
  return std::string();
}
}  // namespace

class BraveVpnDnsObserverServiceUnitTest : public testing::Test {
 public:
  BraveVpnDnsObserverServiceUnitTest()
      : local_state_(TestingBrowserProcess::GetGlobal()) {}

  void SetUp() override {
    dns_service_.reset(new BraveVpnDnsObserverService(
        local_state(),
        base::BindRepeating(
            &BraveVpnDnsObserverServiceUnitTest::ReadDNSPolicyValue,
            base::Unretained(this))));
    stub_resolver_config_reader_ =
        std::make_unique<StubResolverConfigReader>(local_state_.Get());
    SystemNetworkContextManager::set_stub_resolver_config_reader_for_testing(
        stub_resolver_config_reader_.get());
    SetPolicyValue(policy::key::kDnsOverHttpsMode, "");
    brave_vpn::RegisterProfilePrefs(pref_service_.registry());
    dns_service_->SetPrefServiceForTesting(pref_service());
  }

  std::string ReadDNSPolicyValue(const std::string& name) {
    EXPECT_TRUE(policy_map_.count(name));
    return policy_map_.at(name);
  }

  void TearDown() override {
    // BraveVpnDnsObserverService destructor must be called before the task
    // runner is destroyed.
    dns_service_.reset();
  }
  PrefService* local_state() { return local_state_.Get(); }
  PrefService* pref_service() { return &pref_service_; }
  void FireBraveVPNStateChange(mojom::ConnectionState state) {
    dns_service_->OnConnectionStateChanged(state);
  }

  bool ShowPolicyNotification(mojom::ConnectionState state) {
    bool callback_called = false;
    dns_service_->SetPolicyNotificationCallbackForTesting(
        base::BindLambdaForTesting([&]() { callback_called = true; }));
    FireBraveVPNStateChange(state);
    return callback_called;
  }
  void SetDNSMode(const std::string& mode, const std::string& doh_providers) {
    local_state()->SetString(::prefs::kDnsOverHttpsTemplates, doh_providers);
    local_state()->SetString(::prefs::kDnsOverHttpsMode, mode);
  }

  void ExpectDNSMode(const std::string& mode,
                     const std::string& doh_providers) {
    EXPECT_EQ(local_state()->GetString(::prefs::kDnsOverHttpsMode), mode);
    EXPECT_EQ(local_state()->GetString(::prefs::kDnsOverHttpsTemplates),
              doh_providers);
  }
  void AllowUsersChange(bool value) {
    dns_service_->SetAllowExternalChangesForTesting(value);
  }
  void SetPolicyValue(const std::string& name, const std::string& value) {
    policy_map_[name] = value;
  }

 private:
  std::unordered_map<std::string, std::string> policy_map_;
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<BraveVpnDnsObserverService> dns_service_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  ScopedTestingLocalState local_state_;
  std::unique_ptr<StubResolverConfigReader> stub_resolver_config_reader_;
};

TEST_F(BraveVpnDnsObserverServiceUnitTest, AutoEnable) {
  auto default_servers = GetDefaultDNSProvidersForCountry();
  EXPECT_FALSE(default_servers.empty());

  // User has DoH mode off and enables vpn
  {
    SetDNSMode(SecureDnsConfig::kModeOff, "");
    FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
    ExpectDNSMode(SecureDnsConfig::kModeOff, "");
    FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
    ExpectDNSMode(SecureDnsConfig::kModeSecure, default_servers);
    FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTING);
    ExpectDNSMode(SecureDnsConfig::kModeOff, "");
    FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
    ExpectDNSMode(SecureDnsConfig::kModeOff, "");
  }

  // User has DoH mode automatic without servers and enables vpn
  {
    SetDNSMode(SecureDnsConfig::kModeAutomatic, "");
    FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
    ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
    FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
    ExpectDNSMode(SecureDnsConfig::kModeSecure, default_servers);
    FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTING);
    ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
    FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
    ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
  }
  std::string custom_servers =
      "https://server1.com\nhttps://server2.com/{?dns}";
  // User has DoH mode automatic with own servers and enables vpn
  {
    SetDNSMode(SecureDnsConfig::kModeAutomatic, custom_servers);
    FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
    ExpectDNSMode(SecureDnsConfig::kModeAutomatic, custom_servers);
    FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
    ExpectDNSMode(SecureDnsConfig::kModeSecure, custom_servers);
    FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTING);
    ExpectDNSMode(SecureDnsConfig::kModeAutomatic, custom_servers);
    FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
    ExpectDNSMode(SecureDnsConfig::kModeAutomatic, custom_servers);
  }
  // User has DoH mode secure without custom servers and enables vpn
  {
    SetDNSMode(SecureDnsConfig::kModeSecure, "");
    FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
    ExpectDNSMode(SecureDnsConfig::kModeSecure, "");
    FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
    ExpectDNSMode(SecureDnsConfig::kModeSecure, "");
    FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTING);
    ExpectDNSMode(SecureDnsConfig::kModeSecure, "");
    FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
    ExpectDNSMode(SecureDnsConfig::kModeSecure, "");
  }
  // User has DoH mode secure with custom servers and enables vpn
  {
    SetDNSMode(SecureDnsConfig::kModeSecure, custom_servers);
    FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
    ExpectDNSMode(SecureDnsConfig::kModeSecure, custom_servers);
    FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
    ExpectDNSMode(SecureDnsConfig::kModeSecure, custom_servers);
    FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTING);
    ExpectDNSMode(SecureDnsConfig::kModeSecure, custom_servers);
    FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
    ExpectDNSMode(SecureDnsConfig::kModeSecure, custom_servers);
  }
}

TEST_F(BraveVpnDnsObserverServiceUnitTest, AllowUsersChange) {
  auto default_servers = GetDefaultDNSProvidersForCountry();
  EXPECT_FALSE(default_servers.empty());
  AllowUsersChange(true);
  {
    SetDNSMode(SecureDnsConfig::kModeOff, "");
    FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
    ExpectDNSMode(SecureDnsConfig::kModeOff, "");
    FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
    ExpectDNSMode(SecureDnsConfig::kModeSecure, default_servers);
    // User enabled automatic DoH mode while vpn connected.
    SetDNSMode(SecureDnsConfig::kModeAutomatic, "");
    ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
    FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTING);
    ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
    FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
    ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
  }

  {
    SetDNSMode(SecureDnsConfig::kModeAutomatic, "");
    FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
    ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
    FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
    ExpectDNSMode(SecureDnsConfig::kModeSecure, default_servers);
    // User enabled automatic DoH mode while vpn connected.
    SetDNSMode(SecureDnsConfig::kModeOff, "");
    ExpectDNSMode(SecureDnsConfig::kModeOff, "");
    FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTING);
    ExpectDNSMode(SecureDnsConfig::kModeOff, "");
    FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
    ExpectDNSMode(SecureDnsConfig::kModeOff, "");
  }
}

TEST_F(BraveVpnDnsObserverServiceUnitTest, DisallowUsersChange) {
  auto default_servers = GetDefaultDNSProvidersForCountry();
  EXPECT_FALSE(default_servers.empty());
  AllowUsersChange(false);
  {
    SetDNSMode(SecureDnsConfig::kModeOff, "");
    FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
    ExpectDNSMode(SecureDnsConfig::kModeOff, "");
    FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
    ExpectDNSMode(SecureDnsConfig::kModeSecure, default_servers);

    // User disabled DoH mode while vpn connected.
    SetDNSMode(SecureDnsConfig::kModeOff, "");
    ExpectDNSMode(SecureDnsConfig::kModeSecure, default_servers);
    FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTING);
    ExpectDNSMode(SecureDnsConfig::kModeOff, "");
    FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
    ExpectDNSMode(SecureDnsConfig::kModeOff, "");
  }

  {
    SetDNSMode(SecureDnsConfig::kModeAutomatic, "");
    FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
    ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
    FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
    ExpectDNSMode(SecureDnsConfig::kModeSecure, default_servers);
    // User enabled automatic DoH mode while vpn connected.
    SetDNSMode(SecureDnsConfig::kModeAutomatic, "");
    ExpectDNSMode(SecureDnsConfig::kModeSecure, default_servers);
    FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTING);
    ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
    FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
    ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
  }
}

TEST_F(BraveVpnDnsObserverServiceUnitTest, DohDisabledByPolicy) {
  SetPolicyValue(policy::key::kDnsOverHttpsMode, SecureDnsConfig::kModeOff);
  {
    SetDNSMode(SecureDnsConfig::kModeOff, "");
    EXPECT_FALSE(ShowPolicyNotification(mojom::ConnectionState::CONNECTING));
    ExpectDNSMode(SecureDnsConfig::kModeOff, "");
    EXPECT_TRUE(ShowPolicyNotification(mojom::ConnectionState::CONNECTED));
    ExpectDNSMode(SecureDnsConfig::kModeOff, "");
    EXPECT_FALSE(ShowPolicyNotification(mojom::ConnectionState::DISCONNECTING));
    ExpectDNSMode(SecureDnsConfig::kModeOff, "");
    EXPECT_FALSE(ShowPolicyNotification(mojom::ConnectionState::DISCONNECTED));
    ExpectDNSMode(SecureDnsConfig::kModeOff, "");
  }

  SetPolicyValue(policy::key::kDnsOverHttpsMode,
                 SecureDnsConfig::kModeAutomatic);
  {
    SetDNSMode(SecureDnsConfig::kModeAutomatic, "");
    EXPECT_FALSE(ShowPolicyNotification(mojom::ConnectionState::CONNECTING));
    ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
    EXPECT_TRUE(ShowPolicyNotification(mojom::ConnectionState::CONNECTED));
    ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
    EXPECT_FALSE(ShowPolicyNotification(mojom::ConnectionState::DISCONNECTING));
    ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
    EXPECT_FALSE(ShowPolicyNotification(mojom::ConnectionState::DISCONNECTED));
    ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
  }

  // Do not show dialog option enabled
  SetPolicyValue(policy::key::kDnsOverHttpsMode, SecureDnsConfig::kModeOff);
  {
    pref_service()->SetBoolean(prefs::kBraveVPNShowDNSPolicyWarningDialog,
                               false);
    SetDNSMode(SecureDnsConfig::kModeOff, "");
    EXPECT_FALSE(ShowPolicyNotification(mojom::ConnectionState::CONNECTED));
  }
}

}  // namespace brave_vpn
