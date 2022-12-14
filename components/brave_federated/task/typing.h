/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_TYPING_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_TYPING_H_

#include <string>
#include <vector>

#include "brave/components/brave_federated/task/model.h"

namespace brave_federated {

enum TaskType {
  Evaluation,
  Training,
};

class Task {
 public:
  Task(int task_id, TaskType type, std::string token);
  ~Task();

  int GetId();
  TaskType GetType();
  std::string GetToken();

 private:
  int task_id_;
  TaskType type_;
  std::string token_;
};

class TaskResult {
 public:
  explicit TaskResult(Task task, PerformanceReport report);
  ~TaskResult();

  Task GetTask();
  PerformanceReport GetReport();

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
