/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/features_util.h"

#include "base/optional.h"
#include "bat/ads/internal/logging.h"

namespace ads {

std::string GetFieldTrialParamByFeatureAsString(
    const base::Feature& feature,
    const std::string& param_name,
    const std::string& default_value) {
  const std::string value_as_string =
      GetFieldTrialParamValueByFeature(feature, param_name);

  if (value_as_string.empty()) {
    return default_value;
  }

  return value_as_string;
}

base::TimeDelta GetFieldTrialParamByFeatureAsTimeDelta(
    const base::Feature& feature,
    const std::string& param_name,
    const base::TimeDelta& default_value) {
  const std::string value_as_string =
      GetFieldTrialParamValueByFeature(feature, param_name);

  if (value_as_string.empty()) {
    return default_value;
  }

  base::Optional<base::TimeDelta> time_delta =
      base::TimeDelta::FromString(value_as_string);
  if (!time_delta.has_value()) {
    BLOG(1, "Failed to parse field trial param "
                << param_name << " with string value " << value_as_string
                << " under feature " << feature.name
                << " into a base::TimeDelta. Falling back to default value of "
                << default_value);

    return default_value;
  }

  return time_delta.value();
}

}  // namespace ads
