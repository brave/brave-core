/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_WDP_SERVICE_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_WDP_SERVICE_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/components/web_discovery/browser/server_config_loader.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace web_discovery {

class WDPService : public KeyedService {
 public:
  WDPService(
      PrefService* profile_prefs,
      scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory);
  ~WDPService() override;

  WDPService(const WDPService&) = delete;
  WDPService& operator=(const WDPService&) = delete;

 private:
  void Start();
  void Stop();

  void OnEnabledChange();

  void OnConfigChange(const ServerConfig& config);

  raw_ptr<PrefService> profile_prefs_;
  PrefChangeRegistrar pref_change_registrar_;

  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;

  std::unique_ptr<ServerConfigLoader> server_config_loader_;
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_WDP_SERVICE_H_
