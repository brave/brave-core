/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_PROMOTION_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_PROMOTION_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "bat/ledger/mojom_structs.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "chrome/browser/ui/browser.h"

namespace rewards_browsertest {

class RewardsBrowserTestPromotion
    : public brave_rewards::RewardsServiceObserver {
 public:
  RewardsBrowserTestPromotion();
  ~RewardsBrowserTestPromotion() override;

  void Initialize(
      Browser* browser,
      brave_rewards::RewardsServiceImpl* rewards_service);

  void WaitForPromotionInitialization();

  void WaitForPromotionFinished(const bool should_succeed = true);

  void WaitForUnblindedTokensReady();

  ledger::type::PromotionPtr GetPromotion();

  std::string GetPromotionId();

  double ClaimPromotionViaCode();

 private:
  void OnFetchPromotions(
      brave_rewards::RewardsService* rewards_service,
      const ledger::type::Result result,
      const ledger::type::PromotionList& list) override;

  void OnPromotionFinished(
      brave_rewards::RewardsService* rewards_service,
      const ledger::type::Result result,
      ledger::type::PromotionPtr promotion) override;

  void OnUnblindedTokensReady(
      brave_rewards::RewardsService* rewards_service) override;

  std::unique_ptr<base::RunLoop> wait_for_initialization_loop_;
  bool initialized_ = false;
  std::unique_ptr<base::RunLoop> wait_for_finished_loop_;
  bool finished_ = false;
  std::unique_ptr<base::RunLoop> wait_for_unblinded_tokens_loop_;
  bool unblinded_tokens_ = false;
  bool should_succeed_ = true;

  ledger::type::PromotionPtr promotion_;
  raw_ptr<Browser> browser_ = nullptr;  // NOT OWNED
  raw_ptr<brave_rewards::RewardsServiceImpl> rewards_service_ =
      nullptr;  // NOT OWNED
};

}  // namespace rewards_browsertest
#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_PROMOTION_H_
