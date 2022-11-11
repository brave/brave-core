/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/task/task_runner.h"
#include <__fwd/get.h>

#include <list>
#include <map>
#include <sstream>
#include <tuple>

#include "base/check.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/threading/sequence_bound.h"
#include "base/threading/sequenced_task_runner_handle.h"

#include "brave/components/brave_federated/task/model.h"
#include "brave/components/brave_federated/task/typing.h"

#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

#include <iostream>

namespace brave_federated {

TaskRunner::TaskRunner(Task task, Model* model) : task_(task), model_(model) {
  DCHECK(model_);
}

TaskRunner::~TaskRunner() = default;

Model* TaskRunner::GetModel() {
  return model_;
}

TaskResult TaskRunner::Run() {
  PerformanceReport report;
  if (task_.GetType() == TaskType::TrainingTask) {
    report = Train();
  } else if (task_.GetType() == TaskType::EvaluationTask) {
    report = Evaluate();
  }

  TaskResult result(task_, report);
  return result;
}

void TaskRunner::SetTrainingData(DataSet training_data) {
  training_data_ = training_data;
}

void TaskRunner::SetTestData(DataSet test_data) {
  test_data_ = test_data;
}

void TaskRunner::SetWeights(ModelWeights weights) {
  model_->SetWeights(std::get<0>(weights));
  model_->SetBias(std::get<1>(weights));
}

ModelWeights TaskRunner::GetWeights() {
  return std::make_tuple(model_->GetWeights(), model_->GetBias());
}

// flwr::ParametersRes TaskRunner::GetParameters() {
//   // Serialize
//   const Weights prediction_weights = model_->GetPredWeights();
//   const float prediction_bias = model_->Bias();

//   std::list<std::string> tensors;

//   std::ostringstream oss1;
//   oss1.write(reinterpret_cast<const char*>(prediction_weights.data()),
//              prediction_weights.size() * sizeof(float));
//   tensors.push_back(oss1.str());

//   std::ostringstream oss2;
//   oss2.write(reinterpret_cast<const char*>(&prediction_bias), sizeof(float));
//   tensors.push_back(oss2.str());

//   std::string tensor_string = "cpp_float";
//   return flwr::ParametersRes(flwr::Parameters(tensors, tensor_string));
// }

// const float* GetLayerWeightsFromString(std::string layer_string) {
//   const char* weights_char = (layer_string).c_str();
//   return reinterpret_cast<const float*>(weights_char);
// }

// void TaskRunner::SetParameters(flwr::Parameters parameters) {
//   std::list<std::string> tensor_string = parameters.getTensors();

//   if (tensor_string.empty() == 0) {
//     // Layer 1
//     auto layer = tensor_string.begin();
//     size_t num_bytes = (*layer).size();
//     auto* weights_float = GetLayerWeightsFromString(*layer);
//     Weights weights(weights_float,
//                     weights_float + num_bytes / sizeof(float));
//     model_->SetPredWeights(weights);

//     // Layer 2 = Bias
//     auto* bias_float = GetLayerWeightsFromString(*std::next(layer, 1));
//     model_->SetBias(bias_float[0]);
//   }
// }
// flwr::PropertiesRes TaskRunner::GetProperties(flwr::PropertiesIns
// instructions) {
//   flwr::PropertiesRes properties;
//   properties.setPropertiesRes(static_cast<flwr::Properties>(instructions.getPropertiesIns()));
//   return properties;
// }

// flwr::FitRes TaskRunner::Fit(flwr::FitIns instructions) {
//   auto config = instructions.getConfig();
//   flwr::FitRes response;
//   flwr::Parameters parameters = instructions.getParameters();
//   SetParameters(parameters);

//   std::tuple<size_t, float, float> result =
//       model_->Train(training_data_);

//   response.setParameters(GetParameters().getParameters());
//   response.setNum_example(std::get<0>(result));

//   return response;
// }

// flwr::EvaluateRes TaskRunner::Evaluate(flwr::EvaluateIns instructions) {
//   flwr::EvaluateRes response;
//   flwr::Parameters parameters = instructions.getParameters();
//   SetParameters(parameters);

//   // Evaluation returns a number_of_examples, a loss and an "accuracy"
//   std::tuple<size_t, float, float> result =
//       model_->Evaluate(test_data_);

//   response.setNum_example(std::get<0>(result));
//   response.setLoss(std::get<1>(result));

//   flwr::Scalar accuracy = flwr::Scalar();
//   accuracy.setDouble(static_cast<double>(std::get<2>(result)));
//   std::map<std::string, flwr::Scalar> metric = {
//       {"accuracy", accuracy},
//   };
//   response.setMetrics(metric);
//   return response;
// }

}  // namespace brave_federated
