/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated_learning/brave_operational_patterns_features.h"

#include "base/metrics/field_trial_params.h"

namespace operational_patterns {
namespace features {

namespace {
const char kFeatureName[] = "FederatedLearningOperationalPatterns";

const char kFieldTrialParameterCollectionSlotSizeInMinutes[] =
    "collection_slot_size_in_minutes";
const int kDefaultCollectionSlotSizeInMinutes = 30;

const char kFieldTrialParameterSimulateLocalTrainingStepDurationInMinutes[] =
    "simulate_local_training_step_duration_in_minutes";
const int kDefaultSimulateLocalTrainingStepDurationInMinutes = 5;

const char kFieldTrialParameterCollectionIDLifetimeInDays[] =
    "collection_id_lifetime_in_days";
const int kDefaultCollectionIDLifetimeInDays = 1;

}  // namespace

const base::Feature kUserOperationalPatterns{kFeatureName,
                                             base::FEATURE_DISABLED_BY_DEFAULT};

bool IsOperationalPatternsEnabled() {
  return base::FeatureList::IsEnabled(kUserOperationalPatterns);
}

int GetCollectionSlotSizeValue() {
  return GetFieldTrialParamByFeatureAsInt(
      kUserOperationalPatterns, kFieldTrialParameterCollectionSlotSizeInMinutes,
      kDefaultCollectionSlotSizeInMinutes);
}

int GetSimulateLocalTrainingStepDurationValue() {
  return GetFieldTrialParamByFeatureAsInt(
      kUserOperationalPatterns,
      kFieldTrialParameterSimulateLocalTrainingStepDurationInMinutes,
      kDefaultSimulateLocalTrainingStepDurationInMinutes);
}

int GetCollectionIdLifetime() {
  return GetFieldTrialParamByFeatureAsInt(
      kUserOperationalPatterns, kFieldTrialParameterCollectionIDLifetimeInDays,
      kDefaultCollectionIDLifetimeInDays);
}

}  // namespace features
}  // namespace operational_patterns
