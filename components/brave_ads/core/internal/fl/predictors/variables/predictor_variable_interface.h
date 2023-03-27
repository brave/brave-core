/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_FL_PREDICTORS_VARIABLES_PREDICTOR_VARIABLE_INTERFACE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_FL_PREDICTORS_VARIABLES_PREDICTOR_VARIABLE_INTERFACE_H_

#include <string>

#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom-shared.h"

namespace brave_ads {

class PredictorVariableInterface {
 public:
  virtual ~PredictorVariableInterface() = default;

  virtual brave_federated::mojom::DataType GetDataType() const = 0;
  virtual brave_federated::mojom::CovariateType GetType() const = 0;
  virtual std::string GetValue() const = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_FL_PREDICTORS_VARIABLES_PREDICTOR_VARIABLE_INTERFACE_H_
