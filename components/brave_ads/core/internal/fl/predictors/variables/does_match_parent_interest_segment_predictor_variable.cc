/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/does_match_parent_interest_segment_predictor_variable.h"

#include <sstream>

namespace brave_ads {

DoesMatchParentInterestSegmentPredictorVariable::
    DoesMatchParentInterestSegmentPredictorVariable(const float value)
    : value_(value) {}

brave_federated::mojom::DataType
DoesMatchParentInterestSegmentPredictorVariable::GetDataType() const {
  return brave_federated::mojom::DataType::kString;
}

brave_federated::mojom::CovariateType
DoesMatchParentInterestSegmentPredictorVariable::GetType() const {
  return brave_federated::mojom::CovariateType::kDoesMatchParentInterestSegment;
}

std::string DoesMatchParentInterestSegmentPredictorVariable::GetValue() const {
  std::stringstream ss;
  ss << value_;
  return ss.str();
}

} // namespace brave_ads
