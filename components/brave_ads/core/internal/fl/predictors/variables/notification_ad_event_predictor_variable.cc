/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/notification_ad_event_predictor_variable.h"

#include <sstream>

#include "base/check.h"

namespace brave_ads {

NotificationAdEventPredictorVariable::NotificationAdEventPredictorVariable(
    const mojom::NotificationAdEventType event_type)
    : event_type_(event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type_));
}

brave_federated::mojom::DataType
NotificationAdEventPredictorVariable::GetDataType() const {
  return brave_federated::mojom::DataType::kString;
}

brave_federated::mojom::CovariateType
NotificationAdEventPredictorVariable::GetType() const {
  return brave_federated::mojom::CovariateType::kNotificationAdEvent;
}

std::string NotificationAdEventPredictorVariable::GetValue() const {
  std::stringstream ss;
  ss << event_type_;
  return ss.str();
}

}  // namespace brave_ads
