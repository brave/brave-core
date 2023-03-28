/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/adapters/flower_helper.h"

#include <map>
#include <vector>

#include "brave/components/brave_federated/task/typing.h"
#include "brave/components/brave_federated/util/linear_algebra_util.h"
#include "brave/third_party/flower/src/brave/flwr/serde.h"
#include "brave/third_party/flower/src/proto/flwr/proto/fleet.pb.h"
#include "brave/third_party/flower/src/proto/flwr/proto/node.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveFederatedLearning*

namespace brave_federated {

std::vector<Weights> test_parameters = {{1, 2, 3}, {4, 5, 6}};
std::map<std::string, double> config_metrics = {{"loss", 0.42},
                                                {"accuracy", 42.0}};

TEST(BraveFederatedLearningFlowerHelperTest, BuildAnonymousGetTasksPayload) {
  // Arrange

  // Act
  const std::string payload = BuildGetTasksPayload();

  // Assert
  flower::PullTaskInsRequest request;
  request.ParseFromString(payload);

  flower::Node node = request.node();
  EXPECT_EQ(node.node_id(), 0U);
  EXPECT_TRUE(node.anonymous());
}

TEST(BraveFederatedLearningFlowerHelperTest, ParseFitTaskListFromResponseBody) {
  // Arrange
  flower::PullTaskInsResponse fit_task_instruction_response;
  flower::TaskIns* task_instruction =
      fit_task_instruction_response.add_task_ins_list();
  task_instruction->set_task_id("42");
  task_instruction->set_group_id("23");
  task_instruction->set_workload_id("8");

  flower::Task flower_task;
  flower::ServerMessage server_message;
  flower::ServerMessage_FitIns fit_instruction;
  *fit_instruction.mutable_parameters() =
      GetParametersFromVectors(test_parameters);
  *fit_instruction.mutable_config() = MetricsToProto(config_metrics);
  *server_message.mutable_fit_ins() = fit_instruction;
  *flower_task.mutable_legacy_server_message() = server_message;
  *task_instruction->mutable_task() = flower_task;

  const std::string response_string =
      fit_task_instruction_response.SerializeAsString();

  // Act
  TaskList task_list = ParseTaskListFromResponseBody(response_string);

  // Assert
  EXPECT_EQ(task_list.size(), 1U);

  Task task = task_list[0];
  TaskId task_id = task.GetId();
  EXPECT_EQ(task_id.id, "42");
  EXPECT_EQ(task_id.group_id, "23");
  EXPECT_EQ(task_id.family_id, "8");

  TaskType task_type = task.GetType();
  EXPECT_EQ(task_type, TaskType::Training);

  std::vector<Weights> task_parameters = task.GetParameters();
  EXPECT_EQ(task_parameters.size(), 2U);
  EXPECT_EQ(task_parameters, test_parameters);

  std::map<std::string, float> task_config = task.GetConfig();
  EXPECT_EQ(task_config.size(), 2U);
  EXPECT_EQ(task_config["loss"], static_cast<float>(config_metrics["loss"]));
  EXPECT_EQ(task_config["accuracy"],
            static_cast<float>(config_metrics["accuracy"]));
}

TEST(BraveFederatedLearningFlowerHelperTest,
     ParseEvaluateTaskListFromResponseBody) {
  // Arrange
  flower::PullTaskInsResponse eval_task_instruction_response;
  flower::TaskIns* task_instruction =
      eval_task_instruction_response.add_task_ins_list();
  task_instruction->set_task_id("42");
  task_instruction->set_group_id("23");
  task_instruction->set_workload_id("8");

  flower::Task flower_task;
  flower::ServerMessage server_message;
  flower::ServerMessage_EvaluateIns eval_instruction;
  *eval_instruction.mutable_parameters() =
      GetParametersFromVectors(test_parameters);
  *eval_instruction.mutable_config() = MetricsToProto(config_metrics);
  *server_message.mutable_evaluate_ins() = eval_instruction;
  *flower_task.mutable_legacy_server_message() = server_message;
  *task_instruction->mutable_task() = flower_task;

  const std::string response_string =
      eval_task_instruction_response.SerializeAsString();

  // Act
  TaskList task_list = ParseTaskListFromResponseBody(response_string);

  // Assert
  EXPECT_EQ(task_list.size(), 1U);

  Task task = task_list[0];
  TaskId task_id = task.GetId();
  EXPECT_EQ(task_id.id, "42");
  EXPECT_EQ(task_id.group_id, "23");
  EXPECT_EQ(task_id.family_id, "8");

  TaskType task_type = task.GetType();
  EXPECT_EQ(task_type, TaskType::Evaluation);

  std::vector<Weights> task_parameters = task.GetParameters();
  EXPECT_EQ(task_parameters.size(), 2U);
  EXPECT_EQ(task_parameters, test_parameters);

  std::map<std::string, float> task_config = task.GetConfig();
  EXPECT_EQ(task_config.size(), 2U);
  EXPECT_EQ(task_config["loss"], static_cast<float>(config_metrics["loss"]));
  EXPECT_EQ(task_config["accuracy"],
            static_cast<float>(config_metrics["accuracy"]));
}

TEST(BraveFederatedLearningFlowerHelperTest,
     ParseTaskWithoutGroupIdFromResponseBody) {
  // Arrange
  flower::PullTaskInsResponse eval_task_instruction_response;
  flower::TaskIns* task_instruction =
      eval_task_instruction_response.add_task_ins_list();
  task_instruction->set_task_id("42");
  task_instruction->set_workload_id("8");

  flower::Task flower_task;
  flower::ServerMessage server_message;
  flower::ServerMessage_EvaluateIns eval_instruction;
  *eval_instruction.mutable_parameters() =
      GetParametersFromVectors(test_parameters);
  *eval_instruction.mutable_config() = MetricsToProto(config_metrics);
  *server_message.mutable_evaluate_ins() = eval_instruction;
  *flower_task.mutable_legacy_server_message() = server_message;
  *task_instruction->mutable_task() = flower_task;

  const std::string response_string =
      eval_task_instruction_response.SerializeAsString();

  // Act
  TaskList task_list = ParseTaskListFromResponseBody(response_string);

  // Assert
  EXPECT_EQ(task_list.size(), 0U);
}


TEST(BraveFederatedLearningFlowerHelperTest,
     ParseTaskWithoutParametersFromResponseBody) {
  // Arrange
  flower::PullTaskInsResponse eval_task_instruction_response;
  flower::TaskIns* task_instruction =
      eval_task_instruction_response.add_task_ins_list();
  task_instruction->set_task_id("42");
  task_instruction->set_workload_id("8");

  flower::Task flower_task;
  flower::ServerMessage server_message;
  flower::ServerMessage_EvaluateIns eval_instruction;
  *eval_instruction.mutable_config() = MetricsToProto(config_metrics);
  *server_message.mutable_evaluate_ins() = eval_instruction;
  *flower_task.mutable_legacy_server_message() = server_message;
  *task_instruction->mutable_task() = flower_task;

  const std::string response_string =
      eval_task_instruction_response.SerializeAsString();

  // Act
  TaskList task_list = ParseTaskListFromResponseBody(response_string);

  // Assert
  EXPECT_EQ(task_list.size(), 0U);
}

TEST(BraveFederatedLearningFlowerHelperTest, BuildPostTrainTaskResultsPayload) {
  // Arrange
  TaskId task_id = {"42", "23", "8"};
  TaskType task_type = TaskType::Training;
  Task task = Task(task_id, task_type, "", {}, {});

  size_t dataset_size = 500;
  float loss = 0.42;
  float accuracy = 42.0;
  std::map<std::string, double> metrics = {{"alpha", 0.42}, {"beta", 42.0}};

  PerformanceReport performance_report =
      PerformanceReport(dataset_size, loss, accuracy, test_parameters, metrics);
  TaskResult task_result = TaskResult(task, performance_report);

  // Act
  const std::string payload = BuildPostTaskResultsPayload(task_result);

  // Assert
  flower::PushTaskResRequest request;
  request.ParseFromString(payload);

  flower::TaskRes flower_task_result = request.task_res_list(0);
  EXPECT_EQ(flower_task_result.task_id(), "");
  EXPECT_EQ(flower_task_result.group_id(), task_id.group_id);
  EXPECT_EQ(flower_task_result.workload_id(), task_id.family_id);

  flower::Task flower_task = flower_task_result.task();
  flower::ClientMessage client_message = flower_task.legacy_client_message();
  flower::ClientMessage_FitRes fit_result = client_message.fit_res();
  EXPECT_EQ(static_cast<size_t>(fit_result.num_examples()), dataset_size);

  std::vector<Weights> parameters =
      GetVectorsFromParameters(fit_result.parameters());
  EXPECT_EQ(parameters.size(), 2U);
  EXPECT_EQ(parameters, test_parameters);

  std::map<std::string, float> flower_metrics =
      ConfigsFromProto(fit_result.metrics());
  EXPECT_EQ(flower_metrics.size(), 2U);
  EXPECT_EQ(flower_metrics["alpha"], static_cast<float>(metrics["alpha"]));
  EXPECT_EQ(flower_metrics["beta"], static_cast<float>(metrics["beta"]));
}

TEST(BraveFederatedLearningFlowerHelperTest,
     BuildPostEvaluateTaskResultsPayload) {
  // Arrange
  TaskId task_id = {"42", "23", "8"};
  TaskType task_type = TaskType::Evaluation;
  Task task = Task(task_id, task_type, "", {}, {});

  size_t dataset_size = 500;
  float loss = 0.42;
  float accuracy = 42.0;
  std::map<std::string, double> metrics = {{"alpha", 0.42}, {"beta", 42.0}};

  PerformanceReport performance_report =
      PerformanceReport(dataset_size, loss, accuracy, test_parameters, metrics);
  TaskResult task_result = TaskResult(task, performance_report);

  // Act
  const std::string payload = BuildPostTaskResultsPayload(task_result);

  // Assert
  flower::PushTaskResRequest request;
  request.ParseFromString(payload);

  flower::TaskRes flower_task_result = request.task_res_list(0);
  EXPECT_EQ(flower_task_result.task_id(), "");
  EXPECT_EQ(flower_task_result.group_id(), task_id.group_id);
  EXPECT_EQ(flower_task_result.workload_id(), task_id.family_id);

  flower::Task flower_task = flower_task_result.task();
  flower::ClientMessage client_message = flower_task.legacy_client_message();
  flower::ClientMessage_EvaluateRes evaluate_result =
      client_message.evaluate_res();
  EXPECT_EQ(static_cast<size_t>(evaluate_result.num_examples()), dataset_size);
  EXPECT_EQ(evaluate_result.loss(), loss);

  std::map<std::string, float> flower_metrics =
      ConfigsFromProto(evaluate_result.metrics());
  EXPECT_EQ(flower_metrics.size(), 2U);
  EXPECT_EQ(flower_metrics["alpha"], static_cast<float>(metrics["alpha"]));
  EXPECT_EQ(flower_metrics["beta"], static_cast<float>(metrics["beta"]));
}

}  // namespace brave_federated
