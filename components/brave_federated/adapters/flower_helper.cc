/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/adapters/flower_helper.h"

#include <sstream>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "brave/components/brave_federated/task/model.h"
#include "brave/third_party/flower/src/brave/flwr/serde.h"
#include "brave/third_party/flower/src/proto/flwr/proto/fleet.pb.h"
#include "brave/third_party/flower/src/proto/flwr/proto/node.pb.h"
#include "brave/third_party/flower/src/proto/flwr/proto/transport.pb.h"

namespace brave_federated {

std::string BuildGetTasksPayload() {
  std::string request;

  flower::Node node;
  node.set_node_id(0);
  node.set_anonymous(true);

  flower::PullTaskInsRequest pull_task_instructions_request;
  *pull_task_instructions_request.mutable_node() = node;
  pull_task_instructions_request.add_task_ids("0");
  pull_task_instructions_request.SerializeToString(&request);

  return request;
}

absl::optional<Task> ParseTask(const flower::TaskIns& task_instruction) {
  const std::string& id = task_instruction.task_id();
  const std::string& group_id = task_instruction.group_id();
  const std::string& workload_id = task_instruction.workload_id();
  if (id.empty() || group_id.empty() || workload_id.empty()) {
    VLOG(2) << "Invalid task id received from FL service";
    return absl::nullopt;
  }
  const TaskId& task_id = {
      .id = id, .group_id = group_id, .family_id = workload_id};

  if (!task_instruction.has_task()) {
    VLOG(2) << "Task object is missing from task instruction";
    return absl::nullopt;
  }
  flower::Task flower_task = task_instruction.task();

  if (!flower_task.has_legacy_server_message()) {
    VLOG(2) << "Server message is missing from task object";
    return absl::nullopt;
  }
  flower::ServerMessage message = flower_task.legacy_server_message();

  TaskType type;
  std::vector<Weights> parameters;
  Configs config;
  if (message.has_fit_ins()) {
    type = TaskType::kTraining;

    if (!message.fit_ins().has_parameters()) {
      VLOG(2) << "Parameters are missing from fit instruction";
      return absl::nullopt;
    }
    parameters = GetVectorsFromParameters(message.fit_ins().parameters());
    if (parameters.empty()) {
      VLOG(2) << "Parameters vectors received from FL service are empty";
      return absl::nullopt;
    }
    config = ConfigsFromProto(message.fit_ins().config());
  } else if (message.has_evaluate_ins()) {
    type = TaskType::kEvaluation;

    if (!message.evaluate_ins().has_parameters()) {
      VLOG(2) << "Parameters are missing from eval instruction";
      return absl::nullopt;
    }
    parameters = GetVectorsFromParameters(message.evaluate_ins().parameters());
    if (parameters.empty()) {
      VLOG(3) << "Parameters vectors received from FL service are empty";
      return absl::nullopt;
    }
    config = ConfigsFromProto(message.evaluate_ins().config());
  } else {
    VLOG(2) << "**: Received unrecognized instruction from FL service";
    return absl::nullopt;
  }

  return Task(task_id, type, "token", parameters, config);
}

absl::optional<TaskList> ParseTaskListFromResponseBody(
    const std::string& response_body) {
  flower::PullTaskInsResponse response;
  if (!response.ParseFromString(response_body)) {
    VLOG(1) << "Failed to parse response body";
    return absl::nullopt;
  }

  if (response.task_ins_list_size() == 0) {
    VLOG(1) << "No tasks received from FL service";
    return absl::nullopt;
  }

  TaskList task_list;
  for (int i = 0; i < response.task_ins_list_size(); i++) {
    flower::TaskIns task_instruction = response.task_ins_list(i);

    absl::optional<Task> task = ParseTask(task_instruction);
    if (!task.has_value()) {
      VLOG(1) << "Failed to parse task instruction";
      continue;
    }

    task_list.push_back(std::move(task.value()));

    return task_list;
  }

  VLOG(1) << "Failed to parse PullTaskInsRes";
  return absl::nullopt;
}

std::string BuildUploadTaskResultsPayload(const TaskResult& result) {
  const Task task = result.GetTask();
  const TaskId task_id = task.GetId();
  const TaskType task_type = task.GetType();
  const PerformanceReport report = result.GetReport();

  flower::Task flower_task;
  // Client Message Creation
  flower::ClientMessage client_message;
  if (task_type == TaskType::kTraining) {
    flower::ClientMessage_FitRes fit_res;
    fit_res.set_num_examples(report.dataset_size);
    *fit_res.mutable_parameters() = GetParametersFromVectors(report.parameters);
    if (!report.metrics.empty()) {
      *fit_res.mutable_metrics() = MetricsToProto(report.metrics);
    }
    *client_message.mutable_fit_res() = fit_res;
  } else {
    flower::ClientMessage_EvaluateRes eval_res;
    eval_res.set_num_examples(report.dataset_size);
    eval_res.set_loss(report.loss);
    if (!report.metrics.empty()) {
      *eval_res.mutable_metrics() = MetricsToProto(report.metrics);
    }
    *client_message.mutable_evaluate_res() = eval_res;
  }
  flower_task.add_ancestry(task_id.id);

  flower::Node producer_node;
  producer_node.set_node_id(0);
  producer_node.set_anonymous(true);

  flower::Node consumer_node;
  consumer_node.set_node_id(0);
  consumer_node.set_anonymous(true);

  *flower_task.mutable_consumer() = consumer_node;
  *flower_task.mutable_producer() = producer_node;
  *flower_task.mutable_legacy_client_message() = client_message;

  flower::PushTaskResRequest task_results;
  flower::TaskRes* task_result = task_results.add_task_res_list();
  task_result->set_task_id("");
  task_result->set_group_id(task_id.group_id);
  task_result->set_workload_id(task_id.family_id);
  *task_result->mutable_task() = flower_task;

  std::string result_payload;
  task_results.SerializeToString(&result_payload);

  return result_payload;
}

}  // namespace brave_federated
