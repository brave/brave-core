// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_transfer/mdns_discovery_provider.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "chrome/browser/local_discovery/service_discovery_shared_client.h"

namespace brave_transfer {

base::flat_map<std::string, std::string> ParseTxtRecord(
    const std::vector<std::string>& metadata) {
  base::flat_map<std::string, std::string> record;
  for (const std::string& entry : metadata) {
    const size_t separator = entry.find('=');
    if (separator == std::string::npos || separator == 0) {
      continue;
    }
    record.emplace(entry.substr(0, separator), entry.substr(separator + 1));
  }
  return record;
}

std::optional<PeerCandidate> CandidateFromService(
    const local_discovery::ServiceDescription& service,
    const ChainKeys& keys,
    base::Time now) {
  std::optional<Tag> tag =
      keys.MatchTxtRecord(ParseTxtRecord(service.metadata), now);
  if (!tag) {
    return std::nullopt;
  }
  PeerCandidate candidate;
  candidate.provider = "mdns";
  candidate.instance_id = service.service_name;
  candidate.tag = *tag;
  candidate.seen_at = service.last_seen.is_null() ? now : service.last_seen;
  const uint16_t port = service.address.port();
  if (port == 0) {
    return std::nullopt;
  }
  if (service.ip_address.IsValid()) {
    candidate.endpoints.push_back({service.ip_address.ToString(), port});
  }
  if (!service.address.host().empty() &&
      (candidate.endpoints.empty() ||
       candidate.endpoints[0].host != service.address.host())) {
    candidate.endpoints.push_back({service.address.host(), port});
  }
  return candidate;
}

MdnsDiscoveryProvider::MdnsDiscoveryProvider() = default;

MdnsDiscoveryProvider::~MdnsDiscoveryProvider() = default;

void MdnsDiscoveryProvider::Start(const ChainKeys& keys,
                                  CandidateCallback on_candidate,
                                  LostCallback on_lost) {
  keys_ = keys;
  on_candidate_ = std::move(on_candidate);
  on_lost_ = std::move(on_lost);
  if (lister_) {
    Refresh();
    return;
  }
  client_ = local_discovery::ServiceDiscoverySharedClient::GetInstance();
  lister_ = local_discovery::ServiceDiscoveryDeviceLister::Create(
      this, client_.get(), kBraveTransferServiceType);
  lister_->Start();
  lister_->DiscoverNewDevices();
  refresh_timer_.Start(FROM_HERE, kRefreshInterval,
                       base::BindRepeating(&MdnsDiscoveryProvider::Refresh,
                                           base::Unretained(this)));
}

void MdnsDiscoveryProvider::Stop() {
  refresh_timer_.Stop();
  lister_.reset();
  client_.reset();
  services_.clear();
  reported_.clear();
  keys_.reset();
  on_candidate_.Reset();
  on_lost_.Reset();
}

bool MdnsDiscoveryProvider::IsStarted() const {
  return !!lister_;
}

void MdnsDiscoveryProvider::OnDeviceChanged(
    const std::string& service_type,
    bool added,
    const local_discovery::ServiceDescription& service_description) {
  services_.insert_or_assign(service_description.service_name,
                             service_description);
  Evaluate(service_description);
}

void MdnsDiscoveryProvider::OnDeviceRemoved(const std::string& service_type,
                                            const std::string& service_name) {
  services_.erase(service_name);
  ReportLost(service_name);
}

void MdnsDiscoveryProvider::OnDeviceCacheFlushed(
    const std::string& service_type) {
  const base::flat_set<std::string> reported = std::move(reported_);
  reported_.clear();
  services_.clear();
  for (const std::string& name : reported) {
    if (on_lost_) {
      on_lost_.Run(name);
    }
  }
  if (lister_) {
    lister_->DiscoverNewDevices();
  }
}

void MdnsDiscoveryProvider::OnPermissionRejected() {
  LOG(WARNING) << "Brave Transfer: local network access was denied";
}

void MdnsDiscoveryProvider::Evaluate(
    const local_discovery::ServiceDescription& service) {
  if (!keys_ || !on_candidate_) {
    return;
  }
  std::optional<PeerCandidate> candidate =
      CandidateFromService(service, *keys_, base::Time::Now());
  if (!candidate) {
    ReportLost(service.service_name);
    return;
  }
  reported_.insert(service.service_name);
  on_candidate_.Run(*candidate);
}

void MdnsDiscoveryProvider::ReportLost(const std::string& service_name) {
  if (reported_.erase(service_name) && on_lost_) {
    on_lost_.Run(service_name);
  }
}

void MdnsDiscoveryProvider::Refresh() {
  // Callbacks may stop discovery, so iterate over a copy.
  std::vector<local_discovery::ServiceDescription> services;
  for (const auto& [name, service] : services_) {
    services.push_back(service);
  }
  for (const auto& service : services) {
    if (!lister_) {
      return;
    }
    Evaluate(service);
  }
  if (lister_) {
    lister_->DiscoverNewDevices();
  }
}

}  // namespace brave_transfer
