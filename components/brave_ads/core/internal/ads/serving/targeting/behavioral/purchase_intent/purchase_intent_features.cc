/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/targeting/behavioral/purchase_intent/purchase_intent_features.h"

namespace brave_ads::targeting {

BASE_FEATURE(kPurchaseIntentFeature,
             "PurchaseIntent",
             base::FEATURE_ENABLED_BY_DEFAULT);

bool IsPurchaseIntentEnabled() {
  return base::FeatureList::IsEnabled(kPurchaseIntentFeature);
}

}  // namespace brave_ads::targeting
