/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/model/linear/linear.h"

#include <string>
#include <utility>
#include <vector>

#include "base/containers/adapters.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/common/resources/flat/text_classification_linear_model_generated.h"
#include "brave/components/brave_ads/core/internal/ml/ml_prediction_util.h"

namespace brave_ads::ml {

LinearModel::LinearModel(const linear_text_classification::flat::Model& model)
    : model_(model) {}

LinearModel::LinearModel(LinearModel&& other) noexcept = default;

LinearModel& LinearModel::operator=(LinearModel&& other) noexcept = default;

std::optional<PredictionMap> LinearModel::Predict(
    const VectorData& data) const {
  PredictionMap predictions;
  const auto* const classifier = model_->classifier();
  if (!classifier || !classifier->biases() ||
      !classifier->segment_weight_vectors()) {
    return std::nullopt;
  }

  for (const auto* const segment_weight :
       *classifier->segment_weight_vectors()) {
    if (!segment_weight || !segment_weight->segment() ||
        !segment_weight->weights()) {
      return std::nullopt;
    }
    const std::string segment = segment_weight->segment()->str();
    if (segment.empty()) {
      return std::nullopt;
    }
    std::vector<float> weights;
    weights.reserve(segment_weight->weights()->size());
    base::ranges::copy(*segment_weight->weights(), std::back_inserter(weights));
    VectorData weight_vector(std::move(weights));
    double prediction = weight_vector * data;
    const auto* const iter = classifier->biases()->LookupByKey(segment.c_str());
    if (iter) {
      prediction += iter->bias();
    }
    predictions[segment] = prediction;
  }
  return predictions;
}

std::optional<PredictionMap> LinearModel::GetTopPredictions(
    const VectorData& data) const {
  return GetTopCountPredictionsImpl(data, std::nullopt);
}

std::optional<PredictionMap> LinearModel::GetTopCountPredictions(
    const VectorData& data,
    size_t top_count) const {
  return GetTopCountPredictionsImpl(data, top_count);
}

std::optional<PredictionMap> LinearModel::GetTopCountPredictionsImpl(
    const VectorData& data,
    std::optional<size_t> top_count) const {
  const std::optional<PredictionMap> predictions = Predict(data);
  if (!predictions) {
    return std::nullopt;
  }

  const PredictionMap predictions_softmax = Softmax(*predictions);
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
