/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/notification_ad_clicked.h"

#include "bat/ads/internal/base/string_util.h"
#include "brave/components/l10n/browser/locale_helper.h"

namespace ads {

NotificationAdClicked::NotificationAdClicked() = default;

NotificationAdClicked::~NotificationAdClicked() = default;

void NotificationAdClicked::SetClicked(const bool clicked) {
  clicked_ = clicked;
}

brave_federated::mojom::DataType NotificationAdClicked::GetDataType() const {
  return brave_federated::mojom::DataType::kBool;
}

brave_federated::mojom::CovariateType NotificationAdClicked::GetCovariateType()
    const {
  return brave_federated::mojom::CovariateType::kNotificationAdClicked;
}

std::string NotificationAdClicked::GetValue() const {
  return BoolToString(clicked_);
}

}  // namespace ads
