/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"

#include <extensions/common/extension_features.cc>

namespace extensions_features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kExtensionManifestV2DeprecationWarning, base::FEATURE_DISABLED_BY_DEFAULT},
    {kExtensionManifestV2Disabled, base::FEATURE_DISABLED_BY_DEFAULT},
    {kExtensionManifestV2Unsupported, base::FEATURE_DISABLED_BY_DEFAULT},
    {kExtensionsManifestV3Only, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace extensions_features
