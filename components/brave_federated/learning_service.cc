/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/learning_service.h"

#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_federated/communication_adapter.h"
#include "brave/components/brave_federated/eligibility_service.h"
#include "brave/components/brave_federated/features.h"
#include "brave/components/brave_federated/task/federated_task_runner.h"
#include "brave/components/brave_federated/task/model.h"
#include "brave/components/brave_federated/task/typing.h"
#include "brave/components/brave_federated/util/synthetic_dataset.h"
#include "content/public/browser/browser_task_traits.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace brave_federated {

LearningService::LearningService(
    EligibilityService* eligibility_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(url_loader_factory),
      eligibility_service_(eligibility_service) {

    DCHECK(!init_task_timer_);
    const int init_federated_service_wait_time_in_seconds =
      brave_federated::features::GetInitFederatedServiceWaitTimeInSeconds();
    init_task_timer_ = std::make_unique<base::OneShotTimer>();
    init_task_timer_->Start(FROM_HERE,
                            base::Seconds(init_federated_service_wait_time_in_seconds),
                            this, &LearningService::Init);
}

LearningService::~LearningService() {
  StopParticipating();
  eligibility_service_->RemoveObserver(this);
}

void LearningService::Init() {
  DCHECK(url_loader_factory_);
  DCHECK(eligibility_service_);
  DCHECK(init_task_timer_);
  communication_adapter_ = new CommunicationAdapter(url_loader_factory_);
  eligibility_service_->AddObserver(this);

  StartParticipating();
}

void LearningService::StartParticipating() {
  participating_ = true;
  GetTasks();
}

void LearningService::StopParticipating() {
  DCHECK(participating_);
  participating_ = false;
}

void LearningService::GetTasks() {
  communication_adapter_->GetTasks(base::BindOnce(
      &LearningService::HandleTasksOrReconnect, base::Unretained(this)));
}

void LearningService::HandleTasksOrReconnect(TaskList tasks, int reconnect) {
  if (tasks.size() == 0) {
    reconnect_timer_ = std::make_unique<base::RetainingOneShotTimer>();
    reconnect_timer_->Start(FROM_HERE, base::Seconds(reconnect), this,
                            &LearningService::GetTasks);
    VLOG(2) << "No tasks available, reconnecting in " << reconnect << "s";
    return;
  }

  // TODO(lminto): for now, disregard all tasks beyond the first one
  // TODO(stevelaskaridis): extract into a utility function
  Task task = tasks.at(0);
  float lr = 0.01;
  std::map<std::string, float> config = task.GetConfig();
  auto cursor = config.find("lr");
  if (cursor != config.end()) {
    lr = cursor->second;
    VLOG(2) << "Learning rate applied from server: " << lr;
  }

  ModelSpec spec{
      32,   // num_params
      64,   // batch_size
      lr,   // learning_rate
      500,  // num_iterations
      0.5   // threshold
  };

  if (spec.num_params != static_cast<int>(task.GetParameters().at(0).size())) {
    VLOG(2) << "Task specifies a different model size than the client";
    return;
  }
  VLOG(2) << "Task model and client model match!";

  std::unique_ptr<Model> model = std::make_unique<Model>(spec);
  model->SetWeights(task.GetParameters().at(0));
  model->SetBias(task.GetParameters().at(1).at(0));
  auto task_runner =
      std::make_unique<FederatedTaskRunner>(task, std::move(model));

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(
          [](std::unique_ptr<FederatedTaskRunner> task_runner) -> TaskResult {
            auto synthetic_dataset = std::make_unique<SyntheticDataset>(500);
            SyntheticDataset test_dataset =
                synthetic_dataset->SeparateTestData(50);

            task_runner->SetTrainingData(synthetic_dataset->GetDataPoints());
            task_runner->SetTestData(test_dataset.GetDataPoints());
            VLOG(2) << "Model and data set. Task runner initialized.";

            return task_runner->Run();
          },
          std::move(task_runner)),
      base::BindOnce(&LearningService::OnTaskResultComputed,
                     base::Unretained(this)));
}

void LearningService::OnTaskResultComputed(TaskResult result) {
  TaskResultList results;
  results.push_back(result);
  PostTaskResults(results);
}

void LearningService::PostTaskResults(TaskResultList results) {
  for (const auto& result : results) {
    communication_adapter_->PostTaskResult(
        result, base::BindOnce(&LearningService::OnPostTaskResults,
                               base::Unretained(this)));
  }
}

void LearningService::OnPostTaskResults(TaskResultResponse response) {
  int reconnect = 0;

  if (response.IsSuccessful()) {
    reconnect = features::GetFederatedLearningUpdateCycleInSeconds();
    VLOG(2) << "Task results posted successfully";
  } else {
    reconnect = features::GetFederatedLearningUpdateCycleInSeconds() / 2;
    VLOG(2) << "Task results posting failed";
  }

  reconnect_timer_ = std::make_unique<base::RetainingOneShotTimer>();
  reconnect_timer_->Start(FROM_HERE, base::Seconds(reconnect), this,
                          &LearningService::GetTasks);
  VLOG(2) << "Reconnecting in " << reconnect << "s";
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

}  // namespace brave_federated
