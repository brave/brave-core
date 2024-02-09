/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_PUBLISHER_PREFIX_LIST_UPDATER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_PUBLISHER_PREFIX_LIST_UPDATER_H_

#include <functional>
#include <map>
#include <string>

#include "base/memory/raw_ref.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_rewards/core/endpoint/rewards/rewards_server.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace publisher {

using PublisherPrefixListUpdatedCallback = std::function<void()>;

// Automatically updates the publisher prefix list store on regular
// intervals.
class PublisherPrefixListUpdater {
 public:
  explicit PublisherPrefixListUpdater(RewardsEngineImpl& engine);

  PublisherPrefixListUpdater(const PublisherPrefixListUpdater&) = delete;
  PublisherPrefixListUpdater& operator=(const PublisherPrefixListUpdater&) =
      delete;

  ~PublisherPrefixListUpdater();

  // Starts the auto updater
  void StartAutoUpdate(PublisherPrefixListUpdatedCallback callback);

  // Cancels the auto updater
  void StopAutoUpdate();

  static constexpr uint64_t kRefreshInterval =
#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS)
      7 * base::Time::kHoursPerDay * base::Time::kSecondsPerHour;
#else
      3 * base::Time::kHoursPerDay * base::Time::kSecondsPerHour;
#endif

 private:
  void StartFetchTimer(const base::Location& posted_from,
                       base::TimeDelta delay);

  void OnFetchTimerElapsed();
  void OnFetchCompleted(const mojom::Result result, const std::string& body);
  void OnPrefixListInserted(const mojom::Result result);

  base::TimeDelta GetAutoUpdateDelay();
  base::TimeDelta GetRetryAfterFailureDelay();

  const raw_ref<RewardsEngineImpl> engine_;
  base::OneShotTimer timer_;
  bool auto_update_ = false;
  int retry_count_ = 0;
  PublisherPrefixListUpdatedCallback on_updated_callback_;
  endpoint::RewardsServer rewards_server_;
};

}  // namespace publisher
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_PUBLISHER_PREFIX_LIST_UPDATER_H_
