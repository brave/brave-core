/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/number_of_user_activity_events_predictor_variable.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_manager.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_util.h"

namespace brave_ads {

NumberOfUserActivityEventsPredictorVariable::
    NumberOfUserActivityEventsPredictorVariable(
        UserActivityEventType event_type,
        brave_federated::mojom::CovariateType predictor_type)
    : event_type_(event_type), predictor_type_(predictor_type) {}

brave_federated::mojom::DataType
NumberOfUserActivityEventsPredictorVariable::GetDataType() const {
  return brave_federated::mojom::DataType::kInt;
}

brave_federated::mojom::CovariateType
NumberOfUserActivityEventsPredictorVariable::GetType() const {
  return predictor_type_;
}

std::string NumberOfUserActivityEventsPredictorVariable::GetValue() const {
  const UserActivityEventList events =
      UserActivityManager::GetInstance().GetHistoryForTimeWindow(
          base::Minutes(30));

  return base::NumberToString(
      GetNumberOfUserActivityEvents(events, event_type_));
}

}  // namespace brave_ads
