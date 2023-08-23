/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/average_clickthrough_rate_predictor_variable.h"

#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/predictor_variable_constants.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/public/history/history_item_info.h"

namespace brave_ads {

AverageClickthroughRatePredictorVariable::
    AverageClickthroughRatePredictorVariable(base::TimeDelta time_window)
    : time_window_(time_window) {}

brave_federated::mojom::DataType
AverageClickthroughRatePredictorVariable::GetDataType() const {
  return brave_federated::mojom::DataType::kDouble;
}

brave_federated::mojom::CovariateType
AverageClickthroughRatePredictorVariable::GetType() const {
  return brave_federated::mojom::CovariateType::kAverageClickthroughRate;
}

std::string AverageClickthroughRatePredictorVariable::GetValue() const {
  const base::Time now = base::Time::Now();
  const base::Time from_time = now - time_window_;
  const base::Time to_time = now;

  const HistoryItemList history_items = HistoryManager::Get(
      HistoryFilterType::kNone, HistorySortType::kNone, from_time, to_time);
  if (history_items.empty()) {
    return base::NumberToString(kPredictorVariableMissingValue);
  }

  const size_t view_count = base::ranges::count_if(
      history_items, [](const HistoryItemInfo& history_item) {
        return history_item.ad_content.confirmation_type ==
               ConfirmationType::kViewed;
      });

  if (view_count == 0) {
    return base::NumberToString(kPredictorVariableMissingValue);
  }

  const size_t click_count = base::ranges::count_if(
      history_items, [](const HistoryItemInfo& history_item) {
        return history_item.ad_content.confirmation_type ==
               ConfirmationType::kClicked;
      });

  if (click_count > view_count) {
    return base::NumberToString(kPredictorVariableMissingValue);
  }

  const double clickthrough_rate =
      static_cast<double>(click_count) / static_cast<double>(view_count);
  CHECK_GE(clickthrough_rate, 0.0);
  CHECK_LE(clickthrough_rate, 1.0);

  return base::NumberToString(clickthrough_rate);
}

}  // namespace brave_ads
