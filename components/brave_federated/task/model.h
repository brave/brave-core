/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_MODEL_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_MODEL_H_

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "brave/components/brave_federated/util/linear_algebra_util.h"

namespace brave_federated {

struct ModelSpec {
  int num_params;
  int batch_size;
  float learning_rate;
  int num_iterations;
  float threshold;
};

struct PerformanceReport {
  size_t dataset_size;
  float loss;
  float accuracy;
  std::vector<Weights> parameters;
  std::map<std::string, double> metrics;

  PerformanceReport(size_t dataset_size,
                    float loss,
                    float accuracy,
                    std::vector<Weights> parameters,
                    std::map<std::string, double> metrics);
  PerformanceReport(const PerformanceReport& other);
  ~PerformanceReport();
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
  int num_iterations_;
  int batch_size_;
  float learning_rate_;
  float threshold_;

  Weights weights_;
  float bias_;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_MODEL_H_
