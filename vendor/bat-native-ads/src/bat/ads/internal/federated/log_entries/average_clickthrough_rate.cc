/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/federated/log_entries/average_clickthrough_rate.h"

#include "bat/ads/ad_history_info.h"
#include "bat/ads/ads_history_info.h"
#include "bat/ads/internal/ads_history/ads_history.h"
#include "bat/ads/internal/federated/covariate_logs_util.h"

namespace ads {

namespace {
constexpr int kMissingValue = -1;
}  // namespace

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
  const AdsHistoryFilterType filter_type = AdsHistoryFilterType::kNone;
  const AdsHistorySortType sort_type = AdsHistorySortType::kNone;
  const base::Time from = base::Time::Now() - time_window_;
  const base::Time to = base::Time::Now();
  const AdsHistoryInfo history = history::Get(filter_type, sort_type, from, to);
  if (history.items.empty()) {
    return ToString(kMissingValue);
  }

  const int number_of_views = std::count_if(
      history.items.cbegin(), history.items.cend(),
      [](const AdHistoryInfo& info) {
        return info.ad_content.confirmation_type == ConfirmationType::kViewed;
      });

  if (number_of_views == 0) {
    return ToString(kMissingValue);
  }

  const int number_of_clicks = std::count_if(
      history.items.cbegin(), history.items.cend(),
      [](const AdHistoryInfo& info) {
        return info.ad_content.confirmation_type == ConfirmationType::kClicked;
      });

  if (number_of_clicks > number_of_views) {
    return ToString(kMissingValue);
  }

  const double clickthrough_rate =
      static_cast<double>(number_of_clicks) / number_of_views;
  DCHECK_GE(clickthrough_rate, 0.0);
  DCHECK_LE(clickthrough_rate, 1.0);

  return ToString(clickthrough_rate);
}

}  // namespace ads
