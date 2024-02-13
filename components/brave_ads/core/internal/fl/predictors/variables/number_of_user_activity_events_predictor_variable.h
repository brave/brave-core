/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_FL_PREDICTORS_VARIABLES_NUMBER_OF_USER_ACTIVITY_EVENTS_PREDICTOR_VARIABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_FL_PREDICTORS_VARIABLES_NUMBER_OF_USER_ACTIVITY_EVENTS_PREDICTOR_VARIABLE_H_

#include <string>

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/predictor_variable_interface.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_event_types.h"

namespace brave_ads {

class NumberOfUserActivityEventsPredictorVariable final
    : public PredictorVariableInterface {
 public:
  NumberOfUserActivityEventsPredictorVariable(
      UserActivityEventType event_type,
      brave_federated::mojom::CovariateType predictor_type);

  // PredictorVariableInterface:
  brave_federated::mojom::DataType GetDataType() const override;
  brave_federated::mojom::CovariateType GetType() const override;
  std::string GetValue() const override;

 private:
  const UserActivityEventType event_type_;
  const brave_federated::mojom::CovariateType predictor_type_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_FL_PREDICTORS_VARIABLES_NUMBER_OF_USER_ACTIVITY_EVENTS_PREDICTOR_VARIABLE_H_
