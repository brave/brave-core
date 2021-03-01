/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_management.h"

#include <memory>

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/extensions/brave_extension_provider.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_wallet/buildflags/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/components/tor/pref_names.h"
#include "chrome/browser/extensions/extension_management_internal.h"
#include "chrome/browser/extensions/external_policy_loader.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_urls.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/brave_tor_client_updater.h"
#endif

#if BUILDFLAG(IPFS_ENABLED)
#include "brave/components/ipfs/brave_ipfs_client_updater.h"
#include "brave/components/ipfs/ipfs_utils.h"
#endif

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
#include "brave/components/brave_wallet/brave_wallet_constants.h"
#endif

namespace extensions {

BraveExtensionManagement::BraveExtensionManagement(Profile* profile)
    : ExtensionManagement(profile), extension_registry_observer_(this) {
  extension_registry_observer_.Add(
      ExtensionRegistry::Get(static_cast<content::BrowserContext*>(profile)));
  providers_.push_back(std::make_unique<BraveExtensionProvider>());
  local_state_pref_change_registrar_.Init(g_browser_process->local_state());
  local_state_pref_change_registrar_.Add(
      tor::prefs::kTorDisabled,
      base::BindRepeating(&BraveExtensionManagement::OnTorDisabledChanged,
                          base::Unretained(this)));
  // Make IsInstallationExplicitlyAllowed to be true
#if BUILDFLAG(BRAVE_WALLET_ENABLED)
  AccessById(ethereum_remote_client_extension_id)->installation_mode =
      INSTALLATION_RECOMMENDED;
#endif
  Cleanup(profile);
}

BraveExtensionManagement::~BraveExtensionManagement() {
  local_state_pref_change_registrar_.RemoveAll();
}

void BraveExtensionManagement::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const Extension* extension) {
  if (extension->id() == ipfs_companion_extension_id)
    pref_service_->SetBoolean(kIPFSCompanionEnabled, true);
}

void BraveExtensionManagement::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const Extension* extension,
    UnloadedExtensionReason reason) {
  if (extension->id() == ipfs_companion_extension_id)
    pref_service_->SetBoolean(kIPFSCompanionEnabled, false);
}

void BraveExtensionManagement::OnTorDisabledChanged() {
#if BUILDFLAG(ENABLE_TOR)
  if (TorProfileServiceFactory::IsTorDisabled())
    g_brave_browser_process->tor_client_updater()->Cleanup();
#endif
}

void BraveExtensionManagement::Cleanup(content::BrowserContext* context) {
  // BrowserPolicyConnector enforce policy earlier than this constructor so we
  // have to manually cleanup tor executable when tor is disabled by gpo
  OnTorDisabledChanged();

#if BUILDFLAG(IPFS_ENABLED)
  // Remove ipfs executable if it is disabled by GPO.
  if (ipfs::IsIpfsDisabledByPolicy(context))
    g_brave_browser_process->ipfs_client_updater()->Cleanup();
#endif
}

}  // namespace extensions
