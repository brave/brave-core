/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/client/client.h"

#include <iostream>
#include <string>
#include <memory>
#include <vector>

#include "brave/components/brave_federated/client/model.h"
#include "brave/components/brave_federated/linear_algebra_util/linear_algebra_util.h"
#include "brave/components/brave_federated/synthetic_dataset/synthetic_dataset.h"

namespace brave_federated {

Client::Client(const std::string& task_name, Model* model):
    task_name_(task_name),
    model_(model) {}

Client::~Client() {}

void Client::Start() {
    // TODO
    Model* model = GetModel();

    std::vector<float> ms{3.5, 9.3};  //  b + m_0*x0 + m_1*x1
    float b = 1.7;
    
    SyntheticDataset local_test_data = SyntheticDataset(ms, b, 500);
    std::cout << "Test set generated." << std::endl;

    // Define a model
    
    model->Train(training_data_);
    auto evaluation = model->Evaluate(local_test_data);
    std::cout<<std::get<0>(evaluation);
}

void Client::Stop() {
    // TODO
}

Model* Client::GetModel() {
    return model_;
}

void Client::SetTrainingData(std::vector<std::vector<float>> training_data) {
    training_data_ = training_data;
}

}