/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_PUBLISHER_PREFIX_LIST_UPDATER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_PUBLISHER_PREFIX_LIST_UPDATER_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/endpoint/rewards/rewards_server.h"
#include "brave/components/brave_rewards/core/rewards_engine_helper.h"

namespace brave_rewards::internal {

// Automatically updates the publisher prefix list store on regular intervals.
class PublisherPrefixListUpdater : public RewardsEngineHelper {
 public:
  ~PublisherPrefixListUpdater() override;

  // Starts the auto updater
  void StartAutoUpdate(base::RepeatingCallback<void()> callback);

  // Cancels the auto updater
  void StopAutoUpdate();

 private:
  friend RewardsEngineContext;

  inline static const char kUserDataKey[] = "publisher-prefix-list-updater";

  explicit PublisherPrefixListUpdater(RewardsEngineContext& context);

  void StartFetchTimer(const base::Location& posted_from,
                       base::TimeDelta delay);

  void OnFetchTimerElapsed();
  void OnFetchCompleted(mojom::Result result, const std::string& body);
  void OnPrefixListInserted(mojom::Result result);

  base::TimeDelta GetAutoUpdateDelay();
  base::TimeDelta GetRetryAfterFailureDelay();

  base::OneShotTimer timer_;
  bool auto_update_ = false;
  int retry_count_ = 0;
  base::RepeatingCallback<void()> on_updated_callback_;
  endpoint::RewardsServer rewards_server_;
  base::WeakPtrFactory<PublisherPrefixListUpdater> weak_factory_{this};
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_PUBLISHER_PREFIX_LIST_UPDATER_H_
