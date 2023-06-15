/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/task/federated_task_handler.h"

#include <list>
#include <map>
#include <utility>

#include "base/check.h"
#include "base/logging.h"

#include "brave/components/brave_federated/task/model.h"
#include "brave/components/brave_federated/task/typing.h"

namespace brave_federated {

FederatedTaskHandler::FederatedTaskHandler(const Task& task,
                                           std::unique_ptr<Model> model)
    : task_(task), model_(std::move(model)) {
  DCHECK(model_);
}

FederatedTaskHandler::~FederatedTaskHandler() = default;

absl::optional<TaskResult> FederatedTaskHandler::Run() {
  PerformanceReport report(0, 0, 0, {}, {});
  if (task_.GetType() == TaskType::kTraining) {
    if (training_data_.empty()) {
      VLOG(1) << "Training data empty";
      return absl::nullopt;
    }
    if (model_->GetBatchSize() > training_data_.size()) {
      VLOG(1) << "Batch size (" << model_->GetBatchSize()
              << ") < training dataset size(" << training_data_.size() << ")";
      return absl::nullopt;
    }

    report = model_->Train(training_data_);
  } else if (task_.GetType() == TaskType::kEvaluation) {
    if (test_data_.empty()) {
      VLOG(1) << "Test data empty";
      return absl::nullopt;
    }
    if (model_->GetBatchSize() > test_data_.size()) {
      VLOG(1) << "Batch size (" << model_->GetBatchSize()
              << ") < Test dataset size(" << test_data_.size() << ")";
      return absl::nullopt;
    }

    report = model_->Evaluate(test_data_);
  }

  TaskResult result(task_, report);
  return absl::optional<TaskResult>(result);
}

void FederatedTaskHandler::SetTrainingData(const DataSet& training_data) {
  training_data_ = training_data;
}

void FederatedTaskHandler::SetTestData(const DataSet& test_data) {
  test_data_ = test_data;
}

void FederatedTaskHandler::SetWeights(const ModelWeights& weights) {
  model_->SetWeights(std::get<0>(weights));
  model_->SetBias(std::get<1>(weights));
}

ModelWeights FederatedTaskHandler::GetWeights() const {
  return std::make_tuple(model_->GetWeights(), model_->GetBias());
}

}  // namespace brave_federated
