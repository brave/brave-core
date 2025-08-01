// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_PREF_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_PREF_SERVICE_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_thread.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/proxy_resolution/proxy_config_with_annotation.h"

class PrefChangeRegistrar;
class PrefService;
class PrefProxyConfigTracker;

namespace brave_shields {

class AdBlockService;

class AdBlockPrefService : public KeyedService,
                           public net::ProxyConfigService::Observer {
 public:
  explicit AdBlockPrefService(AdBlockService* ad_block_service,
                              PrefService* prefs,
                              PrefService* local_state);
  ~AdBlockPrefService() override;

  void StartProxyTracker(
      std::unique_ptr<PrefProxyConfigTracker> pref_proxy_config_tracker,
      std::unique_ptr<net::ProxyConfigService> proxy_config_service);
  net::ProxyConfigService::ConfigAvailability GetLatestProxyConfig(
      net::ProxyConfigWithAnnotation* config) const;

 private:
  void Shutdown() override;

  void OnPreferenceChanged(const std::string& pref_name);
  void OnDeveloperModeChanged();

  // net::ProxyConfigService::Observer:
  void OnProxyConfigChanged(
      const net::ProxyConfigWithAnnotation& config,
      net::ProxyConfigService::ConfigAvailability availability) override;

  raw_ptr<AdBlockService> ad_block_service_ = nullptr;  // not owned
  raw_ptr<PrefService> prefs_ = nullptr;                // not owned
  std::unique_ptr<PrefChangeRegistrar, content::BrowserThread::DeleteOnUIThread>
      pref_change_registrar_;

  std::unique_ptr<PrefProxyConfigTracker> pref_proxy_config_tracker_;
  std::unique_ptr<net::ProxyConfigService> proxy_config_service_;

  net::ProxyConfigService::ConfigAvailability last_proxy_config_availability_;
  net::ProxyConfigWithAnnotation last_proxy_config_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_PREF_SERVICE_H_
