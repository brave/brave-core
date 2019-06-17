/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/net/proxy_resolution/proxy_config_service_tor.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "crypto/random.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/proxy_resolution/proxy_config_with_annotation.h"
#include "url/origin.h"

namespace net {

const int kTorPasswordLength = 16;
// Default tor circuit life time is 10 minutes
constexpr base::TimeDelta kTenMins = base::TimeDelta::FromMinutes(10);

ProxyConfigServiceTor::ProxyConfigServiceTor(const std::string& proxy_uri) {
  ProxyServer proxy_server =
      ProxyServer::FromURI(proxy_uri, ProxyServer::SCHEME_SOCKS5);
  DCHECK(proxy_server.is_valid());
  proxy_server_ = proxy_server;
}

ProxyConfigServiceTor::~ProxyConfigServiceTor() {}

void ProxyConfigServiceTor::SetUsername(const std::string& username,
                                        TorProxyMap* map) {
  if (map && !username.empty()) {
    std::string password = map->Get(username);
    const HostPortPair old_host_port = proxy_server_.host_port_pair();
    const HostPortPair new_host_port(username, password, old_host_port.host(),
                                     old_host_port.port());
    const ProxyServer proxy_server(ProxyServer::SCHEME_SOCKS5, new_host_port);
    proxy_server_ = proxy_server;
  }
}

// static
std::string ProxyConfigServiceTor::CircuitIsolationKey(const GURL& url) {
  // https://2019.www.torproject.org/projects/torbrowser/design/#privacy
  //
  //    For the purposes of the unlinkability requirements of this
  //    section as well as the descriptions in the implementation
  //    section, a URL bar origin means at least the second-level DNS
  //    name.  For example, for mail.google.com, the origin would be
  //    google.com.  Implementations MAY, at their option, restrict
  //    the URL bar origin to be the entire fully qualified domain
  //    name.
  //
  // In particular, we need not isolate by the scheme,
  // username/password, port, path, or query part of the URL.
  url::Origin origin = url::Origin::Create(url);
  std::string domain = net::registry_controlled_domains::GetDomainAndRegistry(
      origin.host(),
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  if (domain.size() == 0)
    domain = origin.host();
  return domain;
}

ProxyConfigServiceTor::ConfigAvailability
ProxyConfigServiceTor::GetLatestProxyConfig(
    net::ProxyConfigWithAnnotation* config) {
  if (!proxy_server_.is_valid())
    return CONFIG_UNSET;
  ProxyConfig proxy_config;
  proxy_config.proxy_rules().bypass_rules.AddRulesToSubtractImplicit();
  proxy_config.proxy_rules().ParseFromString(proxy_server_.ToURI());
  *config =
      net::ProxyConfigWithAnnotation(proxy_config, NO_TRAFFIC_ANNOTATION_YET);
  return CONFIG_VALID;
}

ProxyConfigServiceTor::TorProxyMap::TorProxyMap() = default;
ProxyConfigServiceTor::TorProxyMap::~TorProxyMap() {
  timer_.Stop();
}

// static
std::string ProxyConfigServiceTor::TorProxyMap::GenerateNewPassword() {
  std::vector<uint8_t> password(kTorPasswordLength);
  crypto::RandBytes(password.data(), password.size());
  return base::HexEncode(password.data(), password.size());
}

std::string ProxyConfigServiceTor::TorProxyMap::Get(
    const std::string& username) {
  // Clear any expired entries, in case this one has expired.
  ClearExpiredEntries();

  // Check for an entry for this username.
  auto found = map_.find(username);
  if (found != map_.end())
    return found->second.first;

  // No entry yet.  Check our watch and create one.
  const base::Time now = base::Time::Now();
  const std::string password = GenerateNewPassword();
  map_.emplace(username, std::make_pair(password, now));
  queue_.emplace(now, username);

  // Reschedule the timer for ten minutes from now so that this entry
  // won't last more than about ten minutes even if the user stops
  // using Tor for a while.
  timer_.Stop();
  timer_.Start(FROM_HERE, kTenMins, this,
               &ProxyConfigServiceTor::TorProxyMap::ClearExpiredEntries);

  return password;
}

void ProxyConfigServiceTor::TorProxyMap::Erase(const std::string& username) {
  // Just erase it from the map.  There will remain an entry in the
  // queue, but it is harmless.  If anyone creates a new entry in the
  // map, the old entry in the queue will cease to affect it because
  // the timestamps won't match, and they will simultaneously create a
  // new entry in the queue.
  map_.erase(username);
}

void ProxyConfigServiceTor::TorProxyMap::ClearExpiredEntries() {
  const base::Time cutoff = base::Time::Now() - kTenMins;
  for (; !queue_.empty(); queue_.pop()) {
    // Check the timestamp.  If it's not older than the cutoff, stop.
    const std::pair<base::Time, std::string>* entry = &queue_.top();
    const base::Time timestamp = entry->first;
    if (!(timestamp < cutoff))
      break;

    // Remove the corresponding entry in the map if there is one and
    // if its timestamp is not newer.
    const std::string& username = entry->second;
    auto found = map_.find(username);
    if (found != map_.end()) {
      // If the timestamp on the map entry is the same as the
      // timestamp on the queue entry, then delete the map entry.
      // Otherwise, we assume the map entry was created by an explicit
      // request for a new identity, which will have its own entry in
      // the queue in order to last the full ten minutes.
      const base::Time map_timestamp = found->second.second;
      if (map_timestamp == timestamp) {
        map_.erase(username);
      }
    }
  }
}

}  // namespace net
