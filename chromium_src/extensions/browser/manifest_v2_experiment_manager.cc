/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "extensions/browser/manifest_v2_experiment_manager.h"

#include "base/feature_list.h"

// This override is used to allow using MV2 extensions based on Brave's
// extensions_mv2::features::kExtensionsManifestV2 feature flag.
#define BRAVE_SHOULD_DISABLE_LEGACY_EXTENSIONS                \
  if (base::FeatureList::IsEnabled(                           \
          extensions_mv2::features::kExtensionsManifestV2)) { \
    return false;                                             \
  }

#include <extensions/browser/manifest_v2_experiment_manager.cc>
#undef BRAVE_SHOULD_DISABLE_LEGACY_EXTENSIONS
