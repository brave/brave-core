/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define IsOnDeviceModelAdaptationEnabled \
  IsOnDeviceModelAdaptationEnabled_ChromiumImpl

#include "src/components/optimization_guide/core/model_execution/model_execution_features.cc"
#undef IsOnDeviceModelAdaptationEnabled

namespace optimization_guide::features::internal {

bool IsOnDeviceModelAdaptationEnabled(ModelBasedCapabilityKey feature) {
  return false;
}

}  // namespace optimization_guide::features::internal
