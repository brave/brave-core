/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/features.h"
#include "brave/components/p3a/metric_log_type.h"

namespace p3a {
namespace features {

BASE_FEATURE(kConstellation,
             "BraveP3AConstellation",
             base::FEATURE_ENABLED_BY_DEFAULT);
BASE_FEATURE(kConstellationEnclaveAttestation,
             "BraveP3AConstellationEnclaveAttestation",
             base::FEATURE_DISABLED_BY_DEFAULT);
BASE_FEATURE(kTypicalJSONDeprecation,
             "BraveP3ATypicalJSONDeprecation",
             base::FEATURE_DISABLED_BY_DEFAULT);
BASE_FEATURE(kOtherJSONDeprecation,
             "BraveP3AOtherJSONDeprecation",
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsConstellationEnabled() {
  return base::FeatureList::IsEnabled(features::kConstellation);
}

bool IsConstellationEnclaveAttestationEnabled() {
  return base::FeatureList::IsEnabled(
      features::kConstellationEnclaveAttestation);
}

bool IsJSONDeprecated(MetricLogType log_type) {
  if (log_type == MetricLogType::kTypical) {
    return base::FeatureList::IsEnabled(features::kTypicalJSONDeprecation);
  }
  return base::FeatureList::IsEnabled(features::kOtherJSONDeprecation);
}

}  // namespace features
}  // namespace p3a
