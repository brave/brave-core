/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_management.h"

#include <memory>

#include "brave/browser/brave_browser_process.h"
#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/browser/extensions/brave_extension_provider.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/extension_management_internal.h"
#include "chrome/browser/extensions/external_policy_loader.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_urls.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/components/tor/brave_tor_client_updater.h"
#include "brave/components/tor/brave_tor_pluggable_transport_updater.h"
#include "brave/components/tor/pref_names.h"
#endif

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_constants.h"
#endif

namespace extensions {

BraveExtensionManagement::BraveExtensionManagement(Profile* profile)
    : ExtensionManagement(profile) {
  extension_registry_observer_.Observe(
      ExtensionRegistry::Get(static_cast<content::BrowserContext*>(profile)));
  providers_.push_back(std::make_unique<BraveExtensionProvider>());
  if (g_browser_process->local_state()) {
    local_state_pref_change_registrar_.Init(g_browser_process->local_state());
#if BUILDFLAG(ENABLE_TOR)
    local_state_pref_change_registrar_.Add(
        tor::prefs::kTorDisabled,
        base::BindRepeating(&BraveExtensionManagement::OnTorDisabledChanged,
                            base::Unretained(this)));
    local_state_pref_change_registrar_.Add(
        tor::prefs::kBridgesConfig,
        base::BindRepeating(
            &BraveExtensionManagement::OnTorPluggableTransportChanged,
            base::Unretained(this)));
#endif
  }
  // Make IsInstallationExplicitlyAllowed to be true
#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
  AccessById(kEthereumRemoteClientExtensionId)->installation_mode =
      INSTALLATION_RECOMMENDED;
#endif
  Cleanup(profile);
}

BraveExtensionManagement::~BraveExtensionManagement() {
  local_state_pref_change_registrar_.RemoveAll();
}

void BraveExtensionManagement::OnTorDisabledChanged() {
#if BUILDFLAG(ENABLE_TOR)
  if (TorProfileServiceFactory::IsTorDisabled(profile_)) {
    TorProfileManager::GetInstance().CloseAllTorWindows();
    if (g_brave_browser_process->tor_client_updater()) {
      g_brave_browser_process->tor_client_updater()->Cleanup();
    }
    if (g_brave_browser_process->tor_pluggable_transport_updater()) {
      g_brave_browser_process->tor_pluggable_transport_updater()->Cleanup();
    }
  }
#endif
}

void BraveExtensionManagement::OnTorPluggableTransportChanged() {
#if BUILDFLAG(ENABLE_TOR)
  if (TorProfileServiceFactory::GetTorBridgesConfig().use_bridges ==
      tor::BridgesConfig::Usage::kNotUsed) {
    if (g_brave_browser_process->tor_pluggable_transport_updater()) {
      g_brave_browser_process->tor_pluggable_transport_updater()->Cleanup();
    }
  }
#endif
}

void BraveExtensionManagement::Cleanup(content::BrowserContext* context) {
  // BrowserPolicyConnector enforce policy earlier than this constructor so we
  // have to manully cleanup tor executable when tor is disabled by gpo
  if (g_browser_process->local_state()) {
    OnTorDisabledChanged();
    OnTorPluggableTransportChanged();
  }
}

}  // namespace extensions
