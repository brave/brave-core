/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_TYPING_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_TYPING_H_

#include <map>
#include <string>
#include <vector>

#include "brave/components/brave_federated/task/model.h"
#include "brave/components/brave_federated/util/linear_algebra_util.h"

namespace brave_federated {

enum TaskType {
  Evaluation,
  Training,
};

struct TaskId {
  std::string id;
  std::string group_id;
  std::string family_id;
};

class Task {
 public:
  Task(TaskId task_id,
       TaskType type,
       std::string token,
       std::vector<Weights> parameters,
       std::map<std::string, float> config);
  Task(const Task& other);
  ~Task();

  TaskId GetId();
  TaskType GetType();
  std::string GetToken();
  std::vector<Weights> GetParameters();
  std::map<std::string, float> GetConfig();

 private:
  TaskId task_id_;
  TaskType type_;
  std::string token_;
  std::vector<Weights> parameters_;
  std::map<std::string, float> config_;
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
