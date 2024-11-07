/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_FEATURES_H_
#define BRAVE_COMPONENTS_P3A_FEATURES_H_

#include "base/feature_list.h"
#include "brave/components/p3a/metric_log_type.h"

namespace p3a {
namespace features {

// See https://github.com/brave/brave-browser/issues/24338 for more info.
BASE_DECLARE_FEATURE(kConstellation);
// See https://github.com/brave/brave-browser/issues/31718 for more info.
BASE_DECLARE_FEATURE(kConstellationEnclaveAttestation);

// See https://github.com/brave/brave-browser/issues/34003 for more info.
// Disables JSON measurements for "typical" cadence.
BASE_DECLARE_FEATURE(kTypicalJSONDeprecation);
// Disables NTT JSON measurements.
BASE_DECLARE_FEATURE(kNTTJSONDeprecation);
// Disables JSON measurements for all other cadences.
BASE_DECLARE_FEATURE(kOtherJSONDeprecation);

// See https://github.com/brave/brave-browser/issues/35841 for more info.
BASE_DECLARE_FEATURE(kNebula);

bool IsConstellationEnabled();
bool IsConstellationEnclaveAttestationEnabled();
bool IsJSONDeprecated(MetricLogType log_type);
bool ShouldOnlyAllowNTTJSON();
bool IsNebulaEnabled();

}  // namespace features
}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_FEATURES_H_
