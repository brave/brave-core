/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/model/neural/neural.h"

#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/adapters.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/common/resources/flat/text_classification_neural_model_generated.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"

namespace brave_ads::ml {

namespace {

constexpr char kPostMatrixFunctionTypeTanh[] = "tanh";
constexpr char kPostMatrixFunctionTypeSoftmax[] = "softmax";

}  // namespace

NeuralModel::NeuralModel(const neural_text_classification::flat::Model& model)
    : model_(model) {}

NeuralModel::NeuralModel(NeuralModel&& other) noexcept = default;

NeuralModel& NeuralModel::operator=(NeuralModel&& other) noexcept = default;

std::optional<PredictionMap> NeuralModel::Predict(
    const VectorData& data) const {
  PredictionMap predictions;

  const neural_text_classification::flat::Classifier* classifier =
      model_->classifier();
  if (!classifier) {
    return std::nullopt;
  }

  const auto* const matrices = classifier->matrices();
  if (!matrices) {
    return std::nullopt;
  }

  const auto* const activation_functions = classifier->activation_functions();
  if (!activation_functions ||
      matrices->size() != activation_functions->size()) {
    return std::nullopt;
  }

  VectorData layer_input = data;
  for (size_t i = 0; i < matrices->size(); i++) {
    std::vector<float> next_layer_input;
    const auto* const matrix = matrices->Get(i);
    if (!matrix || !matrix->weights_rows()) {
      return std::nullopt;
    }

    for (const auto* const matrix_row : *matrix->weights_rows()) {
      if (!matrix_row || !matrix_row->row()) {
        return std::nullopt;
      }

      std::vector<float> row;
      row.reserve(matrix_row->row()->size());
      base::ranges::copy(*matrix_row->row(), std::back_inserter(row));
      VectorData row_data(std::move(row));
      const float dot_product = row_data * layer_input;
      next_layer_input.push_back(dot_product);
    }
    layer_input = VectorData(std::move(next_layer_input));

    const auto* const activation_function = activation_functions->Get(i);
    if (!activation_function) {
      return std::nullopt;
    }
    if (activation_function->str() == kPostMatrixFunctionTypeTanh) {
      layer_input.Tanh();
    } else if (activation_function->str() == kPostMatrixFunctionTypeSoftmax) {
      layer_input.Softmax();
    }
  }

  const std::vector<float> output_layer = layer_input.GetDenseData();
  const auto* const segments = classifier->segments();
  if (!segments || segments->size() != output_layer.size()) {
    return std::nullopt;
  }

  for (size_t i = 0; i < segments->size(); i++) {
    const auto* const segment = segments->Get(i);
    if (!segment) {
      return std::nullopt;
    }
    const std::string segment_value = segment->str();
    if (segment_value.empty()) {
      return std::nullopt;
    }
    predictions[segment_value] = output_layer[i];
  }

  return predictions;
}

std::optional<PredictionMap> NeuralModel::GetTopPredictions(
    const VectorData& data) const {
  return GetTopCountPredictionsImpl(data, data.GetDimensionCount());
}

std::optional<PredictionMap> NeuralModel::GetTopCountPredictions(
    const VectorData& data,
    size_t top_count) const {
  return GetTopCountPredictionsImpl(data, top_count);
}

std::optional<PredictionMap> NeuralModel::GetTopCountPredictionsImpl(
    const VectorData& data,
    size_t top_count) const {
  const std::optional<PredictionMap> predictions = Predict(data);
  if (!predictions) {
    return std::nullopt;
  }

  std::vector<std::pair<double, std::string>> prediction_order;
  prediction_order.reserve(predictions->size());
  for (const auto& [segment, probability] : *predictions) {
    prediction_order.emplace_back(probability, segment);
  }
  base::ranges::sort(base::Reversed(prediction_order));

  PredictionMap top_predictions;
  if (top_count < prediction_order.size()) {
    prediction_order.resize(top_count);
  }
  for (const auto& [probability, segment] : prediction_order) {
    top_predictions[segment] = probability;
  }
  return top_predictions;
}

}  // namespace brave_ads::ml
