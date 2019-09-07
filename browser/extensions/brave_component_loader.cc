/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_component_loader.h"

#include <string>

#include "base/command_line.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/component_updater/brave_component_installer.h"
#include "brave/common/brave_switches.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_extension/grit/brave_extension.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_rewards/resources/extension/grit/brave_rewards_extension_resources.h"
#include "brave/components/brave_webtorrent/grit/brave_webtorrent_resources.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/components_ui.h"
#include "chrome/common/pref_names.h"
#include "components/grit/brave_components_resources.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"

namespace extensions {

BraveComponentLoader::BraveComponentLoader(
    ExtensionServiceInterface* extension_service,
    Profile* profile)
    : ComponentLoader(extension_service, profile),
      profile_(profile),
      profile_prefs_(profile->GetPrefs()) {}

BraveComponentLoader::~BraveComponentLoader() {
}

void BraveComponentLoader::OnComponentRegistered(std::string extension_id) {
  // TODO(bridiver) - I don't think this is correct
  ComponentsUI::OnDemandUpdate(extension_id);
}

void BraveComponentLoader::OnComponentReady(std::string extension_id,
    bool allow_file_access,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  Add(manifest, install_dir);
  if (allow_file_access) {
    ExtensionPrefs::Get(profile_)->SetAllowFileAccess(extension_id, true);
  }
}

void BraveComponentLoader::AddExtension(const std::string& extension_id,
    const std::string& name, const std::string& public_key) {
  brave::RegisterComponent(g_browser_process->component_updater(),
    name,
    public_key,
    base::Bind(&BraveComponentLoader::OnComponentRegistered,
        base::Unretained(this), extension_id),
    base::Bind(&BraveComponentLoader::OnComponentReady,
        base::Unretained(this), extension_id, true));
}

void BraveComponentLoader::AddHangoutServicesExtension() {
  if (!profile_prefs_->FindPreference(kHangoutsEnabled) ||
      profile_prefs_->GetBoolean(kHangoutsEnabled)) {
    ForceAddHangoutServicesExtension();
  }
}

void BraveComponentLoader::ForceAddHangoutServicesExtension() {
  ComponentLoader::AddHangoutServicesExtension();
}

void BraveComponentLoader::AddDefaultComponentExtensions(
    bool skip_session_components) {
  ComponentLoader::AddDefaultComponentExtensions(skip_session_components);

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (!command_line.HasSwitch(switches::kDisableBraveExtension)) {
    base::FilePath brave_extension_path(FILE_PATH_LITERAL(""));
    brave_extension_path =
        brave_extension_path.Append(FILE_PATH_LITERAL("brave_extension"));
    Add(IDR_BRAVE_EXTENSION, brave_extension_path);
  }

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  if (!command_line.HasSwitch(switches::kDisableBraveRewardsExtension)) {
    base::FilePath brave_rewards_path(FILE_PATH_LITERAL(""));
    brave_rewards_path =
        brave_rewards_path.Append(FILE_PATH_LITERAL("brave_rewards"));
    Add(IDR_BRAVE_REWARDS, brave_rewards_path);
  }
#endif

  if (!command_line.HasSwitch(switches::kDisableWebTorrentExtension) &&
      (!profile_prefs_->FindPreference(kWebTorrentEnabled) ||
      profile_prefs_->GetBoolean(kWebTorrentEnabled))) {
    base::FilePath brave_webtorrent_path(FILE_PATH_LITERAL(""));
    brave_webtorrent_path =
      brave_webtorrent_path.Append(FILE_PATH_LITERAL("brave_webtorrent"));
    Add(IDR_BRAVE_WEBTORRENT, brave_webtorrent_path);
  }

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
  // If brave://wallet has been loaded at least once, then load it again.
  if (ExtensionPrefs::Get(profile_)->
      HasPrefForExtension(ethereum_remote_client_extension_id)) {
    AddEthereumRemoteClientExtension();
  }
#endif
}

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
void BraveComponentLoader::AddEthereumRemoteClientExtension() {
  AddExtension(ethereum_remote_client_extension_id,
      ethereum_remote_client_extension_name,
      ethereum_remote_client_extension_public_key);
}
#endif

}  // namespace extensions
