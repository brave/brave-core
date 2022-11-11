/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/task/typing.h"
#include "brave/components/brave_federated/task/model.h"

namespace brave_federated {

Task::Task(int task_id, TaskType type) : task_id_(task_id), type_(type) {}

int Task::GetId() {
  return task_id_;
}

TaskType Task::GetType() {
  return type_;
}

TaskResult::TaskResult(Task task, PerformanceReport report)
    : task_(task), report_(report) {}

TaskResultResponse::TaskResultResponse(bool success) : success_(success) {}
bool TaskResultResponse::IsSuccessful() {
  return success_;
};

}  // namespace brave_federated
