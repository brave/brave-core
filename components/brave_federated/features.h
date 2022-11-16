/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_FEATURES_H_

#include "base/feature_list.h"

namespace brave_federated {

namespace features {

BASE_DECLARE_FEATURE(kFederatedLearning);

bool IsFederatedLearningEnabled();

// Operational Patterns
bool IsOperationalPatternsEnabled();
int GetCollectionIdLifetimeInSeconds();
int GetCollectionSlotSizeInSeconds();
int GetCollectionTimerIntervalInSeconds();
int GetMockTaskDurationInSeconds();
bool MockCollectionRequests();

}  // namespace features

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_FEATURES_H_
