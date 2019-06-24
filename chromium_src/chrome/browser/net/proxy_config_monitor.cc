/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/buildflags.h"
#include "brave/common/tor/pref_names.h"
#include "brave/common/tor/tor_proxy_uri_helper.h"
#include "brave/net/proxy_resolution/proxy_config_service_tor.h"
#include "net/proxy_resolution/proxy_config_service.h"

namespace {

#if BUILDFLAG(ENABLE_TOR)
std::unique_ptr<net::ProxyConfigService> CreateProxyConfigServiceTor() {
  // No need to track proxy pref for tor profile which has to have persistent
  // tor proxy setting
  return std::make_unique<net::ProxyConfigServiceTor>(tor::GetTorProxyURI());
}
#endif  // BUILDFLAG(ENABLE_TOR)

}  // namespace

#if BUILDFLAG(ENABLE_TOR)
#define BRAVE_PROXY_CONFIG_MONITOR \
  if (profile && profile->IsTorProfile()) \
    proxy_config_service_ = CreateProxyConfigServiceTor(); \
  else
#else
#define BRAVE_PROXY_CONFIG_MONITOR
#endif

#include "../../../../../chrome/browser/net/proxy_config_monitor.cc"  // NOLINT
