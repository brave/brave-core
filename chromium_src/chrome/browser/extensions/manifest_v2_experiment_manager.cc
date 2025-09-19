/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/manifest_v2_experiment_manager.h"

#include "brave/browser/extensions/manifest_v2/features.h"
#include "chrome/browser/extensions/mv2_experiment_stage.h"
#include "extensions/browser/extension_registry.h"

namespace {

extensions::MV2ExperimentStage AdjustMV2ExperimentStage(
    extensions::MV2ExperimentStage experiment_stage) {
  if (!base::FeatureList::IsEnabled(
          extensions_mv2::features::kExtensionsManifestV2)) {
    return experiment_stage;
  }
  // We relax any stage into the Warning stage.
  return extensions::MV2ExperimentStage::kWarning;
}

}  // namespace

#define experiment_stage_(...) \
  experiment_stage_(AdjustMV2ExperimentStage(__VA_ARGS__))

#include <chrome/browser/extensions/manifest_v2_experiment_manager.cc>

#undef experiment_stage_
