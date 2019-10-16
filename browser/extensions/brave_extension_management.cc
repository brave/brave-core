/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_management.h"

#include <memory>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/common/brave_switches.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/common/pref_names.h"
#include "brave/browser/extensions/brave_extension_provider.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/external_policy_loader.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_urls.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/extensions/brave_tor_client_updater.h"
#endif

namespace {
void DeleteDir(const base::FilePath& path) {
  base::DeleteFile(path, true);
}
}  // namespace

namespace extensions {

BraveExtensionManagement::BraveExtensionManagement(Profile* profile)
    : ExtensionManagement(profile),
      extension_registry_observer_(this),
      profile_(profile) {
  extension_registry_observer_.Add(ExtensionRegistry::Get(
        static_cast<content::BrowserContext*>(profile)));
  providers_.push_back(
      std::make_unique<BraveExtensionProvider>());
  CleanupBraveExtensions();
  RegisterBraveExtensions();
}

BraveExtensionManagement::~BraveExtensionManagement() {
}

void BraveExtensionManagement::RegisterBraveExtensions() {
#if BUILDFLAG(ENABLE_TOR)
  const bool isTorEnabled = !profile_->GetPrefs()->GetBoolean(kTorDisabled);
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (isTorEnabled &&
      !command_line.HasSwitch(switches::kDisableTorClientUpdaterExtension) &&
      !profile_->AsTestingProfile()) {
    g_brave_browser_process->tor_client_updater()->Register();
  }
#endif
}

void BraveExtensionManagement::CleanupBraveExtensions() {
#if BUILDFLAG(ENABLE_TOR)
  // Delete tor binaries if tor is disabled by gpo.
  if (profile_->GetPrefs()->GetBoolean(kTorDisabled)) {
    ProfileManager* profile_manager = g_browser_process->profile_manager();
    base::FilePath tor_component_dir =
        profile_manager->user_data_dir().AppendASCII(
            extensions::kTorClientComponentId);
    g_brave_browser_process->tor_client_updater()->GetTaskRunner()->PostTask(
        FROM_HERE, base::BindOnce(&DeleteDir, tor_component_dir));
  }
#endif
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

}  // namespace extensions
