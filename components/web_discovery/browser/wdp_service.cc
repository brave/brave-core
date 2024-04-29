/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/wdp_service.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/web_discovery/browser/server_config_loader.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace web_discovery {

WDPService::WDPService(
    PrefService* profile_prefs,
    scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory)
    : profile_prefs_(profile_prefs),
      shared_url_loader_factory_(shared_url_loader_factory) {
  pref_change_registrar_.Init(profile_prefs);
  pref_change_registrar_.Add(kWebDiscoveryEnabled,
                             base::BindRepeating(&WDPService::OnEnabledChange,
                                                 base::Unretained(this)));

  if (profile_prefs_->GetBoolean(kWebDiscoveryEnabled)) {
    Start();
  }
}

WDPService::~WDPService() = default;

void WDPService::Start() {
  server_config_loader_ = std::make_unique<ServerConfigLoader>(
      shared_url_loader_factory_.get(),
      base::BindRepeating(&WDPService::OnConfigChange, base::Unretained(this)));
  server_config_loader_->Load();
}

void WDPService::Stop() {
  server_config_loader_ = nullptr;
}

void WDPService::OnEnabledChange() {
  if (profile_prefs_->GetBoolean(kWebDiscoveryEnabled)) {
    Start();
  } else {
    Stop();
  }
}

void WDPService::OnConfigChange(const ServerConfig& config) {}

}  // namespace web_discovery
