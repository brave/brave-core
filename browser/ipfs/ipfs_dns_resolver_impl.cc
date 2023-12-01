/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_dns_resolver_impl.h"

#include <optional>

#include "base/time/time.h"
#include "chrome/browser/net/secure_dns_config.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "content/public/browser/network_service_instance.h"
#include "services/network/public/mojom/network_service.mojom.h"

namespace ipfs {

constexpr base::TimeDelta kRetryDelay = base::Seconds(1);

mojo::Remote<network::mojom::DnsConfigChangeManager>
GetDnsConfigChangeManager() {
  mojo::Remote<network::mojom::DnsConfigChangeManager>
      dns_config_change_manager_remote;
  content::GetNetworkService()->GetDnsConfigChangeManager(
      dns_config_change_manager_remote.BindNewPipeAndPassReceiver());
  return dns_config_change_manager_remote;
}

IpfsDnsResolverImpl::IpfsDnsResolverImpl() : weak_ptr_factory_(this) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  dns_config_change_manager_ = GetDnsConfigChangeManager();
  SetupDnsConfigChangeNotifications();
}

void IpfsDnsResolverImpl::SetupDnsConfigChangeNotifications() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  dns_config_change_manager_->RequestNotifications(
      receiver_.BindNewPipeAndPassRemote());
  receiver_.set_disconnect_handler(base::BindOnce(
      &IpfsDnsResolverImpl::OnDnsConfigChangeManagerConnectionError,
      base::Unretained(this)));
}

IpfsDnsResolverImpl::~IpfsDnsResolverImpl() = default;

void IpfsDnsResolverImpl::OnDnsConfigChangeManagerConnectionError() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  receiver_.reset();
  // Throttle network service reconnect to prevent possible battery drain.
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&IpfsDnsResolverImpl::SetupDnsConfigChangeNotifications,
                     weak_ptr_factory_.GetWeakPtr()),
      kRetryDelay);
}

void IpfsDnsResolverImpl::OnDnsConfigChanged() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  Notify(GetFirstDnsOverHttpsServer());
}

std::optional<std::string> IpfsDnsResolverImpl::GetFirstDnsOverHttpsServer() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  SecureDnsConfig secure_dns_config =
      SystemNetworkContextManager::GetStubResolverConfigReader()
          ->GetSecureDnsConfiguration(
              false /* force_check_parental_controls_for_automatic_mode */);
  const auto servers = secure_dns_config.doh_servers().servers();

  if (secure_dns_config.mode() == net::SecureDnsMode::kOff || servers.empty()) {
    return std::nullopt;
  }
  std::string server_template = servers[0].server_template();
  if (server_template.empty()) {
    return std::nullopt;
  }
  return server_template;
}

}  // namespace ipfs
