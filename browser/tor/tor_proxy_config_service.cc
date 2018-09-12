/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_proxy_config_service.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "base/time/time.h"
#include "base/values.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "content/public/browser/browser_thread.h"
#include "crypto/random.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/url_request/url_request_context.h"
#include "url/third_party/mozilla/url_parse.h"

namespace tor {

using content::BrowserThread;
const int kTorPasswordLength = 16;
// Default tor circuit life time is 10 minutes
constexpr base::TimeDelta kTenMins = base::TimeDelta::FromMinutes(10);

TorProxyConfigService::TorProxyConfigService(
  const std::string& tor_proxy, const std::string& username,
  TorProxyMap* tor_proxy_map) {
    if (tor_proxy.length()) {
      url::Parsed url;
      url::ParseStandardURL(
        tor_proxy.c_str(),
        std::min(tor_proxy.size(),
                 static_cast<size_t>(std::numeric_limits<int>::max())),
        &url);
      if (url.scheme.is_valid()) {
        scheme_ = tor_proxy.substr(url.scheme.begin, url.scheme.len);
      }
      if (url.host.is_valid()) {
        host_ = tor_proxy.substr(url.host.begin, url.host.len);
      }
      if (url.port.is_valid()) {
        port_ = tor_proxy.substr(url.port.begin, url.port.len);
      }
      if (scheme_.empty() || host_.empty() || port_.empty())
        return;
      std::string proxy_url;
      if (tor_proxy_map && !username.empty()) {
        std::string password = tor_proxy_map->Get(username);
        proxy_url = std::string(scheme_ + "://" + username + ":" + password +
                                "@" + host_ + ":" + port_);
      } else {
        proxy_url = std::string(scheme_ + "://" + host_ + ":" + port_);
      }
      config_.proxy_rules().ParseFromString(proxy_url);
    }
}

TorProxyConfigService::~TorProxyConfigService() {}

// static
void TorProxyConfigService::TorSetProxy(
    net::ProxyResolutionService* service,
    std::string tor_proxy,
    std::string site_url,
    TorProxyMap* tor_proxy_map,
    bool new_password) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!service)
    return;
  if (new_password && tor_proxy_map)
    tor_proxy_map->Erase(site_url);
  std::unique_ptr<TorProxyConfigService>
    config(new TorProxyConfigService(tor_proxy, site_url, tor_proxy_map));
  service->ResetConfigService(std::move(config));
}

TorProxyConfigService::ConfigAvailability
    TorProxyConfigService::GetLatestProxyConfig(
      net::ProxyConfigWithAnnotation* config) {
  if (scheme_ != kSocksProxy || host_.empty() || port_.empty())
    return CONFIG_UNSET;
  *config = net::ProxyConfigWithAnnotation(config_, NO_TRAFFIC_ANNOTATION_YET);
  return CONFIG_VALID;
}

TorProxyConfigService::TorProxyMap::TorProxyMap() = default;
TorProxyConfigService::TorProxyMap::~TorProxyMap() {
  timer_.Stop();
}

// static
std::string TorProxyConfigService::TorProxyMap::GenerateNewPassword() {
  std::vector<uint8_t> password(kTorPasswordLength);
  crypto::RandBytes(password.data(), password.size());
  return base::HexEncode(password.data(), password.size());
}

std::string TorProxyConfigService::TorProxyMap::Get(
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
               &TorProxyConfigService::TorProxyMap::ClearExpiredEntries);

  return password;
}

void TorProxyConfigService::TorProxyMap::Erase(const std::string& username) {
  // Just erase it from the map.  There will remain an entry in the
  // queue, but it is harmless.  If anyone creates a new entry in the
  // map, the old entry in the queue will cease to affect it because
  // the timestamps won't match, and they will simultaneously create a
  // new entry in the queue.
  map_.erase(username);
}

void TorProxyConfigService::TorProxyMap::ClearExpiredEntries() {
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

}  // namespace tor
