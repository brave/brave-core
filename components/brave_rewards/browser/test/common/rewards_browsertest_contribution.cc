/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_helper.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_contribution.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "brave/components/brave_rewards/common/buildflags/buildflags.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
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
  context_helper_ = std::make_unique<RewardsBrowserTestContextHelper>(browser);

  rewards_service_->AddObserver(this);
}

content::WebContents* RewardsBrowserTestContribution::contents() {
  return browser_->tab_strip_model()->GetActiveWebContents();
}

void RewardsBrowserTestContribution::TipViaCode(
    const std::string& publisher_key,
    const double amount,
    const ledger::type::PublisherStatus status,
    const int32_t number_of_contributions,
    const bool recurring) {
  pending_tip_saved_ = false;
  multiple_tip_reconcile_completed_ = false;
  multiple_tip_reconcile_count_ = 0;

  bool should_contribute = number_of_contributions > 0;
  auto publisher = ledger::type::PublisherInfo::New();
  publisher->id = publisher_key;
  publisher->name = publisher_key;
  publisher->url = publisher_key;
  publisher->status = status;
  rewards_service_->OnTip(
      publisher_key,
      amount,
      recurring,
      std::move(publisher));

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
    rewards_browsertest_util::TipAction tip_action,
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

  base::WeakPtr<content::WebContents> site_banner_contents =
      context_helper_->OpenSiteBanner(tip_action);
  ASSERT_TRUE(site_banner_contents);

  double amount = 0.0;

  if (custom_amount > 0) {
    amount = custom_amount;
    rewards_browsertest_util::WaitForElementThenClick(
        site_banner_contents.get(), "[data-test-id=custom-tip-button]");

    rewards_browsertest_util::WaitForElementToAppear(
        site_banner_contents.get(), "[data-test-id=custom-amount-input]");

    constexpr char set_input_script[] = R"(
        const input = document.querySelector(
          '[data-test-id=custom-amount-input]');
        input.value = $1;
        input.blur();
    )";

    ASSERT_TRUE(ExecJs(site_banner_contents.get(),
                       content::JsReplace(set_input_script, amount),
                       content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                       content::ISOLATED_WORLD_ID_CONTENT_END));

    rewards_browsertest_util::WaitForElementThenClick(
        site_banner_contents.get(), "[data-test-id=form-submit-button]");
  } else {
    amount = rewards_browsertest_util::GetSiteBannerTipOptions(
        site_banner_contents.get())[selection];

    // Select the tip amount (default is 1.000 BAT)
    std::string amount_selector = base::StringPrintf(
        "[data-test-id=tip-amount-options] [data-option-index='%u'] button",
        selection);

    rewards_browsertest_util::WaitForElementThenClick(
        site_banner_contents.get(), amount_selector);
  }

  // Send the tip
  rewards_browsertest_util::WaitForElementThenClick(
      site_banner_contents.get(), "[data-test-id=form-submit-button]");

  // Signal that direct tip was made and update wallet with new
  // balance
  if (tip_action == rewards_browsertest_util::TipAction::OneTime &&
      !should_contribute) {
    WaitForPendingTipToBeSaved();
    UpdateContributionBalance(amount, should_contribute);
  }

  // Wait for thank you banner to load
  ASSERT_TRUE(WaitForLoadStop(site_banner_contents.get()));

  if (tip_action == rewards_browsertest_util::TipAction::SetMonthly) {
    WaitForRecurringTipToBeSaved();
    // Trigger contribution process
    rewards_service_->StartMonthlyContributionForTest();

    // Wait for reconciliation to complete
    if (should_contribute) {
      WaitForTipReconcileCompleted();
      const auto result = should_contribute
          ? ledger::type::Result::LEDGER_OK
          : ledger::type::Result::RECURRING_TABLE_EMPTY;
      ASSERT_EQ(tip_reconcile_status_, result);
    }

    // Signal that monthly contribution was made and update wallet
    // with new balance
    if (!should_contribute) {
      UpdateContributionBalance(amount, should_contribute);
    }
  } else if (tip_action == rewards_browsertest_util::TipAction::OneTime &&
      should_contribute) {
    // Wait for reconciliation to complete
    WaitForMultipleTipReconcileCompleted(number_of_contributions);
    ASSERT_EQ(
        static_cast<int32_t>(multiple_tip_reconcile_status_.size()),
        number_of_contributions);
    for (const auto status : multiple_tip_reconcile_status_) {
      ASSERT_EQ(status, ledger::type::Result::LEDGER_OK);
    }
  }

  // Make sure that thank you banner shows correct publisher data
  {
    rewards_browsertest_util::WaitForElementToContain(
        site_banner_contents.get(), "body", "Thanks for the support!");
    rewards_browsertest_util::WaitForElementToContain(
        site_banner_contents.get(), "body",
        base::StringPrintf("%.3f BAT", amount));
  }

  const bool is_monthly =
      tip_action == rewards_browsertest_util::TipAction::SetMonthly;

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

  // Load rewards page
  context_helper_->LoadRewardsPage();

  if (should_contribute) {
    // Make sure that balance is updated correctly
    IsBalanceCorrect();

    // Check that tip table shows the appropriate tip amount
    const std::string selector =
        monthly ? "[data-test-id=rewards-summary-monthly]"
                : "[data-test-id=rewards-summary-one-time]";

    rewards_browsertest_util::WaitForElementToContain(
        contents(), selector, base::StringPrintf("-%.2f BAT", amount));
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
      contents(), "[data-test-id=rewards-balance-text]", GetStringBalance());
}

void RewardsBrowserTestContribution::IsPendingBalanceCorrect() {
  rewards_browsertest_util::WaitForElementToContain(
      contents(), "[data-test-id=rewards-summary-pending]",
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
    const ledger::type::Result result) {
  if (result != ledger::type::Result::LEDGER_OK) {
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
    const ledger::type::Result result,
    const std::string& contribution_id,
    const double amount,
    const ledger::type::RewardsType type,
    const ledger::type::ContributionProcessor processor) {
  if (result == ledger::type::Result::LEDGER_OK) {
    UpdateContributionBalance(
        amount,
        true,
        processor);
  }

  if (type == ledger::type::RewardsType::AUTO_CONTRIBUTE) {
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

  if (type == ledger::type::RewardsType::ONE_TIME_TIP ||
      type == ledger::type::RewardsType::RECURRING_TIP) {
    if (result == ledger::type::Result::LEDGER_OK) {
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
    const ledger::type::ContributionProcessor processor) {
  if (verified) {
    if (processor == ledger::type::ContributionProcessor::BRAVE_TOKENS) {
      balance_ -= amount;
      return;
    }

    if (processor == ledger::type::ContributionProcessor::UPHOLD) {
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
  return base::StringPrintf("%.2f BAT", pending_balance_);
}

ledger::type::Result RewardsBrowserTestContribution::GetACStatus() {
  return ac_reconcile_status_;
}

#if BUILDFLAG(ENABLE_GEMINI_WALLET)
void RewardsBrowserTestContribution::SetUpGeminiWallet(
    brave_rewards::RewardsServiceImpl* rewards_service,
    const double balance,
    const ledger::type::WalletStatus status) {
  if (!base::FeatureList::IsEnabled(brave_rewards::features::kGeminiFeature)) {
    return;
  }
  DCHECK(rewards_service);
  browser_->profile()->GetPrefs()->SetString(
      brave_rewards::prefs::kExternalWalletType, "gemini");
  // we need brave wallet as well
  rewards_browsertest_util::CreateWallet(rewards_service_);

  external_balance_ = balance;

  base::Value wallet(base::Value::Type::DICTIONARY);
  wallet.SetStringKey("token", "token");
  wallet.SetStringKey("address",
                      rewards_browsertest_util::GetGeminiExternalAddress());
  wallet.SetIntKey("status", static_cast<int>(status));
  wallet.SetStringKey("user_name", "Brave Test");

  std::string json;
  base::JSONWriter::Write(wallet, &json);
  auto encrypted =
      rewards_browsertest_util::EncryptPrefString(rewards_service_, json);
  ASSERT_TRUE(encrypted);
  browser_->profile()->GetPrefs()->SetString(
      brave_rewards::prefs::kWalletGemini, *encrypted);
}
#endif

void RewardsBrowserTestContribution::SetUpUpholdWallet(
    brave_rewards::RewardsServiceImpl* rewards_service,
    const double balance,
    const ledger::type::WalletStatus status) {
  DCHECK(rewards_service);
#if BUILDFLAG(ENABLE_GEMINI_WALLET)
  if (base::FeatureList::IsEnabled(brave_rewards::features::kGeminiFeature)) {
    browser_->profile()->GetPrefs()->SetString(
        brave_rewards::prefs::kExternalWalletType, "uphold");
  }
#endif
  // we need brave wallet as well
  rewards_browsertest_util::CreateWallet(rewards_service_);

  external_balance_ = balance;

  base::Value wallet(base::Value::Type::DICTIONARY);
  wallet.SetStringKey("token", "token");
  wallet.SetStringKey(
      "address",
      rewards_browsertest_util::GetUpholdExternalAddress());
  wallet.SetIntKey("status", static_cast<int>(status));
  wallet.SetStringKey("user_name", "Brave Test");

  std::string json;
  base::JSONWriter::Write(wallet, &json);
  auto encrypted =
      rewards_browsertest_util::EncryptPrefString(rewards_service_, json);
  ASSERT_TRUE(encrypted);

  browser_->profile()->GetPrefs()->SetString(
      brave_rewards::prefs::kWalletUphold, *encrypted);
}

double RewardsBrowserTestContribution::GetReconcileTipTotal() {
  return reconciled_tip_total_;
}

std::vector<ledger::type::Result>
RewardsBrowserTestContribution::GetMultipleTipStatus() {
  return multiple_tip_reconcile_status_;
}

ledger::type::Result RewardsBrowserTestContribution::GetTipStatus() {
  return tip_reconcile_status_;
}

std::vector<ledger::type::Result>
RewardsBrowserTestContribution::GetMultipleACStatus() {
  return multiple_ac_reconcile_status_;
}

}  // namespace rewards_browsertest
