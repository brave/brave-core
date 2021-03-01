/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "brave/browser/ipfs/ipfs_host_resolver.h"
#include "chrome/browser/net/secure_dns_config.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "net/base/host_port_pair.h"
#include "net/dns/public/dns_protocol.h"

namespace {

// DNSLink values are of the form: dnslink=<value>
// https://dnslink.io/#dnslink-format
const char kDnsLinkHeader[] = "dnslink";

// Expects dns TXT record in format: name=value
std::string GetDNSRecordValue(const std::vector<std::string>& text_results,
                              const std::string& name) {
  for (const auto& txt : text_results) {
    std::vector<std::string> tokens = base::SplitString(
        txt, "=", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
    if (!tokens.size())
      continue;
    if (tokens.front() != name)
      continue;
    return tokens.back();
  }
  return std::string();
}

}  // namespace

namespace ipfs {

IPFSHostResolver::IPFSHostResolver(
    network::mojom::NetworkContext* network_context,
    const std::string& prefix)
    : prefix_(prefix), network_context_(network_context) {
  DCHECK(network_context);
}
IPFSHostResolver::~IPFSHostResolver() {}

void IPFSHostResolver::Resolve(const net::HostPortPair& host,
                               const net::NetworkIsolationKey& isolation_key,
                               net::DnsQueryType dns_query_type,
                               HostTextResultsCallback callback) {
  if (!callback)
    return;

  if (host.host() == resolving_host_) {
    if (callback && has_dnslink_) {
      std::move(callback).Run(host.host());
    }
    return;
  }

  network::mojom::ResolveHostParametersPtr parameters =
      network::mojom::ResolveHostParameters::New();
  parameters->dns_query_type = dns_query_type;

  receiver_.reset();
  resolved_callback_ = std::move(callback);
  has_dnslink_ = false;
  resolving_host_ = host.host();
  net::HostPortPair local_host_port(prefix_ + resolving_host_, host.port());

  network_context_->ResolveHost(local_host_port, isolation_key,
                                std::move(parameters),
                                receiver_.BindNewPipeAndPassRemote());
}

void IPFSHostResolver::OnComplete(
    int result,
    const net::ResolveErrorInfo& error_info,
    const base::Optional<net::AddressList>& list) {
  if (result != net::OK) {
    VLOG(1) << "DNS resolving error:" << net::ErrorToString(result)
            << " for host: " << prefix_ + resolving_host_;
  }
  if (complete_callback_for_testing_)
    std::move(complete_callback_for_testing_).Run();
}

void IPFSHostResolver::OnTextResults(const std::vector<std::string>& results) {
  VLOG(2) << results.size()
          << " TXT records resolved for host: " << prefix_ + resolving_host_;
  std::string dnslink = GetDNSRecordValue(results, kDnsLinkHeader);
  has_dnslink_ = !dnslink.empty();
  // We intentionally ignore the value since only its presence is important
  // to us. https://docs.ipfs.io/concepts/dnslink/#publish-using-a-subdomain
  if (!has_dnslink_)
    return;

  if (resolved_callback_)
    std::move(resolved_callback_).Run(resolving_host_);
}

}  // namespace ipfs
