/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/task/federated_task_runner.h"

#include <list>
#include <map>
#include <utility>

#include "base/check.h"

#include "brave/components/brave_federated/task/model.h"
#include "brave/components/brave_federated/task/typing.h"

namespace brave_federated {

FederatedTaskRunner::FederatedTaskRunner(Task task,
                                         std::unique_ptr<Model> model)
    : task_(task), model_(std::move(model)) {
  DCHECK(model_);
}

FederatedTaskRunner::~FederatedTaskRunner() = default;

Model* FederatedTaskRunner::GetModel() {
  return model_.get();
}

TaskResult FederatedTaskRunner::Run() {
  PerformanceReport report(0, 0, 0, {}, {});
  if (task_.GetType() == TaskType::Training) {
    report = model_->Train(training_data_);
  } else if (task_.GetType() == TaskType::Evaluation) {
    report = model_->Evaluate(test_data_);
  }

  TaskResult result(task_, report);
  return result;
}

void FederatedTaskRunner::SetTrainingData(DataSet training_data) {
  training_data_ = training_data;
}

void FederatedTaskRunner::SetTestData(DataSet test_data) {
  test_data_ = test_data;
}

void FederatedTaskRunner::SetWeights(ModelWeights weights) {
  model_->SetWeights(std::get<0>(weights));
  model_->SetBias(std::get<1>(weights));
}

ModelWeights FederatedTaskRunner::GetWeights() {
  return std::make_tuple(model_->GetWeights(), model_->GetBias());
}

}  // namespace brave_federated
