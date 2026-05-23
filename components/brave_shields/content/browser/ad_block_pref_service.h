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

class PrefService;
class PrefProxyConfigTracker;
namespace user_prefs {
class PrefRegistrySyncable;
}

namespace brave_shields {

class AdBlockPrefService : public KeyedService,
                           public net::ProxyConfigService::Observer {
 public:
  explicit AdBlockPrefService(bool is_regular_profile,
                              PrefService* prefs,
                              PrefService* local_state,
                              const std::string& locale);
  ~AdBlockPrefService() override;

  void StartProxyTracker(
      std::unique_ptr<PrefProxyConfigTracker> pref_proxy_config_tracker,
      std::unique_ptr<net::ProxyConfigService> proxy_config_service);
  net::ProxyConfigService::ConfigAvailability GetLatestProxyConfig(
      net::ProxyConfigWithAnnotation* config) const;

  static void RegisterProfilePrefsForMigration(
      user_prefs::PrefRegistrySyncable* prefs);

 private:
  void Shutdown() override;

  void OnDeveloperModeChanged();

  // net::ProxyConfigService::Observer:
  void OnProxyConfigChanged(
      const net::ProxyConfigWithAnnotation& config,
      net::ProxyConfigService::ConfigAvailability availability) override;

  std::unique_ptr<PrefProxyConfigTracker> pref_proxy_config_tracker_;
  std::unique_ptr<net::ProxyConfigService> proxy_config_service_;

  net::ProxyConfigService::ConfigAvailability last_proxy_config_availability_;
  net::ProxyConfigWithAnnotation last_proxy_config_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_PREF_SERVICE_H_
