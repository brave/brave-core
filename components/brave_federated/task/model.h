/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_CLIENT_MODEL_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_CLIENT_MODEL_H_

#include <memory>
#include <tuple>
#include <vector>

#include "brave/components/brave_federated/util/linear_algebra_util.h"

namespace brave_federated {

struct ModelSpec {
  int batch_size;
  float learning_rate;
  int num_iterations;
  int num_params;
  float threshold;
};

struct PerformanceReport {
  size_t dataset_size;
  float loss;
  float accuracy;
};

class Model {
 public:
  explicit Model(ModelSpec model_spec);
  ~Model();

  std::vector<float> Predict(const DataSet& dataset);
  PerformanceReport Train(const DataSet& train_dataset);
  PerformanceReport Evaluate(const DataSet& test_dataset);

  Weights GetWeights();
  void SetWeights(Weights new_weights);

  float GetBias();
  void SetBias(float new_bias);

  size_t ModelSize();

 private:
  float ComputeNLL(std::vector<float> true_labels,
                   std::vector<float> predictions);
  float Activation(float z);

  int num_iterations_;
  int batch_size_;
  float learning_rate_;
  float threshold_;

  Weights weights_;
  float bias_;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_CLIENT_MODEL_H_
