/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"

#include <extensions/common/extension_features.cc>

// Adds Brave's MV2 extension features here so that we don't have to patch
// upstream GN files with dependencies.
namespace extensions_mv2::features {

BASE_FEATURE(kExtensionsManifestV2, base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE_PARAM(bool, kBackupSettings, &kExtensionsManifestV2, true);

BASE_FEATURE_PARAM(bool,
                   kImportSettingsOnInstall,
                   &kExtensionsManifestV2,
                   true);

BASE_FEATURE_PARAM(bool, kAutoInstallBraveHosted, &kExtensionsManifestV2, true);

bool IsSettingsBackupEnabled() {
  return base::FeatureList::IsEnabled(features::kExtensionsManifestV2) &&
         features::kBackupSettings.Get();
}

bool IsSettingsImportEnabled() {
  return IsSettingsBackupEnabled() && features::kImportSettingsOnInstall.Get();
}

bool IsExtensionReplacementEnabled() {
  return IsSettingsImportEnabled() && features::kAutoInstallBraveHosted.Get();
}

}  // namespace extensions_mv2::features
