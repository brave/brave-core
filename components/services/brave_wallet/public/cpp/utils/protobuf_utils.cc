/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/brave_wallet/public/cpp/utils/protobuf_utils.h"

#include <utility>

#include "base/big_endian.h"
#include "base/functional/callback.h"

namespace brave_wallet {

namespace {
constexpr size_t kGrpcHeaderSize = 5;
constexpr char kNoCompression = 0;
}  // namespace

std::string GetPrefixedProtobuf(const std::string& serialized_proto) {
  std::string result(kGrpcHeaderSize, 0);
  result[0] = kNoCompression;
  base::WriteBigEndian<uint32_t>(&result[1], serialized_proto.size());
  result.append(serialized_proto);
  return result;
}

std::optional<std::string> ResolveSerializedMessage(
    const std::string& grpc_response_body) {
  if (grpc_response_body.size() < kGrpcHeaderSize) {
    return std::nullopt;
  }
  if (grpc_response_body[0] != kNoCompression) {
    // Compression is not supported yet
    return std::nullopt;
  }
  uint32_t size = 0;
  base::ReadBigEndian(
      reinterpret_cast<const uint8_t*>(&(grpc_response_body[1])), &size);

  if (grpc_response_body.size() != size + kGrpcHeaderSize) {
    return std::nullopt;
  }

  return grpc_response_body.substr(kGrpcHeaderSize);
}

}  // namespace brave_wallet
