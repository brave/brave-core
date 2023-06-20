/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/task/model_util.h"

#include <cmath>

#include "base/check_op.h"

namespace brave_federated {

float ComputeNegativeLogLikelihood(const std::vector<float>& true_labels,
                                   const std::vector<float>& predictions) {
  CHECK_EQ(true_labels.size(), predictions.size());
  float error = 0.0;
  const size_t batch_size = true_labels.size();

  for (size_t i = 0; i < batch_size; i++) {
    DCHECK_GT(predictions.at(i), 0.0);
    error += (true_labels.at(i) * log(predictions.at(i)) +
              (1.0 - true_labels.at(i)) * log(1 - predictions.at(i)));
  }

  return -error / batch_size;
}

float SigmoidActivation(float z) {
  return 1.0 / (1 + exp(-1.0 * z));
}

}  // namespace brave_federated
