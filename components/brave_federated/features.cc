/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/features.h"

#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"

namespace brave_federated::features {

namespace {

constexpr char kFieldTrialParameterOperationalPatternsEnabled[] =
    "operational_patterns_enabled";
constexpr bool kDefaultOperationalPatternsEnabled = false;

constexpr char kFieldTrialParameterCollectionIDLifetimeInSeconds[] =
    "collection_id_lifetime_in_seconds";
constexpr int kDefaultCollectionIdLifetimeInSeconds =
    1 * base::Time::kHoursPerDay * base::Time::kMinutesPerHour *
    base::Time::kSecondsPerMinute;

constexpr char kFieldTrialParameterCollectionSlotSizeInSeconds[] =
    "collection_slot_size_in_seconds";
constexpr int kDefaultCollectionSlotSizeInSeconds =
    30 * base::Time::kSecondsPerMinute;

constexpr char kFieldTrialParameterCollectionTimerIntervalInSeconds[] =
    "collection_timer_interval_in_seconds";
constexpr int kDefaultCollectionTimerIntervalInSeconds =
    1 * base::Time::kSecondsPerMinute;

constexpr char kFieldTrialParameterMockTaskDurationInSeconds[] =
    "mock_task_duration_in_seconds";
constexpr int kDefaultMockTaskDurationInSeconds =
    2 * base::Time::kSecondsPerMinute;

constexpr char kFieldTrialParameterMockCollectionRequests[] =
    "mock_collection_requests";
constexpr bool kDefaultMockCollectionRequests = false;

}  // namespace

BASE_FEATURE(kFederatedLearning,
             "BraveFederated",
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsFederatedLearningEnabled() {
  return base::FeatureList::IsEnabled(kFederatedLearning);
}

bool IsOperationalPatternsEnabled() {
  return GetFieldTrialParamByFeatureAsBool(
      kFederatedLearning, kFieldTrialParameterOperationalPatternsEnabled,
      kDefaultOperationalPatternsEnabled);
}

int GetCollectionIdLifetimeInSeconds() {
  return GetFieldTrialParamByFeatureAsInt(
      kFederatedLearning, kFieldTrialParameterCollectionIDLifetimeInSeconds,
      kDefaultCollectionIdLifetimeInSeconds);
}

int GetCollectionSlotSizeInSeconds() {
  return GetFieldTrialParamByFeatureAsInt(
      kFederatedLearning, kFieldTrialParameterCollectionSlotSizeInSeconds,
      kDefaultCollectionSlotSizeInSeconds);
}

int GetCollectionTimerIntervalInSeconds() {
  return GetFieldTrialParamByFeatureAsInt(
      kFederatedLearning, kFieldTrialParameterCollectionTimerIntervalInSeconds,
      kDefaultCollectionTimerIntervalInSeconds);
}

int GetMockTaskDurationInSeconds() {
  return GetFieldTrialParamByFeatureAsInt(
      kFederatedLearning, kFieldTrialParameterMockTaskDurationInSeconds,
      kDefaultMockTaskDurationInSeconds);
}

bool MockCollectionRequests() {
  return GetFieldTrialParamByFeatureAsBool(
      kFederatedLearning, kFieldTrialParameterMockCollectionRequests,
      kDefaultMockCollectionRequests);
}

}  // namespace brave_federated::features
