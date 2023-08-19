/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_NEURAL_NEURAL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_NEURAL_NEURAL_H_

#include <map>
#include <string>

#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/ml_alias.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::ml {

class NeuralModel final {
 public:
  NeuralModel();

  explicit NeuralModel(const std::string& model);
  NeuralModel(std::vector<std::vector<VectorData>> matricies,
              std::vector<std::string> post_matrix_functions,
              std::vector<std::string> classes);

  NeuralModel(const NeuralModel&);
  NeuralModel& operator=(const NeuralModel&);

  NeuralModel(NeuralModel&&) noexcept;
  NeuralModel& operator=(NeuralModel&&) noexcept;

  ~NeuralModel();

  PredictionMap Predict(const VectorData& data) const;

  PredictionMap GetTopPredictions(const VectorData& data) const;

  PredictionMap GetTopCountPredictions(const VectorData& data,
                                       size_t top_count) const;

 private:
  PredictionMap GetTopCountPredictionsImpl(
      const VectorData& data,
      absl::optional<size_t> top_count) const;

  std::vector<std::vector<VectorData>> matricies_;
  std::vector<std::string> post_matrix_functions_;
  std::vector<std::string> classes_;
};

}  // namespace brave_ads::ml

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_NEURAL_NEURAL_H_
