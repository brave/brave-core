/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/client/client.h"

#include <string>
#include <memory>
#include <vector>

namespace brave_federated {

Client::Client(const std::string& task_name, const std::string& model):
    task_name_(task_name),
    model_(model) {}

Client::~Client() {}

void Client::Start() {
    // TODO
}

void Client::Stop() {
    // TODO
}

void Client::SetTrainingData(std::vector<std::vector<float>> training_data) {
    training_data_ = training_data;
}

}