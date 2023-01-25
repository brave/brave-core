/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/adapters/flower_helper.h"

#include <sstream>

#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "base/values.h"

#include "brave/components/brave_federated/task/model.h"
#include "brave/components/brave_federated/task/typing.h"
#include "brave/third_party/flower/src/proto/flwr/proto/fleet.pb.h"
#include "brave/third_party/flower/src/proto/flwr/proto/node.pb.h"

namespace {
// TODO(lminto): Create this wiki url
// constexpr char kWikiUrl[] =
//     "https://github.com/brave/brave-browser/wiki/Federated-Learning";

}  // namespace

namespace brave_federated {

std::vector<float> GetFloatVectorFromString(std::string string) {
  int vector_size = string.size() / sizeof(float);
  float parameters_array[vector_size];
  std::memcpy(parameters_array, string.data(), string.size());

  std::vector<float> parameters_vector(parameters_array,
                                       parameters_array + vector_size);

  return parameters_vector;
}

std::string GetStringFromFloatVector(std::vector<float> vector) {
  std::ostringstream oss;
  oss.write(reinterpret_cast<const char*>(vector.data()),
            vector.size() * sizeof(float));

  return oss.str();
}

std::vector<std::vector<float>> GetParametersFromMessage(
    flower::Parameters parameters_msg) {
  std::vector<std::vector<float>> tensors;
  for (int i = 0; i < parameters_msg.tensors_size(); i++) {
    std::string parameters_string = parameters_msg.tensors(i);
    std::vector<float> parameters_vector =
        GetFloatVectorFromString(parameters_string);
    tensors.push_back(parameters_vector);
  }

  return tensors;
}

flower::Parameters GetMessageFromParameters(
    std::vector<std::vector<float>> parameters_vector) {
  flower::Parameters flower_parameters;
  flower_parameters.set_tensor_type("cpp_float");

  for (auto const& vector : parameters_vector) {
    std::string string = GetStringFromFloatVector(vector);
    flower_parameters.add_tensors();
  }

  return flower_parameters;
}

flower::PullTaskInsRequest BuildPullTaskInsRequestMessage() {
  flower::Node node;
  node.set_node_id(1);
  node.set_anonymous(true);

  flower::PullTaskInsRequest pull_task_instructions_request;
  *pull_task_instructions_request.mutable_node() = node;
  pull_task_instructions_request.add_task_ids(0);

  return pull_task_instructions_request;
}

std::string BuildGetTasksPayload() {
  std::string request;

  flower::PullTaskInsRequest task_request = BuildPullTaskInsRequestMessage();
  task_request.SerializeToString(&request);

  return request;
}

std::string BuildPostTaskResultsPayload(TaskResult result) {
  Task task = result.GetTask();
  PerformanceReport report = result.GetReport();

  int task_id = task.GetId();
  TaskType task_type = task.GetType();

  flower::Task flower_task;
  flower::ClientMessage client_message;
  if (task_type == TaskType::Training) {
    flower::ClientMessage_FitRes fit_res;
    fit_res.set_num_examples(report.dataset_size);
    *fit_res.mutable_parameters() = GetMessageFromParameters(report.parameters);
    *client_message.mutable_fit_res() = fit_res;
  } else {
    flower::ClientMessage_EvaluateRes eval_res;
    eval_res.set_num_examples(report.dataset_size);
    eval_res.set_loss(report.loss);
    *client_message.mutable_evaluate_res() = eval_res;
  }
  *flower_task.mutable_legacy_client_message() = client_message;

  flower::PushTaskResRequest task_results;
  flower::TaskRes* task_result = task_results.add_task_res_list();
  task_result->set_task_id(std::to_string(task_id));
  task_result->set_group_id("0");
  task_result->set_workload_id("0");
  *task_result->mutable_task() = flower_task;
  //task_result->set_token("fixed_token");

  std::string result_payload;
  task_results.SerializeToString(&result_payload);

  return result_payload;
}

}  // namespace brave_federated
