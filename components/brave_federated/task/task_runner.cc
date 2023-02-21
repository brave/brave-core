/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/task/task_runner.h"
#include <__fwd/get.h>

#include <list>
#include <map>
#include <sstream>

#include "base/check.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/threading/sequence_bound.h"
#include "base/threading/sequenced_task_runner_handle.h"

#include "brave/components/brave_federated/task/model.h"
#include "brave/components/brave_federated/task/typing.h"

#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace brave_federated {

TaskRunner::TaskRunner(Task task, Model* model) : task_(task), model_(model) {
  DCHECK(model_);
}

TaskRunner::~TaskRunner() = default;

Model* TaskRunner::GetModel() {
  return model_;
}

TaskResult TaskRunner::Run() {
  PerformanceReport report(0, 0, 0, {});
  if (task_.GetType() == TaskType::Training) {
    report = model_->Train(training_data_);
  } else if (task_.GetType() == TaskType::Evaluation) {
    report = model_->Evaluate(test_data_);
  }

  TaskResult result(task_, report);
  return result;
}

void TaskRunner::SetTrainingData(DataSet training_data) {
  training_data_ = training_data;
}

void TaskRunner::SetTestData(DataSet test_data) {
  test_data_ = test_data;
}

void TaskRunner::SetWeights(ModelWeights weights) {
  model_->SetWeights(std::get<0>(weights));
  model_->SetBias(std::get<1>(weights));
}

ModelWeights TaskRunner::GetWeights() {
  return std::make_tuple(model_->GetWeights(), model_->GetBias());
}

}  // namespace brave_federated
