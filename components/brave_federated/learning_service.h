/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_SERVICE_H_

#include <map>
#include <memory>
#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_federated/eligibility_service_observer.h"
#include "brave/components/brave_federated/task/typing.h"

namespace network {

class SharedURLLoaderFactory;

}  // namespace network

namespace brave_federated {

class CommunicationAdapter;
class DataStoreService;
class EligibilityService;
class FederatedTaskRunner;

class LearningService : public Observer {
 public:
  explicit LearningService(
      EligibilityService* eligibility_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~LearningService() override;

  void StartParticipating();
  void StopParticipating();

  void GetTasks();
  void PostTaskResults(TaskResultList results);
  void HandleTasksOrReconnect(TaskList tasks, int reconnect);
  void OnPostTaskResults(TaskResultResponse response);

  void OnEligibilityChanged(bool is_eligible) override;

 private:
  scoped_refptr<network::SharedURLLoaderFactory>
      url_loader_factory_;  // NOT OWNED
  EligibilityService* eligibility_service_;
  CommunicationAdapter* communication_adapter_;

  std::unique_ptr<base::RetainingOneShotTimer> reconnect_timer_;
  bool participating_;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_SERVICE_H_
