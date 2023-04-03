/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/task/model_util.h"

#include <cmath>

namespace brave_federated {

float ComputeNLL(std::vector<float> true_labels,
                 std::vector<float> predictions) {
  float error = 0.0;
  size_t batch_size = true_labels.size();

  for (size_t i = 0; i < batch_size; i++) {
    error += (true_labels[i] * log(predictions[i]) +
              (1.0 - true_labels[i]) * log(1 - predictions[i]));
  }

  return -error / batch_size;
}

float SigmoidActivation(float z) {
  return 1.0 / (1 + exp(-1.0 * z));
}

}  // namespace brave_federated
