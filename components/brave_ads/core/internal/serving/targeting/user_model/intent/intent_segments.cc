/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/intent/intent_segments.h"

#include "base/containers/extend.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/model/purchase_intent_model.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_feature.h"

namespace brave_ads {

SegmentList BuildIntentSegments() {
  SegmentList segments;

  if (IsPurchaseIntentFeatureEnabled()) {
    base::Extend(segments, GetPurchaseIntentSegments());
  }

  return segments;
}

}  // namespace brave_ads
