/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/notification_ad_event_predictor_variable_util.h"

#include <memory>

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/predictors_manager.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/notification_ad_event_predictor_variable.h"

namespace brave_ads {

void SetNotificationAdEventPredictorVariable(
    const mojom::NotificationAdEventType event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  PredictorsManager::GetInstance().SetPredictorVariable(
      std::make_unique<NotificationAdEventPredictorVariable>(event_type));
}

}  // namespace brave_ads
