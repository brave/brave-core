/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/client/federated_client.h"

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "brave/components/brave_federated/client/model.h"
#include "brave/components/brave_federated/linear_algebra_util/linear_algebra_util.h"

namespace brave_federated {

FederatedClient::FederatedClient(const std::string& task_name,
                                 Model* model,
                                 std::string client_id)
    : client_id_(client_id), task_name_(task_name), model_(model) {}

FederatedClient::~FederatedClient() {}

void FederatedClient::Start() {
  // TODO

  Model* model = GetModel();

  model->Train(training_data_);
  auto evaluation = model->Evaluate(test_data_);
  std::cout << std::get<0>(evaluation);
}

void FederatedClient::Stop() {
  // TODO
}

Model* FederatedClient::GetModel() {
  return model_;
}

void FederatedClient::SetTrainingData(
    std::vector<std::vector<float>> training_data) {
  training_data_ = training_data;
}

void FederatedClient::SetTestData(std::vector<std::vector<float>> test_data) {
  test_data_ = test_data;
}

/**
 * Return the current local model parameters
 * Simple string are used for now to test communication, needs updates in the
 * future
 */
flwr::ParametersRes FederatedClient::get_parameters() {
  // Serialize
  std::vector<float> pred_weights = this->model_->PredWeights();
  float pred_b = this->model_->Bias();
  std::list<std::string> tensors;

  std::ostringstream oss1, oss2;  // Possibly unnecessary
  oss1.write(reinterpret_cast<const char*>(pred_weights.data()),
             pred_weights.size() * sizeof(float));
  tensors.push_back(oss1.str());

  oss2.write(reinterpret_cast<const char*>(&pred_b), sizeof(float));
  tensors.push_back(oss2.str());

  std::string tensor_str = "cpp_double";
  return flwr::Parameters(tensors, tensor_str);
}

void FederatedClient::set_parameters(flwr::Parameters params) {
  std::list<std::string> s = params.getTensors();
  std::cout << "Received " << s.size() << " Layers from server:" << std::endl;

  if (s.empty() == 0) {
    // Layer 1
    auto layer = s.begin();
    size_t num_bytes = (*layer).size();
    const char* weights_char = (*layer).c_str();
    const float* weights_float = reinterpret_cast<const float*>(weights_char);
    std::vector<float> weights(weights_float,
                               weights_float + num_bytes / sizeof(float));
    this->model_->SetPredWeights(weights);

    // Layer 2 = Bias
    auto layer_2 = std::next(layer, 1);
    num_bytes = (*layer_2).size();
    const char* bias_char = (*layer_2).c_str();
    const float* bias_float = reinterpret_cast<const float*>(bias_char);
    this->model_->SetBias(bias_float[0]);
    std::cout << "  b_server = " << std::fixed << this->model_->Bias()
              << std::endl;
  }
}

flwr::PropertiesRes FederatedClient::get_properties(flwr::PropertiesIns ins) {
  flwr::PropertiesRes p;
  p.setPropertiesRes(static_cast<flwr::Properties>(ins.getPropertiesIns()));
  return p;
}

/**
 * Refine the provided weights using the locally held dataset
 * Simple settings are used for testing, needs updates in the future
 */
flwr::FitRes FederatedClient::fit(flwr::FitIns ins) {
  std::cout << "Fitting..." << std::endl;
  flwr::FitRes resp;

  flwr::Parameters p = ins.getParameters();
  this->set_parameters(p);

  std::tuple<size_t, float, float> result =
      this->model_->Train(this->training_data_);

  resp.setParameters(this->get_parameters().getParameters());
  resp.setNum_example(std::get<0>(result));

  return resp;
}

/**
 * Evaluate the provided weights using the locally held dataset
 * Needs updates in the future
 */
flwr::EvaluateRes FederatedClient::evaluate(flwr::EvaluateIns ins) {
  std::cout << "Evaluating..." << std::endl;
  flwr::EvaluateRes resp;
  flwr::Parameters p = ins.getParameters();
  this->set_parameters(p);

  // Evaluation returns a number_of_examples, a loss and an "accuracy"
  std::tuple<size_t, float, float> result =
      this->model_->Evaluate(this->test_data_);

  resp.setNum_example(std::get<0>(result));
  resp.setLoss(std::get<1>(result));

  flwr::Scalar loss_metric = flwr::Scalar();
  loss_metric.setFloat(std::get<2>(result));
  std::map<std::string, flwr::Scalar> metric = {{"loss", loss_metric}};
  resp.setMetrics(metric);

  return resp;
}

}  // namespace brave_federated