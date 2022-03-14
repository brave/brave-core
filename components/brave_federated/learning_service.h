/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <string>

namespace brave_federated {

class DataStoreService;
class EligibilityService;
class Client;

class LearningService {
 public:
  LearningService(DataStoreService* data_store_service, EligibilityService* eligibility_service);
  ~LearningService();

  void StartLearning();
  void StopLearning();

 private:
  DataStoreService* data_store_service_;
  EligibilityService* eligibility_service_;
  std::map<std::string, Client*> clients_;
};

}