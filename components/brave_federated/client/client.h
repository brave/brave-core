/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_CLIENT_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_CLIENT_CLIENT_H_

#include <string>
#include <memory>
#include <vector>

namespace brave_federated {

class Model;

class Client {
 public:
  Client(const std::string& task_name, Model* model);
  ~Client();

  Model* GetModel();

  void Start();
  // TODO() : Stop client
  void Stop();

  void SetTrainingData(std::vector<std::vector<float>> training_data);

 private:
  // TODO() : Constraints: 1. Bound number of participations
  //                       2. Min batch size
  //                       3. Gradient clipping
  std::string task_name_ = "";
  Model* model_;
  std::vector<std::vector<float>> training_data_;
};

} // namespace brave_federated

#endif //BRAVE_COMPONENTS_BRAVE_FEDERATED_CLIENT_CLIENT_H_