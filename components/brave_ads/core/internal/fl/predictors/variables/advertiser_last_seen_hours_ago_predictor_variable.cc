/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/advertiser_last_seen_hours_ago_predictor_variable.h"

#include <sstream>

namespace brave_ads {

AdvertiserLastSeenHoursAgoPredictorVariable::
    AdvertiserLastSeenHoursAgoPredictorVariable(const float value)
    : value_(value) {}

brave_federated::mojom::DataType
AdvertiserLastSeenHoursAgoPredictorVariable::GetDataType() const {
  return brave_federated::mojom::DataType::kString;
}

brave_federated::mojom::CovariateType
AdvertiserLastSeenHoursAgoPredictorVariable::GetType() const {
  return brave_federated::mojom::CovariateType::kAdvertiserLastSeenHoursAgo;
}

std::string AdvertiserLastSeenHoursAgoPredictorVariable::GetValue() const {
  std::stringstream ss;
  ss << value_;
  return ss.str();
}

} // namespace brave_ads
