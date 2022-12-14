/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_TASK_RUNNER_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_TASK_RUNNER_H_

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_federated/task/model.h"
#include "brave/components/brave_federated/task/typing.h"
#include "brave/components/brave_federated/util/linear_algebra_util.h"
// #include "brave/third_party/flower/src/cc/flwr/include/client.h"

namespace brave_federated {

class Model;
class Task;
using ModelWeights = std::tuple<Weights, float>;

class TaskRunner final {
 public:
  TaskRunner(Task task, Model* model);
  ~TaskRunner();

  Model* GetModel();

  TaskResult Run();

  void SetTrainingData(DataSet training_data);
  void SetTestData(DataSet test_data);

  void SetWeights(ModelWeights weights);
  ModelWeights GetWeights();

 private:
  PerformanceReport Evaluate();
  PerformanceReport Train();

  Task task_;
  raw_ptr<Model> model_ = nullptr;
  DataSet training_data_;
  DataSet test_data_;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_TASK_RUNNER_H_
