/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/last_notification_ad_was_clicked.h"

#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/covariates/covariate_constants.h"
#include "bat/ads/internal/history/history_manager.h"

namespace ads {

namespace {

constexpr int kClickedValue = 1;
constexpr int kNotClickedValue = 0;
constexpr base::TimeDelta kTimeWindow = base::Days(7);

}  // namespace

brave_federated::mojom::DataType LastNotificationAdWasClicked::GetDataType()
    const {
  return brave_federated::mojom::DataType::kBool;
}

brave_federated::mojom::CovariateType LastNotificationAdWasClicked::GetType()
    const {
  return brave_federated::mojom::CovariateType::kLastNotificationAdWasClicked;
}

std::string LastNotificationAdWasClicked::GetValue() const {
  const base::Time now = base::Time::Now();
  const base::Time from_time = now - kTimeWindow;
  const base::Time to_time = now;

  const HistoryItemList history_items = HistoryManager::Get(
      HistoryFilterType::kNone, HistorySortType::kDescendingOrder, from_time,
      to_time);
  if (history_items.empty()) {
    return base::NumberToString(kCovariateMissingValue);
  }

  const HistoryItemInfo& history_item = history_items.front();
  if (history_item.ad_content.confirmation_type == ConfirmationType::kClicked) {
    return base::NumberToString(kClickedValue);
  }

  return base::NumberToString(kNotClickedValue);
}

}  // namespace ads
