/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_MODEL_LINEAR_LINEAR_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_MODEL_LINEAR_LINEAR_H_

#include <map>
#include <string>

#include "bat/ads/internal/ml/data/vector_data.h"
#include "bat/ads/internal/ml/ml_aliases.h"

namespace ads {
namespace ml {
namespace model {

class Linear {
 public:
  Linear();

  Linear(const Linear& other);

  explicit Linear(const std::string& model);

  Linear(const std::map<std::string, VectorData>& weights,
         const std::map<std::string, double>& biases);

  ~Linear();

  PredictionMap Predict(const VectorData& x) const;

  PredictionMap GetTopPredictions(const VectorData& x,
                                  const int top_count = -1) const;

 private:
  std::map<std::string, VectorData> weights_;
  std::map<std::string, double> biases_;
};

}  // namespace model
}  // namespace ml
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_MODEL_LINEAR_LINEAR_H_
