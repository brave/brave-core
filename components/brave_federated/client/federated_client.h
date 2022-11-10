/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_CLIENT_FEDERATED_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_CLIENT_FEDERATED_CLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_federated/client/linear_algebra_util/linear_algebra_util.h"
#include "brave/third_party/flower/src/cc/flwr/include/client.h"
#include "memory/scoped_refptr.h"

namespace network {

class SharedURLLoaderFactory;
class SimpleURLLoader;
struct ResourceRequest;

}  // namespace network

namespace brave_federated {

class Model;

class FederatedClient final : public flwr::Client {
 public:
  FederatedClient(
      const std::string& task_name,
      Model* model,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~FederatedClient();

  Model* GetModel();

  void Start();
  void Stop();

  void SetTrainingData(DataSet training_data);
  void SetTestData(DataSet test_data);

  void SetParameters(flwr::Parameters parameters);
  bool IsCommunicating() override;
  flwr::ParametersRes GetParameters() override;
  flwr::PropertiesRes GetProperties(flwr::PropertiesIns instructions) override;
  flwr::EvaluateRes Evaluate(flwr::EvaluateIns instructions) override;
  flwr::FitRes Fit(flwr::FitIns instructions) override;

 private:
  scoped_refptr<network::SharedURLLoaderFactory>
      url_loader_factory_;  // NOT OWNED
  std::unique_ptr<network::SimpleURLLoader> url_loader_;

  bool is_communicating_ = false;
  std::string client_id_;
  std::string task_name_;
  raw_ptr<Model> model_ = nullptr;
  DataSet training_data_;
  DataSet test_data_;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_CLIENT_FEDERATED_CLIENT_H_
