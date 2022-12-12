/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_UTIL_SYNTHETIC_DATASET_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_UTIL_SYNTHETIC_DATASET_H_

#include <string>
#include <vector>

#include "brave/components/brave_federated/util/linear_algebra_util.h"

namespace brave_federated {

class SyntheticDataset {
 public:
  // Generates the synthetic dataset of size |size| around given vector |m| of
  // size ms_size and given bias |b|.
  explicit SyntheticDataset(int number_of_samples);
  ~SyntheticDataset();
  SyntheticDataset(const SyntheticDataset& synthetic_dataset);

  explicit SyntheticDataset(DataSet data_points);
  // SyntheticDataset(Weights ms, float b, size_t size);
  // SyntheticDataset(float alpha, float beta, int num_features, size_t size);
  SyntheticDataset(std::vector<Weights> W,
                   std::vector<float> b,
                   int num_features,
                   size_t size);

  SyntheticDataset SeparateTestData(int num_training);

  size_t size();
  int CountFeatures();
  DataSet GetDataPoints();

 private:
  float Softmax(float z);
  std::vector<Weights> GetDefaultWeights();
  std::vector<float> GetDefaultBias();

  DataSet data_points_;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_UTIL_SYNTHETIC_DATASET_H_
