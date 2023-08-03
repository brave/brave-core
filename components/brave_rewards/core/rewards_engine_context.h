/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_CONTEXT_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_CONTEXT_H_

#include <tuple>
#include <utility>

#include "base/memory/raw_ref.h"

#include "brave/components/brave_rewards/core/publisher/publisher_prefix_list_updater.h"
#include "brave/components/brave_rewards/core/publisher/publisher_status_helper.h"
#include "brave/components/brave_rewards/core/publisher/server_publisher_fetcher.h"

namespace brave_rewards::internal {

class RewardsEngineImpl;

class RewardsEngineContext {
 public:
  explicit RewardsEngineContext(RewardsEngineImpl& engine_impl);
  ~RewardsEngineContext();

  RewardsEngineContext(const RewardsEngineContext&) = delete;
  RewardsEngineContext& operator=(const RewardsEngineContext&) = delete;

  RewardsEngineImpl& GetEngineImpl() const { return engine_impl_.get(); }

  template <typename T>
  T& GetHelper() {
    return std::get<T>(helpers_);
  }

 private:
  const raw_ref<RewardsEngineImpl> engine_impl_;
  std::tuple<PublisherPrefixListUpdater,
             PublisherStatusHelper,
             ServerPublisherFetcher>
      helpers_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_CONTEXT_H_
