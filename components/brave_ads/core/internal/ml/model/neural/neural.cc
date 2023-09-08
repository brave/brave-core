/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/model/neural/neural.h"

#include <utility>
#include <vector>

#include "base/containers/adapters.h"
#include "base/ranges/algorithm.h"

namespace brave_ads::ml {

NeuralModel::NeuralModel() = default;

NeuralModel::NeuralModel(std::vector<std::vector<VectorData>> matricies,
                         std::vector<std::string>& post_matrix_functions,
                         std::vector<std::string> classes) {
  std::vector<PostMatrixFunctionType> post_matrix_functions_types;
  for (const auto& post_matrix_function : post_matrix_functions) {
    if (post_matrix_function == "tanh") {
      post_matrix_functions_types.push_back(PostMatrixFunctionType::kTanh);
    } else if (post_matrix_function == "softmax") {
      post_matrix_functions_types.push_back(PostMatrixFunctionType::kSoftmax);
    }
  }
  neural_architecture_info_ = NeuralArchitectureInfo(
      std::move(matricies), post_matrix_functions_types, std::move(classes));
}

NeuralModel::NeuralModel(NeuralModel&& other) noexcept = default;

NeuralModel& NeuralModel::operator=(NeuralModel&& other) noexcept = default;

NeuralModel::~NeuralModel() = default;

bool NeuralModel::HasModelParameters() const {
  return !neural_architecture_info_.matricies.empty();
}

PredictionMap NeuralModel::Predict(const VectorData& data) const {
  PredictionMap predictions;

  VectorData layer_input = data;
  for (size_t i = 0; i < neural_architecture_info_.matricies.size(); i++) {
    std::vector<float> next_layer_input;
    for (const auto& vector : neural_architecture_info_.matricies[i]) {
      float dot_product = vector * layer_input;
      next_layer_input.push_back(dot_product);
    }
    layer_input = VectorData(std::move(next_layer_input));
    if (neural_architecture_info_.post_matrix_functions[i] ==
        PostMatrixFunctionType::kTanh) {
      layer_input.Tanh();
    } else if (neural_architecture_info_.post_matrix_functions[i] ==
               PostMatrixFunctionType::kSoftmax) {
      layer_input.Softmax();
    }
  }

  std::vector<float> output_layer(layer_input.GetDimensionCount());
  output_layer = layer_input.GetData(output_layer);
  for (size_t i = 0; i < neural_architecture_info_.classes.size(); i++) {
    predictions[neural_architecture_info_.classes[i]] = output_layer[i];
  }

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
  const PredictionMap predictions = Predict(data);
  std::vector<std::pair<double, std::string>> prediction_order;
  prediction_order.reserve(predictions.size());
  for (const auto& [segment, probability] : predictions) {
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
