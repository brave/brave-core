/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_perf_predictor/browser/bandwidth_linreg.h"

#include "base/containers/flat_map.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_perf_predictor {

TEST(BraveSavingsPredictorTest, FeatureArrayGetsPrediction) {
  const std::array<double, feature_count> features{};
  double result = LinregPredictVector(features);
  EXPECT_NE(result, 0);
}

TEST(BraveSavingsPredictorTest, HandlesSpecificVectorExample) {
  // This test needs to be updated for any change in the model
  constexpr std::array<double, feature_count> sample = {
      20, 129, 225, 142, 925,    5, 34662, 3,  317818,  9,  1702888,
      0,  0,   1,   324, 32,  238315, 9, 90131, 54, 2367498, 59, 2384138,
      0,  1,   0,   0,   0,   0,      0, 0,     0,  1,       0,  0,
      0,  0,   1,   0,   0,   0,      0, 0,     0,  0,       0,  0,
      0,  0,   0,   0,   0,   0,      0, 0,     1,  0,       0,  0,
      0,  0,   0,   0,   0,   0,      0, 0,     0,  0,       0,  0,
      0,  0,   0,   0,   0,   0,      0, 0,     0,  0,       0,  0,
      0,  0,   0,   0,   1,   0,      0, 0,     0,  0,       0,  0,
      0,  0,   0,   0,   0,   0,      0, 0,     0,  0,       0,  0,
      0,  0,   0,   1,   0,   0,      0, 0,     0,  0,       0,  0,
      0,  0,   0,   0,   0,   0,      0, 0,     0,  0,       0,  0,
      0,  0,   0,   0,   0,   0,      0, 0,     0,  0,       0,  0,
      0,  1,   1,   1,   0,   0,      0, 0,     0,  0,       0,  0,
      0,  0,   0,   0,   0,   0,      0, 0,     0,  0,       0,  0,
      0,  0,   0,   0,   0,   0,      0, 0,     0,  0,       0,  0,
      0,  0,   0,   0,   0,   0,      0, 0,     0,  0,       0,  0,
      0,  0,   0,   0,   0,   0,      0, 0,     0,  0,       0,  0,
      0,  0,   0,   0,   0,   0,      0, 0,     0,  0};

  const double result = LinregPredictVector(sample);
  EXPECT_EQ(static_cast<int>(result / 1000),
            794);  // Equal on the order of thousands
}

TEST(BraveSavingsPredictorTest, HandlesEmptyFeatureset) {
  const base::flat_map<std::string, double> features{};
  const double result = LinregPredictNamed(features);
  const std::array<double, feature_count> features_array{};
  const double array_result = LinregPredictVector(features_array);
  EXPECT_EQ(result, array_result);
}

TEST(BraveSavingsPredictorTest, HandlesCompleteFeatureset) {
  base::flat_map<std::string, double> features;
  for (unsigned int i = 0; i < feature_count; i++) {
    features[feature_sequence.at(i)] = 0;
  }
  const double result = LinregPredictNamed(features);
  const std::array<double, feature_count> array_features{};
  const double array_result = LinregPredictVector(array_features);
  EXPECT_EQ(result, array_result);
}

TEST(BraveSavingsPredictorTest, HandesSpecificFeaturemapExample) {
  // This test needs to be updated for any change in the model
  // Third-parties that are not detected are skipped
  const base::flat_map<std::string, double> featuremap = {
      {"thirdParties.Facebook.blocked", 1},
      {"thirdParties.Google Tag Manager.blocked", 1},
      {"thirdParties.YouTube.blocked", 1},
      {"thirdParties.Snapchat.blocked", 1},
      {"thirdParties.Instagram.blocked", 1},
      {"thirdParties.AddThis.blocked", 1},
      {"thirdParties.Micropat.blocked", 1},
      {"thirdParties.Playbuzz.blocked", 1},
      {"thirdParties.Po.st.blocked", 1},
      {"adblockRequests", 20},
      {"metrics.firstMeaningfulPaint", 129},
      {"metrics.observedDomContentLoaded", 225},
      {"metrics.observedFirstVisualChange", 142},
      {"metrics.observedLoad", 925},
      {"resources.document.requestCount", 5},
      {"resources.document.size", 34662},
      {"resources.font.requestCount", 3},
      {"resources.font.size", 317818},
      {"resources.image.requestCount", 9},
      {"resources.image.size", 1702888},
      {"resources.media.requestCount", 0},
      {"resources.media.size", 0},
      {"resources.other.requestCount", 1},
      {"resources.other.size", 324},
      {"resources.script.requestCount", 32},
      {"resources.script.size", 238315},
      {"resources.stylesheet.requestCount", 9},
      {"resources.third-party.requestCount", 54},
      {"resources.third-party.size", 2367498},
      {"resources.stylesheet.size", 90131},
      {"resources.total.requestCount", 59},
      {"resources.total.size", 238413},
  };
  double result = LinregPredictNamed(featuremap);
  EXPECT_EQ(static_cast<int>(result / 1000),
            794);  // Equal on the order of thousands
}

}  // namespace brave_perf_predictor
