/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define GetOptimizationTargetForFeature \
  GetOptimizationTargetForFeature_ChromiumImpl

#include <components/optimization_guide/core/model_execution/on_device_features.cc>

#undef GetOptimizationTargetForFeature

#include "components/optimization_guide/core/model_execution/on_device_features.h"

namespace optimization_guide {

COMPONENT_EXPORT(OPTIMIZATION_GUIDE_FEATURES)
proto::OptimizationTarget GetOptimizationTargetForFeature(
    mojom::OnDeviceFeature feature) {
  return proto::OPTIMIZATION_TARGET_UNKNOWN;
}

}  // namespace optimization_guide
