/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_WDP_SERVICE_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_WDP_SERVICE_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/web_discovery/browser/credential_manager.h"
#include "brave/components/web_discovery/browser/patterns.h"
#include "brave/components/web_discovery/browser/server_config_loader.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

class PrefRegistrySimple;
class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace web_discovery {

class WDPService : public KeyedService {
 public:
  WDPService(
      PrefService* local_state,
      PrefService* profile_prefs,
      base::FilePath user_data_dir,
      scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory);
  ~WDPService() override;

  WDPService(const WDPService&) = delete;
  WDPService& operator=(const WDPService&) = delete;

  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  void Start();
  void Stop();

  void OnEnabledChange();

  void OnConfigChange(std::unique_ptr<ServerConfig> config);
  void OnPatternsLoaded(std::unique_ptr<PatternsGroup> patterns);

  raw_ptr<PrefService> local_state_;
  raw_ptr<PrefService> profile_prefs_;
  PrefChangeRegistrar pref_change_registrar_;

  base::FilePath user_data_dir_;

  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;

  std::unique_ptr<ServerConfigLoader> server_config_loader_;
  std::unique_ptr<ServerConfig> last_loaded_server_config_;
  std::unique_ptr<PatternsGroup> last_loaded_patterns_;
  std::unique_ptr<CredentialManager> credential_manager_;
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_WDP_SERVICE_H_
