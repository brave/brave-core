/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_FEATURES_H_

#include <string>

#include "base/feature_list.h"

namespace brave_federated::features {

BASE_DECLARE_FEATURE(kFederatedLearning);

bool IsFederatedLearningEnabled();

// Federated Learning
int GetFederatedLearningUpdateCycleInSeconds();
std::string GetFederatedLearningTaskEndpoint();
std::string GetFederatedLearningResultsEndpoint();
uint32_t GetInitFederatedServiceWaitTimeInSeconds();

// Operational Patterns
bool IsOperationalPatternsEnabled();
int GetCollectionIdLifetimeInSeconds();
int GetCollectionSlotSizeInSeconds();
int GetCollectionTimerIntervalInSeconds();
int GetMockTaskDurationInSeconds();
bool MockCollectionRequests();

// Ad Timing Local Data Collection
bool IsAdTimingLocalDataCollectionEnabled();

}  // namespace brave_federated::features

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_FEATURES_H_
