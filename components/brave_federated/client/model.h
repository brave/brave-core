/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_CLIENT_MODEL_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_CLIENT_MODEL_H_

#include <memory>
#include <vector>

namespace brave_federated {

class Model {
 public:
  Model(int num_iterations, float learning_rate, int num_params);

  ~Model();

  std::vector<float> Predict(std::vector<std::vector<float>> X);

  std::tuple<size_t, float, float> Train(
      std::vector<std::vector<float>>& dataset);

  std::tuple<size_t, float, float> Evaluate(
      std::vector<std::vector<float>>& test_dataset);

  std::vector<float> PredWeights();

  void SetPredWeights(std::vector<float> new_pred_weights);

  float Bias();

  void SetBias(float new_bias);

  size_t ModelSize();

 private:
  int num_iterations_;
  int batch_size_;
  float learning_rate_;
  float threshold_;

  std::vector<float> pred_weights_;
  float pred_b_;

  float ComputeNLL(std::vector<float> true_y, std::vector<float> pred);

  float Activation(float z);
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_CLIENT_MODEL_H_