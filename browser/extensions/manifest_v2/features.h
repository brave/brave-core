// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_EXTENSIONS_MANIFEST_V2_FEATURES_H_
#define BRAVE_BROWSER_EXTENSIONS_MANIFEST_V2_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace extensions_mv2::features {

BASE_DECLARE_FEATURE(kExtensionsManifestV2);
BASE_DECLARE_FEATURE_PARAM(bool, kExtensionsManifestV2BackupSettings);
BASE_DECLARE_FEATURE_PARAM(bool, kExtensionsManifestV2BImportSettingsOnInstall);
BASE_DECLARE_FEATURE_PARAM(bool, kExtensionsManifestV2AutoInstallBraveHosted);

}  // namespace extensions_mv2::features

#endif  // BRAVE_BROWSER_EXTENSIONS_MANIFEST_V2_FEATURES_H_
