/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define GetOptimizationTargetForCapability \
  GetOptimizationTargetForCapability_ChromiumImpl

#include "src/components/optimization_guide/core/model_execution/model_execution_features.cc"
#undef GetOptimizationTargetForCapability

namespace optimization_guide::features::internal {

std::optional<proto::OptimizationTarget> GetOptimizationTargetForCapability(
    ModelBasedCapabilityKey feature_key) {
  return std::nullopt;
}

}  // namespace optimization_guide::features::internal
