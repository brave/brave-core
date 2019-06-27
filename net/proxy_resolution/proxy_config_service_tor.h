/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_NET_PROXY_RESOLUTION_PROXY_CONFIG_SERVICE_TOR_H_
#define BRAVE_NET_PROXY_RESOLUTION_PROXY_CONFIG_SERVICE_TOR_H_

#include <map>
#include <queue>
#include <string>
#include <utility>

#include "base/compiler_specific.h"
#include "base/observer_list.h"
#include "base/timer/timer.h"
#include "net/base/net_export.h"
#include "net/base/proxy_server.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

class GURL;

namespace base {
class Time;
}

namespace net {

class ProxyConfigWithAnnotation;
class ProxyInfo;
class ProxyResolutionService;

// Implementation of ProxyConfigService that returns a tor specific result.
class NET_EXPORT ProxyConfigServiceTor : public net::ProxyConfigService {
 public:
  // Used to cache <username, password> of proxies
  class TorProxyMap {
   public:
    TorProxyMap();
    ~TorProxyMap();
    std::string Get(const std::string& key);
    void Erase(const std::string& key);
    void MaybeExpire(const std::string& key, const base::Time& timestamp);

   private:
    // Generate a new base 64-encoded 128 bit random tag
    static std::string GenerateNewPassword();
    // Clear expired entries in the queue from the map.
    void ClearExpiredEntries();
    std::map<std::string, std::pair<std::string, base::Time>> map_;
    std::priority_queue<std::pair<base::Time, std::string>> queue_;
    base::OneShotTimer timer_;
    DISALLOW_COPY_AND_ASSIGN(TorProxyMap);
  };

  explicit ProxyConfigServiceTor(const std::string& proxy_uri);
  ~ProxyConfigServiceTor() override;

  static std::string CircuitIsolationKey(const GURL& request_url);
  static bool IsTorProxy(const net::NetworkTrafficAnnotationTag tag);
  static void SetProxyAuthorization(const ProxyConfigWithAnnotation& config,
                                    const GURL& url,
                                    ProxyResolutionService* service,
                                    ProxyInfo* result);


  void SetNewTorCircuit(const GURL& url);

  // ProxyConfigService methods:
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;
  ConfigAvailability GetLatestProxyConfig(
      net::ProxyConfigWithAnnotation* config) override;

 private:
  // void OnProxyConfigChanged(net::ProxyConfigWithAnnotation* config);
  // void SetUsername(const std::string& username);

  // base::RepeatingTimer timer_;
  ProxyConfigServiceTor::TorProxyMap map_;
  base::ObserverList<Observer>::Unchecked observers_;
  ProxyServer proxy_server_;
};

}  // namespace net

#endif  // BRAVE_NET_PROXY_RESOLUTION_PROXY_CONFIG_SERVICE_TOR_H_
