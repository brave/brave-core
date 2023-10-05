/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/model/linear/linear.h"

#include <utility>
#include <vector>

#include "base/containers/adapters.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/ml/ml_prediction_util.h"

namespace brave_ads::ml {

LinearModel::LinearModel() = default;

LinearModel::LinearModel(std::map<std::string, VectorData> weights,
                         std::map<std::string, double> biases) {
  weights_ = std::move(weights);
  biases_ = std::move(biases);
}

LinearModel::LinearModel(const LinearModel& other) = default;

LinearModel& LinearModel::operator=(const LinearModel& other) = default;

LinearModel::LinearModel(LinearModel&& other) noexcept = default;

LinearModel& LinearModel::operator=(LinearModel&& other) noexcept = default;

LinearModel::~LinearModel() = default;

PredictionMap LinearModel::Predict(const VectorData& data) const {
  PredictionMap predictions;
  for (const auto& [segment, weight_vector] : weights_) {
    double prediction = weight_vector * data;
    const auto iter = biases_.find(segment);
    if (iter != biases_.cend()) {
      prediction += iter->second;
    }
    predictions[segment] = prediction;
  }
  return predictions;
}

PredictionMap LinearModel::GetTopPredictions(const VectorData& data) const {
  return GetTopCountPredictionsImpl(data, absl::nullopt);
}

PredictionMap LinearModel::GetTopCountPredictions(const VectorData& data,
                                                  size_t top_count) const {
  return GetTopCountPredictionsImpl(data, top_count);
}

PredictionMap LinearModel::GetTopCountPredictionsImpl(
    const VectorData& data,
    absl::optional<size_t> top_count) const {
  const PredictionMap predictions = Predict(data);
  const PredictionMap predictions_softmax = Softmax(predictions);
  std::vector<std::pair<double, std::string>> prediction_order;
  prediction_order.reserve(predictions_softmax.size());
  for (const auto& [segment, probability] : predictions_softmax) {
    prediction_order.emplace_back(probability, segment);
  }
  base::ranges::sort(base::Reversed(prediction_order));

  PredictionMap top_predictions;
  if (top_count && *top_count < prediction_order.size()) {
    prediction_order.resize(*top_count);
  }
  for (const auto& [probability, segment] : prediction_order) {
    top_predictions[segment] = probability;
  }
  return top_predictions;
}

}  // namespace brave_ads::ml
