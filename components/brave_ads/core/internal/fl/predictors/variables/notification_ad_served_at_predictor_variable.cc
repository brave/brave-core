/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/notification_ad_served_at_predictor_variable.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/predictor_variable_constants.h"

namespace brave_ads {

NotificationAdServedAtPredictorVariable::
    NotificationAdServedAtPredictorVariable(const base::Time time)
    : time_(time) {}

brave_federated::mojom::DataType
NotificationAdServedAtPredictorVariable::GetDataType() const {
  return brave_federated::mojom::DataType::kDouble;
}

brave_federated::mojom::CovariateType
NotificationAdServedAtPredictorVariable::GetType() const {
  return brave_federated::mojom::CovariateType::kNotificationAdServedAt;
}

std::string NotificationAdServedAtPredictorVariable::GetValue() const {
  return base::NumberToString(
      time_.is_null() ? kPredictorVariableMissingValue
                      : time_.ToDeltaSinceWindowsEpoch().InMicroseconds());
}

}  // namespace brave_ads
