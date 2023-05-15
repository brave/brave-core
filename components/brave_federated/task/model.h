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
#include <tuple>
#include <vector>

#include "base/json/json_value_converter.h"
#include "brave/components/brave_federated/util/linear_algebra_util.h"

namespace brave_federated {

struct ModelSpec {
  int num_params;
  int batch_size;
  double learning_rate;
  int num_iterations;
  double threshold;

  static void RegisterJSONConverter(
      base::JSONValueConverter<ModelSpec>* spec_converter) {
    spec_converter->RegisterIntField("num_params", &ModelSpec::num_params);
    spec_converter->RegisterIntField("batch_size", &ModelSpec::batch_size);
    spec_converter->RegisterDoubleField("learning_rate",
                                        &ModelSpec::learning_rate);
    spec_converter->RegisterIntField("num_iterations",
                                     &ModelSpec::num_iterations);
    spec_converter->RegisterDoubleField("threshold", &ModelSpec::threshold);
  }

  friend std::ostream& operator<<(std::ostream& os, const ModelSpec& obj) {
    os << "num_params=" << obj.num_params
       << ", "
          "batch_size="
       << obj.batch_size
       << ", "
          "learning_rate="
       << obj.learning_rate
       << ", "
          "num_iterations="
       << obj.num_iterations
       << ", "
          "threshold="
       << obj.threshold;
    return os;
  }
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
  void SetWeights(const Weights& new_weights);

  float GetBias();
  void SetBias(float new_bias);

  size_t GetModelSize() const;
  size_t GetBatchSize() const;

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
