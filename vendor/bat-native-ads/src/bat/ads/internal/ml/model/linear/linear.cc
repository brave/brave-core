/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/model/linear/linear.h"

#include <utility>
#include <vector>

#include "base/containers/adapters.h"
#include "base/ranges/algorithm.h"
#include "bat/ads/internal/ml/ml_prediction_util.h"

namespace ads::ml::model {

Linear::Linear() = default;

Linear::Linear(std::map<std::string, VectorData> weights,
               std::map<std::string, double> biases) {
  weights_ = std::move(weights);
  biases_ = std::move(biases);
}

Linear::Linear(const Linear& other) = default;

Linear& Linear::operator=(const Linear& other) = default;

Linear::Linear(Linear&& other) noexcept = default;

Linear& Linear::operator=(Linear&& other) noexcept = default;

Linear::~Linear() = default;

PredictionMap Linear::Predict(const VectorData& x) const {
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

PredictionMap Linear::GetTopPredictions(const VectorData& x,
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

}  // namespace ads::ml::model
