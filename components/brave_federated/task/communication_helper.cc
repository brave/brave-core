/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/task/communication_helper.h"

#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "base/values.h"

#include "brave/components/brave_federated/task/model.h"
#include "brave/components/brave_federated/task/typing.h"
#include "brave/third_party/flower/src/proto/flwr/proto/fleet.pb.h"
#include "brave/third_party/flower/src/proto/flwr/proto/transport.pb.h"

namespace {
// TODO(lminto): Create this wiki url
// constexpr char kWikiUrl[] =
//     "https://github.com/brave/brave-browser/wiki/Federated-Learning";

}  // namespace

namespace brave_federated {

flower::GetTasksRequest BuildGetTasksRequestMessage() {
  flower::GetTasksRequest trm;
  trm.set_id(111);

  return trm;
}

std::string BuildGetTasksPayload() {
  flower::GetTasksRequest task_request = BuildGetTasksRequestMessage();

  std::string request;
  task_request.SerializeToString(&request);

  return request;
}

std::string BuildPostTaskResultsPayload(TaskResult result) {
  Task task = result.GetTask();
  int task_id = task.GetId();
  PerformanceReport report = result.GetReport();

  flower::Result flwr_result;
  flwr_result.set_task_id(task_id);

  TaskType task_type = task.GetType();
  flower::ClientMessage client_message;
  if (task_type == TaskType::Training) {
    flower::ClientMessage_FitRes fit_res;
    fit_res.set_num_examples(report.dataset_size);
    // TODO(lminto): Add parameters, status, metrics?
    *client_message.mutable_fit_res() = fit_res;
  } else {
    flower::ClientMessage_EvaluateRes eval_res;
    eval_res.set_num_examples(report.dataset_size);
    eval_res.set_loss(report.loss);
    // TODO(lminto): Add status, metrics?
    *client_message.mutable_evaluate_res() = eval_res;
  }

  *flwr_result.mutable_legacy_client_message() = client_message;

  flower::CreateResultsRequest response;
  flower::TokenizedResult* tok_res = response.add_tokenized_results();
  *tok_res->mutable_result() = flwr_result;
  tok_res->set_token("fixed_token");

  std::string result_payload;
  response.SerializeToString(&result_payload);

  return result_payload;
}

}  // namespace brave_federated
