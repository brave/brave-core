/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_LINEAR_LINEAR_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_LINEAR_LINEAR_H_

#include <cstddef>
#include <optional>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/ml_alias.h"

namespace brave_ads {

namespace linear_text_classification::flat {
struct Model;
}  // namespace linear_text_classification::flat

namespace ml {

class LinearModel final {
 public:
  LinearModel();

  explicit LinearModel(const linear_text_classification::flat::Model* model);

  LinearModel(const LinearModel&) = delete;
  LinearModel& operator=(const LinearModel&) = delete;

  LinearModel(LinearModel&&) noexcept;
  LinearModel& operator=(LinearModel&&) noexcept;

  ~LinearModel() = default;

  std::optional<PredictionMap> Predict(const VectorData& data) const;

  std::optional<PredictionMap> GetTopPredictions(const VectorData& data) const;

  std::optional<PredictionMap> GetTopCountPredictions(const VectorData& data,
                                                      size_t top_count) const;

 private:
  std::optional<PredictionMap> GetTopCountPredictionsImpl(
      const VectorData& data,
      std::optional<size_t> top_count) const;

  raw_ptr<const linear_text_classification::flat::Model> model_ =
      nullptr;  // Not owned.
};

}  // namespace ml
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_LINEAR_LINEAR_H_
