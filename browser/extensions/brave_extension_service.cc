/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_service.h"

#include <string>

#include "base/one_shot_event.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/api/content_settings/content_settings_service.h"

namespace extensions {

BraveExtensionService::BraveExtensionService(
    Profile* profile,
    const base::CommandLine* command_line,
    const base::FilePath& install_directory,
    const base::FilePath& unpacked_install_directory,
    ExtensionPrefs* extension_prefs,
    Blocklist* blocklist,
    ExtensionErrorController* error_controller,
    bool autoupdate_enabled,
    bool extensions_enabled,
    base::OneShotEvent* ready)
    : ExtensionService(profile,
                       command_line,
                       install_directory,
                       unpacked_install_directory,
                       extension_prefs,
                       blocklist,
                       error_controller,
                       autoupdate_enabled,
                       extensions_enabled,
                       ready) {}

BraveExtensionService::~BraveExtensionService() = default;

void BraveExtensionService::AddComponentExtension(const Extension* extension) {
  ExtensionService::AddComponentExtension(extension);

  // ContentSettingsStore::RegisterExtension is only called for default
  // components on the first run with a fresh profile. All restarts of the
  // browser after that do not call it. This causes ContentSettingsStore's
  // `entries_` to never insert the component ID and then
  // ContentSettingsStore::GetValueMap always returns nullptr. I don't think
  // Chromium is affected by this simply because they don't use content settings
  // from default component extensions.
  extension_prefs_->OnExtensionInstalled(
      extension, /*disable_reasons=*/{}, syncer::StringOrdinal(),
      extensions::kInstallFlagNone, std::string(), {} /* ruleset_checksums */);
  extensions::ContentSettingsService::Get(profile_)->OnExtensionPrefsLoaded(
      extension->id(), extension_prefs_);
}

}  // namespace extensions
