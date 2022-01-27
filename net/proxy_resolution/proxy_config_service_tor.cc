/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/net/proxy_resolution/proxy_config_service_tor.h"

#include <stdlib.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "crypto/random.h"
#include "net/base/network_isolation_key.h"
#include "net/base/proxy_string_util.h"
#include "net/base/schemeful_site.h"
#include "net/proxy_resolution/proxy_config_with_annotation.h"
#include "net/proxy_resolution/proxy_resolution_service.h"

namespace net {

// Used to cache <username, password> of proxies
class TorProxyMap {
 public:
  TorProxyMap();
  TorProxyMap(const TorProxyMap&) = delete;
  TorProxyMap& operator=(const TorProxyMap&) = delete;
  ~TorProxyMap();
  std::string Get(const std::string& key);
  void Erase(const std::string& key);
  void MaybeExpire(const std::string& key, const base::Time& timestamp);
  size_t size() const;

 private:
  // Generate a new base 64-encoded 128 bit random tag
  static std::string GenerateNewPassword();
  // Clear expired entries in the queue from the map.
  void ClearExpiredEntries();
  std::map<std::string, std::pair<std::string, base::Time>> map_;
  std::priority_queue<std::pair<base::Time, std::string>> queue_;
  base::OneShotTimer timer_;
};

namespace {

constexpr NetworkTrafficAnnotationTag kTorProxyTrafficAnnotation =
    DefineNetworkTrafficAnnotation("proxy_config_tor", R"(
      semantics {
        sender: "Proxy Config Tor"
        description:
          "Establishing a connection through the tor proxy server"
        trigger:
          "Whenever a network request is made from a tor profile."
        data:
          "Proxy configuration."
        destination: OTHER
        destination_other:
          "The proxy server specified in the configuration."
      }
      policy {
        cookies_allowed: NO
        setting: "This feature cannot be disabled by settings."
        policy_exception_justification: "Not implemented."
      })");

static base::NoDestructor<std::map<
    ProxyResolutionService*, TorProxyMap>> tor_proxy_map_;

TorProxyMap* GetTorProxyMap(
    ProxyResolutionService* service) {
  return &(tor_proxy_map_.get()->operator[](service));
}

bool IsTorProxyConfig(const ProxyConfigWithAnnotation& config) {
  auto tag = config.traffic_annotation();
  // iterate through tor_proxy_map_ and remove any entries with
  // empty TorProxyMap (all entries have expired)
  // we do this here because the other methods are only called when
  // IsTorProxy is true so the last entry will never be deleted
  for (auto it = tor_proxy_map_.get()->cbegin();
       it != tor_proxy_map_.get()->cend(); ) {
    if (it->second.size() == 0) {
      tor_proxy_map_->erase(it++);
    } else {
      ++it;
    }
  }

  return tag.unique_id_hash_code ==
         kTorProxyTrafficAnnotation.unique_id_hash_code;
}

}  // namespace

const int kTorPasswordLength = 16;
// Default tor circuit life time is 10 minutes
constexpr base::TimeDelta kTenMins = base::Minutes(10);

ProxyConfigServiceTor::ProxyConfigServiceTor() {}

ProxyConfigServiceTor::ProxyConfigServiceTor(const std::string& proxy_uri) {
  UpdateProxyURI(proxy_uri);
}

ProxyConfigServiceTor::~ProxyConfigServiceTor() {}

void ProxyConfigServiceTor::UpdateProxyURI(const std::string& uri) {
  ProxyServer proxy_server =
      net::ProxyUriToProxyServer(uri, ProxyServer::SCHEME_SOCKS5);
  DCHECK(proxy_server.is_valid());
  proxy_server_ = proxy_server;

  net::ProxyConfigWithAnnotation proxy_config;
  auto config_valid = GetLatestProxyConfig(&proxy_config);

  for (auto& observer : observers_)
    observer.OnProxyConfigChanged(proxy_config,
                                  config_valid);
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
  const net::SchemefulSite url_site(url);
  const net::NetworkIsolationKey network_isolation_key(url_site, url_site);

  const absl::optional<net::SchemefulSite>& schemeful_site =
      network_isolation_key.GetTopFrameSite();
  DCHECK(schemeful_site.has_value());
  std::string host = GURL(schemeful_site->Serialize()).host();
  return host;
}

void ProxyConfigServiceTor::SetNewTorCircuit(const GURL& url) {
  const HostPortPair old_host_port = proxy_server_.host_port_pair();
  const HostPortPair new_host_port(
      CircuitIsolationKey(url),
      std::to_string(
          base::Time::Now().ToDeltaSinceWindowsEpoch().InMicroseconds()),
      old_host_port.host(),
      old_host_port.port());
  proxy_server_ = ProxyServer(ProxyServer::SCHEME_SOCKS5, new_host_port);

  net::ProxyConfigWithAnnotation proxy_config;
  auto config_valid = GetLatestProxyConfig(&proxy_config);

  for (auto& observer : observers_)
    observer.OnProxyConfigChanged(proxy_config,
                                  config_valid);
}

// static
void ProxyConfigServiceTor::SetProxyAuthorization(
    const ProxyConfigWithAnnotation& config,
    const GURL& url,
    ProxyResolutionService* service,
    ProxyInfo* result) {
  if (!IsTorProxyConfig(config))
    return;

  // Adding username & password to global sock://127.0.0.1:[port] config
  // without actually modifying it when resolving proxy for each url.
  const std::string username = CircuitIsolationKey(url);
  const std::string& proxy_uri = net::ProxyServerToProxyUri(
      config.value().proxy_rules().single_proxies.Get());
  HostPortPair host_port_pair =
      config.value().proxy_rules().single_proxies.Get().host_port_pair();

  if (!username.empty()) {
    auto* map = GetTorProxyMap(service);
    if (host_port_pair.username() == username) {
      // password is a int64_t -> std::to_string in milliseconds
      int64_t time = strtoll(host_port_pair.password().c_str(), nullptr, 10);
      map->MaybeExpire(
          host_port_pair.username(),
          base::Time::FromDeltaSinceWindowsEpoch(base::Microseconds(time)));
    }
    host_port_pair.set_username(username);
    host_port_pair.set_password(map->Get(username));

    ProxyConfigServiceTor tor_proxy_config_service(proxy_uri);
    tor_proxy_config_service.proxy_server_ =
        ProxyServer(ProxyServer::SCHEME_SOCKS5, host_port_pair);

    ProxyConfigWithAnnotation fetched_config;
    tor_proxy_config_service.GetLatestProxyConfig(&fetched_config);
    fetched_config.value().proxy_rules().Apply(url, result);
    result->set_traffic_annotation(MutableNetworkTrafficAnnotationTag(
        fetched_config.traffic_annotation()));
  }
}

void ProxyConfigServiceTor::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void ProxyConfigServiceTor::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

ProxyConfigServiceTor::ConfigAvailability
ProxyConfigServiceTor::GetLatestProxyConfig(
    net::ProxyConfigWithAnnotation* config) {
  if (!proxy_server_.is_valid())
    return CONFIG_UNSET;

  ProxyConfig proxy_config;
  proxy_config.proxy_rules().bypass_rules.AddRulesToSubtractImplicit();
  proxy_config.proxy_rules().ParseFromString(
      net::ProxyServerToProxyUri(proxy_server_));
  *config =
      net::ProxyConfigWithAnnotation(proxy_config, kTorProxyTrafficAnnotation);

  return CONFIG_VALID;
}

TorProxyMap::TorProxyMap() = default;
TorProxyMap::~TorProxyMap() {
  timer_.Stop();
}

// static
std::string TorProxyMap::GenerateNewPassword() {
  std::vector<uint8_t> password(kTorPasswordLength);
  crypto::RandBytes(password.data(), password.size());
  return base::HexEncode(password.data(), password.size());
}

std::string TorProxyMap::Get(
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
  // TODO(bridiver) - the timer should be in the ProxyConfigServiceTor class
  timer_.Stop();
  timer_.Start(FROM_HERE, kTenMins, this,
               &TorProxyMap::ClearExpiredEntries);

  return password;
}

size_t TorProxyMap::size() const {
  return map_.size();
}

void TorProxyMap::Erase(const std::string& username) {
  // Just erase it from the map.  There will remain an entry in the
  // queue, but it is harmless.  If anyone creates a new entry in the
  // map, the old entry in the queue will cease to affect it because
  // the timestamps won't match, and they will simultaneously create a
  // new entry in the queue.
  map_.erase(username);
}

void TorProxyMap::MaybeExpire(
    const std::string& username,
    const base::Time& timestamp) {
  auto found = map_.find(username);
  if (found != map_.end() &&
      timestamp >= found->second.second) {
    Erase(username);
  }
}

void TorProxyMap::ClearExpiredEntries() {
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
