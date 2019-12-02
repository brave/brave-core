/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_perf_predictor/browser/predictor.h"

#include <numeric>
#include <cmath>

namespace brave_perf_predictor {

bool standardise_features_no_outliers(
    std::array<double, standardise_feature_count> &features,
    const std::array<double, standardise_feature_count> &means,
    const std::array<double, standardise_feature_count> &scale) {
  for (unsigned int i = 0; i < standardise_feature_count; i++) {
    features[i] = (features[i] - means[i])/scale[i];
  }
  double outlier_threshold = 3;
  for (unsigned int i = 0; i < standardise_feature_count; i++) {
    if (features[i] > outlier_threshold || features[i] < -outlier_threshold)
      return true;
  }
  return false;
}

double predict(const std::array<double, feature_count> &features) {
  // Standardise numeric features
  std::array<double, standardise_feature_count> numeric_features;
  std::copy(features.begin(), features.begin() + standardise_feature_count,
    numeric_features.begin());
  bool has_outliers = standardise_features_no_outliers(
    numeric_features, standardise_feature_means,
    standardise_feature_scale);
  if (has_outliers) {
    return 0;
  }

  // Create a new feature vector to include all features
  std::array<double, feature_count> standardised_features;
  std::move(numeric_features.begin(), numeric_features.end(),
    standardised_features.begin());
  // Just copy the rest of the features as-is
  std::copy(features.begin() + standardise_feature_count, features.end(),
    standardised_features.begin() + standardise_feature_count);
  
  // Calculate the prediction
  double log_prediction = std::inner_product(standardised_features.begin(),
    standardised_features.end(), model_coefficients.begin(), model_intercept);
  // We know the target is log-scaled but care about the absolute value
  return std::pow(10, log_prediction);
}

double predict(const std::unordered_map<std::string, double> &features) {
  std::array<double, feature_count> feature_vector{};
  for (unsigned int i = 0; i < feature_count; i++) {
    auto it = features.find(feature_sequence[i]);
    if (it != features.end()) {
      feature_vector[i] = it->second;
    }
  }
  return predict(feature_vector);
}

}
