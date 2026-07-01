/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "extensions/browser/mv2_deprecation_impact_checker.h"

#include <algorithm>

#include "base/feature_list.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_features.h"

namespace {

bool IsBraveHostedExtension(const extensions::ExtensionId& extension_id) {
  // If our feature to support MV2 extensions is not enabled then Brave-hosted
  // extensions aren't used.
  if (!base::FeatureList::IsEnabled(
          extensions_mv2::features::kExtensionsManifestV2)) {
    return false;
  }

  // Check against Brave-hosted extension list.
  return std::ranges::contains(
      extensions_mv2::kPreconfiguredManifestV2Extensions, extension_id);
}

}  // namespace

#include <extensions/browser/mv2_deprecation_impact_checker.cc>
