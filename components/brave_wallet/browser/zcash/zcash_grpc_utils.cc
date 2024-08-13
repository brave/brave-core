/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_grpc_utils.h"

#include <utility>

#include "base/containers/span.h"
#include "base/functional/callback.h"
#include "base/numerics/byte_conversions.h"

namespace brave_wallet {

namespace {
constexpr size_t kMaxMessageSizeBytes = 10000;
constexpr size_t kGrpcHeaderSize = 5;
constexpr char kNoCompression = 0;
}  // namespace

GRrpcMessageStreamHandler::GRrpcMessageStreamHandler()
    : message_data_limit_(kMaxMessageSizeBytes) {}
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
      message_body_size = base::numerics::U32FromBigEndian(
          base::as_byte_span(data_view).subspan<1, 4u>());
      if (*message_body_size > message_data_limit_) {
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
