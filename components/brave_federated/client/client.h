/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_CLIENT_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_CLIENT_CLIENT_H_

#include <string>
#include <memory>

namespace brave_federated {

class Client {
 public:
  Client(const std::string& task_name, const std::string& model);
  ~Client();

  void SetLogs(const std::string& logs);
  void Start();
  void Stop();
};

} // namespace brave_federated

#endif //BRAVE_COMPONENTS_BRAVE_FEDERATED_CLIENT_CLIENT_H_