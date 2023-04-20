/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/notification_ad_served_at_predictor_variable_util.h"

#include <memory>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/predictors_manager.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/notification_ad_served_at_predictor_variable.h"

namespace brave_ads {

void SetNotificationAdServedAtPredictorVariable(const base::Time time) {
  PredictorsManager::GetInstance().SetPredictorVariable(
      std::make_unique<NotificationAdServedAtPredictorVariable>(time));
}

}  // namespace brave_ads
