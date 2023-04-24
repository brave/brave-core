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

PredictionMap LinearModel::Predict(const VectorData& x) const {
  PredictionMap predictions;
  for (const auto& kv : weights_) {
    double prediction = kv.second * x;
    const auto iter = biases_.find(kv.first);
    if (iter != biases_.cend()) {
      prediction += iter->second;
    }
    predictions[kv.first] = prediction;
  }
  return predictions;
}

PredictionMap LinearModel::GetTopPredictions(const VectorData& x,
                                             const int top_count) const {
  const PredictionMap prediction_map = Predict(x);
  const PredictionMap prediction_map_softmax = Softmax(prediction_map);
  std::vector<std::pair<double, std::string>> prediction_order;
  prediction_order.reserve(prediction_map_softmax.size());
  for (const auto& prediction : prediction_map_softmax) {
    prediction_order.emplace_back(prediction.second, prediction.first);
  }
  base::ranges::sort(base::Reversed(prediction_order));
  PredictionMap top_predictions;
  if (top_count > 0) {
    prediction_order.resize(top_count);
  }
  for (const auto& prediction_order_item : prediction_order) {
    top_predictions[prediction_order_item.second] = prediction_order_item.first;
  }
  return top_predictions;
}

}  // namespace brave_ads::ml
