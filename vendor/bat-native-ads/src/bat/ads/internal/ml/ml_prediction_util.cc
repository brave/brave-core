/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

#include "base/notreached.h"
#include "bat/ads/internal/ml/ml_prediction_util.h"

namespace ads {
namespace ml {

PredictionMap Softmax(const PredictionMap& predictions) {
  double maximum = -std::numeric_limits<double>::infinity();
  for (const auto& prediction : predictions) {
    maximum = std::max(maximum, prediction.second);
  }
  PredictionMap softmax_predictions;
  double sum_exp = 0.0;
  for (const auto& prediction : predictions) {
    const double val = std::exp(prediction.second - maximum);
    softmax_predictions[prediction.first] = val;
    sum_exp += val;
  }
  for (auto& prediction : softmax_predictions) {
    prediction.second /= sum_exp;
  }
  return softmax_predictions;
}

}  // namespace ml
}  // namespace ads
