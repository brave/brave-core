/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_grpc_utils.h"

#include <utility>

#include "base/big_endian.h"
#include "base/functional/callback.h"
#include "base/logging.h"

namespace brave_wallet {

namespace {
constexpr size_t kMaxMessageSizeBytes = 10000;
}  // namespace

std::string GetPrefixedProtobuf(const std::string& serialized_proto) {
  char compression = 0;  // 0 means no compression
  char buff[4];          // big-endian 4 bytes of message size
  base::WriteBigEndian<uint32_t>(buff, serialized_proto.size());
  std::string result;
  result.append(&compression, 1);
  result.append(buff, 4);
  result.append(serialized_proto);
  return result;
}

// Resolves serialized protobuf from length-prefixed string
absl::optional<std::string> ResolveSerializedMessage(
    const std::string& grpc_response_body) {
  if (grpc_response_body.size() < 5) {
    return absl::nullopt;
  }
  if (grpc_response_body[0] != 0) {
    // Compression is not supported yet
    return absl::nullopt;
  }
  uint32_t size = 0;
  base::ReadBigEndian(
      reinterpret_cast<const uint8_t*>(&(grpc_response_body[1])), &size);

  if (grpc_response_body.size() != size + 5) {
    return absl::nullopt;
  }

  return grpc_response_body.substr(5);
}

GRrpcMessageStreamHandler::GRrpcMessageStreamHandler() = default;
GRrpcMessageStreamHandler::~GRrpcMessageStreamHandler() = default;

void GRrpcMessageStreamHandler::OnDataReceived(std::string_view string_piece,
                                               base::OnceClosure resume) {
  data_.append(string_piece);

  bool should_resume = false;
  while (!should_resume) {
    if (!data_to_read_ && data_.size() > 5) {
      if (data_[0] != 0) {
        OnComplete(false);
        return;
      }
      uint32_t size = 0;
      base::ReadBigEndian(reinterpret_cast<const uint8_t*>(&(data_[1])), &size);
      data_to_read_ = size;
      if (*data_to_read_ > kMaxMessageSizeBytes) {
        // Too large message
        OnComplete(false);
        return;
      }
    }

    if (data_to_read_ && data_.size() > *data_to_read_) {
      if (!ProcessMessage(data_.substr(0, 5 + *data_to_read_))) {
        OnComplete(true);
        return;
      }

      data_ = data_.substr(5 + *data_to_read_);
      data_to_read_.reset();
    } else {
      should_resume = true;
    }
  }
  std::move(resume).Run();
}

void GRrpcMessageStreamHandler::OnRetry(base::OnceClosure start_retry) {
  if (retry_counter_++ < 5) {
    std::move(start_retry).Run();
  }
}

}  // namespace brave_wallet
