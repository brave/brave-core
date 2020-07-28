/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_helper.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_contribution.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace rewards_browsertest {

RewardsBrowserTestContribution::RewardsBrowserTestContribution() = default;

RewardsBrowserTestContribution::~RewardsBrowserTestContribution() = default;

void RewardsBrowserTestContribution::Initialize(
      Browser* browser,
      brave_rewards::RewardsServiceImpl* rewards_service) {
  DCHECK(browser && rewards_service);
  browser_ = browser;
  rewards_service_ = rewards_service;

  rewards_service_->AddObserver(this);
}

content::WebContents* RewardsBrowserTestContribution::contents() {
  return browser_->tab_strip_model()->GetActiveWebContents();
}

void RewardsBrowserTestContribution::TipViaCode(
    const std::string& publisher_key,
    const double amount,
    const ledger::PublisherStatus status,
    const int32_t number_of_contributions,
    const bool recurring) {
  pending_tip_saved_ = false;
  multiple_tip_reconcile_completed_ = false;
  multiple_tip_reconcile_count_ = 0;

  bool should_contribute = number_of_contributions > 0;
  auto site = std::make_unique<brave_rewards::ContentSite>();
  site->id = publisher_key;
  site->name = publisher_key;
  site->url = publisher_key;
  site->status = static_cast<int>(status);
  site->provider = "";
  site->favicon_url = "";
  rewards_service_->OnTip(publisher_key, amount, recurring, std::move(site));

  if (recurring) {
    return;
  }

  if (should_contribute) {
    // Wait for reconciliation to complete
    WaitForMultipleTipReconcileCompleted(number_of_contributions);
    return;
  }

  // Signal to update pending contribution balance
  WaitForPendingTipToBeSaved();
  UpdateContributionBalance(amount, should_contribute);
}

void RewardsBrowserTestContribution::TipPublisher(
    const GURL& url,
    rewards_browsertest_util::ContributionType type,
    int32_t number_of_contributions,
    int32_t selection) {
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

  content::WebContents* site_banner_contents =
      rewards_browsertest_helper::OpenSiteBanner(browser_, type);
  ASSERT_TRUE(site_banner_contents);

  auto tip_options = rewards_browsertest_util::GetSiteBannerTipOptions(
          site_banner_contents);
  const double amount = tip_options.at(selection);
  const std::string amount_str = base::StringPrintf("%.3f", amount);

  // Select the tip amount (default is 1.000 BAT)
  std::string amount_selector = base::StringPrintf(
      "div:nth-of-type(%u)>[data-test-id=amount-wrapper]",
      selection + 1);
  rewards_browsertest_util::WaitForElementThenClick(
      site_banner_contents,
      amount_selector);

  // Send the tip
  rewards_browsertest_util::WaitForElementThenClick(
      site_banner_contents,
      "[data-test-id='send-tip-button']");

  // Signal that direct tip was made and update wallet with new
  // balance
  if (type == rewards_browsertest_util::ContributionType::OneTimeTip &&
      !should_contribute) {
    WaitForPendingTipToBeSaved();
    UpdateContributionBalance(amount, should_contribute);
  }

  // Wait for thank you banner to load
  ASSERT_TRUE(WaitForLoadStop(site_banner_contents));

  const std::string confirmationText =
      type == rewards_browsertest_util::ContributionType::MonthlyTip
      ? "Monthly contribution has been set!"
      : "Tip sent!";

  if (type == rewards_browsertest_util::ContributionType::MonthlyTip) {
    WaitForRecurringTipToBeSaved();
    // Trigger contribution process
    rewards_service_->StartMonthlyContributionForTest();

    // Wait for reconciliation to complete
    if (should_contribute) {
      WaitForTipReconcileCompleted();
      const auto result = should_contribute
          ? ledger::Result::LEDGER_OK
          : ledger::Result::RECURRING_TABLE_EMPTY;
      ASSERT_EQ(tip_reconcile_status_, result);
    }

    // Signal that monthly contribution was made and update wallet
    // with new balance
    if (!should_contribute) {
      UpdateContributionBalance(amount, should_contribute);
    }
  } else if (type == rewards_browsertest_util::ContributionType::OneTimeTip &&
      should_contribute) {
    // Wait for reconciliation to complete
    WaitForMultipleTipReconcileCompleted(number_of_contributions);
    ASSERT_EQ(
        static_cast<int32_t>(multiple_tip_reconcile_status_.size()),
        number_of_contributions);
    for (const auto status : multiple_tip_reconcile_status_) {
      ASSERT_EQ(status, ledger::Result::LEDGER_OK);
    }
  }

  // Make sure that thank you banner shows correct publisher data
  // (domain and amount)
  {
    rewards_browsertest_util::WaitForElementToContain(
        site_banner_contents,
        "body",
        confirmationText);
    rewards_browsertest_util::WaitForElementToContain(
        site_banner_contents,
        "body",
        amount_str + " BAT");
    rewards_browsertest_util::WaitForElementToContain(
        site_banner_contents,
        "body",
        "Share the good news:");
    rewards_browsertest_util::WaitForElementToContain(
        site_banner_contents,
        "body",
        GetStringBalance());
  }

  const bool is_monthly =
      type == rewards_browsertest_util::ContributionType::MonthlyTip;

  VerifyTip(amount, should_contribute, is_monthly);
}

void RewardsBrowserTestContribution::VerifyTip(
    const double amount,
    const bool should_contribute,
    const bool monthly,
    const bool via_code) {
  if (via_code && monthly) {
    return;
  }

  // Activate the Rewards settings page tab
  rewards_browsertest_util::ActivateTabAtIndex(browser_, 0);

  if (should_contribute) {
    // Make sure that balance is updated correctly
    IsBalanceCorrect();

    // Check that tip table shows the appropriate tip amount
    const std::string selector = monthly
        ? "[data-test-id='summary-monthly']"
        : "[data-test-id='summary-tips']";

    rewards_browsertest_util::WaitForElementToContain(
        contents(),
        selector,
        "-" + rewards_browsertest_util::BalanceDoubleToString(amount) + "BAT");
    return;
  }

  // Make sure that balance did not change
  IsBalanceCorrect();

  // Make sure that pending contribution box shows the correct
  // amount
  IsPendingBalanceCorrect();

  rewards_browsertest_util::WaitForElementToEqual(
      contents(),
      "#tip-box-total",
      "0.000BAT0.00 USD");
}

void RewardsBrowserTestContribution::IsBalanceCorrect() {
  rewards_browsertest_util::WaitForElementToEqual(
      contents(),
      "[data-test-id='balance']",
      GetStringBalance());
}

void RewardsBrowserTestContribution::IsPendingBalanceCorrect() {
  rewards_browsertest_util::WaitForElementToContain(
      contents(),
      "[data-test-id='pending-contribution-box']",
      GetStringPendingBalance());
}

void RewardsBrowserTestContribution::AddBalance(const double balance) {
  balance_ += balance;
}

double RewardsBrowserTestContribution::GetBalance() {
  return balance_;
}

std::string RewardsBrowserTestContribution::GetExternalBalance() {
  return rewards_browsertest_util::BalanceDoubleToString(external_balance_);
}

void RewardsBrowserTestContribution::WaitForPendingTipToBeSaved() {
  if (pending_tip_saved_) {
    return;
  }

  wait_for_pending_tip_saved_loop_.reset(new base::RunLoop);
  wait_for_pending_tip_saved_loop_->Run();
}

void RewardsBrowserTestContribution::OnPendingContributionSaved(
    brave_rewards::RewardsService* rewards_service,
    int result) {
  if (result != 0) {
    return;
  }

  pending_tip_saved_ = true;
  if (wait_for_pending_tip_saved_loop_) {
    wait_for_pending_tip_saved_loop_->Quit();
  }
}

void RewardsBrowserTestContribution::WaitForTipReconcileCompleted() {
  if (tip_reconcile_completed_) {
    return;
  }

  wait_for_tip_completed_loop_.reset(new base::RunLoop);
  wait_for_tip_completed_loop_->Run();
}

void RewardsBrowserTestContribution::OnReconcileComplete(
    brave_rewards::RewardsService* rewards_service,
    unsigned int result,
    const std::string& contribution_id,
    const double amount,
    const int32_t type,
    const int32_t processor) {
  const auto converted_result = static_cast<ledger::Result>(result);
  const auto converted_type = static_cast<ledger::RewardsType>(type);

  if (converted_result == ledger::Result::LEDGER_OK) {
    UpdateContributionBalance(
        amount,
        true,
        static_cast<ledger::ContributionProcessor>(processor));
  }

  if (converted_type == ledger::RewardsType::AUTO_CONTRIBUTE) {
    ac_reconcile_completed_ = true;
    ac_reconcile_status_ = converted_result;
    if (wait_for_ac_completed_loop_) {
      wait_for_ac_completed_loop_->Quit();
    }

    // Multiple ac
    multiple_ac_reconcile_count_++;
    multiple_ac_reconcile_status_.push_back(converted_result);

    if (multiple_ac_reconcile_count_ == multiple_ac_reconcile_needed_) {
      multiple_ac_reconcile_completed_ = true;
      if (wait_for_multiple_ac_completed_loop_) {
        wait_for_multiple_ac_completed_loop_->Quit();
      }
    }
  }

  if (converted_type == ledger::RewardsType::ONE_TIME_TIP ||
      converted_type == ledger::RewardsType::RECURRING_TIP) {
    if (converted_result == ledger::Result::LEDGER_OK) {
      reconciled_tip_total_ += amount;
    }

    // Single tip tracking
    tip_reconcile_completed_ = true;
    tip_reconcile_status_ = converted_result;
    if (wait_for_tip_completed_loop_) {
      wait_for_tip_completed_loop_->Quit();
    }

    // Multiple tips
    multiple_tip_reconcile_count_++;
    multiple_tip_reconcile_status_.push_back(converted_result);

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
    const ledger::ContributionProcessor processor) {
  if (verified) {
    if (processor == ledger::ContributionProcessor::BRAVE_TOKENS ||
        processor == ledger::ContributionProcessor::BRAVE_USER_FUNDS) {
      balance_ -= amount;
      return;
    }

    if (processor == ledger::ContributionProcessor::UPHOLD) {
      external_balance_ -= amount;
      return;
    }

    return;
  }

  pending_balance_ += amount;
}

void RewardsBrowserTestContribution::WaitForRecurringTipToBeSaved() {
  if (recurring_tip_saved_) {
    return;
  }

  wait_for_recurring_tip_saved_loop_.reset(new base::RunLoop);
  wait_for_recurring_tip_saved_loop_->Run();
}

void RewardsBrowserTestContribution::OnRecurringTipSaved(
    brave_rewards::RewardsService* rewards_service,
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

  wait_for_multiple_tip_completed_loop_.reset(new base::RunLoop);
  wait_for_multiple_tip_completed_loop_->Run();
}

void RewardsBrowserTestContribution::WaitForMultipleACReconcileCompleted(
    const int32_t needed) {
  multiple_ac_reconcile_needed_ = needed;
  if (multiple_ac_reconcile_completed_ ||
      multiple_ac_reconcile_count_ == needed) {
    return;
  }

  wait_for_multiple_ac_completed_loop_.reset(new base::RunLoop);
  wait_for_multiple_ac_completed_loop_->Run();
}

void RewardsBrowserTestContribution::WaitForACReconcileCompleted() {
  if (ac_reconcile_completed_) {
    return;
  }

  wait_for_ac_completed_loop_.reset(new base::RunLoop);
  wait_for_ac_completed_loop_->Run();
}

std::string RewardsBrowserTestContribution::GetStringBalance() {
  const std::string balance = rewards_browsertest_util::BalanceDoubleToString(
      balance_ + external_balance_);
  return balance + " BAT";
}

std::string RewardsBrowserTestContribution::GetStringPendingBalance() {
  const std::string balance =
      rewards_browsertest_util::BalanceDoubleToString(pending_balance_);
  return balance + " BAT";
}

ledger::Result RewardsBrowserTestContribution::GetACStatus() {
  return ac_reconcile_status_;
}

void RewardsBrowserTestContribution::SetUpUpholdWallet(
    const double balance,
    const ledger::WalletStatus status) {
  external_balance_ = balance;
  auto wallet = ledger::ExternalWallet::New();
  wallet->token = "token";
  wallet->address = rewards_browsertest_util::GetUpholdExternalAddress();
  wallet->status = status;
  wallet->one_time_string = "";
  wallet->user_name = "Brave Test";
  rewards_service_->SaveExternalWallet("uphold", std::move(wallet));
}

double RewardsBrowserTestContribution::GetReconcileTipTotal() {
  return reconciled_tip_total_;
}

std::vector<ledger::Result>
RewardsBrowserTestContribution::GetMultipleTipStatus() {
  return multiple_tip_reconcile_status_;
}

ledger::Result RewardsBrowserTestContribution::GetTipStatus() {
  return tip_reconcile_status_;
}

std::vector<ledger::Result>
RewardsBrowserTestContribution::GetMultipleACStatus() {
  return multiple_ac_reconcile_status_;
}

}  // namespace rewards_browsertest
