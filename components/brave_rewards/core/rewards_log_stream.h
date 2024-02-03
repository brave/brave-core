/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_LOG_STREAM_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_LOG_STREAM_H_

#include <sstream>
#include <utility>

#include "base/location.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/brave_rewards/common/mojom/rewards_engine.mojom.h"

namespace brave_rewards::internal {

// Convenience class for writing to the Rewards event log using stream
// operators via a RewardsEngineClient interface. When the instance is
// destroyed, the completed message will be sent to the client for logging.
class RewardsLogStream {
 public:
  RewardsLogStream(mojom::RewardsEngineClient& client,
                   base::Location location,
                   int log_level);

  RewardsLogStream(const RewardsLogStream&) = delete;
  RewardsLogStream& operator=(const RewardsLogStream&) = delete;

  RewardsLogStream(RewardsLogStream&& other);
  RewardsLogStream& operator=(RewardsLogStream&& other);

  ~RewardsLogStream();

  template <typename T>
  RewardsLogStream& operator<<(T&& value) {
    stream_ << std::forward<T>(value);
    return *this;
  }

 private:
  raw_ptr<mojom::RewardsEngineClient> client_ = nullptr;
  base::Location location_;
  int log_level_ = 0;
  std::ostringstream stream_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_LOG_STREAM_H_
