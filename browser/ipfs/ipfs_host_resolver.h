/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IPFS_IPFS_HOST_RESOLVER_H_
#define BRAVE_BROWSER_IPFS_IPFS_HOST_RESOLVER_H_

#include <string>
#include <utility>
#include <vector>

#include "base/callback_forward.h"
#include "net/base/host_port_pair.h"
#include "net/base/network_isolation_key.h"
#include "net/dns/public/dns_query_type.h"
#include "services/network/public/cpp/resolve_host_client_base.h"
#include "services/network/public/mojom/host_resolver.mojom.h"
#include "services/network/public/mojom/network_context.mojom.h"

namespace ipfs {

// Resolves DNS TXT record for hosts. If prefix passed then
// automatically adds it to the host.
class IPFSHostResolver : public network::ResolveHostClientBase {
 public:
  explicit IPFSHostResolver(network::mojom::NetworkContext* network_context,
                            const std::string& prefix = std::string());
  ~IPFSHostResolver() override;

  using HostTextResultsCallback =
      base::OnceCallback<void(const std::string& host)>;

  virtual void Resolve(const net::HostPortPair& host,
                       const net::NetworkIsolationKey& isolation_key,
                       net::DnsQueryType dns_query_type,
                       HostTextResultsCallback callback);

  std::string host() const { return resolving_host_; }

  void SetCompleteCallbackForTesting(base::OnceClosure complete_callback) {
    complete_callback_for_testing_ = std::move(complete_callback);
  }

 private:
  // network::mojom::ResolveHostClient implementation:
  void OnComplete(
      int result,
      const net::ResolveErrorInfo& resolve_error_info,
      const base::Optional<net::AddressList>& resolved_addresses) override;
  void OnTextResults(const std::vector<std::string>& text_results) override;

  std::string resolving_host_;
  std::string prefix_;
  bool has_dnslink_ = false;
  network::mojom::NetworkContext* network_context_ = nullptr;
  HostTextResultsCallback resolved_callback_;
  base::OnceClosure complete_callback_for_testing_;

  mojo::Receiver<network::mojom::ResolveHostClient> receiver_{this};
};

}  // namespace ipfs

#endif  // BRAVE_BROWSER_IPFS_IPFS_HOST_RESOLVER_H_
