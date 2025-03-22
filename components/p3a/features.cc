/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/features.h"
#include "brave/components/p3a/metric_log_type.h"

namespace p3a::features {

// Report answers to P3A questions encrypted to the STAR/Constellation
// threshold aggregation scheme.
BASE_FEATURE(kConstellation,
             "BraveP3AConstellation",
             base::FEATURE_ENABLED_BY_DEFAULT);
// Verify Constellation randomness server secure enclave certificate.
BASE_FEATURE(kConstellationEnclaveAttestation,
             "BraveP3AConstellationEnclaveAttestationV2",
             base::FEATURE_DISABLED_BY_DEFAULT);
// Disable reporting answers over direct https+json
// for typical (weekly) cadence P3A questions.
BASE_FEATURE(kTypicalJSONDeprecation,
             "BraveP3ATypicalJSONDeprecation",
             base::FEATURE_DISABLED_BY_DEFAULT);
// Disable reporting answers over direct https+json
// for other (daily or monthly) cadence P3A questions.
BASE_FEATURE(kOtherJSONDeprecation,
             "BraveP3AOtherJSONDeprecation",
             base::FEATURE_DISABLED_BY_DEFAULT);
// Report P3A responses with "Nebula" differential privacy
// sampling enabled. See https://github.com/brave/brave-browser/issues/35841
BASE_FEATURE(kNebula,
             "BraveP3ADifferentialSampling",
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

bool IsNebulaEnabled() {
  return base::FeatureList::IsEnabled(features::kNebula);
}

}  // namespace p3a::features
