/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/creative_ad_priority_predictor_variable.h"

#include <sstream>

namespace brave_ads {

CreativeAdPriorityPredictorVariable::CreativeAdPriorityPredictorVariable(
    const float value)
    : value_(value) {}

brave_federated::mojom::DataType
CreativeAdPriorityPredictorVariable::GetDataType() const {
  return brave_federated::mojom::DataType::kString;
}

brave_federated::mojom::CovariateType
CreativeAdPriorityPredictorVariable::GetType() const {
  return brave_federated::mojom::CovariateType::kCreativeAdPriority;
}

std::string CreativeAdPriorityPredictorVariable::GetValue() const {
  std::stringstream ss;
  ss << value_;
  return ss.str();
}

} // namespace brave_ads
