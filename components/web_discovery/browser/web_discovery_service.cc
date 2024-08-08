/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/web_discovery_service.h"

#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/web_discovery/browser/pref_names.h"
#include "brave/components/web_discovery/browser/server_config_loader.h"
#include "brave/components/web_discovery/common/features.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "extensions/buildflags/buildflags.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace web_discovery {

WebDiscoveryService::WebDiscoveryService(
    PrefService* local_state,
    PrefService* profile_prefs,
    base::FilePath user_data_dir,
    scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory)
    : local_state_(local_state),
      profile_prefs_(profile_prefs),
      user_data_dir_(user_data_dir),
      shared_url_loader_factory_(shared_url_loader_factory) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (profile_prefs_->GetBoolean(kWebDiscoveryExtensionEnabled)) {
    profile_prefs_->ClearPref(kWebDiscoveryExtensionEnabled);
    profile_prefs_->SetBoolean(kWebDiscoveryNativeEnabled, true);
  }
#endif

  pref_change_registrar_.Init(profile_prefs);
  pref_change_registrar_.Add(
      kWebDiscoveryNativeEnabled,
      base::BindRepeating(&WebDiscoveryService::OnEnabledChange,
                          base::Unretained(this)));

  if (profile_prefs_->GetBoolean(kWebDiscoveryNativeEnabled)) {
    Start();
  }
}

WebDiscoveryService::~WebDiscoveryService() = default;

void WebDiscoveryService::RegisterLocalStatePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterTimePref(kPatternsRetrievalTime, {});
}

void WebDiscoveryService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kWebDiscoveryNativeEnabled, false);
  registry->RegisterDictionaryPref(kAnonymousCredentialsDict);
  registry->RegisterStringPref(kCredentialRSAPrivateKey, {});
}

void WebDiscoveryService::SetExtensionPrefIfNativeDisabled(
    PrefService* profile_prefs) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (!base::FeatureList::IsEnabled(features::kBraveWebDiscoveryNative) &&
      profile_prefs->GetBoolean(kWebDiscoveryNativeEnabled)) {
    profile_prefs->SetBoolean(kWebDiscoveryExtensionEnabled, true);
  }
#endif
}

void WebDiscoveryService::Shutdown() {
  Stop();
  pref_change_registrar_.RemoveAll();
}

void WebDiscoveryService::Start() {
  if (!server_config_loader_) {
    server_config_loader_ = std::make_unique<ServerConfigLoader>(
        local_state_, user_data_dir_, shared_url_loader_factory_.get(),
        base::BindRepeating(&WebDiscoveryService::OnConfigChange,
                            base::Unretained(this)),
        base::BindRepeating(&WebDiscoveryService::OnPatternsLoaded,
                            base::Unretained(this)));
    server_config_loader_->LoadConfigs();
  }
  if (!credential_manager_) {
    credential_manager_ = std::make_unique<CredentialManager>(
        profile_prefs_, shared_url_loader_factory_.get(),
        server_config_loader_.get());
  }
}

void WebDiscoveryService::Stop() {
  server_config_loader_ = nullptr;
  credential_manager_ = nullptr;
}

void WebDiscoveryService::ClearPrefs() {
  profile_prefs_->ClearPref(kAnonymousCredentialsDict);
  profile_prefs_->ClearPref(kCredentialRSAPrivateKey);
}

void WebDiscoveryService::OnEnabledChange() {
  if (profile_prefs_->GetBoolean(kWebDiscoveryNativeEnabled)) {
    Start();
  } else {
    Stop();
    ClearPrefs();
  }
}

void WebDiscoveryService::OnConfigChange() {
  credential_manager_->JoinGroups();
}

void WebDiscoveryService::OnPatternsLoaded() {}

}  // namespace web_discovery
