/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/learning_service.h"

#include <map>
#include <utility>
#include <vector>

#include "base/check_op.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_federated/communication_adapter.h"
#include "brave/components/brave_federated/eligibility_service.h"
#include "brave/components/brave_federated/features.h"
#include "brave/components/brave_federated/resources/grit/brave_federated_resources.h"
#include "brave/components/brave_federated/task/federated_task_handler.h"
#include "brave/components/brave_federated/task/model.h"
#include "brave/components/brave_federated/util/synthetic_dataset.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "ui/base/resource/resource_bundle.h"

namespace brave_federated {

namespace {

base::expected<TaskResult, std::string> LoadDatasetAndRunTask(
    std::unique_ptr<FederatedTaskHandler> task_runner) {
  auto synthetic_dataset = std::make_unique<SyntheticDataset>(500);
  SyntheticDataset test_dataset = synthetic_dataset->SeparateTestData(50);

  task_runner->SetTrainingData(synthetic_dataset->GetDataPoints());
  task_runner->SetTestData(test_dataset.GetDataPoints());
  VLOG(2) << "FL: Model and data set. Task runner initialized.";

  return task_runner->Run();
}

}  // namespace

LearningService::LearningService(
    EligibilityService* eligibility_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(std::move(url_loader_factory)),
      eligibility_service_(eligibility_service) {
  CHECK(!init_task_timer_);

  std::string data_resource;
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  if (resource_bundle.IsGzipped(IDR_BRAVE_FEDERATED_CONFIG)) {
    data_resource =
        resource_bundle.LoadDataResourceString(IDR_BRAVE_FEDERATED_CONFIG);
  } else {
    data_resource = static_cast<std::string>(
        resource_bundle.GetRawDataResource(IDR_BRAVE_FEDERATED_CONFIG));
  }
  CHECK_GT(data_resource.size(), 0U);

  learning_service_config_ =
      std::make_unique<LearningServiceConfig>(data_resource);
  const net::BackoffEntry::Policy post_results_policy =
      learning_service_config_->GetPostResultsPolicy();
  model_spec_ = std::make_unique<api::config::ModelSpec>(
      std::move(learning_service_config_->GetModelSpec()));

  post_results_policy_ =
      std::make_unique<const net::BackoffEntry::Policy>(post_results_policy);
  post_results_backoff_entry_ =
      std::make_unique<net::BackoffEntry>(post_results_policy_.get());

  init_task_timer_ = std::make_unique<base::OneShotTimer>();
  init_task_timer_->Start(FROM_HERE,
                          kInitFederatedServiceWaitTimeInSeconds.Get(), this,
                          &LearningService::Init);
}

LearningService::~LearningService() {
  if (participating_) {
    StopParticipating();
  }

  eligibility_observation_.Reset();
}

void LearningService::Init() {
  CHECK(url_loader_factory_);
  CHECK(eligibility_service_);
  CHECK(init_task_timer_);

  VLOG(1) << "FL: Initializing federated learning service.";
  eligibility_observation_.Observe(eligibility_service_);
  if (eligibility_service_->IsEligible()) {
    StartParticipating();
  }

  VLOG(1) << "FL: Eligibility: " << eligibility_service_->IsEligible();
}

void LearningService::StartParticipating() {
  if (participating_) {
    return;
  }

  participating_ = true;
  StartCommunicationAdapter();
  GetTasks();
}

void LearningService::StopParticipating() {
  if (!participating_) {
    return;
  }

  participating_ = false;
  communication_adapter_.reset();
  reconnect_timer_.reset();
}

void LearningService::GetTasks() {
  if (communication_adapter_ == nullptr) {
    VLOG(1) << "FL: Communication adapter not initialized";
    return;
  }

  communication_adapter_->GetTasks(base::BindOnce(
      &LearningService::HandleTasksOrReconnect, weak_factory_.GetWeakPtr()));
}

void LearningService::StartCommunicationAdapter() {
  const net::BackoffEntry::Policy reconnect_policy =
      learning_service_config_->GetReconnectPolicy();
  const net::BackoffEntry::Policy request_task_policy =
      learning_service_config_->GetRequestTaskPolicy();

  communication_adapter_ = std::make_unique<CommunicationAdapter>(
      url_loader_factory_, reconnect_policy, request_task_policy);
}

void LearningService::HandleTasksOrReconnect(TaskList tasks,
                                             base::TimeDelta reconnect) {
  if (tasks.empty()) {
    reconnect_timer_ = std::make_unique<base::RetainingOneShotTimer>();
    reconnect_timer_->Start(FROM_HERE, reconnect, this,
                            &LearningService::GetTasks);
    VLOG(2) << "FL: No tasks available, reconnecting in " << reconnect;
    return;
  }

  Task task = tasks.at(0);
  float learning_rate = 0.01;
  std::map<std::string, float> config = task.GetConfig();
  auto cursor = config.find("lr");
  if (cursor != config.end()) {
    learning_rate = cursor->second;
    VLOG(2) << "FL: Learning rate applied from server: " << learning_rate;
  }
  model_spec_->learning_rate = learning_rate;

  if (static_cast<int>(task.GetParameters().at(0).size()) !=
          model_spec_->num_params &&
      task.GetParameters().at(1).size() != 1U) {
    VLOG(1) << "FL: Task specifies a different model size than the client";
    return;
  }
  VLOG(1) << "FL: Task model and client model match!";

  std::unique_ptr<Model> model = std::make_unique<Model>(*model_spec_);
  model->SetWeights(task.GetParameters().at(0));
  model->SetBias(task.GetParameters().at(1).at(0));
  auto task_runner =
      std::make_unique<FederatedTaskHandler>(task, std::move(model));

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&LoadDatasetAndRunTask, std::move(task_runner)),
      base::BindOnce(&LearningService::OnTaskResultComputed,
                     weak_factory_.GetWeakPtr()));
}

void LearningService::OnTaskResultComputed(
    base::expected<TaskResult, std::string> result) {
  if (!result.has_value()) {
    VLOG(1) << "FL: " << result.error();
    return;
  }

  if (communication_adapter_ == nullptr) {
    VLOG(1) << "FL: Communication adapter not initialized";
    return;
  }

  communication_adapter_->UploadTaskResult(
      result.value(), base::BindOnce(&LearningService::OnUploadTaskResults,
                                     weak_factory_.GetWeakPtr()));
}

void LearningService::OnUploadTaskResults(TaskResultResponse response) {
  post_results_backoff_entry_->InformOfRequest(response.IsSuccessful());

  if (response.IsSuccessful()) {
    VLOG(1) << "FL: Task results posted successfully";
  } else {
    VLOG(1) << "FL: Task results posting failed";
  }

  base::TimeDelta reconnect =
      post_results_backoff_entry_->GetTimeUntilRelease();
  reconnect_timer_ = std::make_unique<base::RetainingOneShotTimer>();
  reconnect_timer_->Start(FROM_HERE, reconnect, this,
                          &LearningService::GetTasks);
  VLOG(1) << "FL: Reconnecting in " << reconnect;
}

void LearningService::OnEligibilityChanged(bool is_eligible) {
  if (is_eligible) {
    StartParticipating();
    VLOG(1) << "FL: Eligibility changed, started participating.";
  } else {
    StopParticipating();
    VLOG(1) << "FL: Eligibility changed, stopped participating.";
  }
}

}  // namespace brave_federated
