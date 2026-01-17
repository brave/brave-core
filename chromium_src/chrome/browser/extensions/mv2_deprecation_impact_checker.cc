/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/extension_management.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_features.h"

// Combine the upstream MV2 exception list with all Brave-hosted MV2 extension
// hashes so that our extensions are always exempt from deprecation
#define kExtensionManifestV2ExceptionListParam         \
  kExtensionManifestV2ExceptionListParam.Get() + "," + \
      extensions_mv2::BuildBraveMV2ExceptionList();    \
  extensions_features::kExtensionManifestV2ExceptionListParam

#include <chrome/browser/extensions/mv2_deprecation_impact_checker.cc>

#undef kExtensionManifestV2ExceptionListParam
