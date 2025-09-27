/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/manifest_v2_experiment_manager.h"

#include "brave/browser/extensions/manifest_v2/features.h"
#include "chrome/browser/extensions/mv2_experiment_stage.h"
#include "extensions/browser/extension_registry.h"

namespace {

void AdjustMV2ExperimentStage(
    extensions::MV2ExperimentStage& experiment_stage) {
  if (!base::FeatureList::IsEnabled(
          extensions_mv2::features::kExtensionsManifestV2)) {
    return;
  }
  if (experiment_stage != extensions::MV2ExperimentStage::kNone) {
    experiment_stage = extensions::MV2ExperimentStage::kWarning;
  }
}

}  // namespace

#define Observe(...)        \
  Observe(__VA_ARGS__);     \
  AdjustMV2ExperimentStage( \
      const_cast<extensions::MV2ExperimentStage&>(experiment_stage_))

#include <chrome/browser/extensions/manifest_v2_experiment_manager.cc>

#undef Observe
