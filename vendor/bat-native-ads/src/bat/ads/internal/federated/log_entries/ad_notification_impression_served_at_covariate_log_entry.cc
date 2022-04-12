/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/federated/log_entries/ad_notification_impression_served_at_covariate_log_entry.h"

#include "bat/ads/internal/federated/covariate_logs_util.h"

namespace ads {

AdNotificationImpressionServedAtCovariateLogEntry::
    AdNotificationImpressionServedAtCovariateLogEntry() = default;

AdNotificationImpressionServedAtCovariateLogEntry::
    ~AdNotificationImpressionServedAtCovariateLogEntry() = default;

void AdNotificationImpressionServedAtCovariateLogEntry::SetLastImpressionAt(
    const base::Time time) {
  impression_served_at_ = time;
}

brave_federated::mojom::DataType
AdNotificationImpressionServedAtCovariateLogEntry::GetDataType() const {
  return brave_federated::mojom::DataType::kDouble;
}

brave_federated::mojom::CovariateType
AdNotificationImpressionServedAtCovariateLogEntry::GetCovariateType() const {
  return brave_federated::mojom::CovariateType::
      kAdNotificationImpressionServedAt;
}

std::string AdNotificationImpressionServedAtCovariateLogEntry::GetValue()
    const {
  return ToString(impression_served_at_);
}

}  // namespace ads
