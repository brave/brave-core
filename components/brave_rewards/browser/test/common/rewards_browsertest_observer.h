/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_OBSERVER_H_

#include <memory>

#include "base/run_loop.h"
#include "bat/ledger/mojom_structs.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"

namespace rewards_browsertest {

class RewardsBrowserTestObserver
    : public brave_rewards::RewardsServiceObserver {
 public:
  RewardsBrowserTestObserver();
  ~RewardsBrowserTestObserver() override;

  void Initialize(brave_rewards::RewardsServiceImpl* rewards_service);

  void WaitForWalletInitialization();

 private:
  void OnWalletInitialized(
      brave_rewards::RewardsService* rewards_service,
      int32_t result) override;

  std::unique_ptr<base::RunLoop> wait_for_wallet_initialization_loop_;
  bool wallet_initialized_ = false;

  brave_rewards::RewardsServiceImpl* rewards_service_;
};

}  // namespace rewards_browsertest
#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_OBSERVER_H_
