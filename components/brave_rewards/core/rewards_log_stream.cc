/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/rewards_log_stream.h"

namespace brave_rewards::internal {

RewardsLogStream::RewardsLogStream(mojom::RewardsEngineClient& client,
                                   base::Location location,
                                   int log_level)
    : client_(&client), location_(location), log_level_(log_level) {}

RewardsLogStream::~RewardsLogStream() {
  if (client_) {
    client_->Log(location_.file_name(), location_.line_number(), log_level_,
                 stream_.str());
  }
}

RewardsLogStream::RewardsLogStream(RewardsLogStream&& other) {
  *this = std::move(other);
}

RewardsLogStream& RewardsLogStream::operator=(RewardsLogStream&& other) {
  if (this != &other) {
    client_ = std::exchange(other.client_, nullptr);
    location_ = other.location_;
    log_level_ = other.log_level_;
    stream_ = std::move(other.stream_);
  }
  return *this;
}

}  // namespace brave_rewards::internal
