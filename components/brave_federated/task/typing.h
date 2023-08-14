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

enum class TaskType { kEvaluation, kTraining, kUndefined };

struct TaskId {
  std::string id;
  std::string group_id;
  std::string family_id;

  [[nodiscard]] bool IsValid() const {
    return !id.empty() && !group_id.empty() && !family_id.empty();
  }
};

class Task {
 public:
  Task(TaskId task_id,
       TaskType type,
       std::string token,
       const std::vector<Weights>& parameters,
       const std::map<std::string, float>& config);
  Task(const Task& other);
  ~Task();

  const TaskId& GetId() const { return task_id_; }
  const TaskType& GetType() const { return type_; }
  const std::string& GetToken() const { return token_; }
  const std::vector<Weights>& GetParameters() const { return parameters_; }
  const std::map<std::string, float>& GetConfig() const { return config_; }

 private:
  const TaskId task_id_;
  const TaskType type_;
  const std::string token_;
  const std::vector<Weights> parameters_;
  const std::map<std::string, float> config_;
};

class TaskResult {
 public:
  TaskResult(const Task& task, const PerformanceReportInfo& report);
  TaskResult(const TaskResult& other);
  ~TaskResult();

  const Task& GetTask() const { return task_; }
  const PerformanceReportInfo& GetReport() const { return report_; }

 private:
  const Task task_;
  const PerformanceReportInfo report_;
};

class TaskResultResponse {
 public:
  explicit TaskResultResponse(bool success);
  ~TaskResultResponse() = default;

  bool IsSuccessful() const;

 private:
  const bool success_;
};

using TaskList = std::vector<Task>;
using TaskResultList = std::vector<TaskResult>;

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_TYPING_H_
