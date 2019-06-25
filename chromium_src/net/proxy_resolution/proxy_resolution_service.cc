/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/proxy_resolution/proxy_resolution_service.h"

#include <string>

#include "brave/net/proxy_resolution/proxy_config_service_tor.h"

namespace net {
namespace {

bool IsTorProxy(ProxyResolutionService* service) {
  return ProxyConfigServiceTor::GetTorProxyMap(service) != nullptr;
}

void SetTorCircuitIsolation(const ProxyConfig& config,
                            const GURL& url,
                            ProxyInfo* result,
                            ProxyConfigServiceTor::TorProxyMap* map) {
  std::string proxy_uri = config.proxy_rules().single_proxies.Get().ToURI();

  // Adding username & password to global sock://127.0.0.1:[port] config without
  // actually modifying it when resolving proxy for each url.
  // username is derived from url and we keep password for 10 minutes, detail
  // encapsulated in ProxyConfigServiceTor.
  // TorProxyMap stores username/password mapping and it can only be manipulated
  // by ProxyConfigServiceTor.
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

#define BRAVE_TRY_TO_COMPLETE_SYNCHRONOUSLY \
  if (IsTorProxy(this)) \
    SetTorCircuitIsolation(config_->value(), \
                           url, \
                           result, \
                           ProxyConfigServiceTor::GetTorProxyMap( \
                                this)); \
  else

#include "../../../../net/proxy_resolution/proxy_resolution_service.cc"  // NOLINT

#undef BRAVE_RESOLVE_PROXY
#undef BRAVE_TRY_TO_COMPLETE_SYNCHRONOUSLY
