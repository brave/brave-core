/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/task/typing.h"

namespace brave_federated {

Task::Task(TaskId task_id,
           TaskType type,
           std::string token,
           const std::vector<Weights>& parameters,
           const std::map<std::string, float>& config)
    : task_id_(task_id),
      type_(type),
      token_(token),
      parameters_(parameters),
      config_(config) {}
Task::Task(const Task& other) = default;
Task::~Task() = default;

TaskResult::TaskResult(const Task& task, const PerformanceReportInfo& report)
    : task_(task), report_(report) {}

TaskResult::TaskResult(const TaskResult& other) = default;
TaskResult::~TaskResult() = default;

TaskResultResponse::TaskResultResponse(bool success) : success_(success) {}

bool TaskResultResponse::IsSuccessful() const {
  return success_;
}

}  // namespace brave_federated
