/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_FL_PREDICTORS_PREDICTORS_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_FL_PREDICTORS_PREDICTORS_MANAGER_H_

#include <memory>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/predictor_variable_interface.h"
#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom-forward.h"

namespace brave_ads {

// `PredictorsManager` collects training data for federated services such as
// learning, tuning and evaluation. A row in the training data set is called an
// "instance ". A column is called a "feature". Predictor variables can be of
// different data types defined in `brave_federated::mojom::CovariateInfoPtr`.
// Predictors are only session based at the moment, i.e. no measurements are
// persisted across sessions.
class PredictorsManager final {
 public:
  PredictorsManager();

  PredictorsManager(const PredictorsManager&) = delete;
  PredictorsManager& operator=(const PredictorsManager&) = delete;

  PredictorsManager(PredictorsManager&&) noexcept = delete;
  PredictorsManager& operator=(PredictorsManager&&) noexcept = delete;

  ~PredictorsManager();

  static PredictorsManager& GetInstance();

  void SetPredictorVariable(
      std::unique_ptr<PredictorVariableInterface> predictor_variable);
  std::vector<brave_federated::mojom::CovariateInfoPtr> GetTrainingSample()
      const;

  void AddTrainingSample() const;

 private:
  base::flat_map<brave_federated::mojom::CovariateType,
                 std::unique_ptr<PredictorVariableInterface>>
      predictor_variables_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_FL_PREDICTORS_PREDICTORS_MANAGER_H_
