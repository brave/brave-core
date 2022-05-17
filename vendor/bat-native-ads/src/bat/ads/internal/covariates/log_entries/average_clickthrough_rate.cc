/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/average_clickthrough_rate.h"

#include <algorithm>

#include "base/strings/string_number_conversions.h"
#include "bat/ads/history_info.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/covariates/covariates_constants.h"
#include "bat/ads/internal/history/history.h"

namespace ads {

AverageClickthroughRate::AverageClickthroughRate(base::TimeDelta time_window)
    : time_window_(time_window) {}

AverageClickthroughRate::~AverageClickthroughRate() = default;

brave_federated::mojom::DataType AverageClickthroughRate::GetDataType() const {
  return brave_federated::mojom::DataType::kDouble;
}

brave_federated::mojom::CovariateType
AverageClickthroughRate::GetCovariateType() const {
  return brave_federated::mojom::CovariateType::kAverageClickthroughRate;
}

std::string AverageClickthroughRate::GetValue() const {
  const base::Time now = base::Time::Now();
  const base::Time from_time = now - time_window_;
  const base::Time to_time = now;

  const HistoryInfo history = history::Get(
      HistoryFilterType::kNone, HistorySortType::kNone, from_time, to_time);
  if (history.items.empty()) {
    return base::NumberToString(kCovariateMissingValue);
  }

  const int number_of_views =
      std::count_if(history.items.cbegin(), history.items.cend(),
                    [](const HistoryItemInfo& history_item) {
                      return history_item.ad_content.confirmation_type ==
                             ConfirmationType::kViewed;
                    });

  if (number_of_views == 0) {
    return base::NumberToString(kCovariateMissingValue);
  }

  const int number_of_clicks =
      std::count_if(history.items.cbegin(), history.items.cend(),
                    [](const HistoryItemInfo& history_item) {
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
