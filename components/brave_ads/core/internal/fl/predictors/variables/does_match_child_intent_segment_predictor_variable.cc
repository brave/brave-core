/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/does_match_child_intent_segment_predictor_variable.h"

#include <sstream>

namespace brave_ads {

DoesMatchChildIntentSegmentPredictorVariable::
    DoesMatchChildIntentSegmentPredictorVariable(const float value)
    : value_(value) {}

brave_federated::mojom::DataType
DoesMatchChildIntentSegmentPredictorVariable::GetDataType() const {
  return brave_federated::mojom::DataType::kString;
}

brave_federated::mojom::CovariateType
DoesMatchChildIntentSegmentPredictorVariable::GetType() const {
  return brave_federated::mojom::CovariateType::kDoesMatchChildIntentSegment;
}

std::string DoesMatchChildIntentSegmentPredictorVariable::GetValue() const {
  std::stringstream ss;
  ss << value_;
  return ss.str();
}

} // namespace brave_ads
