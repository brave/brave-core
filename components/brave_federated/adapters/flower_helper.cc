/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/adapters/flower_helper.h"

#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/types/expected.h"
#include "brave/components/brave_federated/task/model.h"
#include "brave/third_party/flower/src/brave/flwr/serde.h"
#include "brave/third_party/flower/src/proto/flwr/proto/fleet.pb.h"
#include "brave/third_party/flower/src/proto/flwr/proto/node.pb.h"
#include "brave/third_party/flower/src/proto/flwr/proto/transport.pb.h"

namespace brave_federated {

absl::optional<std::string> BuildGetTasksPayload() {
  flower::Node node;
  node.set_node_id(0);
  node.set_anonymous(true);

  flower::PullTaskInsRequest pull_task_instructions_request;
  *pull_task_instructions_request.mutable_node() = node;
  pull_task_instructions_request.add_task_ids("0");

  std::string request;
  const bool success =
      pull_task_instructions_request.SerializeToString(&request);

  if (!success) {
    return absl::nullopt;
  }

  return request;
}

base::expected<Task, std::string> ParseTask(
    const flower::TaskIns& task_instruction) {
  const TaskId task_id = {
      .id = task_instruction.task_id(),
      .group_id = task_instruction.group_id(),
      .family_id = task_instruction.workload_id(),
  };
  if (!task_id.IsValid()) {
    return base::unexpected("Invalid task id received from FL service");
  }

  if (!task_instruction.has_task()) {
    return base::unexpected("Task object is missing from task instruction");
  }
  const flower::Task& flower_task = task_instruction.task();

  if (!flower_task.has_legacy_server_message()) {
    return base::unexpected("Server message is missing from task object");
  }
  const flower::ServerMessage& message = flower_task.legacy_server_message();

  Configs config;
  TaskType type = TaskType::kUndefined;
  std::vector<Weights> parameters;
  if (message.has_fit_ins()) {
    type = TaskType::kTraining;

    if (!message.fit_ins().has_parameters()) {
      return base::unexpected("Parameters are missing from fit instruction");
    }
    parameters = GetVectorsFromParameters(message.fit_ins().parameters());
    if (parameters.empty()) {
      return base::unexpected(
          "Parameters vectors received from FL service are empty");
    }
    config = ConfigsFromProto(message.fit_ins().config());
  } else if (message.has_evaluate_ins()) {
    type = TaskType::kEvaluation;

    if (!message.evaluate_ins().has_parameters()) {
      return base::unexpected("Parameters are missing from eval instruction");
    }
    parameters = GetVectorsFromParameters(message.evaluate_ins().parameters());
    if (parameters.empty()) {
      return base::unexpected(
          "Parameters vectors received from FL service are empty");
    }
    config = ConfigsFromProto(message.evaluate_ins().config());
  } else {
    return base::unexpected(
        "Received unrecognized instruction from FL service");
  }

  return Task(task_id, type, "token", parameters, config);
}

base::expected<TaskList, std::string> ParseTaskListFromResponseBody(
    const std::string& response_body) {
  flower::PullTaskInsResponse response;
  if (!response.ParseFromString(response_body)) {
    return base::unexpected("Failed to parse response body");
  }

  if (response.task_ins_list_size() == 0) {
    return base::unexpected("No tasks received from FL service");
  }

  TaskList task_list;
  for (int i = 0; i < response.task_ins_list_size(); i++) {
    const flower::TaskIns& task_instruction = response.task_ins_list(i);

    auto task = ParseTask(task_instruction);
    if (!task.has_value()) {
      VLOG(2) << task.error();
      continue;
    }

    task_list.push_back(std::move(task.value()));

    return task_list;
  }

  return base::unexpected("Failed to parse PullTaskInsRes");
}

absl::optional<std::string> BuildUploadTaskResultsPayload(
    const TaskResult& result) {
  const Task& task = result.GetTask();
  const TaskId& task_id = task.GetId();
  const TaskType& task_type = task.GetType();
  const PerformanceReportInfo& report = result.GetReport();

  flower::Task flower_task;

  flower::ClientMessage client_message;
  switch (task_type) {
    case TaskType::kTraining: {
      flower::ClientMessage_FitRes fit_res;
      fit_res.set_num_examples(static_cast<int64_t>(report.dataset_size));
      *fit_res.mutable_parameters() =
          GetParametersFromVectors(report.parameters);
      if (!report.metrics.empty()) {
        *fit_res.mutable_metrics() = MetricsToProto(report.metrics);
      }
      *client_message.mutable_fit_res() = fit_res;
      break;
    }
    case TaskType::kEvaluation: {
      flower::ClientMessage_EvaluateRes eval_res;
      eval_res.set_num_examples(static_cast<int64_t>(report.dataset_size));
      eval_res.set_loss(report.loss);
      if (!report.metrics.empty()) {
        *eval_res.mutable_metrics() = MetricsToProto(report.metrics);
      }
      *client_message.mutable_evaluate_res() = eval_res;
      break;
    }
    case TaskType::kUndefined: {
      return absl::nullopt;
    }
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
