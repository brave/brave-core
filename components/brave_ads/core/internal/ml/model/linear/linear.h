/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_LINEAR_LINEAR_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_LINEAR_LINEAR_H_

#include <map>
#include <string>

#include "base/allocator/partition_allocator/pointers/raw_ptr.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/ml_alias.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::text_classification::flat {
struct LinearModel;
}  // namespace brave_ads::text_classification::flat

namespace brave_ads::ml {

class LinearModel final {
 public:
  LinearModel();

  explicit LinearModel(const text_classification::flat::LinearModel* model);

  LinearModel(const LinearModel&);
  LinearModel& operator=(const LinearModel&);

  LinearModel(LinearModel&&) noexcept;
  LinearModel& operator=(LinearModel&&) noexcept;

  ~LinearModel();

  absl::optional<PredictionMap> Predict(const VectorData& data) const;

  absl::optional<PredictionMap> GetTopPredictions(const VectorData& data) const;

  absl::optional<PredictionMap> GetTopCountPredictions(const VectorData& data,
                                                       size_t top_count) const;

 private:
  absl::optional<PredictionMap> GetTopCountPredictionsImpl(
      const VectorData& data,
      absl::optional<size_t> top_count) const;

  // std::map<std::string, VectorData> weights_;
  // std::map<std::string, double> biases_;

  raw_ptr<const text_classification::flat::LinearModel> model_;
};

}  // namespace brave_ads::ml

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_LINEAR_LINEAR_H_
