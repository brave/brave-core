/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_HELPER_H_

#include <sstream>
#include <utility>

#include "base/location.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/common/mojom/rewards_engine.mojom.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal {

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

// Base class for Rewards engine helpers. Provides convenient accessors and
// utility methods.
class RewardsEngineHelper {
 protected:
  explicit RewardsEngineHelper(RewardsEngineImpl& engine);
  virtual ~RewardsEngineHelper();

  RewardsEngineHelper(const RewardsEngineHelper&) = delete;
  RewardsEngineHelper& operator=(const RewardsEngineHelper&) = delete;

  RewardsEngineImpl& engine() { return engine_.get(); }
  const RewardsEngineImpl& engine() const { return engine_.get(); }

  mojom::RewardsEngineClient& client();

  // Performs logging to the Rewards logging file as implemented by the client.
  //
  //   Log(FROM_HERE) << "This will appear in the log file when verbose logging"
  //                     "is enabled.";
  //
  //   LogError(FROM_HERE) << "This will always appear in the log file."
  //                          "Do not use with arbitrary strings or data!";
  //
  // NOTE: Do not use arbitrary strings when using `LogError`, as this can
  // result in sensitive data being written to the Rewards log file.
  RewardsLogStream Log(base::Location location);
  RewardsLogStream LogError(base::Location location);

 private:
  const raw_ref<RewardsEngineImpl> engine_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_HELPER_H_
