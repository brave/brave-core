/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/model/neural/neural.h"

#include <iterator>
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

NeuralModel::NeuralModel(const text_classification::flat::NeuralModel* model)
    : model_(model) {
  CHECK(model_);
}

NeuralModel::NeuralModel(NeuralModel&& other) noexcept = default;

NeuralModel& NeuralModel::operator=(NeuralModel&& other) noexcept = default;

NeuralModel::~NeuralModel() = default;

absl::optional<PredictionMap> NeuralModel::Predict(
    const VectorData& data) const {
  PredictionMap predictions;

  const text_classification::flat::Classifier* classifier =
      model_->classifier();
  if (!classifier) {
    return absl::nullopt;
  }

  const auto* matricies_data = classifier->neural_matricies_data();
  if (!matricies_data) {
    return absl::nullopt;
  }

  const auto* post_matrix_functions =
      classifier->neural_post_matrix_functions();
  if (!post_matrix_functions ||
      matricies_data->size() != post_matrix_functions->size()) {
    return absl::nullopt;
  }

  VectorData layer_input = data;
  for (size_t i = 0; i < matricies_data->size(); i++) {
    std::vector<float> next_layer_input;
    const auto* matrix_data = matricies_data->Get(i);
    if (!matrix_data || !matrix_data->weights_rows()) {
      return absl::nullopt;
    }

    for (const auto* matrix_row : *matrix_data->weights_rows()) {
      if (!matrix_row || !matrix_row->row()) {
        return absl::nullopt;
      }

      std::vector<float> row;
      row.reserve(matrix_row->row()->size());
      base::ranges::copy(*matrix_row->row(), std::back_inserter(row));
      VectorData row_data(std::move(row));
      const float dot_product = row_data * layer_input;
      next_layer_input.push_back(dot_product);
    }
    layer_input = VectorData(std::move(next_layer_input));

    const auto* post_matrix_function = post_matrix_functions->Get(i);
    if (!post_matrix_function) {
      return absl::nullopt;
    }
    if (post_matrix_function->str() == kPostMatrixFunctionTypeTanh) {
      layer_input.Tanh();
    } else if (post_matrix_function->str() == kPostMatrixFunctionTypeSoftmax) {
      layer_input.Softmax();
    }
  }

  std::vector<float> output_layer;
  output_layer = layer_input.GetData(output_layer);

  const auto* classes = classifier->classes();
  if (!classes || classes->size() != output_layer.size()) {
    return absl::nullopt;
  }

  for (size_t i = 0; i < classes->size(); i++) {
    const auto* classes_entry = classes->Get(i);
    if (!classes_entry) {
      return absl::nullopt;
    }
    const std::string class_value = classes_entry->str();
    predictions[class_value] = output_layer[i];
  }

  return predictions;
}

absl::optional<PredictionMap> NeuralModel::GetTopPredictions(
    const VectorData& data) const {
  return GetTopCountPredictionsImpl(data, data.GetDimensionCount());
}

absl::optional<PredictionMap> NeuralModel::GetTopCountPredictions(
    const VectorData& data,
    size_t top_count) const {
  return GetTopCountPredictionsImpl(data, top_count);
}

absl::optional<PredictionMap> NeuralModel::GetTopCountPredictionsImpl(
    const VectorData& data,
    size_t top_count) const {
  const absl::optional<PredictionMap> predictions = Predict(data);
  if (!predictions) {
    return absl::nullopt;
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
