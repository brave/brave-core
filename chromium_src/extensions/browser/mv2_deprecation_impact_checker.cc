/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <string_view>
#include <vector>

#include "base/strings/string_util.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_features.h"
#include "extensions/common/extension_id.h"
#include "extensions/common/hashed_extension_id.h"
#include "extensions/common/manifest.h"

namespace {

std::string BuildBraveMV2ExceptionList() {
  std::vector<std::string> hashed_ids;
  hashed_ids.reserve(extensions_mv2::kPreconfiguredManifestV2Extensions.size());
  for (std::string_view id :
       extensions_mv2::kPreconfiguredManifestV2Extensions) {
    hashed_ids.push_back(
        extensions::HashedExtensionId(extensions::ExtensionId(id)).value());
  }
  return base::JoinString(hashed_ids, ",");
}

}  // namespace

// Combine the upstream MV2 exception list with all Brave-hosted MV2 extension
// hashes so that our extensions are always exempt from deprecation
#define kExtensionManifestV2ExceptionListParam         \
  kExtensionManifestV2ExceptionListParam.Get() + "," + \
      BuildBraveMV2ExceptionList();                    \
  extensions_features::kExtensionManifestV2ExceptionListParam

// We want to extend support for all MV2 extensions until August 31, 2026.
// This forces IsExtensionAffected(...) to return false.
#define IsComponentLocation(...) IsComponentLocation(__VA_ARGS__) || true

#include <extensions/browser/mv2_deprecation_impact_checker.cc>

#undef kExtensionManifestV2ExceptionListParam
#undef IsComponentLocation
