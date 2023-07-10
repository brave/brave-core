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
#include "brave/components/brave_rewards/core/rewards_engine_context.h"

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

class RewardsEngineHelper : public base::SupportsUserData::Data {
 protected:
  explicit RewardsEngineHelper(RewardsEngineContext& context);
  ~RewardsEngineHelper() override;

  RewardsEngineHelper(const RewardsEngineHelper&) = delete;
  RewardsEngineHelper& operator=(const RewardsEngineHelper&) = delete;

  RewardsEngineContext& context() { return context_.get(); }
  const RewardsEngineContext& context() const { return context_.get(); }

  mojom::RewardsEngineClient& client() const;

  RewardsLogStream Log(base::Location location);
  RewardsLogStream LogError(base::Location location);

  template <typename T>
  T& GetHelper() {
    return context().GetHelper<T>();
  }

 private:
  const raw_ref<RewardsEngineContext> context_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_HELPER_H_
