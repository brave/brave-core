/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/learning_service.h"

#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

#include "brave/components/brave_federated/client/federated_client.h"
#include "brave/components/brave_federated/client/model.h"
#include "brave/components/brave_federated/data_store_service.h"
#include "brave/components/brave_federated/data_stores/ad_notification_timing_data_store.h"
#include "brave/components/brave_federated/synthetic_dataset/synthetic_dataset.h"
#include "brave/components/brave_federated/eligibility_service.h"
#include "brave/components/brave_federated/eligibility_service_observer.h"

#include <iostream>

namespace brave_federated {

LearningService::LearningService(DataStoreService* data_store_service,
                                 EligibilityService* eligibility_service)
    : data_store_service_(data_store_service),
      eligibility_service_(eligibility_service) {
  auto* ad_timing_data_store =
      data_store_service_->GetAdNotificationTimingDataStore();

  // Populate local datasets
  std::vector<float> ms{3.5, 9.3};  //  b + m_0*x0 + m_1*x1
  float b = 1.7;

  Model* model = new Model(500, 0.01, ms.size());

  std::string client_id = "placeholder";
  FederatedClient* ad_notification_client =
      new FederatedClient(kAdNotificationTaskName, model, client_id);

  SyntheticDataset local_training_data = SyntheticDataset(ms, b, 1000);
  std::cout << "Training set generated." << std::endl;

  SyntheticDataset local_test_data = SyntheticDataset(ms, b, 100);
  std::cout << "Test set generated." << std::endl;

  ad_notification_client->SetTrainingData(local_training_data.DataPoints());
  ad_notification_client->SetTestData(local_test_data.DataPoints());

  clients_.insert(std::make_pair(kAdNotificationTaskName,
                                 std::move(ad_notification_client)));

  auto callback = base::BindOnce(
      &LearningService::AdNotificationLogsLoadComplete, base::Unretained(this));
  ad_timing_data_store->LoadLogs(std::move(callback));

  eligibility_service_->AddObserver(this);
}

LearningService::~LearningService() {
  eligibility_service_->RemoveObserver(this);
}

void LearningService::StartParticipating() {
  // TODO(): Add Task Budgeting
  for (auto it = clients_.begin(); it != clients_.end(); ++it) {
    // TODO() : Add Probabilistic Participation
    it->second->Start();
  }
}

void LearningService::StopParticipating() {
  for (auto it = clients_.begin(); it != clients_.end(); ++it) {
    it->second->Stop();
  }
}

void LearningService::OnEligibilityChanged(bool is_eligible) {
  if (is_eligible) {
    StartParticipating();
  } else {
    StopParticipating();
  }
}

void LearningService::AdNotificationLogsLoadComplete(
    base::flat_map<int, AdNotificationTimingTaskLog> logs) {
  if (logs.size() == 0)
    return;

  auto training_data =
      AdNotificationTimingDataStore::ConvertToTrainingData(logs);
  clients_[kAdNotificationTaskName]->SetTrainingData(training_data);
}

}  // namespace brave_federated