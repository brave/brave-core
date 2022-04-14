/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/federated/log_entries/last_ad_notification_was_clicked.h"

#include "base/time/time.h"
#include "bat/ads/ad_history_info.h"
#include "bat/ads/ads_history_info.h"
#include "bat/ads/internal/ads_history/ads_history.h"
#include "bat/ads/internal/federated/covariate_logs_util.h"

namespace ads {

namespace {

constexpr int kMissingValue = -1;
constexpr int kClickedValue = 1;
constexpr int kNotClickedValue = 0;
constexpr base::TimeDelta kTimeWindow = base::Days(7);

}  // namespace

LastAdNotificationWasClicked::LastAdNotificationWasClicked() = default;

LastAdNotificationWasClicked::~LastAdNotificationWasClicked() = default;

brave_federated::mojom::DataType LastAdNotificationWasClicked::GetDataType()
    const {
  return brave_federated::mojom::DataType::kBool;
}

brave_federated::mojom::CovariateType
LastAdNotificationWasClicked::GetCovariateType() const {
  return brave_federated::mojom::CovariateType::kLastAdNotificationWasClicked;
}

std::string LastAdNotificationWasClicked::GetValue() const {
  const AdsHistoryFilterType filter_type = AdsHistoryFilterType::kNone;
  const AdsHistorySortType sort_type = AdsHistorySortType::kDescendingOrder;
  const base::Time now = base::Time::Now();
  const base::Time from_time = now - kTimeWindow;
  const base::Time to_time = now;
  const AdsHistoryInfo history =
      history::Get(filter_type, sort_type, from_time, to_time);
  if (history.items.empty()) {
    return ToString(kMissingValue);
  }

  const AdHistoryInfo& ad = history.items.front();
  if (ad.ad_content.confirmation_type == ConfirmationType::kClicked) {
    return ToString(kClickedValue);
  }

  return ToString(kNotClickedValue);
}

}  // namespace ads
