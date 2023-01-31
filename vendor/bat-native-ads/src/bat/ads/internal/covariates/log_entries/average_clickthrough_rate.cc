/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/average_clickthrough_rate.h"

#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/covariates/covariate_constants.h"
#include "bat/ads/internal/history/history_manager.h"

namespace ads {

AverageClickthroughRate::AverageClickthroughRate(base::TimeDelta time_window)
    : time_window_(time_window) {}

brave_federated::mojom::DataType AverageClickthroughRate::GetDataType() const {
  return brave_federated::mojom::DataType::kDouble;
}

brave_federated::mojom::CovariateType AverageClickthroughRate::GetType() const {
  return brave_federated::mojom::CovariateType::kAverageClickthroughRate;
}

std::string AverageClickthroughRate::GetValue() const {
  const base::Time now = base::Time::Now();
  const base::Time from_time = now - time_window_;
  const base::Time to_time = now;

  const HistoryItemList history_items = HistoryManager::Get(
      HistoryFilterType::kNone, HistorySortType::kNone, from_time, to_time);
  if (history_items.empty()) {
    return base::NumberToString(kCovariateMissingValue);
  }

  const int number_of_views = base::ranges::count_if(
      history_items, [](const HistoryItemInfo& history_item) {
        return history_item.ad_content.confirmation_type ==
               ConfirmationType::kViewed;
      });

  if (number_of_views == 0) {
    return base::NumberToString(kCovariateMissingValue);
  }

  const int number_of_clicks = base::ranges::count_if(
      history_items, [](const HistoryItemInfo& history_item) {
        return history_item.ad_content.confirmation_type ==
               ConfirmationType::kClicked;
      });

  if (number_of_clicks > number_of_views) {
    return base::NumberToString(kCovariateMissingValue);
  }

  const double clickthrough_rate =
      static_cast<double>(number_of_clicks) / number_of_views;
  DCHECK_GE(clickthrough_rate, 0.0);
  DCHECK_LE(clickthrough_rate, 1.0);

  return base::NumberToString(clickthrough_rate);
}

}  // namespace ads
