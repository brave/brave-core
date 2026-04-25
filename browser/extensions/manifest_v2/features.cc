// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/extensions/manifest_v2/features.h"

namespace extensions_mv2::features {

BASE_FEATURE(kExtensionsManifestV2,
             base::FEATURE_DISABLED_BY_DEFAULT);

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
