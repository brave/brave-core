/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_EXTENSIONS_COMMON_EXTENSION_FEATURES_H_
#define BRAVE_CHROMIUM_SRC_EXTENSIONS_COMMON_EXTENSION_FEATURES_H_

#include <extensions/common/extension_features.h>  // IWYU pragma: export

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

// Adds Brave's MV2 extension features here so that we don't have to patch
// upstream GN files with dependencies.
namespace extensions_mv2::features {

BASE_DECLARE_FEATURE(kExtensionsManifestV2);
BASE_DECLARE_FEATURE_PARAM(bool, kExtensionsManifestV2BackupSettings);
BASE_DECLARE_FEATURE_PARAM(bool, kExtensionsManifestV2BImportSettingsOnInstall);
BASE_DECLARE_FEATURE_PARAM(bool, kExtensionsManifestV2AutoInstallBraveHosted);

bool IsSettingsBackupEnabled();
bool IsSettingsImportEnabled();
bool IsExtensionReplacementEnabled();

}  // namespace extensions_mv2::features

#endif  // BRAVE_CHROMIUM_SRC_EXTENSIONS_COMMON_EXTENSION_FEATURES_H_
