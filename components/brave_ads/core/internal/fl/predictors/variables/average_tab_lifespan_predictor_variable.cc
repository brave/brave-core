/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/average_tab_lifespan_predictor_variable.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_manager.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_util.h"

#include "brave/components/brave_ads/core/internal/common/logging_util.h"

#include <map>

namespace brave_ads {

AverageTabLifespanPredictorVariable::AverageTabLifespanPredictorVariable(
    UserActivityEventType event_type,
    brave_federated::mojom::CovariateType predictor_type)
    : event_type_(event_type), predictor_type_(predictor_type) {}

brave_federated::mojom::DataType
AverageTabLifespanPredictorVariable::GetDataType() const {
  return brave_federated::mojom::DataType::kInt;
}

brave_federated::mojom::CovariateType
AverageTabLifespanPredictorVariable::GetType() const {
  return predictor_type_;
}

std::string AverageTabLifespanPredictorVariable::GetValue() const {
  const UserActivityEventList events =
      UserActivityManager::GetInstance().GetHistoryForTimeWindow(
          base::Minutes(120)); // make this window longer

  //
  std::map<std::string, base::Time> tab_openings;
  std::map<std::string, base::Time> tab_closings;
  for (const auto &event : events) {
    if (event.type == UserActivityEventType::kOpenedNewTab) {
      tab_openings["hi"] = event.created_at;
      BLOG(2, "opening found");
      BLOG(2, event.id);
    }
    if (event.type == UserActivityEventType::kClosedTab) {
      tab_closings["hi"] = event.created_at;
      BLOG(2, "closing found");
      BLOG(2, event.id);
    }
  }

  base::Time opening;
  const auto iter = tab_openings.find("hi");
  if (iter == tab_openings.end()) {
    BLOG(2, "no opening recorded");
  } else {
    opening = iter->second;
  }
  BLOG(2, opening);
  //

  return base::NumberToString(
      GetNumberOfUserActivityEvents(events, event_type_));
}

} // namespace brave_ads
