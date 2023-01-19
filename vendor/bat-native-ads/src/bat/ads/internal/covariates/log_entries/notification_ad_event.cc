/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/notification_ad_event.h"

#include <sstream>

#include "base/check.h"

namespace ads {

void NotificationAdEvent::SetEventType(
    const mojom::NotificationAdEventType event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  event_type_ = event_type;
}

brave_federated::mojom::DataType NotificationAdEvent::GetDataType() const {
  return brave_federated::mojom::DataType::kString;
}

brave_federated::mojom::CovariateType NotificationAdEvent::GetType() const {
  return brave_federated::mojom::CovariateType::kNotificationAdEvent;
}

std::string NotificationAdEvent::GetValue() const {
  std::stringstream ss;
  ss << event_type_;
  return ss.str();
}

}  // namespace ads
