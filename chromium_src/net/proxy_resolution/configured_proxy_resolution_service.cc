/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/proxy_resolution/configured_proxy_resolution_service.h"

#include "brave/net/proxy_resolution/proxy_config_service_tor.h"

namespace net {
namespace {

void SetTorCircuitIsolation(const ProxyConfigWithAnnotation& config,
                            const GURL& url,
                            ProxyInfo* result,
                            ProxyResolutionService* service) {
  ProxyConfigServiceTor::SetProxyAuthorization(config, url, service, result);
}

}  // namespace
}  // namespace net

#define BRAVE_TRY_TO_COMPLETE_SYNCHRONOUSLY \
  SetTorCircuitIsolation(config_.value(), url, result, this);

#include "../../../../net/proxy_resolution/configured_proxy_resolution_service.cc"
#undef BRAVE_TRY_TO_COMPLETE_SYNCHRONOUSLY
