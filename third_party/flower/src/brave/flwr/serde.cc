/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/flower/src/brave/flwr/serde.h"

#include <sstream>
#include <vector>

#include "base/strings/string_util.h"
#include "brave/third_party/flower/src/proto/flwr/proto/node.pb.h"

/**
 * Deserialize bytes to float vector
 */
std::vector<float> GetVectorFromString(std::string string) {
  int vector_size = string.size() / sizeof(double);
  double parameters_array[vector_size];
  std::memcpy(parameters_array, string.data(), string.size());

  std::vector<float> parameters_vector(parameters_array,
                                       parameters_array + vector_size);
  return parameters_vector;
}

/**
 * Serialize float vector into bytes
 */
std::string GetStringFromVector(std::vector<float> vector) {
  std::ostringstream oss;
  oss.write(reinterpret_cast<const char*>(vector.data()),
            vector.size() * sizeof(float));

  return oss.str();
}

/**
 * Get list of vectors from flower::Parameters
 */
std::vector<std::vector<float>> GetVectorsFromParameters(
    flower::Parameters parameters_msg) {
  std::vector<std::vector<float>> tensors;
  for (int i = 0; i < parameters_msg.tensors_size(); i++) {
    std::string parameters_string = parameters_msg.tensors(i);
    std::vector<float> parameters_vector =
        GetVectorFromString(parameters_string);
    tensors.push_back(parameters_vector);
  }

  return tensors;
}

/**
 * Get flower::Parameters from list of vectors
 */
flower::Parameters GetParametersFromVectors(
    std::vector<std::vector<float>> parameters_vector) {
  flower::Parameters flower_parameters;
  flower_parameters.set_tensor_type("cpp_double");

  for (auto const& vector : parameters_vector) {
    std::string string = GetStringFromVector(vector);
    flower_parameters.add_tensors(string);
  }

  return flower_parameters;
}

/**
 * Build anonymous pull task request message
 */
flower::PullTaskInsRequest BuildAnonymousPullTaskInsRequestMessage() {
  flower::Node node;
  node.set_node_id(1);
  node.set_anonymous(true);

  flower::PullTaskInsRequest pull_task_instructions_request;
  *pull_task_instructions_request.mutable_node() = node;
  pull_task_instructions_request.add_task_ids("0");

  return pull_task_instructions_request;
}
