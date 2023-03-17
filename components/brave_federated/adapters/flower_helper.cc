/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/adapters/flower_helper.h"

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "brave/components/brave_federated/task/model.h"
#include "brave/components/brave_federated/task/typing.h"
#include "brave/third_party/flower/src/brave/flwr/serde.h"
#include "brave/third_party/flower/src/proto/flwr/proto/fleet.pb.h"
#include "brave/third_party/flower/src/proto/flwr/proto/node.pb.h"

namespace {
// TODO(lminto): Create this wiki url
// constexpr char kWikiUrl[] =
//     "https://github.com/brave/brave-browser/wiki/Federated-Learning";

}  // namespace

namespace brave_federated {

TaskList ParseTaskListFromResponseBody(const std::string& response_body) {
  flower::PullTaskInsResponse response;
  if ((response.ParseFromString(response_body))) {
    TaskList task_list;
    for (int i = 0; i < response.task_ins_list_size(); i++) {
      flower::TaskIns task_instruction = response.task_ins_list(i);

      std::string id = task_instruction.task_id();
      std::string group_id = task_instruction.group_id();
      std::string workload_id = task_instruction.workload_id();
      TaskId task_id = TaskId{id, group_id, workload_id};
      flower::Task flower_task = task_instruction.task();

      flower::ServerMessage message = flower_task.legacy_server_message();

      TaskType type;
      std::vector<Weights> parameters = {};
      Configs config = {};
      if (message.has_fit_ins()) {
        type = TaskType::Training;
        parameters = GetVectorsFromParameters(message.fit_ins().parameters());
        config = ConfigsFromProto(message.fit_ins().config());
      } else if (message.has_evaluate_ins()) {
        type = TaskType::Evaluation;
        parameters =
            GetVectorsFromParameters(message.evaluate_ins().parameters());
        config = ConfigsFromProto(message.evaluate_ins().config());
      } else if (message.has_reconnect_ins()) {
        VLOG(2) << "**: Legacy reconnect instruction received from FL service";
        continue;
      } else {
        VLOG(2) << "**: Received unrecognized instruction from FL service";
        continue;
      }

      Task task = Task(task_id, type, "token", parameters, config);
      task_list.push_back(task);
    }

    return task_list;
  }

  VLOG(1) << "Failed to parse PullTaskInsRes";
  return {};
}

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

std::string BuildPostTaskResultsPayload(TaskResult result) {
  Task task = result.GetTask();
  TaskId task_id = task.GetId();
  TaskType task_type = task.GetType();
  PerformanceReport report = result.GetReport();

  flower::Task flower_task;
  // Client Message Creation
  flower::ClientMessage client_message;
  if (task_type == TaskType::Training) {
    flower::ClientMessage_FitRes fit_res;
    fit_res.set_num_examples(report.dataset_size);
    *fit_res.mutable_parameters() = GetParametersFromVectors(report.parameters);
    if (report.metrics.size() > 0) {
      *fit_res.mutable_metrics() = MetricsToProto(report.metrics);
    }
    *client_message.mutable_fit_res() = fit_res;
  } else {
    flower::ClientMessage_EvaluateRes eval_res;
    eval_res.set_num_examples(report.dataset_size);
    eval_res.set_loss(report.loss);
    if (report.metrics.size() > 0) {
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
