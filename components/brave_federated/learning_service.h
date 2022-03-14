/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_SERVICE_H_

#include <map>
#include <string>

#include "base/containers/flat_map.h"
#include "brave/components/brave_federated/eligibility_service_observer.h"

namespace brave_federated {

class DataStoreService;
class EligibilityService;
class FederatedClient;

class LearningService : public Observer {
 public:
  explicit LearningService(EligibilityService* eligibility_service);
  ~LearningService() override;

  void StartParticipating();
  void StopParticipating();

  void OnEligibilityChanged(bool is_eligible) override;

 private:
  EligibilityService* eligibility_service_;
  std::map<std::string, FederatedClient*> clients_;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_SERVICE_H_
