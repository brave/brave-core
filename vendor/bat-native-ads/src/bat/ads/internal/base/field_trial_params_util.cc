/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/field_trial_params_util.h"

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "base/time/time_delta_from_string.h"
#include "bat/ads/internal/base/logging_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

std::string GetFieldTrialParamByFeatureAsString(
    const base::Feature& feature,
    const std::string& param_name,
    const std::string& default_value) {
  const std::string value =
      GetFieldTrialParamValueByFeature(feature, param_name);

  if (value.empty()) {
    return default_value;
  }

  return value;
}

base::TimeDelta GetFieldTrialParamByFeatureAsTimeDelta(
    const base::Feature& feature,
    const std::string& param_name,
    const base::TimeDelta default_value) {
  const std::string value =
      GetFieldTrialParamValueByFeature(feature, param_name);

  if (value.empty()) {
    return default_value;
  }

  absl::optional<base::TimeDelta> time_delta = base::TimeDeltaFromString(value);
  if (!time_delta.has_value()) {
    BLOG(1, "Failed to parse field trial param "
                << param_name << " with string value " << value
                << " under feature " << feature.name
                << " into a base::TimeDelta. Falling back to default value of "
                << default_value);

    return default_value;
  }

  return time_delta.value();
}

}  // namespace ads
