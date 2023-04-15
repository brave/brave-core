/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_features.h"

namespace brave_ads {

BASE_FEATURE(kExclusionRulesFeature,
             "ExclusionRules",
             base::FEATURE_ENABLED_BY_DEFAULT);

bool IsExclusionRulesEnabled() {
  return base::FeatureList::IsEnabled(kExclusionRulesFeature);
}

}  // namespace brave_ads
