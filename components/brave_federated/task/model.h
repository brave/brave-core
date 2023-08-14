/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_MODEL_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_MODEL_H_

#include <map>
#include <string>
#include <vector>

#include "base/types/expected.h"
#include "brave/components/brave_federated/api/config.h"
#include "brave/components/brave_federated/util/linear_algebra_util.h"

namespace brave_federated {

struct PerformanceReportInfo {
  size_t dataset_size;
  float loss;
  float accuracy;
  std::vector<Weights> parameters;
  std::map<std::string, double> metrics;

  PerformanceReportInfo(size_t dataset_size,
                        float loss,
                        float accuracy,
                        const std::vector<Weights>& parameters,
                        const std::map<std::string, double>& metrics);
  PerformanceReportInfo(const PerformanceReportInfo& other);
  ~PerformanceReportInfo();
};

// This class implements basic linear model functionality to support training
// and evaluation of federated learning Tasks in the Browser. Results are
// returned as a PerformanceReport, which includes the loss, accuracy and
// parameters of the model, together with any additional metadata.
class Model {
 public:
  explicit Model(const api::config::ModelSpec& model_spec);
  ~Model();

  base::expected<std::vector<float>, std::string> Predict(
      const DataSet& dataset);
  base::expected<PerformanceReportInfo, std::string> Train(
      const DataSet& train_dataset);
  base::expected<PerformanceReportInfo, std::string> Evaluate(
      const DataSet& test_dataset);

  Weights GetWeights() const;
  void SetWeights(Weights new_weights);

  float GetBias() const;
  void SetBias(float new_bias);

  size_t GetModelSize() const;
  size_t GetBatchSize() const;

 private:
  const int num_iterations_;
  const int batch_size_;
  const float learning_rate_;
  const float threshold_;

  Weights weights_;
  float bias_ = 0.0;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_MODEL_H_
