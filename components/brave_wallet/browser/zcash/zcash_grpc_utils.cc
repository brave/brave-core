/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_grpc_utils.h"

#include <utility>

#include "base/big_endian.h"
#include "base/functional/callback.h"

namespace brave_wallet {

namespace {
constexpr size_t kMaxMessageSizeBytes = 10000;
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

// Resolves serialized protobuf from length-prefixed string
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

GRrpcMessageStreamHandler::GRrpcMessageStreamHandler() = default;
GRrpcMessageStreamHandler::~GRrpcMessageStreamHandler() = default;

void GRrpcMessageStreamHandler::OnDataReceived(std::string_view string_piece,
                                               base::OnceClosure resume) {
  data_.append(string_piece);
  std::string_view data_view(data_);

  bool should_resume = false;
  while (!should_resume) {
    std::optional<size_t> message_body_size;
    if (data_view.size() > kGrpcHeaderSize) {
      if (data_view[0] != kNoCompression) {
        OnComplete(false);
        return;
      }
      uint32_t size = 0;
      base::ReadBigEndian(reinterpret_cast<const uint8_t*>(&(data_view[1])),
                          &size);
      message_body_size = size;
      if (*message_body_size > kMaxMessageSizeBytes) {
        // Too large message
        OnComplete(false);
        return;
      }
    }

    if (message_body_size &&
        data_view.size() >= (kGrpcHeaderSize + *message_body_size)) {
      if (!ProcessMessage(
              data_view.substr(0, kGrpcHeaderSize + *message_body_size))) {
        OnComplete(true);
        return;
      }

      data_view = data_view.substr(kGrpcHeaderSize + *message_body_size);
    } else {
      should_resume = true;
    }
  }
  data_ = std::string(data_view);
  std::move(resume).Run();
}

void GRrpcMessageStreamHandler::OnRetry(base::OnceClosure start_retry) {
  if (retry_counter_++ < 5) {
    std::move(start_retry).Run();
  }
}

}  // namespace brave_wallet
