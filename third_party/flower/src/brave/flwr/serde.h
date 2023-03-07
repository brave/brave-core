/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_FLOWER_SRC_BRAVE_FLWR_SERDE_H_
#define BRAVE_THIRD_PARTY_FLOWER_SRC_BRAVE_FLWR_SERDE_H_

#include <map>
#include <string>
#include <vector>

#include "brave/third_party/flower/src/proto/flwr/proto/fleet.pb.h"
#include "brave/third_party/flower/src/proto/flwr/proto/transport.pb.h"

class ScalarValue {
 public:
  // Getters
  std::optional<bool> GetBool() { return bool_value_; }
  std::optional<std::string> GetBytes() { return bytes_value_; }
  std::optional<double> GetDouble() { return double_value_; }
  std::optional<int> GetInt() { return integer_value_; }
  std::optional<std::string> GetString() { return string_value_; }

  // Setters
  void SetBool(bool b) { this->bool_value_ = b; }
  void SetBytes(std::string bytes) { this->bytes_value_ = bytes; }
  void SetDouble(double d) { this->double_value_ = d; }
  void SetInt(int i) { this->integer_value_ = i; }
  void SetString(std::string string) { this->string_value_ = string; }

 private:
  std::optional<bool> bool_value_ = std::nullopt;
  std::optional<std::string> bytes_value_ = std::nullopt;
  std::optional<double> double_value_ = std::nullopt;
  std::optional<int> integer_value_ = std::nullopt;
  std::optional<std::string> string_value_ = std::nullopt;
};

typedef std::map<std::string, float> Configs;
typedef std::map<std::string, double> Metrics;

/**
 * Serialize client scalar type to protobuf scalar type
 */
flower::Scalar ScalarToProto(ScalarValue scalar_msg);

/**
 * Deserialize protobuf scalar type to client scalar type
 */
ScalarValue ScalarFromProto(flower::Scalar scalar_msg);

/**
 * Deserialize protobuf configs type to client metrics type
 */
Configs ConfigsFromProto(
    google::protobuf::Map<std::string, flower::Scalar> proto);

/**
 * Serialize client metrics type to protobuf metrics type
 */
google::protobuf::Map<std::string, flower::Scalar> MetricsToProto(
    Metrics metrics);

/**
 * Deserialize bytes to float vector
 */
std::vector<float> GetVectorFromString(std::string string);

/**
 * Serialize float vector into bytes
 */
std::string GetStringFromVector(std::vector<float> vector);

/**
 * Get flower::Parameters from list of vectors
 */
flower::Parameters GetParametersFromVectors(
    std::vector<std::vector<float>> parameters_vectors);

/**
 * Get list of vectors fromm flower::Parameters
 */
std::vector<std::vector<float>> GetVectorsFromParameters(
    flower::Parameters parameters_msg);

#endif  // BRAVE_THIRD_PARTY_FLOWER_SRC_BRAVE_FLWR_SERDE_H_
