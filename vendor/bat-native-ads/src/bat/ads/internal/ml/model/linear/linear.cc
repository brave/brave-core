/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/model/linear/linear.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "bat/ads/internal/ml/ml_prediction_util.h"

namespace ads {
namespace ml {
namespace model {

Linear::Linear() {}

Linear::Linear(const std::map<std::string, VectorData>& weights,
               const std::map<std::string, double>& biases) {
  weights_ = weights;
  biases_ = biases;
}

Linear::Linear(const Linear& linear_model) = default;

Linear::~Linear() = default;

PredictionMap Linear::Predict(const VectorData& x) const {
  PredictionMap predictions;
  for (const auto& kv : weights_) {
    double prediction = kv.second * x;
    const auto iter = biases_.find(kv.first);
    if (iter != biases_.end()) {
      prediction += iter->second;
    }
    predictions[kv.first] = prediction;
  }
  return predictions;
}

PredictionMap Linear::GetTopPredictions(const VectorData& x,
                                        const int top_count) const {
  PredictionMap prediction_map = Predict(x);
  PredictionMap prediction_map_softmax = Softmax(prediction_map);
  std::vector<std::pair<double, std::string>> prediction_order;
  prediction_order.reserve(prediction_map_softmax.size());
  for (const auto& prediction : prediction_map_softmax) {
    prediction_order.push_back(
        std::make_pair(prediction.second, prediction.first));
  }
  std::sort(prediction_order.rbegin(), prediction_order.rend());
  PredictionMap top_predictions;
  if (top_count > 0) {
    prediction_order.resize(top_count);
  }
  for (const auto& prediction_order_item : prediction_order) {
    top_predictions[prediction_order_item.second] = prediction_order_item.first;
  }
  return top_predictions;
}

}  // namespace model
}  // namespace ml
}  // namespace ads
