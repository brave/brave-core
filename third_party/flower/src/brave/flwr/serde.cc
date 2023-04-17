/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/flower/src/brave/flwr/serde.h"

#include <sstream>

#include "brave/third_party/flower/src/proto/flwr/proto/transport.pb.h"

flower::Scalar ScalarToProto(ScalarValue scalar_msg) {
  flower::Scalar s;
  if (scalar_msg.GetBool() != std::nullopt) {
    s.set_bool_(scalar_msg.GetBool().value());
    return s;
  }
  if (scalar_msg.GetBytes() != std::nullopt) {
    s.set_bytes(scalar_msg.GetBytes().value());
    return s;
  }
  if (scalar_msg.GetDouble() != std::nullopt) {
    s.set_double_(scalar_msg.GetDouble().value());
    return s;
  }
  if (scalar_msg.GetInt() != std::nullopt) {
    s.set_int64(scalar_msg.GetInt().value());
    return s;
  }
  if (scalar_msg.GetString() != std::nullopt) {
    s.set_string(scalar_msg.GetString().value());
    return s;
  }

  return s;
}

ScalarValue ScalarFromProto(flower::Scalar scalar_msg) {
  ScalarValue scalar;
  switch (scalar_msg.scalar_case()) {
    case flower::Scalar::kDouble:
      scalar.SetDouble(scalar_msg.double_());
      return scalar;
    case flower::Scalar::kInt64:
      scalar.SetInt(scalar_msg.int64());
      return scalar;
    case flower::Scalar::kBool:
      scalar.SetBool(scalar_msg.bool_());
      return scalar;
    case flower::Scalar::kString:
      scalar.SetString(scalar_msg.string());
      return scalar;
    case flower::Scalar::kBytes:
      scalar.SetBytes(scalar_msg.bytes());
      return scalar;
    case 0:
      return scalar;
  }
}

google::protobuf::Map<std::string, flower::Scalar> MetricsToProto(
    Metrics metrics) {
  google::protobuf::Map<std::string, flower::Scalar> proto;

  for (auto& [key, value] : metrics) {
    ScalarValue scalar = ScalarValue();
    scalar.SetDouble(value);
    proto[key] = ScalarToProto(scalar);
  }

  return proto;
}

Configs ConfigsFromProto(
    google::protobuf::Map<std::string, flower::Scalar> proto) {
  Configs config;

  for (auto& [key, value] : proto) {
    config[key] = ScalarFromProto(value).GetDouble().value();
  }
  return config;
}

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

std::vector<float> GetVectorFromString(std::string string) {
  const int k_vector_size = string.size() / sizeof(double);
  std::vector<float> parameters_vector;

  for (int i = 0; i < k_vector_size; i++) {
    double value;
    std::memcpy(&value, string.data() + i * sizeof(double), sizeof(double));
    parameters_vector.push_back(value);
  }

  return parameters_vector;
}

std::string GetStringFromVector(std::vector<float> vector) {
  std::string string;
  for (double const value : vector) {
    string.append(reinterpret_cast<const char*>(&value), sizeof(double));
  }

  return string;
}
