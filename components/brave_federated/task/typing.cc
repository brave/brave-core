/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/task/typing.h"

#include <vector>

#include "brave/components/brave_federated/task/model.h"

namespace brave_federated {

Task::Task(TaskId task_id,
           TaskType type,
           std::string token,
           std::vector<Weights> parameters,
           std::map<std::string, float> config)
    : task_id_(task_id),
      type_(type),
      token_(token),
      parameters_(parameters),
      config_(config) {}
Task::Task(const Task& other) = default;
Task::~Task() = default;

TaskId Task::GetId() {
  return task_id_;
}

TaskType Task::GetType() {
  return type_;
}

std::string Task::GetToken() {
  return token_;
}

std::vector<Weights> Task::GetParameters() {
  return parameters_;
}

std::map<std::string, float> Task::GetConfig() {
  return config_;
}

TaskResult::TaskResult(Task task, PerformanceReport report)
    : task_(task), report_(report) {}

Task TaskResult::GetTask() {
  return task_;
}

PerformanceReport TaskResult::GetReport() {
  return report_;
}

TaskResult::~TaskResult() = default;

TaskResultResponse::TaskResultResponse(bool success) : success_(success) {}

bool TaskResultResponse::IsSuccessful() {
  return success_;
}

TaskResultResponse::~TaskResultResponse() = default;

}  // namespace brave_federated
