/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_SERVICE_H_

#include <map>
#include <memory>
#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_federated/eligibility_service_observer.h"
#include "brave/components/brave_federated/features.h"
#include "brave/components/brave_federated/task/typing.h"
#include "net/base/backoff_entry.h"

namespace network {

class SharedURLLoaderFactory;

}  // namespace network

namespace brave_federated {

class CommunicationAdapter;
class EligibilityService;
class FederatedTaskRunner;

class LearningService : public Observer {
 public:
  explicit LearningService(
      EligibilityService* eligibility_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~LearningService() override;

  void OnEligibilityChanged(bool is_eligible) override;

 private:
  void Init();

  void StartParticipating();
  void StopParticipating();

  void GetTasks();
  void PostTaskResults(TaskResultList results);

  void HandleTasksOrReconnect(TaskList tasks, int reconnect);
  TaskResult LoadDatasetAndRunTask(
      std::unique_ptr<FederatedTaskRunner> task_runner);

  void OnTaskResultComputed(TaskResult result);
  void OnPostTaskResults(TaskResultResponse response);

  scoped_refptr<network::SharedURLLoaderFactory>
      url_loader_factory_;  // NOT OWNED
  EligibilityService* eligibility_service_;
  std::unique_ptr<CommunicationAdapter> communication_adapter_;
  std::unique_ptr<base::OneShotTimer> init_task_timer_;

  std::unique_ptr<base::RetainingOneShotTimer> reconnect_timer_;
  bool participating_ = false;
  bool initialized_ = false;

  std::unique_ptr<const net::BackoffEntry::Policy> post_results_policy_;
  std::unique_ptr<net::BackoffEntry> post_results_backoff_entry_;

  base::WeakPtrFactory<LearningService> weak_factory_{this};
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_SERVICE_H_
