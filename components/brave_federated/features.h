/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_FEATURES_H_

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"

namespace brave_federated {

const char kFeatureName[] = "BraveFederated";

const char kDefaultFederatedLearningTaskEndpoint[] =
    "https://fl.brave.com/api/v0/fleet/pull-task-ins";
const char kDefaultFederatedLearningResultsEndpoint[] =
    "https://fl.brave.com/api/v0/fleet/push-task-res";

const bool kDefaultOperationalPatternsEnabled = false;
const bool kDefaultMockCollectionRequests = false;
const bool kDefaultAdTimingLocalDataCollectionEnabled = false;

BASE_DECLARE_FEATURE(kFederatedLearning);

bool IsFederatedLearningEnabled();

// Federated Learning
constexpr base::FeatureParam<std::string> kFederatedLearningTaskEndpoint(
    &kFederatedLearning,
    "federated_learning_task_endpoint",
    kDefaultFederatedLearningTaskEndpoint);

constexpr base::FeatureParam<std::string> kFederatedLearningResultsEndpoint(
    &kFederatedLearning,
    "federated_learning_results_endpoint",
    kDefaultFederatedLearningResultsEndpoint);

constexpr base::FeatureParam<base::TimeDelta>
    kInitFederatedServiceWaitTimeInSeconds(
        &kFederatedLearning,
        "init_federated_service_wait_time_in_seconds",
        base::Seconds(30));

constexpr base::FeatureParam<base::TimeDelta>
    kFederatedLearningUpdateCycleInSeconds(
        &kFederatedLearning,
        "federated_learning_update_cycle_in_seconds",
        base::Seconds(5 * base::Time::kSecondsPerMinute));

// Operational Patterns
constexpr base::FeatureParam<bool> kOperationalPatternsEnabled(
    &kFederatedLearning,
    "operational_patterns_enabled",
    kDefaultOperationalPatternsEnabled);

constexpr base::FeatureParam<base::TimeDelta> kCollectionIdLifetimeInSeconds(
    &kFederatedLearning,
    "collection_id_lifetime_in_seconds",
    base::Seconds(1 * base::Time::kHoursPerDay * base::Time::kMinutesPerHour *
                  base::Time::kSecondsPerMinute));

constexpr base::FeatureParam<base::TimeDelta> kCollectionSlotSizeInSeconds(
    &kFederatedLearning,
    "collection_slot_size_in_seconds",
    base::Seconds(30 * base::Time::kSecondsPerMinute));

constexpr base::FeatureParam<base::TimeDelta> kCollectionTimerIntervalInSeconds(
    &kFederatedLearning,
    "collection_timer_interval_in_seconds",
    base::Seconds(1 * base::Time::kSecondsPerMinute));

constexpr base::FeatureParam<base::TimeDelta> kMockTaskDurationInSeconds(
    &kFederatedLearning,
    "mock_task_duration_in_seconds",
    base::Seconds(2 * base::Time::kSecondsPerMinute));

constexpr base::FeatureParam<bool> kMockCollectionRequests(
    &kFederatedLearning,
    "mock_collection_requests",
    kDefaultMockCollectionRequests);

// Ad Timing Local Data Collection
constexpr base::FeatureParam<bool> kAdTimingLocalDataCollectionEnabled(
    &kFederatedLearning,
    "ad_timing_local_data_collection_enabled",
    kDefaultAdTimingLocalDataCollectionEnabled);

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_FEATURES_H_
