/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_OPERATIONAL_PROFILING_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_OPERATIONAL_PROFILING_FEATURES_H_

#include "base/feature_list.h"

namespace operational_profiling {
namespace features {

extern const base::Feature kUserOperationalProfiling;

bool IsOperationalProfilingEnabled();

int GetCollectionSlotSizeValue();
int GetSimulateLocalTrainingStepDurationValue();
int GetCollectionIdLifetime();

}  // namespace features
}  // namespace operational_profiling

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_OPERATIONAL_PROFILING_FEATURES_H_
