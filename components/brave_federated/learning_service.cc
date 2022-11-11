/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/learning_service.h"

#include <utility>
#include <vector>

#include "brave/components/brave_federated/communication_adapter.h"
#include "brave/components/brave_federated/eligibility_service.h"
#include "brave/components/brave_federated/notification_ad_task_constants.h"
#include "brave/components/brave_federated/task/communication_helper.h"
#include "brave/components/brave_federated/task/model.h"
#include "brave/components/brave_federated/task/task_runner.h"
#include "brave/components/brave_federated/task/typing.h"
#include "brave/components/brave_federated/util/synthetic_dataset.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

#include <iostream>

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
  communication_adapter_->GetTasks(
      base::BindOnce(&LearningService::HandleTasks, base::Unretained(this)));
}

void LearningService::StopParticipating() {
  DCHECK(participating_);
  participating_ = false;
}

void LearningService::HandleTasks(TaskList tasks) {
  Task task = tasks.at(0);
  std::cerr << "**: Received tasks to handle" << std::endl;
  // TODO(lminto): implement actual task handling
  ModelSpec spec{32, 0.01, 500, 10, 0.5};
  Model* model = new Model(spec);
  TaskRunner* notification_ad_task_runner = new TaskRunner(task, model);

  SyntheticDataset local_training_data = CreateDefaultSyntheticDataset();
  SyntheticDataset local_test_data = local_training_data.SeparateTestData(5000);

  notification_ad_task_runner->SetTrainingData(
      local_training_data.GetDataPoints());
  notification_ad_task_runner->SetTestData(local_test_data.GetDataPoints());

  task_runners_.insert(std::make_pair(kNotificationAdTaskName,
                                      std::move(notification_ad_task_runner)));
  // TODO(lminto): run task runner async and use callback
  TaskResultList results;
  TaskResult result = notification_ad_task_runner->Run();
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
  if (response.IsSuccessful()) {
    std::cerr << "**: Succesfully posted results" << std::endl;
  }

  std::cerr << "**: Failed posting results" << std::endl;
}

void LearningService::OnEligibilityChanged(bool is_eligible) {
  if (is_eligible) {
    StartParticipating();
  } else {
    StopParticipating();
  }
}

}  // namespace brave_federated
