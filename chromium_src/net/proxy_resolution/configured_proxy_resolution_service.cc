/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/proxy_resolution/configured_proxy_resolution_service.h"

#include "base/types/optional_ref.h"
#include "brave/net/proxy_resolution/proxy_config_service_tor.h"
#include "net/base/network_anonymization_key.h"

namespace net {
namespace {

// Called at the top of ProcessResolutionResult(), which both synchronous and
// asynchronous resolutions reach, so every request resolved to the Tor proxy
// gets per-site SOCKS credentials. A failed resolution, or one without a
// config, has no Tor proxy to add credentials to.
void MaybeSetTorCircuitIsolation(
    int result_code,
    base::optional_ref<const ProxyConfigWithAnnotation> config,
    const GURL& url,
    const NetworkAnonymizationKey& key,
    ProxyInfo* result,
    ProxyResolutionService* service) {
  if (result_code != OK || !config) {
    return;
  }
  ProxyConfigServiceTor::SetProxyAuthorization(*config, url, key, service,
                                               result);
}

}  // namespace
}  // namespace net

#include <net/proxy_resolution/configured_proxy_resolution_service.cc>
