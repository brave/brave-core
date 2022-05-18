/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/ad_notification_clicked.h"

#include "bat/ads/internal/base/string_util.h"
#include "brave/components/l10n/browser/locale_helper.h"

namespace ads {

AdNotificationClicked::AdNotificationClicked() = default;

AdNotificationClicked::~AdNotificationClicked() = default;

void AdNotificationClicked::SetClicked(const bool clicked) {
  clicked_ = clicked;
}

brave_federated::mojom::DataType AdNotificationClicked::GetDataType() const {
  return brave_federated::mojom::DataType::kBool;
}

brave_federated::mojom::CovariateType AdNotificationClicked::GetCovariateType()
    const {
  return brave_federated::mojom::CovariateType::kAdNotificationClicked;
}

std::string AdNotificationClicked::GetValue() const {
  return BoolToString(clicked_);
}

}  // namespace ads
