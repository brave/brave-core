/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_observer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace rewards_browsertest {

RewardsBrowserTestObserver::RewardsBrowserTestObserver() = default;

RewardsBrowserTestObserver::~RewardsBrowserTestObserver() = default;

void RewardsBrowserTestObserver::Initialize(
      brave_rewards::RewardsServiceImpl* rewards_service) {
  DCHECK(rewards_service);
  rewards_service_ = rewards_service;
  rewards_service_->AddObserver(this);
}

void RewardsBrowserTestObserver::WaitForWalletInitialization() {
  if (wallet_initialized_) {
    return;
  }

  wait_for_wallet_initialization_loop_.reset(new base::RunLoop);
  wait_for_wallet_initialization_loop_->Run();
}

void RewardsBrowserTestObserver::OnWalletInitialized(
    brave_rewards::RewardsService* rewards_service,
    int32_t result) {
  const auto converted_result = static_cast<ledger::Result>(result);
  ASSERT_TRUE(converted_result == ledger::Result::WALLET_CREATED ||
              converted_result == ledger::Result::LEDGER_OK);
  wallet_initialized_ = true;

  if (wait_for_wallet_initialization_loop_) {
    wait_for_wallet_initialization_loop_->Quit();
  }
}

}  // namespace rewards_browsertest
