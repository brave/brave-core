/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/targeting/behavioral/purchase_intent/purchase_intent_features.h"

#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/metrics/field_trial_params_util.h"

namespace brave_ads::targeting::features {

namespace {

constexpr char kResourceVersionFieldTrialParamName[] =
    "purchase_intent_resource_version";
constexpr int kResourceVersionDefaultValue = 1;

constexpr char kThresholdFieldTrialParamName[] = "threshold";
constexpr uint16_t kThresholdDefaultValue = 3;

constexpr char kTimeWindowFieldTrialParamName[] = "time_window_in_seconds";
constexpr base::TimeDelta kTimeWindowDefaultValue = base::Days(7);

}  // namespace

BASE_FEATURE(kPurchaseIntent,
             "PurchaseIntent",
             base::FEATURE_ENABLED_BY_DEFAULT);

bool IsPurchaseIntentEnabled() {
  return base::FeatureList::IsEnabled(kPurchaseIntent);
}

int GetPurchaseIntentResourceVersion() {
  return GetFieldTrialParamByFeatureAsInt(kPurchaseIntent,
                                          kResourceVersionFieldTrialParamName,
                                          kResourceVersionDefaultValue);
}

uint16_t GetPurchaseIntentThreshold() {
  return GetFieldTrialParamByFeatureAsInt(
      kPurchaseIntent, kThresholdFieldTrialParamName, kThresholdDefaultValue);
}

base::TimeDelta GetPurchaseIntentTimeWindow() {
  return GetFieldTrialParamByFeatureAsTimeDelta(
      kPurchaseIntent, kTimeWindowFieldTrialParamName, kTimeWindowDefaultValue);
}

}  // namespace brave_ads::targeting::features
