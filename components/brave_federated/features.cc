/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/features.h"

#include "base/metrics/field_trial_params.h"

namespace brave_federated {
namespace features {

namespace {

const char kFeatureName[] = "BraveFederatedLearning";

const char kFieldTrialParameterOperationalPatternsEnabled[] =
    "operational_patterns_enabled";
const bool kDefaultOperationalPatternsEnabled = false;

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

const base::Feature kFederatedLearning{kFeatureName,
                                       base::FEATURE_DISABLED_BY_DEFAULT};

bool IsFederatedLearningEnabled() {
  return base::FeatureList::IsEnabled(kFederatedLearning);
}

bool IsOperationalPatternsEnabled() {
  return GetFieldTrialParamByFeatureAsBool(
      kFederatedLearning, kFieldTrialParameterOperationalPatternsEnabled,
      kDefaultOperationalPatternsEnabled);
}

int GetCollectionSlotSizeValue() {
  return GetFieldTrialParamByFeatureAsInt(
      kFederatedLearning, kFieldTrialParameterCollectionSlotSizeInMinutes,
      kDefaultCollectionSlotSizeInMinutes);
}

int GetSimulateLocalTrainingStepDurationValue() {
  return GetFieldTrialParamByFeatureAsInt(
      kFederatedLearning,
      kFieldTrialParameterSimulateLocalTrainingStepDurationInMinutes,
      kDefaultSimulateLocalTrainingStepDurationInMinutes);
}

int GetCollectionIdLifetime() {
  return GetFieldTrialParamByFeatureAsInt(
      kFederatedLearning, kFieldTrialParameterCollectionIDLifetimeInDays,
      kDefaultCollectionIDLifetimeInDays);
}

}  // namespace features
}  // namespace brave_federated
