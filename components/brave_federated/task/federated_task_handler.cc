/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/task/federated_task_handler.h"

#include <map>
#include <string>
#include <utility>

#include "base/check.h"

#include "base/types/expected.h"
#include "brave/components/brave_federated/task/model.h"

namespace brave_federated {

FederatedTaskHandler::FederatedTaskHandler(const Task& task,
                                           std::unique_ptr<Model> model)
    : task_(task), model_(std::move(model)) {
  CHECK(model_);
}

FederatedTaskHandler::~FederatedTaskHandler() = default;

base::expected<TaskResult, std::string> FederatedTaskHandler::Run() {
  PerformanceReportInfo report(0, 0, 0, {}, {});
  if (task_.GetType() == TaskType::kTraining) {
    auto result = model_->Train(training_data_);
    if (!result.has_value()) {
      base::unexpected(result.error());
    }

    report = result.value();
  } else if (task_.GetType() == TaskType::kEvaluation) {
    auto result = model_->Evaluate(test_data_);

    if (!result.has_value()) {
      base::unexpected(result.error());
    }

    report = result.value();
  }

  TaskResult result(task_, report);
  return result;
}

void FederatedTaskHandler::SetTrainingData(const DataSet& training_data) {
  training_data_ = training_data;
}

void FederatedTaskHandler::SetTestData(const DataSet& test_data) {
  test_data_ = test_data;
}

}  // namespace brave_federated
