/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_OPERATIONAL_PATTERNS_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_OPERATIONAL_PATTERNS_FEATURES_H_

#include "base/feature_list.h"

namespace operational_patterns {
namespace features {

extern const base::Feature kUserOperationalPatterns;

bool IsOperationalPatternsEnabled();

int GetCollectionSlotSizeValue();
int GetSimulateLocalTrainingStepDurationValue();
int GetCollectionIdLifetime();

}  // namespace features
}  // namespace operational_patterns

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_OPERATIONAL_PATTERNS_FEATURES_H_
