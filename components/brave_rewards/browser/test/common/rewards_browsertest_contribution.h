/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_CONTRIBUTION_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_CONTRIBUTION_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_helper.h"
#include "brave/components/brave_rewards/common/buildflags/buildflags.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"

class browser;

namespace brave_rewards::test_util {

class RewardsBrowserTestContribution : public RewardsServiceObserver {
 public:
  RewardsBrowserTestContribution();
  ~RewardsBrowserTestContribution() override;

  void Initialize(Browser* browser, RewardsServiceImpl* rewards_service);

  void TipViaCode(const std::string& publisher_key,
                  const double amount,
                  const mojom::PublisherStatus status,
                  const bool recurring = false);

  void TipPublisher(const GURL& url,
                    bool set_monthly,
                    const int32_t number_of_contributions = 0,
                    const int32_t selection = 0,
                    double custom_amount = 0.0);

  void VerifyTip(const double amount,
                 const bool monthly,
                 const bool via_code = false);

  void AddBalance(const double balance);

  double GetBalance();

  std::string GetExternalBalance();

  double GetReconcileTipTotal();

  void WaitForTipReconcileCompleted();

  void UpdateContributionBalance(
      const double amount,
      const bool verified = false,
      const mojom::ContributionProcessor processor =
          mojom::ContributionProcessor::BRAVE_TOKENS);

  void WaitForMultipleTipReconcileCompleted(const int32_t needed);

  void WaitForACReconcileCompleted();

  void IsBalanceCorrect();

  void WaitForMultipleACReconcileCompleted(
    const int32_t needed);

  mojom::Result GetACStatus();

  std::vector<mojom::Result> GetMultipleACStatus();

  void StartProcessWithBalance(double balance);

  std::vector<mojom::Result> GetMultipleTipStatus();

  mojom::Result GetTipStatus();

 private:
  content::WebContents* contents();

  void OnReconcileComplete(
      RewardsService* rewards_service,
      const mojom::Result result,
      const std::string& contribution_id,
      const double amount,
      const mojom::RewardsType type,
      const mojom::ContributionProcessor processor) override;

  void WaitForRecurringTipToBeSaved();

  void OnRecurringTipSaved(RewardsService* rewards_service,
                           const bool success) override;

  std::string GetStringBalance();

  double balance_ = 0;
  double external_balance_ = 0;
  double reconciled_tip_total_ = 0;

  bool tip_reconcile_completed_ = false;
  std::unique_ptr<base::RunLoop> wait_for_tip_completed_loop_;
  mojom::Result tip_reconcile_status_ = mojom::Result::FAILED;
  bool recurring_tip_saved_ = false;
  std::unique_ptr<base::RunLoop> wait_for_recurring_tip_saved_loop_;
  bool multiple_tip_reconcile_completed_ = false;
  std::unique_ptr<base::RunLoop> wait_for_multiple_tip_completed_loop_;
  int32_t multiple_tip_reconcile_count_ = 0;
  int32_t multiple_tip_reconcile_needed_ = 0;
  std::vector<mojom::Result> multiple_tip_reconcile_status_ = {};
  bool multiple_ac_reconcile_completed_ = false;
  std::unique_ptr<base::RunLoop> wait_for_multiple_ac_completed_loop_;
  int32_t multiple_ac_reconcile_count_ = 0;
  int32_t multiple_ac_reconcile_needed_ = 0;
  std::vector<mojom::Result> multiple_ac_reconcile_status_ = {};
  bool ac_reconcile_completed_ = false;
  std::unique_ptr<base::RunLoop> wait_for_ac_completed_loop_;
  mojom::Result ac_reconcile_status_ = mojom::Result::FAILED;

  raw_ptr<Browser> browser_ = nullptr;  // NOT OWNED
  raw_ptr<RewardsServiceImpl> rewards_service_ = nullptr;  // NOT OWNED
  std::unique_ptr<test_util::RewardsBrowserTestContextHelper> context_helper_;
};

}  // namespace brave_rewards::test_util

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_CONTRIBUTION_H_
