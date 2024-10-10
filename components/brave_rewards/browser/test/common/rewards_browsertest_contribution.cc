/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_contribution.h"

#include <memory>
#include <utility>

#include "base/strings/stringprintf.h"
#include "base/test/bind.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_helper.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "brave/components/brave_rewards/common/buildflags/buildflags.h"
#include "brave/components/brave_rewards/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_rewards::test_util {

RewardsBrowserTestContribution::RewardsBrowserTestContribution() = default;

RewardsBrowserTestContribution::~RewardsBrowserTestContribution() = default;

void RewardsBrowserTestContribution::Initialize(
    Browser* browser,
    RewardsServiceImpl* rewards_service) {
  DCHECK(browser && rewards_service);
  browser_ = browser;
  rewards_service_ = rewards_service;
  context_helper_ =
      std::make_unique<test_util::RewardsBrowserTestContextHelper>(browser);

  rewards_service_->AddObserver(this);
}

content::WebContents* RewardsBrowserTestContribution::contents() {
  return browser_->tab_strip_model()->GetActiveWebContents();
}

void RewardsBrowserTestContribution::TipViaCode(
    const std::string& publisher_key,
    const double amount,
    const mojom::PublisherStatus status,
    const bool recurring) {
  multiple_tip_reconcile_completed_ = false;
  multiple_tip_reconcile_count_ = 0;

  auto publisher = mojom::PublisherInfo::New();
  publisher->id = publisher_key;
  publisher->name = publisher_key;
  publisher->url = publisher_key;
  publisher->status = status;

  rewards_service_->SavePublisherInfo(0, std::move(publisher),
                                      base::DoNothing());

  rewards_service_->SendContribution(publisher_key, amount, recurring,
                                     base::DoNothing());

  // Wait for reconciliation to complete
  WaitForMultipleTipReconcileCompleted(1);
}

void RewardsBrowserTestContribution::TipPublisher(
    const GURL& url,
    bool set_monthly,
    int32_t number_of_contributions,
    int32_t selection,
    double custom_amount) {
  bool should_contribute = number_of_contributions > 0;
  const std::string publisher = url.host();
  // we shouldn't be adding publisher to AC list,
  // so that we can focus only on tipping part
  rewards_service_->SetPublisherMinVisitTime(8);

  // Navigate to a site in a new tab
  ui_test_utils::NavigateToURLWithDisposition(
      browser_,
      url,
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // Ensure that the tip button is disabled for unverified publishers.
  if (!should_contribute) {
    base::WeakPtr<content::WebContents> popup_contents =
        context_helper_->OpenRewardsPopup();

    test_util::WaitForElementToAppear(popup_contents.get(),
                                      "[data-test-id=tip-button]");

    content::EvalJsResult js_result =
        EvalJs(popup_contents.get(),
               "document.querySelector('[data-test-id=tip-button]').disabled",
               content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
               content::ISOLATED_WORLD_ID_CONTENT_END);

    EXPECT_TRUE(js_result.ExtractBool());
    return;
  }

  base::WeakPtr<content::WebContents> site_banner_contents =
      context_helper_->OpenSiteBanner();
  ASSERT_TRUE(site_banner_contents);

  double amount = 0.0;

  if (custom_amount > 0) {
    amount = custom_amount;
    test_util::WaitForElementThenClick(site_banner_contents.get(),
                                       "[data-test-id=custom-tip-button]");

    test_util::WaitForElementToAppear(site_banner_contents.get(),
                                      "[data-test-id=custom-amount-input]");

    constexpr char set_input_script[] = R"(
        new Promise(resolve => {
          const input =
            document.querySelector('[data-test-id=custom-amount-input]');
          input[Symbol.for('updateCustomAmountForTesting')](`$1`);
          setTimeout(resolve, 30);
        })
    )";

    ASSERT_TRUE(ExecJs(site_banner_contents.get(),
                       content::JsReplace(set_input_script, amount)));
  } else {
    amount = test_util::GetSiteBannerTipOptions(
        site_banner_contents.get())[selection];

    // Select the tip amount (default is 1.000 BAT)
    std::string amount_selector = base::StringPrintf(
        "[data-test-id=tip-amount-options] [data-option-index='%u']",
        selection);

    test_util::WaitForElementThenClick(site_banner_contents.get(),
                                       amount_selector);
  }

  if (set_monthly) {
    test_util::WaitForElementThenClick(site_banner_contents.get(),
                                       "[data-test-id=monthly-toggle] button");
  }

  // Send the tip
  test_util::WaitForElementThenClick(site_banner_contents.get(),
                                     "[data-test-id=send-button]");

  // Wait for thank you banner to load
  ASSERT_TRUE(WaitForLoadStop(site_banner_contents.get()));

  // Make sure that thank you banner shows correct publisher data
  test_util::WaitForElementToContain(site_banner_contents.get(), "body",
                                     "Contribution sent");

  // Wait for reconciliation to complete
  WaitForMultipleTipReconcileCompleted(number_of_contributions);
  ASSERT_EQ(static_cast<int32_t>(multiple_tip_reconcile_status_.size()),
            number_of_contributions);
  for (const auto status : multiple_tip_reconcile_status_) {
    ASSERT_EQ(status, mojom::Result::OK);
  }

  if (set_monthly) {
    WaitForRecurringTipToBeSaved();
    // Trigger contribution process
    rewards_service_->StartContributionsForTesting();

    // Wait for reconciliation to complete
    WaitForTipReconcileCompleted();
    ASSERT_EQ(tip_reconcile_status_, mojom::Result::OK);
  }

  VerifyTip(amount, set_monthly);
}

void RewardsBrowserTestContribution::VerifyTip(const double amount,
                                               const bool monthly,
                                               const bool via_code) {
  if (via_code && monthly) {
    return;
  }

  // Load rewards page
  context_helper_->LoadRewardsPage();

  // Make sure that balance is updated correctly
  IsBalanceCorrect();

  // Check that tip table shows the appropriate tip amount
  const std::string selector = monthly
                                   ? "[data-test-id=rewards-summary-monthly]"
                                   : "[data-test-id=rewards-summary-one-time]";

  test_util::WaitForElementToContain(contents(), selector,
                                     base::StringPrintf("%.2f BAT", amount));
}

void RewardsBrowserTestContribution::IsBalanceCorrect() {
  test_util::WaitForElementToEqual(
      contents(), "[data-test-id=rewards-balance-text]", GetStringBalance());
}

void RewardsBrowserTestContribution::AddBalance(const double balance) {
  balance_ += balance;
}

double RewardsBrowserTestContribution::GetBalance() {
  return balance_;
}

std::string RewardsBrowserTestContribution::GetExternalBalance() {
  return test_util::BalanceDoubleToString(external_balance_);
}

void RewardsBrowserTestContribution::WaitForTipReconcileCompleted() {
  if (tip_reconcile_completed_) {
    return;
  }

  wait_for_tip_completed_loop_ = std::make_unique<base::RunLoop>();
  wait_for_tip_completed_loop_->Run();
}

void RewardsBrowserTestContribution::OnReconcileComplete(
    RewardsService* rewards_service,
    const mojom::Result result,
    const std::string& contribution_id,
    const double amount,
    const mojom::RewardsType type,
    const mojom::ContributionProcessor processor) {
  if (result == mojom::Result::OK) {
    UpdateContributionBalance(
        amount,
        true,
        processor);
  }

  if (type == mojom::RewardsType::AUTO_CONTRIBUTE) {
    ac_reconcile_completed_ = true;
    ac_reconcile_status_ = result;
    if (wait_for_ac_completed_loop_) {
      wait_for_ac_completed_loop_->Quit();
    }

    // Multiple ac
    multiple_ac_reconcile_count_++;
    multiple_ac_reconcile_status_.push_back(result);

    if (multiple_ac_reconcile_count_ == multiple_ac_reconcile_needed_) {
      multiple_ac_reconcile_completed_ = true;
      if (wait_for_multiple_ac_completed_loop_) {
        wait_for_multiple_ac_completed_loop_->Quit();
      }
    }
  }

  if (type == mojom::RewardsType::ONE_TIME_TIP ||
      type == mojom::RewardsType::RECURRING_TIP) {
    if (result == mojom::Result::OK) {
      reconciled_tip_total_ += amount;
    }

    // Single tip tracking
    tip_reconcile_completed_ = true;
    tip_reconcile_status_ = result;
    if (wait_for_tip_completed_loop_) {
      wait_for_tip_completed_loop_->Quit();
    }

    // Multiple tips
    multiple_tip_reconcile_count_++;
    multiple_tip_reconcile_status_.push_back(result);

    if (multiple_tip_reconcile_count_ == multiple_tip_reconcile_needed_) {
      multiple_tip_reconcile_completed_ = true;
      if (wait_for_multiple_tip_completed_loop_) {
        wait_for_multiple_tip_completed_loop_->Quit();
      }
    }
  }
}

void RewardsBrowserTestContribution::UpdateContributionBalance(
    const double amount,
    const bool verified,
    const mojom::ContributionProcessor processor) {
  if (verified) {
    if (processor == mojom::ContributionProcessor::BRAVE_TOKENS) {
      balance_ -= amount;
      return;
    }

    if (processor == mojom::ContributionProcessor::UPHOLD) {
      external_balance_ -= amount;
      return;
    }

    return;
  }
}

void RewardsBrowserTestContribution::WaitForRecurringTipToBeSaved() {
  if (recurring_tip_saved_) {
    return;
  }

  wait_for_recurring_tip_saved_loop_ = std::make_unique<base::RunLoop>();
  wait_for_recurring_tip_saved_loop_->Run();
}

void RewardsBrowserTestContribution::OnRecurringTipSaved(
    RewardsService* rewards_service,
    const bool success) {
  if (!success) {
    return;
  }

  recurring_tip_saved_ = true;
  if (wait_for_recurring_tip_saved_loop_) {
    wait_for_recurring_tip_saved_loop_->Quit();
  }
}

void RewardsBrowserTestContribution::WaitForMultipleTipReconcileCompleted(
    const int32_t needed) {
  multiple_tip_reconcile_needed_ = needed;
  if (multiple_tip_reconcile_completed_||
      multiple_tip_reconcile_count_ == needed) {
    return;
  }

  wait_for_multiple_tip_completed_loop_ = std::make_unique<base::RunLoop>();
  wait_for_multiple_tip_completed_loop_->Run();
}

void RewardsBrowserTestContribution::WaitForMultipleACReconcileCompleted(
    const int32_t needed) {
  multiple_ac_reconcile_needed_ = needed;
  if (multiple_ac_reconcile_completed_ ||
      multiple_ac_reconcile_count_ == needed) {
    return;
  }

  wait_for_multiple_ac_completed_loop_ = std::make_unique<base::RunLoop>();
  wait_for_multiple_ac_completed_loop_->Run();
}

void RewardsBrowserTestContribution::WaitForACReconcileCompleted() {
  if (ac_reconcile_completed_) {
    return;
  }

  wait_for_ac_completed_loop_ = std::make_unique<base::RunLoop>();
  wait_for_ac_completed_loop_->Run();
}

std::string RewardsBrowserTestContribution::GetStringBalance() {
  const std::string balance =
      test_util::BalanceDoubleToString(balance_ + external_balance_);
  return balance + " BAT";
}

mojom::Result RewardsBrowserTestContribution::GetACStatus() {
  return ac_reconcile_status_;
}

void RewardsBrowserTestContribution::StartProcessWithBalance(double balance) {
  external_balance_ = balance;

  test_util::StartProcessWithConnectedUser(browser_->profile());

  {
    // Verify that the balance is fetched correctly.
    double user_balance = 0;
    base::RunLoop run_loop;
    rewards_service_->FetchBalance(
        base::BindLambdaForTesting([&](mojom::BalancePtr balance) {
          user_balance = balance->total;
          run_loop.Quit();
        }));
    run_loop.Run();
    ASSERT_EQ(user_balance, external_balance_);
  }
}

double RewardsBrowserTestContribution::GetReconcileTipTotal() {
  return reconciled_tip_total_;
}

std::vector<mojom::Result>
RewardsBrowserTestContribution::GetMultipleTipStatus() {
  return multiple_tip_reconcile_status_;
}

mojom::Result RewardsBrowserTestContribution::GetTipStatus() {
  return tip_reconcile_status_;
}

std::vector<mojom::Result>
RewardsBrowserTestContribution::GetMultipleACStatus() {
  return multiple_ac_reconcile_status_;
}

}  // namespace brave_rewards::test_util
