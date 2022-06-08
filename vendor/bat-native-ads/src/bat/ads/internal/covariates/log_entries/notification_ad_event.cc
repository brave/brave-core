/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/notification_ad_event.h"

#include "bat/ads/internal/base/strings/string_conversions_util.h"
#include "brave/components/l10n/browser/locale_helper.h"

namespace ads {

NotificationAdEvent::NotificationAdEvent() = default;

NotificationAdEvent::~NotificationAdEvent() = default;

void NotificationAdEvent::SetEvent(const Event event) {
  event_ = event;
}

brave_federated::mojom::DataType NotificationAdEvent::GetDataType() const {
  return brave_federated::mojom::DataType::kBool;
}

brave_federated::mojom::CovariateType NotificationAdEvent::GetType() const {
  return brave_federated::mojom::CovariateType::kNotificationAdEvent;
}

std::string NotificationAdEvent::GetValue() const {
  if (event_ == kClicked) {
    return "clicked";
  } else if (event_ == kDismissed) {
    return "dismissed";
  }

  return "timedOut";
}

}  // namespace ads
