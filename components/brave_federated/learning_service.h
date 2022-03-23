/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <string>

#include "base/containers/flat_map.h"
#include "brave/components/brave_federated/eligibility_service_observer.h"

namespace brave_federated {

class DataStoreService;
struct AdNotificationTimingTaskLog;
class EligibilityService;
class Client;
class Model;

class LearningService: public Observer {
 public:
  LearningService(
      DataStoreService* data_store_service,
      EligibilityService* eligibility_service);
  ~LearningService() override;

  void StartParticipating();
  void StopParticipating();

  void OnEligibilityChanged(bool is_eligible) override;

 private:
  void AdNotificationLogsLoadComplete(
      base::flat_map<int, AdNotificationTimingTaskLog> logs);

  DataStoreService* data_store_service_;
  EligibilityService* eligibility_service_;
  std::map<std::string, Client*> clients_;
};

}