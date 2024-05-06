/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/wdp_service.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/web_discovery/browser/pref_names.h"
#include "brave/components/web_discovery/browser/server_config_loader.h"
#include "components/prefs/pref_registry_simple.h"
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

void WDPService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kAnonymousCredentialsDict);
  registry->RegisterStringPref(kCredentialRSAPrivateKey, {});
  registry->RegisterStringPref(kCredentialRSAPublicKey, {});
}

void WDPService::Start() {
  credential_manager_ = std::make_unique<CredentialManager>(
      profile_prefs_, shared_url_loader_factory_.get(),
      &last_loaded_server_config_,
      base::BindRepeating(&WDPService::OnCredentialsLoaded,
                          base::Unretained(this)));
  server_config_loader_ = std::make_unique<ServerConfigLoader>(
      shared_url_loader_factory_.get(),
      base::BindRepeating(&WDPService::OnConfigChange, base::Unretained(this)));
  server_config_loader_->Load();
}

void WDPService::Stop() {
  server_config_loader_ = nullptr;
  credential_manager_ = nullptr;
  last_loaded_server_config_ = nullptr;
}

void WDPService::OnEnabledChange() {
  if (profile_prefs_->GetBoolean(kWebDiscoveryEnabled)) {
    Start();
  } else {
    Stop();
  }
}

void WDPService::OnConfigChange(std::unique_ptr<ServerConfig> config) {
  last_loaded_server_config_ = std::move(config);
  credential_manager_->JoinGroups();
}

void WDPService::OnCredentialsLoaded() {
  // TODO(djandries): send queued messages if any, or remove this method
  // if not needed
}

}  // namespace web_discovery
