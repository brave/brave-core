/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/learning_service.h"

#include <utility>
#include <vector>

#include "base/check_op.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_federated/communication_adapter.h"
#include "brave/components/brave_federated/config_utils.h"
#include "brave/components/brave_federated/eligibility_service.h"
#include "brave/components/brave_federated/features.h"
#include "brave/components/brave_federated/resources/grit/brave_federated_resources.h"
#include "brave/components/brave_federated/task/federated_task_handler.h"
#include "brave/components/brave_federated/task/model.h"
#include "brave/components/brave_federated/task/typing.h"
#include "brave/components/brave_federated/util/synthetic_dataset.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/resource/resource_bundle.h"

namespace brave_federated {

namespace {

absl::optional<TaskResult> LoadDatasetAndRunTask(
    std::unique_ptr<FederatedTaskHandler> task_runner) {
  auto synthetic_dataset = std::make_unique<SyntheticDataset>(500);
  SyntheticDataset test_dataset = synthetic_dataset->SeparateTestData(50);

  task_runner->SetTrainingData(synthetic_dataset->GetDataPoints());
  task_runner->SetTestData(test_dataset.GetDataPoints());
  VLOG(2) << "Model and data set. Task runner initialized.";

  return task_runner->Run();
}

}  // namespace

LearningService::LearningService(
    EligibilityService* eligibility_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(url_loader_factory),
      eligibility_service_(eligibility_service) {
  DCHECK(!init_task_timer_);

  std::string data_resource;
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  if (resource_bundle.IsGzipped(IDR_BRAVE_FEDERATED_CONFIG)) {
    data_resource =
        resource_bundle.LoadDataResourceString(IDR_BRAVE_FEDERATED_CONFIG);
  } else {
    data_resource = static_cast<std::string>(
        resource_bundle.GetRawDataResource(IDR_BRAVE_FEDERATED_CONFIG));
  }
  DCHECK_GT(data_resource.size(), 0U);

  lsc_ = std::make_unique<LearningServiceConfig>(data_resource);
  const net::BackoffEntry::Policy reconnect_policy = lsc_->GetReconnectPolicy();
  const net::BackoffEntry::Policy request_task_policy =
      lsc_->GetRequestTaskPolicy();
  const net::BackoffEntry::Policy post_results_policy =
      lsc_->GetPostResultsPolicy();
  model_spec_ = std::make_unique<ModelSpec>(lsc_->GetModelSpec());

  communication_adapter_ = std::make_unique<CommunicationAdapter>(
      url_loader_factory_, reconnect_policy, request_task_policy);
  post_results_policy_ =
      std::make_unique<const net::BackoffEntry::Policy>(post_results_policy);
  post_results_backoff_entry_ =
      std::make_unique<net::BackoffEntry>(post_results_policy_.get());

  const int init_federated_service_wait_time_in_seconds =
      brave_federated::features::GetInitFederatedServiceWaitTimeInSeconds();
  init_task_timer_ = std::make_unique<base::OneShotTimer>();
  init_task_timer_->Start(
      FROM_HERE, base::Seconds(init_federated_service_wait_time_in_seconds),
      this, &LearningService::Init);
}

LearningService::~LearningService() {
  if (participating_) {
    StopParticipating();
  }

  eligibility_observation_.Reset();
}

void LearningService::Init() {
  DCHECK(url_loader_factory_);
  DCHECK(eligibility_service_);
  DCHECK(init_task_timer_);

  VLOG(1) << "Initializing federated learning service.";
  eligibility_observation_.Observe(eligibility_service_);
  if (eligibility_service_->IsEligible()) {
    StartParticipating();
  }

  VLOG(1) << "Eligibility: " << eligibility_service_->IsEligible();
}

void LearningService::OnEligibilityChanged(bool is_eligible) {
  if (is_eligible) {
    StartParticipating();
    VLOG(2) << "Eligibility changed, started participating.";
  } else {
    StopParticipating();
    VLOG(2) << "Eligibility changed, stopped participating.";
  }
}

void LearningService::StartParticipating() {
  if (participating_) {
    return;
  }

  participating_ = true;
  GetTasks();
}

void LearningService::StopParticipating() {
  if (!participating_) {
    return;
  }

  participating_ = false;
  reconnect_timer_.reset();
}

void LearningService::GetTasks() {
  communication_adapter_->GetTasks(base::BindOnce(
      &LearningService::HandleTasksOrReconnect, weak_factory_.GetWeakPtr()));
}

void LearningService::HandleTasksOrReconnect(TaskList tasks, int reconnect) {
  if (tasks.size() == 0) {
    reconnect_timer_ = std::make_unique<base::RetainingOneShotTimer>();
    reconnect_timer_->Start(FROM_HERE, base::Seconds(reconnect), this,
                            &LearningService::GetTasks);
    VLOG(2) << "No tasks available, reconnecting in " << reconnect << "s";
    return;
  }

  Task task = tasks.at(0);
  float lr = 0.01;
  std::map<std::string, float> config = task.GetConfig();
  auto cursor = config.find("lr");
  if (cursor != config.end()) {
    lr = cursor->second;
    VLOG(2) << "Learning rate applied from server: " << lr;
  }

  model_spec_->learning_rate = lr;

  if (static_cast<int>(task.GetParameters().at(0).size()) !=
          model_spec_->num_params &&
      task.GetParameters().at(1).size() != 1U) {
    VLOG(2) << "Task specifies a different model size than the client";
    return;
  }
  VLOG(2) << "Task model and client model match!";

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

void LearningService::OnTaskResultComputed(absl::optional<TaskResult> result) {
  if (!result.has_value()) {
    VLOG(2) << "Task result computation failed";
    return;
  }

  communication_adapter_->UploadTaskResult(
      result.value(), base::BindOnce(&LearningService::OnUploadTaskResults,
                                     weak_factory_.GetWeakPtr()));
}

void LearningService::OnUploadTaskResults(TaskResultResponse response) {
  post_results_backoff_entry_->InformOfRequest(response.IsSuccessful());

  if (response.IsSuccessful()) {
    VLOG(2) << "Task results posted successfully";
  } else {
    VLOG(2) << "Task results posting failed";
  }

  base::TimeDelta reconnect =
      post_results_backoff_entry_->GetTimeUntilRelease();
  reconnect_timer_ = std::make_unique<base::RetainingOneShotTimer>();
  reconnect_timer_->Start(FROM_HERE, reconnect, this,
                          &LearningService::GetTasks);
  VLOG(2) << "Reconnecting in " << reconnect.InSeconds() << "s";
}

}  // namespace brave_federated
