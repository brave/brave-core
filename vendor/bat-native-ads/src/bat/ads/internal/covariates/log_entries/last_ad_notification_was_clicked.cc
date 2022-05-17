/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/last_ad_notification_was_clicked.h"

#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "bat/ads/history_info.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/covariates/covariates_constants.h"
#include "bat/ads/internal/history/history.h"

namespace ads {

namespace {

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
  const base::Time now = base::Time::Now();
  const base::Time from_time = now - kTimeWindow;
  const base::Time to_time = now;

  const HistoryInfo history =
      history::Get(HistoryFilterType::kNone, HistorySortType::kDescendingOrder,
                   from_time, to_time);
  if (history.items.empty()) {
    return base::NumberToString(kCovariateMissingValue);
  }

  const HistoryItemInfo& history_item = history.items.front();
  if (history_item.ad_content.confirmation_type == ConfirmationType::kClicked) {
    return base::NumberToString(kClickedValue);
  }

  return base::NumberToString(kNotClickedValue);
}

}  // namespace ads
