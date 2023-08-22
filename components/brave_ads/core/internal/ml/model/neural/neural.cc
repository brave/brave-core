/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/model/neural/neural.h"

#include <utility>
#include <vector>

#include "base/containers/adapters.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/ml/ml_prediction_util.h"

#include "brave/components/brave_ads/core/internal/common/logging_util.h"

namespace brave_ads::ml {

NeuralModel::NeuralModel() = default;

NeuralModel::NeuralModel(std::vector<std::vector<VectorData>> matricies,
                         std::vector<std::string> post_matrix_functions,
                         std::vector<std::string> classes) {
  matricies_ = std::move(matricies);
  post_matrix_functions_ = std::move(post_matrix_functions);
  classes_ = std::move(classes);
}

NeuralModel::NeuralModel(const NeuralModel& other) = default;

NeuralModel& NeuralModel::operator=(const NeuralModel& other) = default;

NeuralModel::NeuralModel(NeuralModel&& other) noexcept = default;

NeuralModel& NeuralModel::operator=(NeuralModel&& other) noexcept = default;

NeuralModel::~NeuralModel() = default;

PredictionMap NeuralModel::Predict(const VectorData& data) const {
  PredictionMap predictions;
  BLOG(2, "\nneural predict go");
  BLOG(2, matricies_.size());

  VectorData layer_input = data;
  for (size_t i = 0; i < matricies_.size(); i++) {
    std::vector<float> next_layer_input;
    for (const auto& vector : matricies_[i]) {
      float dot_product = vector * layer_input;
      next_layer_input.push_back(dot_product);
      // BLOG(2, dot_product);
    }
    layer_input = VectorData(std::move(next_layer_input));
    if (post_matrix_functions_[i] == "tanh") {
      layer_input.Tanh();
    }
    if (post_matrix_functions_[i] == "softmax") {
      layer_input.Softmax();
    }
  }

  std::vector<float> output_layer = layer_input.GetData();
  for (size_t i = 0; i < classes_.size(); i++) {
    predictions[classes_[i]] = output_layer[i];
  }

  // for (const auto& kv : weights_) {
  //   const std::vector<float> vector_temp{1.0F, 2.0F, 3.0F, 4.0F, 5.0F};
  //   const VectorData vector_data(vector_temp);
  //   double prediction = vector_data * data;
  //   // double prediction = kv.second * data;
  //   // const auto iter = biases_.find(kv.first);
  //   // if (iter != biases_.cend()) {
  //   //   prediction += iter->second;
  //   // }
  //   predictions[kv.first] = prediction;

  //   BLOG(2, kv.first);
  //   BLOG(2, prediction);
  // }
  return predictions;
}

PredictionMap NeuralModel::GetTopPredictions(const VectorData& data) const {
  return GetTopCountPredictionsImpl(data, absl::nullopt);
}

PredictionMap NeuralModel::GetTopCountPredictions(const VectorData& data,
                                                  size_t top_count) const {
  return GetTopCountPredictionsImpl(data, top_count);
}

PredictionMap NeuralModel::GetTopCountPredictionsImpl(
    const VectorData& data,
    absl::optional<size_t> top_count) const {
  const PredictionMap prediction_map = Predict(data);
  const PredictionMap prediction_map_softmax = Softmax(prediction_map);
  std::vector<std::pair<double, std::string>> prediction_order;
  prediction_order.reserve(prediction_map_softmax.size());
  for (const auto& prediction : prediction_map_softmax) {
    prediction_order.emplace_back(prediction.second, prediction.first);
  }
  base::ranges::sort(base::Reversed(prediction_order));
  PredictionMap top_predictions;
  if (top_count && *top_count < prediction_order.size()) {
    prediction_order.resize(*top_count);
  }
  for (const auto& prediction_order_item : prediction_order) {
    top_predictions[prediction_order_item.second] = prediction_order_item.first;
  }
  return top_predictions;
}

}  // namespace brave_ads::ml
