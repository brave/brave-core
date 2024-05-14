/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/containers/fixed_flat_map.h"
#include "base/feature_list.h"
#include "brave/net/dns/secure_dns_endpoints.h"
#include "net/base/features.h"
#include "net/dns/dns_util.h"
#include "net/dns/public/dns_over_https_server_config.h"

#include <vector>

namespace net {
namespace {

constexpr auto kDohFallbackEndpointAddresses{
    base::MakeFixedFlatMap<DohFallbackEndpointType, const char*>({
        {DohFallbackEndpointType::kQuad9,
         "https://doh-brave.quad9.net/dns-query"},
        {DohFallbackEndpointType::kWikimedia,
         "https://wikimedia-dns.org/dns-query"},
        {DohFallbackEndpointType::kCloudflare,
         "https://cloudflare-dns.com/dns-query"},
    })};

std::vector<DnsOverHttpsServerConfig> MaybeAddFallbackDohServer(
    std::vector<DnsOverHttpsServerConfig> doh_servers) {
  if (!base::FeatureList::IsEnabled(net::features::kBraveFallbackDoHProvider)) {
    return doh_servers;
  }
  static const DohFallbackEndpointType endpoint =
      net::features::kBraveFallbackDoHProviderEndpoint.Get();
  if (endpoint == DohFallbackEndpointType::kNone) {
    return doh_servers;
  }
  const char* endpointAddress = kDohFallbackEndpointAddresses.at(endpoint);
  auto fallbackDohServer =
      DnsOverHttpsServerConfig::FromString(endpointAddress);
  if (fallbackDohServer.has_value()) {
    doh_servers.push_back(fallbackDohServer.value());
  }
  return doh_servers;
}

}  // namespace
}  // namespace net

#define GetDohUpgradeServersFromNameservers(...) \
  MaybeAddFallbackDohServer(GetDohUpgradeServersFromNameservers(__VA_ARGS__))

#include "src/net/dns/dns_client.cc"

#undef GetDohUpgradeServersFromNameservers
