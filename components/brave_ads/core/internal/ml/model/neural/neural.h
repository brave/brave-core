/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_NEURAL_NEURAL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_NEURAL_NEURAL_H_

#include <cstddef>
#include <optional>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/ml_alias.h"

namespace brave_ads {

namespace neural_text_classification::flat {
struct Model;
}  // namespace neural_text_classification::flat

namespace ml {

class NeuralModel final {
 public:
  explicit NeuralModel(const neural_text_classification::flat::Model* model);

  NeuralModel(const NeuralModel&) = delete;
  NeuralModel& operator=(const NeuralModel&) = delete;

  NeuralModel(NeuralModel&&) noexcept;
  NeuralModel& operator=(NeuralModel&&) noexcept;

  ~NeuralModel() = default;

  const neural_text_classification::flat::Model* model() const {
    return model_.get();
  }

  std::optional<PredictionMap> Predict(const VectorData& data) const;

  std::optional<PredictionMap> GetTopPredictions(const VectorData& data) const;

  std::optional<PredictionMap> GetTopCountPredictions(const VectorData& data,
                                                      size_t top_count) const;

 private:
  std::optional<PredictionMap> GetTopCountPredictionsImpl(
      const VectorData& data,
      size_t top_count) const;

  raw_ptr<const neural_text_classification::flat::Model> model_ =
      nullptr;  // Not owned.
};

}  // namespace ml
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_NEURAL_NEURAL_H_
