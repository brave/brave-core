/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/util/synthetic_dataset.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <random>

#include "base/time/time.h"

namespace brave_federated {

SyntheticDataset::SyntheticDataset(std::vector<std::vector<float>> data_points)
    : data_points_(data_points) {}

SyntheticDataset::SyntheticDataset(std::vector<std::vector<float>> W,
                                   std::vector<float> b,
                                   int num_features,
                                   size_t size) {
  // Generate time of day and day of week uniformly.
  std::uniform_int_distribution<> distrday(0, 7);
  std::uniform_int_distribution<> distrtime(0, 144);

  std::default_random_engine generator;
  generator.seed(base::Time::Now().ToInternalValue());

  std::vector<float> cov_x(num_features, 0.0);
  for (int j = 0; j < num_features; j++) {
    cov_x[j] = pow((j + 1), -1.2);
  }

  std::vector<float> mean_x(num_features, 0.0);

  std::vector<std::vector<float>> xs(size,
                                     std::vector<float>(num_features, 0.0));

  for (size_t j = 0; j < size; j++) {
    int day_index = distrday(generator);
    int time_index = distrtime(generator);

    xs[j][0] = sin(day_index * 2.0 * M_PI / 7.0);
    xs[j][1] = cos(day_index * 2.0 * M_PI / 7.0);

    xs[j][2] = sin(time_index * 2.0 * M_PI / 144.0);
    xs[j][3] = cos(time_index * 2.0 * M_PI / 144.0);
  }

  for (int i = 4; i < num_features; i++) {
    std::normal_distribution<float> normal_i(mean_x[i], cov_x[i]);
    for (size_t j = 0; j < size; j++) {
      xs[j][i] = normal_i(generator);
    }
  }

  std::vector<std::vector<float>> data_points;
  for (size_t i = 0; i < size; i++) {
    std::vector<float> tmp = LinearAlgebraUtil::AddVectors(
        LinearAlgebraUtil::MultiplyMatrixVector(W, xs[i]), b);

    float ymax = 0.0;

    if (Softmax(tmp[0]) >= Softmax(tmp[1])) {
      ymax = 1.0;
    } else {
      ymax = 0.0;
    }

    std::vector<float> data_point;
    data_point.insert(data_point.end(), xs[i].begin(), xs[i].end());
    data_point.push_back(ymax);

    data_points.push_back(data_point);
  }

  data_points_ = data_points;
}

SyntheticDataset::SyntheticDataset(int number_of_samples)
    : SyntheticDataset(GetDefaultWeights(),
                       GetDefaultBias(),
                       32,
                       number_of_samples) {}

// SyntheticDataset::SyntheticDataset(std::vector<float> ms,
//                                    float b,
//                                    size_t size) {
//   std::random_device rd;
//   std::mt19937 mt(rd());
//   std::uniform_int_distribution<> distr(-10.0, 10.0);

//   std::vector<std::vector<float>> xs(size, std::vector<float>(ms.size()));
//   std::vector<float> ys(size, 0);
//   for (size_t m_ind = 0; m_ind < ms.size(); m_ind++) {
//     std::uniform_real_distribution<float> distx(-10.0, 10.0);

//     for (size_t i = 0; i < size; i++) {
//       xs[i][m_ind] = distx(mt);
//     }
//   }

//   for (size_t i = 0; i < size; i++) {
//     ys[i] = b;
//     for (size_t m_ind = 0; m_ind < ms.size(); m_ind++) {
//       ys[i] += ms[m_ind] * xs[i][m_ind];
//     }
//   }

//   std::vector<std::vector<float>> data_points;
//   for (size_t i = 0; i < size; i++) {
//     std::vector<float> data_point;
//     data_point.insert(data_point.end(), xs[i].begin(), xs[i].end());
//     if (ys[i] >= 0.0) {
//       data_point.push_back(1.0);
//     } else {
//       data_point.push_back(0.0);
//     }

//     data_points.push_back(data_point);
//   }

//   data_points_ = data_points;
// }

// SyntheticDataset::SyntheticDataset(float alpha,
//                                    float beta,
//                                    int num_features,
//                                    size_t size) {
//   // Generate time of day and day of week uniformly.
//   std::uniform_int_distribution<> distrday(0, 7);
//   std::uniform_int_distribution<> distrtime(0, 144);

//   std::default_random_engine generator;
//   generator.seed(base::Time::Now().ToInternalValue());

//   std::normal_distribution<float> normal_zero_alpha(0.0, alpha);
//   std::normal_distribution<float> normal_zero_beta(0.0, beta);

//   std::vector<float> cov_x(num_features, 0.0);
//   for (int j = 0; j < num_features; j++) {
//     cov_x[j] = pow((j + 1), -1.2);
//   }

//   std::vector<float> mean_x(num_features, 1.0);
//   std::normal_distribution<float> normal_aux_1(normal_zero_beta(generator),
//                                                1.0);
//   for (int i = 0; i < num_features; i++) {
//     mean_x[i] = normal_aux_1(generator);
//   }

//   float mean_W = normal_zero_alpha(generator);
//   std::vector<std::vector<float>> W(2, std::vector<float>(num_features));
//   std::vector<float> b(2);
//   std::normal_distribution<float> normal_W(mean_W, 1.0);
//   for (int i = 0; i < 2; i++) {
//     for (int j = 0; j < num_features; j++) {
//       W[i][j] = normal_W(generator);
//     }
//     b[i] = normal_W(generator);
//   }

//   std::vector<std::vector<float>> xs(size,
//                                      std::vector<float>(num_features, 0.0));

//   for (size_t j = 0; j < size; j++) {
//     int day_index = distrday(generator);
//     int time_index = distrtime(generator);

//     xs[j][0] = sin(day_index * 2.0 * M_PI / 7.0);
//     xs[j][1] = cos(day_index * 2.0 * M_PI / 7.0);

//     xs[j][2] = sin(time_index * 2.0 * M_PI / 144.0);
//     xs[j][3] = cos(time_index * 2.0 * M_PI / 144.0);
//   }

//   for (int i = 4; i < num_features; i++) {
//     std::normal_distribution<float> normal_i(mean_x[i], cov_x[i]);
//     for (size_t j = 0; j < size; j++) {
//       xs[j][i] = normal_i(generator);
//     }
//   }

//   std::vector<std::vector<float>> data_points;
//   for (size_t i = 0; i < size; i++) {
//     std::vector<float> tmp = LinearAlgebraUtil::AddVectors(
//         LinearAlgebraUtil::MultiplyMatrixVector(W, xs[i]), b);

//     float ymax = 0.0;
//     // float val_ymax = -100.0;

//     if (Softmax(tmp[0]) >= Softmax(tmp[1])) {
//       ymax = 1.0;
//     } else {
//       ymax = 0.0;
//     }

//     std::vector<float> data_point;
//     data_point.insert(data_point.end(), xs[i].begin(), xs[i].end());
//     data_point.push_back(ymax);

//     data_points.push_back(data_point);
//   }

//   data_points_ = data_points;
// }

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
  std::vector<std::vector<float>> W(2, std::vector<float>(32));
  std::vector<float> W0 = {
      0.720553,   -0.22378,   0.724898,   1.05209,   0.171692,  -2.08635,
      0.00898889, 0.00195967, -0.521962,  -1.69172,  -0.906425, -1.05066,
      -0.920127,  -0.200614,  -0.0248187, -0.510679, 0.139501,  1.44922,
      -0.0535475, -0.497441,  -0.902036,  1.08325,   -1.31984,  0.413791,
      -1.44259,   0.757306,   0.670382,   -1.13497,  -0.278086, -1.30519,
      0.111584,   -0.362997};
  W[0] = W0;

  std::vector<float> W1 = {
      -1.20866,  -0.385986,   -1.37335,   1.54405,     1.19847,   0.185225,
      0.446334,  -0.00641536, -0.439716,  2.525,       -0.638792, 1.5815,
      -0.933648, -0.240064,   -1.0451,    -0.00015671, -0.543405, 0.560255,
      -1.80757,  -0.907905,   2.27475,    0.42947,     0.725056,  -1.54398,
      -2.43804,  -1.07677,    0.00487297, -1.25289,    -0.708508, 0.322749,
      0.91749,   -0.598813};
  W[1] = W1;

  return W;
}

std::vector<float> SyntheticDataset::GetDefaultBias() {
  std::vector<float> b(2);
  b[0] = -1.45966;
  b[1] = 1.12165;

  return b;
}

}  // namespace brave_federated
