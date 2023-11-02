/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_NEURAL_NEURAL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_NEURAL_NEURAL_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/ml_alias.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::neural_text_classification::flat {
struct Model;
}  // namespace brave_ads::neural_text_classification::flat

namespace brave_ads::ml {

class NeuralModel final {
 public:
  explicit NeuralModel(const neural_text_classification::flat::Model* model);

  NeuralModel(const NeuralModel&) = delete;
  NeuralModel& operator=(const NeuralModel&) = delete;

  NeuralModel(NeuralModel&&) noexcept;
  NeuralModel& operator=(NeuralModel&&) noexcept;

  ~NeuralModel();

  const neural_text_classification::flat::Model* model() const {
    return model_.get();
  }

  absl::optional<PredictionMap> Predict(const VectorData& data) const;

  absl::optional<PredictionMap> GetTopPredictions(const VectorData& data) const;

  absl::optional<PredictionMap> GetTopCountPredictions(const VectorData& data,
                                                       size_t top_count) const;

 private:
  absl::optional<PredictionMap> GetTopCountPredictionsImpl(
      const VectorData& data,
      size_t top_count) const;

  raw_ptr<const neural_text_classification::flat::Model> model_;
};

}  // namespace brave_ads::ml

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_NEURAL_NEURAL_H_
