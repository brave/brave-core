/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/util/synthetic_dataset.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <utility>

#include "base/check_op.h"
#include "base/rand_util.h"

namespace brave_federated {

SyntheticDataset::SyntheticDataset(std::vector<std::vector<float>> data_points)
    : data_points_(std::move(data_points)) {}

SyntheticDataset::SyntheticDataset(std::vector<std::vector<float>> weights,
                                   std::vector<float> b,
                                   int num_features,
                                   size_t size) {
  // Generate time of day and day of week uniformly.
  const int max_day = 7;
  const int max_time = 144;

  std::vector<float> cov_x(num_features, 0.0);
  for (int j = 0; j < num_features; j++) {
    cov_x[j] = pow((j + 1), -1.2);
  }

  std::vector<float> mean_x(num_features, 0.0);

  std::vector<std::vector<float>> xs(size,
                                     std::vector<float>(num_features, 0.0));

  for (size_t j = 0; j < size; j++) {
    int day_index = base::RandInt(0, max_day);
    int time_index = base::RandInt(0, max_time);

    xs[j][0] = sin(day_index * 2.0 * M_PI / 7.0);
    xs[j][1] = cos(day_index * 2.0 * M_PI / 7.0);

    xs[j][2] = sin(time_index * 2.0 * M_PI / 144.0);
    xs[j][3] = cos(time_index * 2.0 * M_PI / 144.0);
  }

  base::RandomBitGenerator generator;
  for (int i = 4; i < num_features; i++) {
    std::normal_distribution<float> normal_i(mean_x[i], cov_x[i]);
    for (size_t j = 0; j < size; j++) {
      xs[j][i] = normal_i(generator);
    }
  }

  std::vector<std::vector<float>> data_points;
  for (size_t i = 0; i < size; i++) {
    std::vector<float> y_s = LinearAlgebraUtil::AddVectors(
        LinearAlgebraUtil::MultiplyMatrixVector(weights, xs[i]), b);
    DCHECK_EQ(y_s.size(), 2U);

    float y_max = 0.0;
    if (Softmax(y_s[0]) >= Softmax(y_s[1])) {
      y_max = 1.0;
    } else {
      y_max = 0.0;
    }

    std::vector<float> data_point;
    data_point.insert(data_point.end(), xs[i].begin(), xs[i].end());
    data_point.push_back(y_max);

    data_points.push_back(data_point);
  }

  data_points_ = data_points;
}

SyntheticDataset::SyntheticDataset(int number_of_samples)
    : SyntheticDataset(GetDefaultWeights(),
                       GetDefaultBias(),
                       32,
                       number_of_samples) {}

SyntheticDataset::SyntheticDataset(const SyntheticDataset& synthetic_dataset) =
    default;

SyntheticDataset::~SyntheticDataset() = default;

size_t SyntheticDataset::size() {
  return data_points_.size();
}

int SyntheticDataset::CountFeatures() {
  return data_points_[0].size() - 1;
}

std::vector<std::vector<float>> SyntheticDataset::GetDataPoints() {
  return data_points_;
}

SyntheticDataset SyntheticDataset::SeparateTestData(int num_training) {
  std::vector<std::vector<float>> split_lo(data_points_.begin(),
                                           data_points_.begin() + num_training);
  std::vector<std::vector<float>> split_hi(data_points_.begin() + num_training,
                                           data_points_.end());

  data_points_ = split_lo;
  return SyntheticDataset(split_hi);
}

float SyntheticDataset::Softmax(float z) {
  return 1.0 / (1 + exp(-1.0 * z));
}

std::vector<Weights> SyntheticDataset::GetDefaultWeights() {
  std::vector<std::vector<float>> weights(2, std::vector<float>(32));
  weights[0] = {0.720553,  -0.22378,   0.724898,   1.05209,    0.171692,
                -2.08635,  0.00898889, 0.00195967, -0.521962,  -1.69172,
                -0.906425, -1.05066,   -0.920127,  -0.200614,  -0.0248187,
                -0.510679, 0.139501,   1.44922,    -0.0535475, -0.497441,
                -0.902036, 1.08325,    -1.31984,   0.413791,   -1.44259,
                0.757306,  0.670382,   -1.13497,   -0.278086,  -1.30519,
                0.111584,  -0.362997};

  weights[1] = {-1.20866,    -0.385986,  -1.37335,    1.54405,   1.19847,
                0.185225,    0.446334,   -0.00641536, -0.439716, 2.525,
                -0.638792,   1.5815,     -0.933648,   -0.240064, -1.0451,
                -0.00015671, -0.543405,  0.560255,    -1.80757,  -0.907905,
                2.27475,     0.42947,    0.725056,    -1.54398,  -2.43804,
                -1.07677,    0.00487297, -1.25289,    -0.708508, 0.322749,
                0.91749,     -0.598813};

  return weights;
}

std::vector<float> SyntheticDataset::GetDefaultBias() {
  std::vector<float> bias = {-1.45966, 1.12165};

  return bias;
}

}  // namespace brave_federated
