/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated_learning/brave_federated_learning_features.h"

#include "base/metrics/field_trial_params.h"

namespace brave {
namespace federated_learning {
namespace features {

namespace {

const char kFeatureName[] = "FederatedLearning";

}  // namespace

const base::Feature kFederatedLearning{kFeatureName,
                                       base::FEATURE_DISABLED_BY_DEFAULT};

bool IsFederatedLearningEnabled() {
  return base::FeatureList::IsEnabled(kFederatedLearning);
}

}  // namespace features
}  // namespace federated_learning
}  // namespace brave
