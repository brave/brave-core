/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_MODEL_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_MODEL_H_

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_federated/api/config.h"
#include "brave/components/brave_federated/util/linear_algebra_util.h"

namespace brave_federated {

struct PerformanceReport {
  size_t dataset_size;
  float loss;
  float accuracy;
  std::vector<Weights> parameters;
  std::map<std::string, double> metrics;

  PerformanceReport(size_t dataset_size,
                    float loss,
                    float accuracy,
                    const std::vector<Weights>& parameters,
                    const std::map<std::string, double>& metrics);
  PerformanceReport(const PerformanceReport& other);
  ~PerformanceReport();
};

// This class implements basic linear model functionality to support training
// and evaluation of federated learning Tasks in the Browser. Results are
// returned as a PerformanceReport, which includes the loss, accuracy and
// parameters of the model, together with any additional metadata.
class Model {
 public:
  explicit Model(const api::config::ModelSpec& model_spec);
  ~Model();

  std::vector<float> Predict(const DataSet& dataset);
  PerformanceReport Train(const DataSet& train_dataset);
  PerformanceReport Evaluate(const DataSet& test_dataset);

  Weights GetWeights();
  void SetWeights(const Weights& new_weights);

  float GetBias();
  void SetBias(float new_bias);

  size_t GetModelSize() const;
  size_t GetBatchSize() const;

 private:
  const int num_iterations_;
  const int batch_size_;
  const float learning_rate_;
  const float threshold_;

  Weights weights_ = {};
  float bias_ = 0.0;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_MODEL_H_
