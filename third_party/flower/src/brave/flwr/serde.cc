/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/flower/src/brave/flwr/serde.h"

#include <sstream>
#include <vector>

#include "base/strings/string_util.h"
#include "brave/third_party/flower/src/proto/flwr/proto/transport.pb.h"

/**
 * Serialize client scalar type to protobuf scalar type
 */
flower::Scalar scalar_to_proto(ScalarValue scalar_msg) {
  flower::Scalar s;
  if (scalar_msg.getBool() != std::nullopt) {
    s.set_bool_(scalar_msg.getBool().value());
    return s;
  }
  if (scalar_msg.getBytes() != std::nullopt) {
    s.set_bytes(scalar_msg.getBytes().value());
    return s;
  }
  if (scalar_msg.getDouble() != std::nullopt) {
    s.set_double_(scalar_msg.getDouble().value());
    return s;
  }
  if (scalar_msg.getInt() != std::nullopt) {
    s.set_int64(scalar_msg.getInt().value());
    return s;
  }
  if (scalar_msg.getString() != std::nullopt) {
    s.set_string(scalar_msg.getString().value());
    return s;
  }

  return s;
}

/**
 * Deserialize protobuf scalar type to client scalar type
 */
ScalarValue scalar_from_proto(flower::Scalar scalar_msg) {
  ScalarValue scalar;
  switch (scalar_msg.scalar_case()) {
    case flower::Scalar::kDouble:
      scalar.setDouble(scalar_msg.double_());
      return scalar;
    case flower::Scalar::kInt64:
      scalar.setInt(scalar_msg.int64());
      return scalar;
    case flower::Scalar::kBool:
      scalar.setBool(scalar_msg.bool_());
      return scalar;
    case flower::Scalar::kString:
      scalar.setString(scalar_msg.string());
      return scalar;
    case flower::Scalar::kBytes:
      scalar.setBytes(scalar_msg.bytes());
      return scalar;
    case 0:
      return scalar;
  }
}

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
