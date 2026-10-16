// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>
#include <string_view>
#include <utility>

#include "base/strings/strcat.h"
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/components/tor/pref_names.h"
#include "brave/components/tor/tor_navigation_throttle.h"
#include "brave/net/proxy_resolution/proxy_config_service_tor.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/test/browser_test.h"
#include "net/base/host_port_pair.h"
#include "net/base/net_errors.h"
#include "net/base/network_anonymization_key.h"
#include "net/dns/mock_host_resolver.h"
#include "services/network/public/mojom/host_resolver.mojom.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/test/test_dns_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kOnionHost[] = "example.onion";
constexpr char kNonOnionHost[] = "example.com";

struct OnionResolutionScenario {
  bool tor_window = false;
  bool onion_only_in_tor_windows = true;
  bool expect_blocked = true;
};

std::string OnionResolutionScenarioName(
    const testing::TestParamInfo<OnionResolutionScenario>& info) {
  const OnionResolutionScenario& scenario = info.param;
  return base::StrCat(
      {scenario.tor_window ? "TorWindow" : "NormalWindow",
       scenario.onion_only_in_tor_windows ? "_PrefOn" : "_PrefOff",
       scenario.expect_blocked ? "_Blocked" : "_Allowed"});
}

// Skips the host cache, so the result reflects whether the lookup would reach
// a DNS resolver.
int Resolve(network::mojom::NetworkContext* network_context,
            std::string_view host) {
  auto params = network::mojom::ResolveHostParameters::New();
  params->cache_usage =
      network::mojom::ResolveHostParameters::CacheUsage::DISALLOWED;
  return network::BlockingDnsLookup(
             network_context, net::HostPortPair(host, 80), std::move(params),
             net::NetworkAnonymizationKey())
      .error;
}

network::mojom::NetworkContext* GetDefaultNetworkContext(Profile* profile) {
  return profile->GetDefaultStoragePartition()->GetNetworkContext();
}

}  // namespace

class OnionHostnameResolutionBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    net::ProxyConfigServiceTor::SetBypassTorProxyConfigForTesting(true);
    tor::TorNavigationThrottle::SetSkipWaitForTorConnectedForTesting(true);
  }

  void TearDownOnMainThread() override {
    tor::TorNavigationThrottle::SetSkipWaitForTorConnectedForTesting(false);
    net::ProxyConfigServiceTor::SetBypassTorProxyConfigForTesting(false);
    InProcessBrowserTest::TearDownOnMainThread();
  }

  void SetOnionOnlyInTorWindows(bool enabled) {
    browser()->GetProfile()->GetPrefs()->SetBoolean(
        tor::prefs::kOnionOnlyInTorWindows, enabled);
  }
};

IN_PROC_BROWSER_TEST_F(OnionHostnameResolutionBrowserTest,
                       FollowsPrefChangesAtRuntime) {
  network::mojom::NetworkContext* network_context =
      GetDefaultNetworkContext(browser()->GetProfile());

  EXPECT_EQ(net::ERR_NAME_NOT_RESOLVED, Resolve(network_context, kOnionHost));
  EXPECT_EQ(net::OK, Resolve(network_context, kNonOnionHost));

  SetOnionOnlyInTorWindows(false);
  EXPECT_EQ(net::OK, Resolve(network_context, kOnionHost));

  SetOnionOnlyInTorWindows(true);
  EXPECT_EQ(net::ERR_NAME_NOT_RESOLVED, Resolve(network_context, kOnionHost));
}

IN_PROC_BROWSER_TEST_F(OnionHostnameResolutionBrowserTest,
                       IncognitoFollowsPref) {
  Profile* incognito_profile = CreateIncognitoBrowser()->GetProfile();
  network::mojom::NetworkContext* network_context =
      GetDefaultNetworkContext(incognito_profile);

  EXPECT_EQ(net::ERR_NAME_NOT_RESOLVED, Resolve(network_context, kOnionHost));

  SetOnionOnlyInTorWindows(false);
  EXPECT_EQ(net::OK, Resolve(network_context, kOnionHost));
}

IN_PROC_BROWSER_TEST_F(OnionHostnameResolutionBrowserTest,
                       NonDefaultStoragePartitionFollowsPref) {
  Profile* profile = browser()->GetProfile();
  SetOnionOnlyInTorWindows(false);

  network::mojom::NetworkContext* network_context =
      profile
          ->GetStoragePartition(content::StoragePartitionConfig::Create(
              profile, "onion", "test", /*in_memory=*/true))
          ->GetNetworkContext();
  EXPECT_EQ(net::OK, Resolve(network_context, kOnionHost));

  SetOnionOnlyInTorWindows(true);
  EXPECT_EQ(net::ERR_NAME_NOT_RESOLVED, Resolve(network_context, kOnionHost));
}

IN_PROC_BROWSER_TEST_F(OnionHostnameResolutionBrowserTest,
                       SystemNetworkContextAlwaysBlocks) {
  SetOnionOnlyInTorWindows(false);

  network::mojom::NetworkContext* network_context =
      g_browser_process->system_network_context_manager()->GetContext();
  EXPECT_EQ(net::ERR_NAME_NOT_RESOLVED, Resolve(network_context, kOnionHost));
  EXPECT_EQ(net::OK, Resolve(network_context, kNonOnionHost));
}

class OnionHostnameResolutionScenarioBrowserTest
    : public OnionHostnameResolutionBrowserTest,
      public testing::WithParamInterface<OnionResolutionScenario> {};

IN_PROC_BROWSER_TEST_P(OnionHostnameResolutionScenarioBrowserTest,
                       ResolveOnion) {
  const OnionResolutionScenario& scenario = GetParam();
  SetOnionOnlyInTorWindows(scenario.onion_only_in_tor_windows);

  Profile* profile =
      scenario.tor_window
          ? TorProfileManager::SwitchToTorProfile(browser()->GetProfile())
                ->GetProfile()
          : browser()->GetProfile();
  network::mojom::NetworkContext* network_context =
      GetDefaultNetworkContext(profile);

  EXPECT_EQ(scenario.expect_blocked ? net::ERR_NAME_NOT_RESOLVED : net::OK,
            Resolve(network_context, kOnionHost));
  EXPECT_EQ(net::OK, Resolve(network_context, kNonOnionHost));
}

INSTANTIATE_TEST_SUITE_P(
    ,
    OnionHostnameResolutionScenarioBrowserTest,
    testing::Values(OnionResolutionScenario{.tor_window = false,
                                            .onion_only_in_tor_windows = true,
                                            .expect_blocked = true},
                    OnionResolutionScenario{.tor_window = false,
                                            .onion_only_in_tor_windows = false,
                                            .expect_blocked = false},
                    OnionResolutionScenario{.tor_window = true,
                                            .onion_only_in_tor_windows = true,
                                            .expect_blocked = false},
                    OnionResolutionScenario{.tor_window = true,
                                            .onion_only_in_tor_windows = false,
                                            .expect_blocked = false}),
    OnionResolutionScenarioName);
