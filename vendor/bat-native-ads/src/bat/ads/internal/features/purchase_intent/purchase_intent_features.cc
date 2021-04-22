/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/purchase_intent/purchase_intent_features.h"

#include "base/metrics/field_trial_params.h"
#include "bat/ads/internal/ad_serving/ad_targeting/models/behavioral/purchase_intent/purchase_intent_model_values.h"

namespace ads {
namespace features {

namespace {
const char kFeatureName[] = "PurchaseIntent";
const char kFieldTrialParameterThreshold[] = "threshold";
const char kFieldTrialParameterTimeWindowInSeconds[] = "time_window_in_seconds";
const char kFieldTrialParameterResourceVersion[] =
    "purchase_intent_resource_version";
const int kDefaultResourceVersion = 1;
}  // namespace

const base::Feature kPurchaseIntent{kFeatureName,
                                    base::FEATURE_ENABLED_BY_DEFAULT};

bool IsPurchaseIntentEnabled() {
  return base::FeatureList::IsEnabled(kPurchaseIntent);
}

uint16_t GetPurchaseIntentThreshold() {
  return GetFieldTrialParamByFeatureAsInt(kPurchaseIntent,
                                          kFieldTrialParameterThreshold,
                                          ad_targeting::model::kThreshold);
}

int64_t GetPurchaseIntentTimeWindowInSeconds() {
  return GetFieldTrialParamByFeatureAsInt(
      kPurchaseIntent, kFieldTrialParameterTimeWindowInSeconds,
      ad_targeting::model::kTimeWindowInSeconds);
}

int GetPurchaseIntentResourceVersion() {
  return GetFieldTrialParamByFeatureAsInt(kPurchaseIntent,
                                          kFieldTrialParameterResourceVersion,
                                          kDefaultResourceVersion);
}

}  // namespace features
}  // namespace ads
