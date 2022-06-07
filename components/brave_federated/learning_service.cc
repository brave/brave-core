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
#include "brave/components/brave_federated/eligibility_service.h"
#include "brave/components/brave_federated/eligibility_service_observer.h"
#include "brave/components/brave_federated/synthetic_dataset/synthetic_dataset.h"

#include "brave/third_party/flower/src/cc/flwr/include/start.h"

namespace brave_federated {

LearningService::LearningService(DataStoreService* data_store_service,
                                 EligibilityService* eligibility_service)
    : data_store_service_(data_store_service),
      eligibility_service_(eligibility_service) {
  auto* ad_timing_data_store =
      data_store_service_->GetAdNotificationTimingDataStore();

  // Populate local datasets
  Model* model = new Model(500, 0.01, 32);

  FederatedClient* ad_notification_client =
      new FederatedClient(kAdNotificationTaskName, model);

  std::vector<std::vector<float>> W(2, std::vector<float>(32));
  std::vector<float> b(2);
 
  std::vector<float> W0 = {0.720553, -0.22378, 0.724898, 1.05209, 0.171692, -2.08635, 0.00898889, 0.00195967, -0.521962, -1.69172, -0.906425, -1.05066, -0.920127, -0.200614, -0.0248187, -0.510679, 0.139501, 1.44922, -0.0535475, -0.497441, -0.902036, 1.08325, -1.31984, 0.413791, -1.44259, 0.757306, 0.670382, -1.13497, -0.278086, -1.30519, 0.111584, -0.362997};
  W[0] = W0;
  b[0] = -1.45966;
  std::vector<float> W1 = {-1.20866, -0.385986, -1.37335, 1.54405, 1.19847, 0.185225, 0.446334, -0.00641536, -0.439716, 2.525, -0.638792, 1.5815, -0.933648, -0.240064, -1.0451, -0.00015671, -0.543405, 0.560255, -1.80757, -0.907905, 2.27475, 0.42947, 0.725056, -1.54398, -2.43804, -1.07677, 0.00487297, -1.25289, -0.708508, 0.322749, 0.91749, -0.598813};
  W[1] = W1;
  b[1] = 1.12165;

  SyntheticDataset local_training_data = SyntheticDataset(W, b, 32, 5500);
  std::cout << "Training set generated." << std::endl;
  
  SyntheticDataset local_test_data = local_training_data.SeparateTestData(5000);
  std::cout << "Test set generated." << std::endl;

  local_training_data.DumpToCSV("training_data_");
  local_test_data.DumpToCSV("test_data_");

  ad_notification_client->SetTrainingData(local_training_data.DataPoints());
  ad_notification_client->SetTestData(local_test_data.DataPoints());

  clients_.insert(std::make_pair(kAdNotificationTaskName,
                                 std::move(ad_notification_client)));
  auto callback = base::BindOnce(
      &LearningService::AdNotificationLogsLoadComplete, base::Unretained(this));
  ad_timing_data_store->LoadLogs(std::move(callback));

  eligibility_service_->AddObserver(this);

  StartParticipating();
}

LearningService::~LearningService() {
  StopParticipating();
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