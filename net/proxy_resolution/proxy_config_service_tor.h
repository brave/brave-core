/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_NET_PROXY_RESOLUTION_PROXY_CONFIG_SERVICE_TOR_H_
#define BRAVE_NET_PROXY_RESOLUTION_PROXY_CONFIG_SERVICE_TOR_H_

#include <string>

#include "base/observer_list.h"
#include "net/base/net_export.h"
#include "net/base/proxy_server.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

class GURL;

namespace base {
class Time;
}

namespace net {

class NetworkAnonymizationKey;
class ProxyConfigWithAnnotation;
class ProxyInfo;
class ProxyResolutionService;

// Implementation of ProxyConfigService that returns a tor specific result.
class NET_EXPORT ProxyConfigServiceTor : public net::ProxyConfigService {
 public:
  ProxyConfigServiceTor();
  explicit ProxyConfigServiceTor(const std::string& proxy_uri);
  ~ProxyConfigServiceTor() override;

  void UpdateProxyURI(const std::string& proxy_uri);

  static std::string CircuitAnonymizationKey(const GURL& url);
  void SetNewTorCircuit(const GURL& url);
  static void SetProxyAuthorization(const ProxyConfigWithAnnotation& config,
                                    const GURL& url,
                                    const NetworkAnonymizationKey& key,
                                    ProxyResolutionService* service,
                                    ProxyInfo* result);

  // ProxyConfigService methods:
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;
  ConfigAvailability GetLatestProxyConfig(
      net::ProxyConfigWithAnnotation* config) override;

  // This is useful when we want to test mock requests/responses in tor context
  // with embedded test server.
  static void SetBypassTorProxyConfigForTesting(bool bypass);

  static NetworkTrafficAnnotationTag GetTorAnnotationTagForTesting();

 private:
  static bool bypass_tor_proxy_config_for_testing_;
  base::ObserverList<Observer>::Unchecked observers_;
  ProxyServer proxy_server_;
};

}  // namespace net

#endif  // BRAVE_NET_PROXY_RESOLUTION_PROXY_CONFIG_SERVICE_TOR_H_
