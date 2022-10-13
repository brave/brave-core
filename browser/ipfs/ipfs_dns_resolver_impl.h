/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IPFS_IPFS_DNS_RESOLVER_IMPL_H_
#define BRAVE_BROWSER_IPFS_IPFS_DNS_RESOLVER_IMPL_H_

#include "brave/components/ipfs/ipfs_dns_resolver.h"

#include <string>

#include "base/sequence_checker.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/mojom/host_resolver.mojom.h"
#include "services/network/public/mojom/network_service.mojom.h"

namespace ipfs {

using base::SequenceChecker;

class IpfsDnsResolverImpl
    : public IpfsDnsResolver,
      public network::mojom::DnsConfigChangeManagerClient {
 public:
  IpfsDnsResolverImpl();
  ~IpfsDnsResolverImpl() override;
  IpfsDnsResolverImpl(const IpfsDnsResolverImpl&) = delete;
  IpfsDnsResolverImpl& operator=(const IpfsDnsResolverImpl&) = delete;

  void OnDnsConfigChanged() override;
  void OnDnsConfigChangeManagerConnectionError();

  absl::optional<std::string> GetFirstDnsOverHttpsServer() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(IpfsDnsResolverImplUnitTest, ReconnectOnMojoError);
  void SetupDnsConfigChangeNotifications();

  mojo::Remote<network::mojom::DnsConfigChangeManager>
      dns_config_change_manager_;
  mojo::Receiver<network::mojom::DnsConfigChangeManagerClient> receiver_{this};

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<IpfsDnsResolverImpl> weak_ptr_factory_;
};

}  // namespace ipfs

#endif  // BRAVE_BROWSER_IPFS_IPFS_DNS_RESOLVER_IMPL_H_
