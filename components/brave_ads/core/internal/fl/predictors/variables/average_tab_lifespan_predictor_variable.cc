/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/average_tab_lifespan_predictor_variable.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_manager.h"

#include "brave/components/brave_ads/core/internal/common/logging_util.h"

#include <map>

namespace brave_ads {

AverageTabLifespanPredictorVariable::AverageTabLifespanPredictorVariable() =
    default;

brave_federated::mojom::DataType
AverageTabLifespanPredictorVariable::GetDataType() const {
  return brave_federated::mojom::DataType::kInt;
}

brave_federated::mojom::CovariateType
AverageTabLifespanPredictorVariable::GetType() const {
  return brave_federated::mojom::CovariateType::kAverageTabLifespan;
}

std::string AverageTabLifespanPredictorVariable::GetValue() const {
  const UserActivityEventList events =
      UserActivityManager::GetInstance().GetHistoryForTimeWindow(
          base::Minutes(120)); // make this window longer

  std::map<int32_t, base::Time> tab_openings;
  std::map<int32_t, base::Time> tab_closings;
  for (const auto &event : events) {
    if (event.type == UserActivityEventType::kOpenedNewTab) {
      tab_openings[event.id] = event.created_at;
      BLOG(2, "opening found");
      BLOG(2, event.id);
    }
    if (event.type == UserActivityEventType::kClosedTab) {
      tab_closings[event.id] = event.created_at;
      BLOG(2, "closing found");
      BLOG(2, event.id);
    }
  }

  std::map<int32_t, base::TimeDelta> tab_lifespans;
  for (auto iter_opening : tab_openings) {
    const auto iter_closing = tab_closings.find(iter_opening.first);
    if (!(iter_closing == tab_closings.end())) {
      tab_lifespans[iter_opening.first] =
          iter_closing->second - iter_opening.second;
    }
  }

  double count_tabs = 0.0;
  double total_lifespan = 0.0;
  for (auto iter : tab_lifespans) {
    count_tabs += 1;
    total_lifespan += iter.second.InSecondsF();
  }

  const double average_lifespan = total_lifespan / count_tabs;
  return base::NumberToString(average_lifespan);
}

} // namespace brave_ads
