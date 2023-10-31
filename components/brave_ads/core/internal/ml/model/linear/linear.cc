/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/model/linear/linear.h"

#include <utility>
#include <vector>

#include "base/containers/adapters.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/common/resources/flat/text_classification_linear_model_generated.h"
#include "brave/components/brave_ads/core/internal/ml/ml_prediction_util.h"

namespace brave_ads::ml {

LinearModel::LinearModel() = default;

LinearModel::LinearModel(const text_classification::flat::LinearModel* model)
    : model_(model) {
  CHECK(model_);
}

// LinearModel::LinearModel(std::map<std::string, VectorData> weights,
//                          std::map<std::string, double> biases) {
//   weights_ = std::move(weights);
//   biases_ = std::move(biases);
// }

LinearModel::LinearModel(const LinearModel& other) = default;

LinearModel& LinearModel::operator=(const LinearModel& other) = default;

LinearModel::LinearModel(LinearModel&& other) noexcept = default;

LinearModel& LinearModel::operator=(LinearModel&& other) noexcept = default;

LinearModel::~LinearModel() = default;

absl::optional<PredictionMap> LinearModel::Predict(
    const VectorData& data) const {
  PredictionMap predictions;
  const auto* classifier = model_->classifier();
  if (!classifier || !classifier->biases() || !classifier->segment_weights()) {
    return absl::nullopt;
  }

  for (const auto* segment_weight : *classifier->segment_weights()) {
    if (!segment_weight->segment() || !segment_weight->numbers()) {
      return absl::nullopt;
    }
    const std::string segment = segment_weight->segment()->str();

    std::vector<float> weights_numbers;
    weights_numbers.reserve(segment_weight->numbers()->size());
    base::ranges::copy(*segment_weight->numbers(),
                       std::back_inserter(weights_numbers));
    VectorData weight_vector(std::move(weights_numbers));
    double prediction = weight_vector * data;
    const auto* iter = classifier->biases()->LookupByKey(segment.c_str());
    if (iter) {
      prediction += iter->bias();
    }
    predictions[segment] = prediction;
  }
  return predictions;
}

absl::optional<PredictionMap> LinearModel::GetTopPredictions(
    const VectorData& data) const {
  return GetTopCountPredictionsImpl(data, absl::nullopt);
}

absl::optional<PredictionMap> LinearModel::GetTopCountPredictions(
    const VectorData& data,
    size_t top_count) const {
  return GetTopCountPredictionsImpl(data, top_count);
}

absl::optional<PredictionMap> LinearModel::GetTopCountPredictionsImpl(
    const VectorData& data,
    absl::optional<size_t> top_count) const {
  const absl::optional<PredictionMap> predictions = Predict(data);
  if (!predictions) {
    return absl::nullopt;
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
