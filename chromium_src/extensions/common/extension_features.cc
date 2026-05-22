/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"

#include <extensions/common/extension_features.cc>

namespace extensions_features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kExtensionManifestV2Disabled, base::FEATURE_DISABLED_BY_DEFAULT},
    {kExtensionManifestV2ExceptionList, base::FEATURE_ENABLED_BY_DEFAULT},
    {kExtensionManifestV2Unsupported, base::FEATURE_DISABLED_BY_DEFAULT},
    {kExtensionsManifestV3Only, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace extensions_features

// Adds Brave's MV2 extension features here so that we don't have to patch
// upstream GN files with dependencies.
namespace extensions_mv2::features {

BASE_FEATURE(kExtensionsManifestV2, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE_PARAM(bool,
                   kExtensionsManifestV2BackupSettings,
                   &kExtensionsManifestV2,
                   "backup_settings",
                   false);

BASE_FEATURE_PARAM(bool,
                   kExtensionsManifestV2BImportSettingsOnInstall,
                   &kExtensionsManifestV2,
                   "import_settings",
                   false);

BASE_FEATURE_PARAM(bool,
                   kExtensionsManifestV2AutoInstallBraveHosted,
                   &kExtensionsManifestV2,
                   "auto_install_brave_hosted",
                   false);

bool IsSettingsBackupEnabled() {
  return base::FeatureList::IsEnabled(features::kExtensionsManifestV2) &&
         features::kExtensionsManifestV2BackupSettings.Get();
}

bool IsSettingsImportEnabled() {
  return IsSettingsBackupEnabled() &&
         features::kExtensionsManifestV2BImportSettingsOnInstall.Get();
}

bool IsExtensionReplacementEnabled() {
  return IsSettingsImportEnabled() &&
         features::kExtensionsManifestV2AutoInstallBraveHosted.Get();
}

}  // namespace extensions_mv2::features
