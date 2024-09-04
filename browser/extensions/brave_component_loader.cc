/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_component_loader.h"

#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/components/brave_component_updater/browser/brave_component_installer.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/brave_extension/grit/brave_extension.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_webtorrent/grit/brave_webtorrent_resources.h"
#include "brave/components/constants/brave_switches.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"
#include "extensions/common/mojom/manifest.mojom.h"
#include "ui/base/resource/resource_bundle.h"

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_constants.h"
#include "brave/browser/ethereum_remote_client/pref_names.h"
#include "brave/browser/extensions/ethereum_remote_client_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#endif

using extensions::mojom::ManifestLocation;

namespace extensions {

BraveComponentLoader::BraveComponentLoader(ExtensionSystem* extension_system,
                                           Profile* profile)
    : ComponentLoader(extension_system, profile),
      profile_(profile),
      profile_prefs_(profile->GetPrefs()) {
  pref_change_registrar_.Init(profile_prefs_);

  pref_change_registrar_.Add(
      kWebDiscoveryEnabled,
      base::BindRepeating(&BraveComponentLoader::UpdateBraveExtension,
                          base::Unretained(this)));

  pref_change_registrar_.Add(
      brave_rewards::prefs::kEnabled,
      base::BindRepeating(&BraveComponentLoader::UpdateBraveExtension,
                          base::Unretained(this)));
}

BraveComponentLoader::~BraveComponentLoader() = default;

void BraveComponentLoader::OnComponentRegistered(std::string extension_id) {
  brave_component_updater::BraveOnDemandUpdater::GetInstance()->EnsureInstalled(
      extension_id);
}

void BraveComponentLoader::OnComponentReady(std::string extension_id,
                                            bool allow_file_access,
                                            const base::FilePath& install_dir,
                                            const std::string& manifest) {
  Add(manifest, install_dir);
  if (allow_file_access) {
    ExtensionPrefs::Get(profile_)->SetAllowFileAccess(extension_id, true);
  }
#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
  if (extension_id == kEthereumRemoteClientExtensionId) {
    ReinstallAsNonComponent(kEthereumRemoteClientExtensionId);
  }
#endif
}

void BraveComponentLoader::ReinstallAsNonComponent(
    const std::string extension_id) {
  extensions::ExtensionService* service =
      extensions::ExtensionSystem::Get(profile_)->extension_service();
  extensions::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(profile_);
  const Extension* extension = registry->GetInstalledExtension(extension_id);
  DCHECK(extension);
  if (extension->location() == ManifestLocation::kComponent) {
    service->RemoveComponentExtension(extension_id);
    std::string error;
    scoped_refptr<Extension> normal_extension = Extension::Create(
        extension->path(), ManifestLocation::kExternalPref,
        *extension->manifest()->value(), extension->creation_flags(), &error);
    service->AddExtension(normal_extension.get());
  }
}

void BraveComponentLoader::AddExtension(const std::string& extension_id,
                                        const std::string& name,
                                        const std::string& public_key) {
  brave::RegisterComponent(
      g_browser_process->component_updater(), name, public_key,
      base::BindOnce(&BraveComponentLoader::OnComponentRegistered,
                     base::Unretained(this), extension_id),
      base::BindRepeating(&BraveComponentLoader::OnComponentReady,
                          base::Unretained(this), extension_id, true));
}

void BraveComponentLoader::AddDefaultComponentExtensions(
    bool skip_session_components) {
  ComponentLoader::AddDefaultComponentExtensions(skip_session_components);
  UpdateBraveExtension();
}

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
void BraveComponentLoader::AddEthereumRemoteClientExtension() {
  AddExtension(kEthereumRemoteClientExtensionId,
               kEthereumRemoteClientExtensionName,
               kEthereumRemoteClientExtensionPublicKey);
}

void BraveComponentLoader::AddEthereumRemoteClientExtensionOnStartup() {
  // Only load Crypto Wallets if it is set as the default wallet
  auto default_wallet = brave_wallet::GetDefaultEthereumWallet(profile_prefs_);
  const bool is_opted_into_cw =
      profile_prefs_->GetBoolean(kERCOptedIntoCryptoWallets);
  if (HasInfuraProjectID() && is_opted_into_cw &&
      default_wallet == brave_wallet::mojom::DefaultWallet::CryptoWallets) {
    AddEthereumRemoteClientExtension();
  }
}

void BraveComponentLoader::UnloadEthereumRemoteClientExtension() {
  extensions::ExtensionService* service =
      extensions::ExtensionSystem::Get(profile_)->extension_service();
  DCHECK(service);
  service->UnloadExtension(kEthereumRemoteClientExtensionId,
                           extensions::UnloadedExtensionReason::DISABLE);
}
#endif

void BraveComponentLoader::AddWebTorrentExtension() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (!command_line.HasSwitch(switches::kDisableWebTorrentExtension) &&
      (!profile_prefs_->FindPreference(kWebTorrentEnabled) ||
       profile_prefs_->GetBoolean(kWebTorrentEnabled))) {
    base::FilePath brave_webtorrent_path(FILE_PATH_LITERAL(""));
    brave_webtorrent_path =
        brave_webtorrent_path.Append(FILE_PATH_LITERAL("brave_webtorrent"));
    Add(IDR_BRAVE_WEBTORRENT, brave_webtorrent_path);
  }
}

bool BraveComponentLoader::UseBraveExtensionBackgroundPage() {
  // Keep sync with `pref_change_registrar_` in the ctor.
  return profile_prefs_->GetBoolean(brave_rewards::prefs::kEnabled) ||
         profile_prefs_->GetBoolean(kWebDiscoveryEnabled);
}

void BraveComponentLoader::UpdateBraveExtension() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kDisableBraveExtension)) {
    return;
  }

  base::FilePath brave_extension_path(FILE_PATH_LITERAL(""));
  brave_extension_path =
      brave_extension_path.Append(FILE_PATH_LITERAL("brave_extension"));
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  std::optional<base::Value::Dict> manifest = base::JSONReader::ReadDict(
      resource_bundle.GetRawDataResource(IDR_BRAVE_EXTENSION));
  CHECK(manifest) << "invalid Brave Extension manifest";

  // The background page is a conditional. Replace MAYBE_background in the
  // manifest to "background" or remove it.
  auto background_value = manifest->Extract("MAYBE_background");
  if (UseBraveExtensionBackgroundPage() && background_value) {
    manifest->Set("background", std::move(*background_value));
  }

  extensions::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(profile_);
  const Extension* current_extension =
      registry->GetInstalledExtension(brave_extension_id);

  if (current_extension) {
    const auto* current_manifest = current_extension->manifest();
    if (current_manifest && *current_manifest->value() == *manifest) {
      return;  // Skip reload, nothing is actually changed.
    }
    Remove(brave_extension_id);
  }

  const auto id = Add(std::move(*manifest), brave_extension_path);
  CHECK_EQ(id, brave_extension_id);
}

}  // namespace extensions
