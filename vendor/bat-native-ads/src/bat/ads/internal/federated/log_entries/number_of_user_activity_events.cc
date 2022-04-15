/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/federated/log_entries/number_of_user_activity_events.h"

#include "bat/ads/internal/federated/covariate_logs_util.h"
#include "bat/ads/internal/user_activity/user_activity.h"
#include "bat/ads/internal/user_activity/user_activity_util.h"

namespace ads {

NumberOfUserActivityEvents::NumberOfUserActivityEvents(
    UserActivityEventType event_type,
    brave_federated::mojom::CovariateType covariate_type)
    : event_type_(event_type), covariate_type_(covariate_type) {}

NumberOfUserActivityEvents::~NumberOfUserActivityEvents() = default;

brave_federated::mojom::DataType NumberOfUserActivityEvents::GetDataType()
    const {
  return brave_federated::mojom::DataType::kInt;
}

brave_federated::mojom::CovariateType
NumberOfUserActivityEvents::GetCovariateType() const {
  return covariate_type_;
}

std::string NumberOfUserActivityEvents::GetValue() const {
  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(base::Minutes(30));

  return ToString(GetNumberOfUserActivityEvents(events, event_type_));
}

}  // namespace ads
