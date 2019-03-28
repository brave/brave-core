/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_component_loader.h"

#include <string>

#include "base/command_line.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/component_updater/brave_component_installer.h"
#include "brave/browser/extensions/brave_component_extension.h"
#include "brave/common/brave_switches.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_extension/grit/brave_extension.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_rewards/resources/extension/grit/brave_rewards_extension_resources.h"
#include "brave/components/brave_webtorrent/grit/brave_webtorrent_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/grit/brave_components_resources.h"
#include "extensions/browser/extension_prefs.h"

namespace extensions {

// static
bool BraveComponentLoader::IsPdfjsDisabled() {
  const base::CommandLine& command_line =
    *base::CommandLine::ForCurrentProcess();
  return command_line.HasSwitch(switches::kDisablePDFJSExtension);
}

BraveComponentLoader::BraveComponentLoader(
    ExtensionServiceInterface* extension_service,
    PrefService* profile_prefs,
    PrefService* local_state,
    Profile* profile)
    : ComponentLoader(extension_service, profile_prefs, local_state, profile),
      profile_(profile),
      profile_prefs_(profile_prefs),
      testing_callbacks_(nullptr) {
  ObserveOpenPdfExternallySetting();
}

BraveComponentLoader::~BraveComponentLoader() {
}

void BraveComponentLoader::OnComponentRegistered(std::string extension_id) {
  ComponentsUI demand_updater;
  // This weird looking call is ok, it is just like this to not need
  // to patch for friend access.
  demand_updater.OnDemandUpdate(g_browser_process->component_updater(),
      extension_id);
  if (testing_callbacks_) {
    testing_callbacks_->OnComponentRegistered(extension_id);
  }
}

void BraveComponentLoader::OnComponentReady(std::string extension_id,
    bool allow_file_access,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  Add(manifest, install_dir);
  if (allow_file_access) {
    ExtensionPrefs::Get((content::BrowserContext *)profile_)->
        SetAllowFileAccess(extension_id, true);
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

  if (!profile_prefs_->GetBoolean(prefs::kPluginsAlwaysOpenPdfExternally) &&
      !command_line.HasSwitch(switches::kDisablePDFJSExtension)) {
    AddExtension(pdfjs_extension_id, pdfjs_extension_name,
                 pdfjs_extension_public_key);
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

  if (profile_prefs_->GetBoolean(kIPFSCompanionEnabled)) {
    AddExtension(ipfs_companion_extension_id, ipfs_companion_extension_name,
      ipfs_companion_extension_public_key);
  }
}

void BraveComponentLoader::ObserveOpenPdfExternallySetting() {
  // Observe the setting change only in regular profiles since the PDF settings
  // page is not available in Guest/Tor profiles.
  DCHECK(profile_ && profile_prefs_);
  if (!profile_->IsGuestSession()) {
    registrar_.Init(profile_prefs_);
    registrar_.Add(prefs::kPluginsAlwaysOpenPdfExternally,
      base::Bind(&BraveComponentLoader::UpdatePdfExtension,
        base::Unretained(this)));
  }
}

void BraveComponentLoader::UpdatePdfExtension(const std::string& pref_name) {
  DCHECK(pref_name == prefs::kPluginsAlwaysOpenPdfExternally);
  DCHECK(profile_prefs_);
  if (profile_prefs_->GetBoolean(prefs::kPluginsAlwaysOpenPdfExternally) ||
      IsPdfjsDisabled()) {
    if (testing_callbacks_)
      testing_callbacks_->OnPdfExtensionAction(TestingCallbacks::WILL_REMOVE);
    Remove(pdfjs_extension_id);
  } else if (!Exists(pdfjs_extension_id)) {
    if (testing_callbacks_)
      testing_callbacks_->OnPdfExtensionAction(TestingCallbacks::WILL_ADD);
    AddExtension(pdfjs_extension_id, pdfjs_extension_name,
                 pdfjs_extension_public_key);
  }
}

void BraveComponentLoader::set_testing_callbacks(
    TestingCallbacks* testing_callbacks) {
  testing_callbacks_ = testing_callbacks;
}

}  // namespace extensions
