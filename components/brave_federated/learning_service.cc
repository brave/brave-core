/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/learning_service.h"

#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_federated/communication_adapter.h"
#include "brave/components/brave_federated/eligibility_service.h"
#include "brave/components/brave_federated/features.h"
#include "brave/components/brave_federated/notification_ad_task_constants.h"
#include "brave/components/brave_federated/task/model.h"
#include "brave/components/brave_federated/task/task_runner.h"
#include "brave/components/brave_federated/task/typing.h"
#include "brave/components/brave_federated/util/synthetic_dataset.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace brave_federated {

LearningService::LearningService(
    EligibilityService* eligibility_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(url_loader_factory),
      eligibility_service_(eligibility_service) {
  DCHECK(url_loader_factory);
  DCHECK(eligibility_service);

  communication_adapter_ = new CommunicationAdapter(url_loader_factory);
  eligibility_service_->AddObserver(this);

  StartParticipating();
}

LearningService::~LearningService() {
  StopParticipating();
  eligibility_service_->RemoveObserver(this);
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
  Task task = tasks.at(0);
  ModelSpec spec{
      32,    // num_params
      64,    // batch_size
      0.01,  // learning_rate
      500,   // num_iterations
      0.5    // threshold
  };

  if (spec.num_params != static_cast<int>(task.GetParameters().at(0).size())) {
    VLOG(2) << "Task specifies a different model size than the client";
    return;
  }
  VLOG(2) << "Task model and client model match!";

  Model* model = new Model(spec);
  model->SetWeights(task.GetParameters().at(0));
  model->SetBias(task.GetParameters().at(1).at(0));
  TaskRunner* task_runner = new TaskRunner(task, model);

  SyntheticDataset* local_training_data = new SyntheticDataset(500);
  SyntheticDataset* local_test_data = new SyntheticDataset(50);

  task_runner->SetTrainingData(local_training_data->GetDataPoints());
  task_runner->SetTestData(local_test_data->GetDataPoints());
  VLOG(2) << "Model and data set. Task runner initialized.";

  // TODO(lminto): should we run the task runner on another thread?
  TaskResultList results;
  TaskResult result = task_runner->Run();
  results.push_back(result);
  PostTaskResults(results);
  delete task_runner;
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
    reconnect = features::GetFederatedLearningUpdateCycleInMinutes() * 60;
    VLOG(2) << "Task results posted successfully";
  } else {
    reconnect = features::GetFederatedLearningUpdateCycleInMinutes()/2 * 60;
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
