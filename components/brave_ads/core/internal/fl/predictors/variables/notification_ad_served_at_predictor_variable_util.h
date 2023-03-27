/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_FL_PREDICTORS_VARIABLES_NOTIFICATION_AD_SERVED_AT_PREDICTOR_VARIABLE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_FL_PREDICTORS_VARIABLES_NOTIFICATION_AD_SERVED_AT_PREDICTOR_VARIABLE_UTIL_H_

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

void SetNotificationAdServedAtPredictorVariable(base::Time time);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_FL_PREDICTORS_VARIABLES_NOTIFICATION_AD_SERVED_AT_PREDICTOR_VARIABLE_UTIL_H_
