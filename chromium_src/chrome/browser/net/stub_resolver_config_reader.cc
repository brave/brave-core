/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/unstoppable_domains/buildflags/buildflags.h"

#if BUILDFLAG(UNSTOPPABLE_DOMAINS_ENABLED)
#include "brave/components/unstoppable_domains/utils.h"
#include "brave/net/unstoppable_domains/constants.h"
#include "chrome/browser/net/secure_dns_config.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/network_service_instance.h"
#include "services/network/public/mojom/network_service.mojom.h"

namespace {

// Add DoH servers to support Unstoppable Domains (and ENS in the future PR).
// These servers are controlled using its own prefs and not
// kDnsOverHttpsTemplates pref as the servers we added here are special and
// only applies to certain TLD which is different from user's global DoH
// provider settings.
SecureDnsConfig BraveSecureDnsConfig(
    net::SecureDnsMode mode,
    std::vector<net::DnsOverHttpsServerConfig> servers,
    SecureDnsConfig::ManagementMode management_mode,
    bool update_network_service,
    PrefService* local_state,
    bool force_check_parental_controls_for_automatic_mode) {
  // force_check_parental_controls_for_automatic_mode is only true for
  // settings UI which we specifically do not want to display those special
  // resolvers we added, so we will early return with the original config if it
  // is true.
  if (mode == net::SecureDnsMode::kOff ||
      !unstoppable_domains::IsResolveMethodDoH(local_state) ||
      force_check_parental_controls_for_automatic_mode) {
    return SecureDnsConfig(mode, std::move(servers), management_mode);
  }

  std::vector<net::DnsOverHttpsServerConfig> new_servers;
  new_servers.emplace_back(std::string(unstoppable_domains::kDoHResolver),
                           true /* use_post */);
  new_servers.insert(new_servers.end(), servers.begin(), servers.end());

  // Update network service again with added DoH servers.
  if (update_network_service) {
    base::Optional<std::vector<network::mojom::DnsOverHttpsServerPtr>>
        servers_mojo = base::make_optional<
            std::vector<network::mojom::DnsOverHttpsServerPtr>>();
    for (auto server : new_servers) {
      if (!servers_mojo.has_value()) {
        servers_mojo = base::make_optional<
            std::vector<network::mojom::DnsOverHttpsServerPtr>>();
      }

      network::mojom::DnsOverHttpsServerPtr server_mojo =
          network::mojom::DnsOverHttpsServer::New();
      server_mojo->server_template = server.server_template;
      server_mojo->use_post = server.use_post;
      servers_mojo->emplace_back(std::move(server_mojo));
    }

    content::GetNetworkService()->ConfigureStubHostResolver(
        SystemNetworkContextManager::GetStubResolverConfigReader()
            ->GetInsecureStubResolverEnabled(),
        mode, std::move(servers_mojo));
  }

  return SecureDnsConfig(mode, std::move(new_servers), management_mode);
}

}  // namespace

#define SecureDnsConfig(SecureDnsMode, DnsOverHttpsServers,          \
                        ForcedManagementMode)                        \
  BraveSecureDnsConfig(SecureDnsMode, DnsOverHttpsServers,           \
                       ForcedManagementMode, update_network_service, \
                       local_state_,                                 \
                       force_check_parental_controls_for_automatic_mode)

#endif  // BUILDFLAG(UNSTOPPABLE_DOMAINS_ENABLED)

#include "../../../../../chrome/browser/net/stub_resolver_config_reader.cc"
