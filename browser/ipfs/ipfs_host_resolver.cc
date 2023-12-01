/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_host_resolver.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "chrome/browser/net/secure_dns_config.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "net/base/host_port_pair.h"
#include "net/dns/public/dns_protocol.h"

namespace {

// DNSLink values are of the form: dnslink=<value>
// https://dnslink.io/#dnslink-format
const char kDnsLinkHeader[] = "dnslink";

// Expects dns TXT record in format: name=value
std::optional<std::string> GetDNSRecordValue(
    const std::vector<std::string>& text_results,
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
  return std::nullopt;
}

}  // namespace

namespace ipfs {

IPFSHostResolver::IPFSHostResolver(content::BrowserContext* browser_context,
                                   const std::string& prefix)
    : prefix_(prefix), browser_context_(browser_context) {}
IPFSHostResolver::~IPFSHostResolver() = default;

void IPFSHostResolver::Resolve(
    const net::HostPortPair& host,
    const net::NetworkAnonymizationKey& anonymization_key,
    net::DnsQueryType dns_query_type,
    HostTextResultsCallback callback) {
  if (!callback)
    return;

  if (host.host() == resolving_host_) {
    if (callback) {
      std::move(callback).Run(host.host(), dnslink_);
    }
    return;
  }

  network::mojom::ResolveHostParametersPtr parameters =
      network::mojom::ResolveHostParameters::New();
  parameters->dns_query_type = dns_query_type;

  receiver_.reset();
  resolved_callback_ = std::move(callback);
  dnslink_ = std::nullopt;
  resolving_host_ = host.host();
  net::HostPortPair local_host_port(prefix_ + resolving_host_, host.port());
  auto* network_context = GetNetworkContext();
  if (!network_context) {
    return;
  }
  network_context->ResolveHost(
      network::mojom::HostResolverHost::NewHostPortPair(local_host_port),
      anonymization_key, std::move(parameters),
      receiver_.BindNewPipeAndPassRemote());
}

network::mojom::NetworkContext* IPFSHostResolver::GetNetworkContext() {
  if (network_context_for_testing_.has_value()) {
    return network_context_for_testing_.value();
  }
  auto* storage_partition = browser_context_->GetDefaultStoragePartition();
  if (!storage_partition) {
    return nullptr;
  }
  return storage_partition->GetNetworkContext();
}

void IPFSHostResolver::OnComplete(
    int result,
    const net::ResolveErrorInfo& error_info,
    const std::optional<net::AddressList>& list,
    const std::optional<net::HostResolverEndpointResults>&
        endpoint_results_with_metadata) {
  if (result != net::OK) {
    VLOG(1) << "DNS resolving error:" << net::ErrorToString(result)
            << " for host: " << prefix_ + resolving_host_;
    if (resolved_callback_) {
      std::move(resolved_callback_).Run(resolving_host_, std::nullopt);
    }
  }
}

void IPFSHostResolver::OnTextResults(const std::vector<std::string>& results) {
  VLOG(2) << results.size()
          << " TXT records resolved for host: " << prefix_ + resolving_host_;
  dnslink_ = GetDNSRecordValue(results, kDnsLinkHeader);

  if (resolved_callback_)
    std::move(resolved_callback_).Run(resolving_host_, dnslink_);
}

}  // namespace ipfs
