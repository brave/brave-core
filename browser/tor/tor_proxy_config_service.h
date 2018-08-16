/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_TOR_PROXY_CONFIG_SERVICE_
#define BRAVE_BROWSER_TOR_TOR_PROXY_CONFIG_SERVICE_

#include <string>
#include <map>
#include <queue>
#include <utility>

#include "base/compiler_specific.h"
#include "base/timer/timer.h"
#include "net/base/net_errors.h"
#include "net/base/net_export.h"
#include "net/proxy_resolution/proxy_config.h"
#include "net/proxy_resolution/proxy_config_service.h"

namespace base {
class Time;
}

namespace net {
class ProxyResolutionService;
}

namespace tor {

const char kSocksProxy[] = "socks5";

// Implementation of ProxyConfigService that returns a tor specific result.
class TorProxyConfigService : public net::ProxyConfigService {
 public:
  // Used to cache <username, password> of proxies
  class TorProxyMap {
   public:
    TorProxyMap();
    ~TorProxyMap();
    std::string Get(const std::string&);
    void Erase(const std::string&);
   private:
    // Generate a new 128 bit random tag
    static std::string GenerateNewPassword();
    // Clear expired entries in the queue from the map.
    void ClearExpiredEntries();
    std::map<std::string, std::pair<std::string, base::Time> > map_;
    std::priority_queue<std::pair<base::Time, std::string> > queue_;
    base::OneShotTimer timer_;
    DISALLOW_COPY_AND_ASSIGN(TorProxyMap);
  };

  explicit TorProxyConfigService(const std::string& tor_proxy,
                                 const std::string& username,
                                 TorProxyMap* map);
  ~TorProxyConfigService() override;

  static void TorSetProxy(
    net::ProxyResolutionService* service,
    const std::string& tor_proxy,
    const std::string& site_url,
    TorProxyMap* tor_proxy_map,
    bool new_password);

  // ProxyConfigService methods:
  void AddObserver(Observer* observer) override {}
  void RemoveObserver(Observer* observer) override {}
  ConfigAvailability GetLatestProxyConfig(
    net::ProxyConfigWithAnnotation* config) override;

 private:
  net::ProxyConfig config_;

  std::string scheme_;
  std::string host_;
  std::string port_;
};

}  // namespace tor

#endif  // BRAVE_BROWSER_TOR_TOR_PROXY_CONFIG_SERVICE_
