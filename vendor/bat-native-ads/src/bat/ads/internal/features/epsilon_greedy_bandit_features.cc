/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/epsilon_greedy_bandit_features.h"

#include "base/metrics/field_trial_params.h"

namespace ads::targeting::features {

namespace {

constexpr char kFeatureName[] = "EpsilonGreedyBandit";

constexpr char kFieldTrialParameterEpsilonValue[] = "epsilon_value";
constexpr double kDefaultEpsilonValue = 0.25;

}  // namespace

BASE_FEATURE(kEpsilonGreedyBandit,
             kFeatureName,
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsEpsilonGreedyBanditEnabled() {
  return base::FeatureList::IsEnabled(kEpsilonGreedyBandit);
}

double GetEpsilonGreedyBanditEpsilonValue() {
  return GetFieldTrialParamByFeatureAsDouble(kEpsilonGreedyBandit,
                                             kFieldTrialParameterEpsilonValue,
                                             kDefaultEpsilonValue);
}

}  // namespace ads::targeting::features
