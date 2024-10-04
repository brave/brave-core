/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/dns/brave_vpn_dns_observer_service_win.h"

#include <unordered_map>

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_profile_prefs.h"
#include "brave/browser/brave_vpn/dns/brave_vpn_dns_observer_factory_win.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/features.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "chrome/browser/net/secure_dns_config.h"
#include "chrome/browser/net/secure_dns_util.h"
#include "chrome/browser/net/stub_resolver_config_reader.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "net/dns/public/secure_dns_mode.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn {
namespace {
constexpr char kCustomServersURLs[] =
    "https://server1.com\nhttps://server2.com/{?dns}";
constexpr char kCloudflareDnsProviderURL[] =
    "https://chrome.cloudflare-dns.com/dns-query";
}  // namespace

class BraveVpnDnsObserverServiceUnitTest : public testing::Test {
 public:
  BraveVpnDnsObserverServiceUnitTest() {}

  void SetUp() override {
    RegisterLocalState(local_state_.registry());
    TestingBrowserProcess::GetGlobal()->SetLocalState(&local_state_);
    BraveVpnDnsObserverFactory::GetInstance()->RegisterProfilePrefs(
        profile_pref_service_.registry());
    stub_resolver_config_reader_ =
        std::make_unique<StubResolverConfigReader>(&local_state_);
    SystemNetworkContextManager::set_stub_resolver_config_reader_for_testing(
        stub_resolver_config_reader_.get());
    CreateDnsObserverService();
  }

  void CreateDnsObserverService() {
    dns_observer_service_.reset(
        new BraveVpnDnsObserverService(local_state(), pref_service()));
    dns_observer_service_->SetVPNNotificationCallbackForTesting(
        base::DoNothing());
    SetDNSHelperLive(false);
  }
  void SetDNSHelperLive(bool value) {
    dns_observer_service_->SetDNSHelperLiveForTesting(value);
  }
  void ResetDnsObserverService() { dns_observer_service_.reset(); }

  void TearDown() override {
    // BraveVpnDnsObserverService destructor must be called before the task
    // runner is destroyed.
    ResetDnsObserverService();
    TestingBrowserProcess::GetGlobal()->SetLocalState(nullptr);
  }
  void EnableParentalControl(bool value) {
    StubResolverConfigReader* config_reader =
        SystemNetworkContextManager::GetStubResolverConfigReader();
    config_reader->OverrideParentalControlsForTesting(value);
  }
  PrefService* local_state() { return &local_state_; }
  PrefService* pref_service() { return &profile_pref_service_; }

  void FireBraveVPNStateChange(mojom::ConnectionState state) {
    dns_observer_service_->OnConnectionStateChanged(state);
  }

  bool WasVpnNotificationShownForState(mojom::ConnectionState state) {
    bool callback_called = false;
    dns_observer_service_->SetVPNNotificationCallbackForTesting(
        base::BindLambdaForTesting([&]() { callback_called = true; }));
    FireBraveVPNStateChange(state);
    return callback_called;
  }

  void CheckUserNotifiedAndDnsOverridenOnLaunch(
      const std::string& user_dns_mode,
      const std::string& user_servers,
      const std::string& expected_servers,
      bool expected_dialog_shown) {
    ResetDnsObserverService();
    // DNS mode was set to off by user.
    SetDNSMode(user_dns_mode, user_servers);
    // Set vpn config to indicate vpn was enabled when browser closed
    local_state()->SetString(::prefs::kBraveVpnDnsConfig,
                             kCloudflareDnsProviderURL);
    CreateDnsObserverService();
    EXPECT_TRUE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
    // After launch BraveVPNService will notify observers with actual state.
    // and we expect the dns notification dialog will not be shown as vpn is
    // enabled.
    EXPECT_EQ(
        WasVpnNotificationShownForState(mojom::ConnectionState::CONNECTED),
        expected_dialog_shown);

    // the dns config was overriden.
    ExpectDNSMode(SecureDnsConfig::kModeSecure, expected_servers);
  }

  void SetDNSMode(const std::string& mode, const std::string& doh_providers) {
    local_state()->SetString(::prefs::kDnsOverHttpsTemplates, doh_providers);
    local_state()->SetString(::prefs::kDnsOverHttpsMode, mode);
    SystemNetworkContextManager::GetStubResolverConfigReader()
        ->UpdateNetworkService(false);
  }

  bool WasPolicyNotificationShownForState(mojom::ConnectionState state) {
    bool callback_called = false;
    dns_observer_service_->SetPolicyNotificationCallbackForTesting(
        base::BindLambdaForTesting([&]() { callback_called = true; }));
    FireBraveVPNStateChange(state);
    return callback_called;
  }

  void ExpectDNSMode(const std::string& mode,
                     const std::string& doh_providers) {
    auto dns_config = SystemNetworkContextManager::GetStubResolverConfigReader()
                          ->GetSecureDnsConfiguration(false);
    auto* current_mode = SecureDnsConfig::ModeToString(dns_config.mode());
    auto current_servers = dns_config.doh_servers().ToString();
    EXPECT_EQ(current_mode, mode);
    EXPECT_EQ(current_servers, doh_providers);
  }

  void SetManagedMode(const std::string& value) {
    local_state_.SetManagedPref(::prefs::kDnsOverHttpsMode, base::Value(value));
  }

 private:
  std::unordered_map<std::string, std::string> policy_map_;
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<BraveVpnDnsObserverService> dns_observer_service_;
  sync_preferences::TestingPrefServiceSyncable profile_pref_service_;
  TestingPrefServiceSimple local_state_;
  std::unique_ptr<StubResolverConfigReader> stub_resolver_config_reader_;
};

TEST_F(BraveVpnDnsObserverServiceUnitTest, OverrideDohConfigForOffMode) {
  // Browser DoH mode off -> override browser config and enable vpn
  local_state()->ClearPref(::prefs::kBraveVpnDnsConfig);
  SetDNSMode(SecureDnsConfig::kModeOff, "");
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeOff, "");
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  EXPECT_FALSE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeOff, "");
  EXPECT_TRUE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
}

TEST_F(BraveVpnDnsObserverServiceUnitTest, OverrideDohConfigForAutomaticMode) {
  // Browser DoH mode automatic -> override browser config and enable vpn
  SetDNSMode(SecureDnsConfig::kModeAutomatic, "");
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  EXPECT_FALSE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
  EXPECT_TRUE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
}

TEST_F(BraveVpnDnsObserverServiceUnitTest, OverrideDohConfigForSecureMode) {
  // Browser DoH mode secure -> override browser config and enable vpn
  SetDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  EXPECT_FALSE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  EXPECT_TRUE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
}

TEST_F(BraveVpnDnsObserverServiceUnitTest,
       OverrideDohConfigForAutomaticModeWithValidCustomServers) {
  // Browser DoH mode automatic with custom servers
  // -> we override browser config and enable vpn
  SetDNSMode(SecureDnsConfig::kModeAutomatic, kCustomServersURLs);
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeAutomatic, kCustomServersURLs);
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  EXPECT_FALSE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeAutomatic, kCustomServersURLs);
  EXPECT_TRUE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
}

TEST_F(BraveVpnDnsObserverServiceUnitTest,
       OverrideDohConfigForAutomaticModeWithBrokenCustomServers) {
  // Browser DoH mode automatic with broken custom servers
  // -> override browser config and enable vpn
  SetDNSMode(SecureDnsConfig::kModeAutomatic, std::string());
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeAutomatic, std::string());
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  EXPECT_FALSE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeAutomatic, std::string());
  EXPECT_TRUE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
}

TEST_F(BraveVpnDnsObserverServiceUnitTest,
       OverrideDohConfigForSecureModeWithCustomServers) {
  // Browser DoH mode secure with custom servers
  // -> override browser config and enable vpn
  SetDNSMode(SecureDnsConfig::kModeSecure, kCustomServersURLs);
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCustomServersURLs);
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCustomServersURLs);
  EXPECT_FALSE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCustomServersURLs);
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCustomServersURLs);
  EXPECT_TRUE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
}

TEST_F(BraveVpnDnsObserverServiceUnitTest,
       DoNotOverrideDoHConfigWithPolicyOff) {
  SetManagedMode(SecureDnsConfig::kModeOff);

  SetDNSMode(SecureDnsConfig::kModeOff, "");
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::CONNECTING));
  ExpectDNSMode(SecureDnsConfig::kModeOff, "");
  EXPECT_TRUE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::CONNECTED));
  ExpectDNSMode(SecureDnsConfig::kModeOff, "");
  EXPECT_FALSE(WasPolicyNotificationShownForState(
      mojom::ConnectionState::DISCONNECTING));
  ExpectDNSMode(SecureDnsConfig::kModeOff, "");
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::DISCONNECTED));
  ExpectDNSMode(SecureDnsConfig::kModeOff, "");
}

TEST_F(BraveVpnDnsObserverServiceUnitTest,
       DoNotOverrideDoHConfigWithPolicyAutomatic) {
  SetManagedMode(SecureDnsConfig::kModeAutomatic);

  SetDNSMode(SecureDnsConfig::kModeAutomatic, "");
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::CONNECTING));
  ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
  EXPECT_TRUE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::CONNECTED));
  ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
  EXPECT_FALSE(WasPolicyNotificationShownForState(
      mojom::ConnectionState::DISCONNECTING));
  ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::DISCONNECTED));
  ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
}

TEST_F(BraveVpnDnsObserverServiceUnitTest,
       DoNotOverrideDoHConfigWithPolicySecure) {
  SetManagedMode(SecureDnsConfig::kModeSecure);
  SetDNSMode(SecureDnsConfig::kModeSecure, "");
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::CONNECTING));
  ExpectDNSMode(SecureDnsConfig::kModeSecure, "");
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::CONNECTED));
  ExpectDNSMode(SecureDnsConfig::kModeSecure, "");
  EXPECT_FALSE(WasPolicyNotificationShownForState(
      mojom::ConnectionState::DISCONNECTING));
  ExpectDNSMode(SecureDnsConfig::kModeSecure, "");
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::DISCONNECTED));
  ExpectDNSMode(SecureDnsConfig::kModeSecure, "");
}

TEST_F(BraveVpnDnsObserverServiceUnitTest,
       DoNotOverrideDoHConfigWithPolicySecureAndCustomServers) {
  SetDNSMode(SecureDnsConfig::kModeSecure, kCustomServersURLs);
  SetManagedMode(SecureDnsConfig::kModeSecure);
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::CONNECTING));
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCustomServersURLs);
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::CONNECTED));
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCustomServersURLs);
  EXPECT_FALSE(WasPolicyNotificationShownForState(
      mojom::ConnectionState::DISCONNECTING));
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCustomServersURLs);
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::DISCONNECTED));
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCustomServersURLs);
}

TEST_F(BraveVpnDnsObserverServiceUnitTest,
       DoNotOverrideDoHConfigWithParentalControlEnabled) {
  SetDNSMode(SecureDnsConfig::kModeSecure, "");
  EnableParentalControl(true);
  EXPECT_TRUE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::CONNECTED));
  ExpectDNSMode(SecureDnsConfig::kModeOff, "");
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::DISCONNECTED));
  ExpectDNSMode(SecureDnsConfig::kModeOff, "");

  EnableParentalControl(false);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, "");
}

TEST_F(BraveVpnDnsObserverServiceUnitTest, DoNotShowPolicyDialogIfUserSkipped) {
  // Do not show dialog option enabled
  SetManagedMode(SecureDnsConfig::kModeOff);
  pref_service()->SetBoolean(prefs::kBraveVpnShowDNSPolicyWarningDialog, false);
  SetDNSMode(SecureDnsConfig::kModeOff, "");
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::CONNECTED));
}

TEST_F(BraveVpnDnsObserverServiceUnitTest, DnsOverridenOnLaunchIfVPNEnabled) {
  CheckUserNotifiedAndDnsOverridenOnLaunch(SecureDnsConfig::kModeOff,
                                           std::string(),
                                           kCloudflareDnsProviderURL, true);
  CheckUserNotifiedAndDnsOverridenOnLaunch(SecureDnsConfig::kModeAutomatic,
                                           std::string(),
                                           kCloudflareDnsProviderURL, true);
  CheckUserNotifiedAndDnsOverridenOnLaunch(SecureDnsConfig::kModeSecure, "", "",
                                           false);
}

TEST_F(BraveVpnDnsObserverServiceUnitTest,
       VPNConnectedOnBrowserShutDownAndDisconnectedOnStart) {
  ResetDnsObserverService();
  // DNS mode was set to off by user.
  SetDNSMode(SecureDnsConfig::kModeOff, "");
  // Set vpn config to indicate vpn was enabled when browser closed.
  local_state()->SetString(::prefs::kBraveVpnDnsConfig,
                           kCloudflareDnsProviderURL);
  CreateDnsObserverService();
  // Before VPN service initialization we have vpn overriden.
  ExpectDNSMode(SecureDnsConfig::kModeOff, "");
  EXPECT_TRUE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
  // On launch vpn service notifies it is disconnected.
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
  // Do not override anymore as vpn is disconnected.
  ExpectDNSMode(SecureDnsConfig::kModeOff, "");
  EXPECT_TRUE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
}

TEST_F(BraveVpnDnsObserverServiceUnitTest, FeatureDisabledWhenVPNConnected) {
  // DNS mode was set to off by user and the vpn dns observer feature
  // is enabled by default.
  SetDNSMode(SecureDnsConfig::kModeOff, "");
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      brave_vpn::features::kBraveVPNDnsProtection);
  // Do not override anymore because the feature is disabled.
  ExpectDNSMode(SecureDnsConfig::kModeOff, "");
  EXPECT_FALSE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
}

TEST_F(BraveVpnDnsObserverServiceUnitTest, HelperServerLiveWhenVPNConnected) {
  SetDNSHelperLive(true);
  // DNS mode was set to off by user and the vpn dns observer feature
  // is enabled by default.
  SetDNSMode(SecureDnsConfig::kModeOff, "");
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
  // DNS mode was not overriden.
  ExpectDNSMode(SecureDnsConfig::kModeOff, "");
}

}  // namespace brave_vpn
