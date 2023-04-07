/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_features.h"

#include "base/metrics/field_trial_params.h"

namespace brave_ads::targeting::features {

namespace {

constexpr char kEpsilonValueFieldTrialParamName[] = "epsilon_value";
constexpr double kEpsilonValueDefaultValue = 0.25;

}  // namespace

BASE_FEATURE(kEpsilonGreedyBandit,
             "EpsilonGreedyBandit",
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsEpsilonGreedyBanditEnabled() {
  return base::FeatureList::IsEnabled(kEpsilonGreedyBandit);
}

double GetEpsilonGreedyBanditEpsilonValue() {
  return GetFieldTrialParamByFeatureAsDouble(kEpsilonGreedyBandit,
                                             kEpsilonValueFieldTrialParamName,
                                             kEpsilonValueDefaultValue);
}

}  // namespace brave_ads::targeting::features
