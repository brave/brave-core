/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/chromium_src/net/proxy_resolution/proxy_resolution_service.h"

class GURL;

namespace net {
class ProxyConfig;
class ProxyInfo;
namespace {
bool IsTorProxy(const ProxyConfig& config);
void SetTorCircuitIsolation(const ProxyConfig& config,
                            const GURL& url,
                            ProxyInfo* result,
                            ProxyConfigServiceTor::TorProxyMap* map);
}  // namespace
}  // namespace net
#include "../../../../net/proxy_resolution/proxy_resolution_service.cc"  // NOLINT

#include <string>

namespace net {
namespace {

bool IsTorProxy(const ProxyConfig& config) {
  if (config.proxy_rules().single_proxies.IsEmpty())
    return false;
  ProxyServer server = config.proxy_rules().single_proxies.Get();
  HostPortPair host_port = server.host_port_pair();
  if (host_port.host() == "127.0.0.1" &&
      server.scheme() == ProxyServer::SCHEME_SOCKS5)
    return true;
  return false;
}

void SetTorCircuitIsolation(const ProxyConfig& config,
                            const GURL& url,
                            ProxyInfo* result,
                            ProxyConfigServiceTor::TorProxyMap* map) {
  DCHECK(IsTorProxy(config));
  std::string proxy_uri = config.proxy_rules().single_proxies.Get().ToURI();

  ProxyConfigServiceTor tor_proxy_config_service(proxy_uri);
  ProxyConfigWithAnnotation fetched_config;
  tor_proxy_config_service.GetLatestProxyConfig(&fetched_config);
  tor_proxy_config_service.SetUsername(
      ProxyConfigServiceTor::CircuitIsolationKey(url), map);
  tor_proxy_config_service.GetLatestProxyConfig(&fetched_config);
  fetched_config.value().proxy_rules().Apply(url, result);
}

}  // namespace
}  // namespace net
