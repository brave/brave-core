// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string_view>
#include <utility>

#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/components/tor/tor_navigation_throttle.h"
#include "brave/net/proxy_resolution/proxy_config_service_tor.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/test/base/in_process_browser_test.h"
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
};

IN_PROC_BROWSER_TEST_F(OnionHostnameResolutionBrowserTest, NormalWindowBlocks) {
  network::mojom::NetworkContext* network_context =
      GetDefaultNetworkContext(browser()->GetProfile());

  EXPECT_EQ(net::ERR_NAME_NOT_RESOLVED, Resolve(network_context, kOnionHost));
  EXPECT_EQ(net::OK, Resolve(network_context, kNonOnionHost));
}

IN_PROC_BROWSER_TEST_F(OnionHostnameResolutionBrowserTest, IncognitoBlocks) {
  network::mojom::NetworkContext* network_context =
      GetDefaultNetworkContext(CreateIncognitoBrowser()->GetProfile());

  EXPECT_EQ(net::ERR_NAME_NOT_RESOLVED, Resolve(network_context, kOnionHost));
  EXPECT_EQ(net::OK, Resolve(network_context, kNonOnionHost));
}

IN_PROC_BROWSER_TEST_F(OnionHostnameResolutionBrowserTest,
                       NonDefaultStoragePartitionBlocks) {
  Profile* profile = browser()->GetProfile();
  network::mojom::NetworkContext* network_context =
      profile
          ->GetStoragePartition(content::StoragePartitionConfig::Create(
              profile, "onion", "test", /*in_memory=*/true))
          ->GetNetworkContext();

  EXPECT_EQ(net::ERR_NAME_NOT_RESOLVED, Resolve(network_context, kOnionHost));
  EXPECT_EQ(net::OK, Resolve(network_context, kNonOnionHost));
}

IN_PROC_BROWSER_TEST_F(OnionHostnameResolutionBrowserTest,
                       SystemNetworkContextBlocks) {
  network::mojom::NetworkContext* network_context =
      g_browser_process->system_network_context_manager()->GetContext();

  EXPECT_EQ(net::ERR_NAME_NOT_RESOLVED, Resolve(network_context, kOnionHost));
  EXPECT_EQ(net::OK, Resolve(network_context, kNonOnionHost));
}

IN_PROC_BROWSER_TEST_F(OnionHostnameResolutionBrowserTest, TorWindowResolves) {
  Profile* tor_profile =
      TorProfileManager::SwitchToTorProfile(browser()->GetProfile())
          ->GetProfile();
  network::mojom::NetworkContext* network_context =
      GetDefaultNetworkContext(tor_profile);

  EXPECT_EQ(net::OK, Resolve(network_context, kOnionHost));
  EXPECT_EQ(net::OK, Resolve(network_context, kNonOnionHost));
}
