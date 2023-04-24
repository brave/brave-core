/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/time_since_last_user_activity_event_predictor_variable.h"

#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/predictor_variable_constants.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_manager.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_util.h"

namespace brave_ads {

namespace {
constexpr base::TimeDelta kTimeWindow = base::Minutes(30);
}  // namespace

TimeSinceLastUserActivityEventPredictorVariable::
    TimeSinceLastUserActivityEventPredictorVariable(
        UserActivityEventType event_type,
        brave_federated::mojom::CovariateType predictor_type)
    : event_type_(event_type), predictor_type_(predictor_type) {}

brave_federated::mojom::DataType
TimeSinceLastUserActivityEventPredictorVariable::GetDataType() const {
  return brave_federated::mojom::DataType::kInt;
}

brave_federated::mojom::CovariateType
TimeSinceLastUserActivityEventPredictorVariable::GetType() const {
  return predictor_type_;
}

std::string TimeSinceLastUserActivityEventPredictorVariable::GetValue() const {
  const UserActivityEventList events =
      UserActivityManager::GetInstance().GetHistoryForTimeWindow(kTimeWindow);

  const base::TimeDelta time_delta =
      GetTimeSinceLastUserActivityEvent(events, event_type_);

  return base::NumberToString(time_delta.is_zero()
                                  ? kPredictorVariableMissingValue
                                  : time_delta.InSeconds());
}

}  // namespace brave_ads
