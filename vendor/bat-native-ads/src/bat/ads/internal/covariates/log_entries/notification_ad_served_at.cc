/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/notification_ad_served_at.h"

#include "base/strings/string_number_conversions.h"
#include "bat/ads/internal/covariates/covariate_constants.h"

namespace ads {

void NotificationAdServedAt::SetTime(const base::Time time) {
  time_ = time;
}

brave_federated::mojom::DataType NotificationAdServedAt::GetDataType() const {
  return brave_federated::mojom::DataType::kDouble;
}

brave_federated::mojom::CovariateType NotificationAdServedAt::GetType() const {
  return brave_federated::mojom::CovariateType::kNotificationAdServedAt;
}

std::string NotificationAdServedAt::GetValue() const {
  if (time_.is_null()) {
    return base::NumberToString(kCovariateMissingValue);
  }

  return base::NumberToString(time_.ToDoubleT());
}

}  // namespace ads
