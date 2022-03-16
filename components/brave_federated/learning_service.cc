/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/learning_service.h"

#include <string>
#include <map>

#include "brave/components/brave_federated/client/client.h"
#include "brave/components/brave_federated/data_store_service.h"
#include "brave/components/brave_federated/data_stores/ad_notification_timing_data_store.h"
#include "brave/components/brave_federated/eligibility_service.h"

#include <iostream>

namespace brave_federated {

LearningService::LearningService(
    DataStoreService* data_store_service, 
    EligibilityService* eligibility_service)
    : data_store_service_(data_store_service),
      eligibility_service_(eligibility_service) {
  
    auto* ad_timing_data_store = data_store_service_->GetAdNotificationTimingDataStore();
    std::string model = "model";
    Client* ad_notification_client = new Client(kAdNotificationTaskName, model);
    auto callback = base::BindOnce(&LearningService::AdNotificationLogsLoadComplete,
                                 base::Unretained(this));
    ad_timing_data_store->LoadLogs(std::move(callback));

    clients_.insert(std::make_pair("task_name", std::move(ad_notification_client)));
    eligibility_service_->IsEligibileForFederatedTask();

    
}

LearningService::~LearningService() {}

void LearningService::StartLearning() {
    for ( auto it = clients_.begin(); it != clients_.end(); ++it ) {
        it->second->Start();
    } 
}

void LearningService::StopLearning() {
    for ( auto it = clients_.begin(); it != clients_.end(); ++it ) {
        it->second->Stop();
    } 
}

void LearningService::AdNotificationLogsLoadComplete(
    base::flat_map<int, AdNotificationTimingTaskLog> logs) {
    std::cerr << "Yo" << std::endl;
}

}