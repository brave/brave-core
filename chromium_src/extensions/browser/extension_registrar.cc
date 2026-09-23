// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "extensions/browser/extension_registrar.h"

#include "extensions/browser/api/content_settings/content_settings_service.h"

#define AddComponentExtension AddComponentExtension_ChromiumImpl
#include <extensions/browser/extension_registrar.cc>
#undef AddComponentExtension

namespace extensions {

void ExtensionRegistrar::AddComponentExtension(const Extension* extension) {
  AddComponentExtension_ChromiumImpl(extension);

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
  extensions::ContentSettingsService::Get(browser_context_)
      ->OnExtensionPrefsLoaded(extension->id(), extension_prefs_);
}

}  // namespace extensions
