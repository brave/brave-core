/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_service.h"

#include <string>

#include "base/one_shot_event.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/api/content_settings/content_settings_service.h"
#include "extensions/browser/extension_action.h"
#include "extensions/browser/extension_action_manager.h"
#include "extensions/common/constants.h"

namespace extensions {

BraveExtensionService::BraveExtensionService(Profile* profile,
    const base::CommandLine* command_line,
    const base::FilePath& install_directory,
    ExtensionPrefs* extension_prefs,
    Blocklist* blocklist,
    bool autoupdate_enabled,
    bool extensions_enabled,
    base::OneShotEvent* ready) :
    ExtensionService(profile, command_line, install_directory, extension_prefs,
        blocklist, autoupdate_enabled, extensions_enabled, ready) {
}

BraveExtensionService::~BraveExtensionService() {
}

void BraveExtensionService::AddComponentExtension(const Extension* extension) {
  ExtensionService::AddComponentExtension(extension);

  // Disable Brave Rewards extension action for Guest and Tor profiles on all
  // tabs right after loading the extension for these profiles. Can't do the
  // same for the regular off the record (incognito) profile as there doesn't
  // appear to be a separate from the regular profile action manager for it, so
  // disabling it would apply to the regular profile as well. Instead, catch
  // the extension when BraveActionViewController is queried about the
  // visibility of the action.
  if ((extension->id() == brave_rewards_extension_id) &&
      (profile_->IsGuestSession() || profile_->IsTor())) {
    extensions::ExtensionActionManager* extension_action_manager =
        ExtensionActionManager::Get(profile_);
    extensions::ExtensionAction* action =
        extension_action_manager->GetExtensionAction(*extension);
    action->SetIsVisible(ExtensionAction::kDefaultTabId, false);
  }

  // ContentSettingsStore::RegisterExtension is only called for default
  // components on the first run with a fresh profile. All restarts of the
  // browser after that do not call it. This causes ContentSettingsStore's
  // `entries_` to never insert the component ID and then
  // ContentSettingsStore::GetValueMap always returns nullptr. I don't think
  // Chromium is affected by this simply because they don't use content settings
  // from default component extensions.
  extension_prefs_->OnExtensionInstalled(extension, Extension::ENABLED,
                                         syncer::StringOrdinal(),
                                         extensions::kInstallFlagNone,
                                         std::string(),
                                         {} /* ruleset_checksums */);
  extensions::ContentSettingsService::Get(profile_)->OnExtensionPrefsLoaded(
      extension->id(), extension_prefs_);
}

}  // namespace extensions
