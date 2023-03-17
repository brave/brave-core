/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_MODEL_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_MODEL_UTIL_H_

#include <vector>

namespace brave_federated {

float ComputeNLL(std::vector<float> true_labels,
                 std::vector<float> predictions);
float SigmoidActivation(float z);

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_MODEL_UTIL_H_
