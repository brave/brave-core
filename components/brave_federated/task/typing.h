/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_TYPING_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_TYPING_H_

#include <vector>
#include "brave/components/brave_federated/task/model.h"

namespace brave_federated {

enum TaskType {
  EvaluationTask,
  TrainingTask,
};

class Task {
 public:
  Task(int task_id, TaskType type);
  ~Task();

  int GetId();
  TaskType GetType();

 private:
  int task_id_;
  TaskType type_;
};

class TaskResult {
 public:
  explicit TaskResult(Task task, PerformanceReport report);
  ~TaskResult();

 private:
  Task task_;
  PerformanceReport report_;
};

class TaskResultResponse {
 public:
  explicit TaskResultResponse(bool success);
  ~TaskResultResponse();

  bool IsSuccessful();

 private:
  bool success_;
};

using TaskList = std::vector<Task>;
using TaskResultList = std::vector<TaskResult>;

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_TYPING_H_
