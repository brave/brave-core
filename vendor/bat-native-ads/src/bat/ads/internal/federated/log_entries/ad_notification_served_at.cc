/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/federated/log_entries/ad_notification_served_at.h"

#include "bat/ads/internal/federated/covariate_logs_util.h"
#include "bat/ads/internal/federated/covariates_constants.h"

namespace ads {

AdNotificationServedAt::AdNotificationServedAt() = default;

AdNotificationServedAt::~AdNotificationServedAt() = default;

void AdNotificationServedAt::SetTime(const base::Time time) {
  time_ = time;
}

brave_federated::mojom::DataType AdNotificationServedAt::GetDataType() const {
  return brave_federated::mojom::DataType::kDouble;
}

brave_federated::mojom::CovariateType AdNotificationServedAt::GetCovariateType()
    const {
  return brave_federated::mojom::CovariateType::kAdNotificationServedAt;
}

std::string AdNotificationServedAt::GetValue() const {
  if (time_.is_null()) {
    return ToString(kCovariateMissingValue);
  }

  return ToString(time_);
}

}  // namespace ads
