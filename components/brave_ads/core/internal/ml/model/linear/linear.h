/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_LINEAR_LINEAR_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_LINEAR_LINEAR_H_

#include <map>
#include <string>

#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/ml_alias.h"

namespace brave_ads::ml::model {

class Linear final {
 public:
  Linear();

  explicit Linear(const std::string& model);
  Linear(std::map<std::string, VectorData> weights,
         std::map<std::string, double> biases);

  Linear(const Linear&);
  Linear& operator=(const Linear&);

  Linear(Linear&&) noexcept;
  Linear& operator=(Linear&&) noexcept;

  ~Linear();

  PredictionMap Predict(const VectorData& x) const;

  PredictionMap GetTopPredictions(const VectorData& x,
                                  int top_count = -1) const;

 private:
  std::map<std::string, VectorData> weights_;
  std::map<std::string, double> biases_;
};

}  // namespace brave_ads::ml::model

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_LINEAR_LINEAR_H_
