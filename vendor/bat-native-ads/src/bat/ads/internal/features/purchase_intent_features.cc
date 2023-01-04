/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/purchase_intent_features.h"

#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "bat/ads/internal/common/metrics/field_trial_params_util.h"

namespace ads::targeting::features {

namespace {

constexpr char kFeatureName[] = "PurchaseIntent";

constexpr char kFieldTrialParameterThreshold[] = "threshold";
constexpr uint16_t kDefaultThreshold = 3;

constexpr char kFieldTrialParameterTimeWindow[] = "time_window_in_seconds";
constexpr base::TimeDelta kDefaultTimeWindow = base::Days(7);

constexpr char kFieldTrialParameterResourceVersion[] =
    "purchase_intent_resource_version";

constexpr int kDefaultResourceVersion = 1;

}  // namespace

BASE_FEATURE(kPurchaseIntent, kFeatureName, base::FEATURE_ENABLED_BY_DEFAULT);

bool IsPurchaseIntentEnabled() {
  return base::FeatureList::IsEnabled(kPurchaseIntent);
}

uint16_t GetPurchaseIntentThreshold() {
  return GetFieldTrialParamByFeatureAsInt(
      kPurchaseIntent, kFieldTrialParameterThreshold, kDefaultThreshold);
}

base::TimeDelta GetPurchaseIntentTimeWindow() {
  return GetFieldTrialParamByFeatureAsTimeDelta(
      kPurchaseIntent, kFieldTrialParameterTimeWindow, kDefaultTimeWindow);
}

int GetPurchaseIntentResourceVersion() {
  return GetFieldTrialParamByFeatureAsInt(kPurchaseIntent,
                                          kFieldTrialParameterResourceVersion,
                                          kDefaultResourceVersion);
}

}  // namespace ads::targeting::features
