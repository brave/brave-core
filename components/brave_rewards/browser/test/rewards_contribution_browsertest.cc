/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/test/bind_test_util.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_helper.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_contribution.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_network_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_promotion.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_response.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

// npm run test -- brave_browser_tests --filter=RewardsContributionBrowserTest.*

namespace rewards_browsertest {

class RewardsContributionBrowserTest : public InProcessBrowserTest {
 public:
  RewardsContributionBrowserTest() {
    contribution_ = std::make_unique<RewardsBrowserTestContribution>();
    promotion_ = std::make_unique<RewardsBrowserTestPromotion>();
    response_ = std::make_unique<RewardsBrowserTestResponse>();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    // HTTP resolver
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->RegisterRequestHandler(
        base::BindRepeating(&rewards_browsertest_util::HandleRequest));
    ASSERT_TRUE(https_server_->Start());

    // Rewards service
    brave::RegisterPathProvider();
    auto* profile = browser()->profile();
    rewards_service_ = static_cast<brave_rewards::RewardsServiceImpl*>(
        brave_rewards::RewardsServiceFactory::GetForProfile(profile));

    // Response mock
    base::ScopedAllowBlockingForTesting allow_blocking;
    response_->LoadMocks();
    rewards_service_->ForTestingSetTestResponseCallback(
        base::BindRepeating(
            &RewardsContributionBrowserTest::GetTestResponse,
            base::Unretained(this)));
    rewards_service_->SetLedgerEnvForTesting();

    // Other
    promotion_->Initialize(browser(), rewards_service_);
    contribution_->Initialize(browser(), rewards_service_);
  }

  void TearDown() override {
    InProcessBrowserTest::TearDown();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // HTTPS server only serves a valid cert for localhost, so this is needed
    // to load pages from other hosts without an error
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  void GetTestResponse(
      const std::string& url,
      int32_t method,
      int* response_status_code,
      std::string* response,
      std::map<std::string, std::string>* headers) {
    response_->SetExternalBalance(contribution_->GetExternalBalance());
    response_->Get(
        url,
        method,
        response_status_code,
        response);
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  std::string ExpectedTipSummaryAmountString() {
    // The tip summary page formats 2.4999 as 2.4, so we do the same here.
    double truncated_amount =
        floor(contribution_->GetReconcileTipTotal() * 10) / 10;
    return rewards_browsertest_util::BalanceDoubleToString(-truncated_amount);
  }

  void RefreshPublisherListUsingRewardsPopup() {
    rewards_browsertest_util::WaitForElementThenClick(
        rewards_browsertest_helper::OpenRewardsPopup(browser()),
        "[data-test-id='unverified-check-button']");
  }

  brave_rewards::RewardsServiceImpl* rewards_service_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<RewardsBrowserTestContribution> contribution_;
  std::unique_ptr<RewardsBrowserTestPromotion> promotion_;
  std::unique_ptr<RewardsBrowserTestResponse> response_;
};

IN_PROC_BROWSER_TEST_F(RewardsContributionBrowserTest, AutoContribution) {
  rewards_browsertest_helper::EnableRewards(browser());
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  rewards_browsertest_helper::VisitPublisher(
      browser(),
      rewards_browsertest_util::GetUrl(https_server_.get(), "duckduckgo.com"),
      true);

  rewards_service_->StartMonthlyContributionForTest();

  contribution_->WaitForACReconcileCompleted();
  ASSERT_EQ(contribution_->GetACStatus(), ledger::Result::LEDGER_OK);

  contribution_->IsBalanceCorrect();

  rewards_browsertest_util::WaitForElementToContain(
      contents(),
      "[color=contribute]",
      "-20.000BAT");
}

IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    AutoContributionMultiplePublishers) {
  rewards_browsertest_helper::EnableRewards(browser());
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  rewards_browsertest_helper::VisitPublisher(
      browser(),
      rewards_browsertest_util::GetUrl(https_server_.get(), "duckduckgo.com"),
      true);
  rewards_browsertest_helper::VisitPublisher(
      browser(),
      rewards_browsertest_util::GetUrl(
          https_server_.get(),
          "laurenwags.github.io"),
      true);
  rewards_browsertest_helper::VisitPublisher(
      browser(),
      rewards_browsertest_util::GetUrl(https_server_.get(), "site1.com"),
      true);
  rewards_browsertest_helper::VisitPublisher(
      browser(),
      rewards_browsertest_util::GetUrl(https_server_.get(), "site2.com"),
      true);
  rewards_browsertest_helper::VisitPublisher(
      browser(),
      rewards_browsertest_util::GetUrl(https_server_.get(), "site3.com"),
      true);
  rewards_browsertest_helper::VisitPublisher(
      browser(),
      rewards_browsertest_util::GetUrl(https_server_.get(), "3zsistemi.si"),
      true);

  rewards_service_->StartMonthlyContributionForTest();

  contribution_->WaitForACReconcileCompleted();
  ASSERT_EQ(contribution_->GetACStatus(), ledger::Result::LEDGER_OK);

  contribution_->IsBalanceCorrect();

  rewards_browsertest_util::WaitForElementToContain(
      contents(),
      "[color=contribute]",
      "-20.000BAT");

  rewards_browsertest_helper::LoadURL(
      browser(),
      rewards_browsertest_util::GetRewardsInternalsUrl());

  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "#internals-tabs > div > div:nth-of-type(4)");

  for (int i = 1; i <= 6; i++) {
    const std::string query = base::StringPrintf(
        "[data-test-id='publisher-wrapper'] > div:nth-of-type(%d) "
        "[data-test-id='contributed-amount']",
        i);
    LOG(ERROR) << query;
    EXPECT_NE(
      rewards_browsertest_util::WaitForElementThenGetContent(contents(), query),
      "0 BAT");
  }
}

IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    AutoContributionMultiplePublishersUphold) {
  response_->SetVerifiedWallet(true);
  contribution_->SetUpUpholdWallet(50.0);
  rewards_browsertest_helper::EnableRewards(browser());

  ledger::SKUOrderItemList items;
  auto item = ledger::SKUOrderItem::New();
  item->order_item_id = "ed193339-e58c-483c-8d61-7decd3c24827";
  item->order_id = "a38b211b-bf78-42c8-9479-b11e92e3a76c";
  item->quantity = 80;
  item->price = 0.25;
  item->description = "description";
  item->type = ledger::SKUOrderItemType::SINGLE_USE;
  items.push_back(std::move(item));

  auto order = ledger::SKUOrder::New();
  order->order_id = "a38b211b-bf78-42c8-9479-b11e92e3a76c";
  order->total_amount = 20;
  order->merchant_id = "";
  order->location = "brave.com";
  order->items = std::move(items);
  response_->SetSKUOrder(std::move(order));

  rewards_browsertest_helper::VisitPublisher(
      browser(),
      rewards_browsertest_util::GetUrl(https_server_.get(), "duckduckgo.com"),
      true);
  rewards_browsertest_helper::VisitPublisher(
      browser(),
      rewards_browsertest_util::GetUrl(
          https_server_.get(),
          "laurenwags.github.io"),
      true);

  rewards_service_->StartMonthlyContributionForTest();

  contribution_->WaitForACReconcileCompleted();
  ASSERT_EQ(contribution_->GetACStatus(), ledger::Result::LEDGER_OK);

  contribution_->IsBalanceCorrect();

  rewards_browsertest_util::WaitForElementToContain(
      contents(),
      "[color=contribute]",
      "-20.000BAT");
}

IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    AutoContributeWhenACOff) {
  rewards_browsertest_helper::EnableRewards(browser());
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  rewards_browsertest_helper::VisitPublisher(
      browser(),
      rewards_browsertest_util::GetUrl(https_server_.get(), "duckduckgo.com"),
      true);

  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "[data-test-id2='autoContribution']");
  std::string value =
      rewards_browsertest_util::WaitForElementThenGetAttribute(
        contents(),
        "[data-test-id2='autoContribution']",
        "data-toggled");
  ASSERT_STREQ(value.c_str(), "false");

  rewards_service_->StartMonthlyContributionForTest();
}

IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    TipVerifiedPublisher) {
  rewards_browsertest_helper::EnableRewards(browser());
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  contribution_->TipPublisher(
      rewards_browsertest_util::GetUrl(https_server_.get(), "duckduckgo.com"),
      rewards_browsertest_util::ContributionType::OneTimeTip,
      1);
}

IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    TipUnverifiedPublisher) {
  rewards_browsertest_helper::EnableRewards(browser());
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  contribution_->TipPublisher(
      rewards_browsertest_util::GetUrl(https_server_.get(), "brave.com"),
      rewards_browsertest_util::ContributionType::OneTimeTip);
}

IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    RecurringTipForVerifiedPublisher) {
  rewards_browsertest_helper::EnableRewards(browser());
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  contribution_->TipPublisher(
      rewards_browsertest_util::GetUrl(https_server_.get(), "duckduckgo.com"),
      rewards_browsertest_util::ContributionType::MonthlyTip,
      1);
}

IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    RecurringTipForUnverifiedPublisher) {
  rewards_browsertest_helper::EnableRewards(browser());
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  contribution_->TipPublisher(
      rewards_browsertest_util::GetUrl(https_server_.get(), "brave.com"),
      rewards_browsertest_util::ContributionType::MonthlyTip);
}

// Check pending contributions
IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    PendingContributionTip) {
  const std::string publisher = "example.com";

  rewards_browsertest_helper::EnableRewards(browser());
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  // Tip unverified publisher
  contribution_->TipPublisher(
      rewards_browsertest_util::GetUrl(https_server_.get(), publisher),
      rewards_browsertest_util::ContributionType::OneTimeTip);

  // Check that link for pending is shown and open modal
  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "[data-test-id='reservedAllLink']");

  // Make sure that table is populated
  rewards_browsertest_util::WaitForElementToContain(
      contents(),
      "[id='pendingContributionTable'] a",
      publisher);
}

IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    ProcessPendingContributions) {
  response_->SetAlternativePublisherList(true);

  rewards_browsertest_helper::EnableRewards(browser());

  rewards_browsertest_helper::ReloadCurrentSite(browser());

  // Tip unverified publisher
  contribution_->TipViaCode(
      "brave.com",
      1.0,
      ledger::PublisherStatus::NOT_VERIFIED);
  contribution_->TipViaCode(
      "brave.com",
      5.0,
      ledger::PublisherStatus::NOT_VERIFIED);
  contribution_->TipViaCode(
      "3zsistemi.si",
      10.0,
      ledger::PublisherStatus::NOT_VERIFIED);
  contribution_->TipViaCode(
      "3zsistemi.si",
      5.0,
      ledger::PublisherStatus::NOT_VERIFIED);
  contribution_->TipViaCode(
      "3zsistemi.si",
      10.0,
      ledger::PublisherStatus::NOT_VERIFIED);
  contribution_->TipViaCode(
      "3zsistemi.si",
      10.0,
      ledger::PublisherStatus::NOT_VERIFIED);
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  response_->SetAlternativePublisherList(false);
  contribution_->VerifyTip(41.0, false, false, true);

  // Visit publisher
  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "3zsistemi.si");

  // Refresh publisher list
  RefreshPublisherListUsingRewardsPopup();

  // Activate the Rewards settings page tab
  rewards_browsertest_util::ActivateTabAtIndex(browser(), 0);

  // Wait for new verified publisher to be processed
  contribution_->WaitForMultipleTipReconcileCompleted(3);
  for (const auto status : contribution_->GetMultipleTipStatus()) {
    ASSERT_EQ(status, ledger::Result::LEDGER_OK);
  }
  contribution_->UpdateContributionBalance(-25.0, false);

  // Make sure that balance is updated correctly
  contribution_->IsBalanceCorrect();

  // Check that wallet summary shows the appropriate tip amount
  rewards_browsertest_util::WaitForElementToEqual(
      contents(),
      "[data-test-id=summary-tips] [color=contribute] span span",
      ExpectedTipSummaryAmountString());

  // Make sure that pending contribution box shows the correct
  // amount
  contribution_->IsPendingBalanceCorrect();

  // Open the Rewards popup
  content::WebContents* popup_contents =
      rewards_browsertest_helper::OpenRewardsPopup(browser());
  ASSERT_TRUE(popup_contents);

  // Check if verified notification is shown
  rewards_browsertest_util::WaitForElementToContain(
      popup_contents,
      "#root",
      "3zsistemi.si");

  // Close notification
  rewards_browsertest_util::WaitForElementThenClick(
      popup_contents,
      "[data-test-id=notification-close]");

  // Check if insufficient funds notification is shown
  rewards_browsertest_util::WaitForElementToContain(
      popup_contents,
      "#root",
      "Insufficient Funds");
}

IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    TipWithVerifiedWallet) {
  response_->SetVerifiedWallet(true);
  contribution_->SetUpUpholdWallet(50.0);
  rewards_browsertest_helper::EnableRewards(browser());

  const double amount = 5.0;
  contribution_->TipViaCode(
      "duckduckgo.com",
      amount,
      ledger::PublisherStatus::VERIFIED,
      1);
  contribution_->VerifyTip(amount, true, false, true);
}

IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    MultipleTipsProduceMultipleFeesWithVerifiedWallet) {
  response_->SetVerifiedWallet(true);
  contribution_->SetUpUpholdWallet(50.0);

  rewards_browsertest_helper::EnableRewards(browser());

  double total_amount = 0.0;
  const double amount = 5.0;
  const double fee_percentage = 0.05;
  const double tip_fee = amount * fee_percentage;
  contribution_->TipViaCode(
      "duckduckgo.com",
      amount,
      ledger::PublisherStatus::VERIFIED,
      1);
  total_amount += amount;

  contribution_->TipViaCode(
      "laurenwags.github.io",
      amount,
      ledger::PublisherStatus::VERIFIED,
      1);
  total_amount += amount;

  ledger::TransferFeeList transfer_fees =
      rewards_service_->GetTransferFeesForTesting("uphold");

  ASSERT_EQ(transfer_fees.size(), 2UL);

  contribution_->VerifyTip(total_amount, true, false, true);

  for (auto const& value : transfer_fees) {
    ASSERT_EQ(value.second->amount, tip_fee);
  }
}

IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    TipConnectedPublisherAnon) {
  rewards_browsertest_helper::EnableRewards(browser());
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  const double amount = 5.0;
  contribution_->TipViaCode(
      "bumpsmack.com",
      amount,
      ledger::PublisherStatus::CONNECTED,
      1);
  contribution_->VerifyTip(amount, true, false, true);
}

IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    TipConnectedPublisherAnonAndConnected) {
  response_->SetVerifiedWallet(true);
  contribution_->SetUpUpholdWallet(50.0);

  rewards_browsertest_helper::EnableRewards(browser());
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  const double amount = 5.0;
  contribution_->TipViaCode(
      "bumpsmack.com",
      amount,
      ledger::PublisherStatus::CONNECTED,
      1);
  contribution_->VerifyTip(amount, true, false, true);
}

IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    TipConnectedPublisherConnected) {
  response_->SetVerifiedWallet(true);
  contribution_->SetUpUpholdWallet(50.0, ledger::WalletStatus::CONNECTED);

  rewards_browsertest_helper::EnableRewards(browser());
  rewards_browsertest_helper::ReloadCurrentSite(browser());

  const double amount = 5.0;
  contribution_->TipViaCode(
      "bumpsmack.com",
      amount,
      ledger::PublisherStatus::CONNECTED,
      0);

  contribution_->IsBalanceCorrect();

  // Make sure that tips table is empty
  rewards_browsertest_util::WaitForElementToEqual(
      contents(),
      "#tips-table > div > div",
      "Have you tipped your favorite content creator today?");
}

IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    TipConnectedPublisherVerified) {
  response_->SetVerifiedWallet(true);
  contribution_->SetUpUpholdWallet(50.0);

  rewards_browsertest_helper::EnableRewards(browser());
  rewards_browsertest_helper::ReloadCurrentSite(browser());

  const double amount = 5.0;
  contribution_->TipViaCode(
      "bumpsmack.com",
      amount,
      ledger::PublisherStatus::CONNECTED,
      0);

  contribution_->IsBalanceCorrect();

  // Make sure that tips table is empty
  rewards_browsertest_util::WaitForElementToEqual(
      contents(),
      "#tips-table > div > div",
      "Have you tipped your favorite content creator today?");
}

// Ensure that we can make a one-time tip of a non-integral amount.
IN_PROC_BROWSER_TEST_F(RewardsContributionBrowserTest, TipNonIntegralAmount) {
  rewards_browsertest_helper::EnableRewards(browser());
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  rewards_service_->OnTip("duckduckgo.com", 2.5, false);
  contribution_->WaitForTipReconcileCompleted();
  ASSERT_EQ(contribution_->GetTipStatus(), ledger::Result::LEDGER_OK);
  ASSERT_EQ(contribution_->GetReconcileTipTotal(), 2.5);
}

// Ensure that we can make a recurring tip of a non-integral amount.
IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    RecurringTipNonIntegralAmount) {
  rewards_browsertest_helper::EnableRewards(browser());
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  const bool verified = true;
  rewards_browsertest_helper::VisitPublisher(
      browser(),
      rewards_browsertest_util::GetUrl(https_server_.get(), "duckduckgo.com"),
      verified);

  rewards_service_->OnTip("duckduckgo.com", 2.5, true);
  rewards_service_->StartMonthlyContributionForTest();
  contribution_->WaitForTipReconcileCompleted();
  ASSERT_EQ(contribution_->GetTipStatus(), ledger::Result::LEDGER_OK);

  ASSERT_EQ(contribution_->GetReconcileTipTotal(), 2.5);
}

IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    RecurringAndPartialAutoContribution) {
  rewards_browsertest_helper::EnableRewards(browser());
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  // Visit verified publisher
  const bool verified = true;
  rewards_browsertest_helper::VisitPublisher(
      browser(),
      rewards_browsertest_util::GetUrl(https_server_.get(), "duckduckgo.com"),
      true);

  // Set monthly recurring
  contribution_->TipViaCode(
      "duckduckgo.com",
      25.0,
      ledger::PublisherStatus::VERIFIED,
      0,
      true);

  rewards_browsertest_helper::VisitPublisher(
      browser(),
      rewards_browsertest_util::GetUrl(https_server_.get(), "brave.com"),
      !verified);

  // Trigger contribution process
  rewards_service_->StartMonthlyContributionForTest();

  // Wait for reconciliation to complete
  contribution_->WaitForTipReconcileCompleted();
  ASSERT_EQ(contribution_->GetTipStatus(), ledger::Result::LEDGER_OK);

  // Wait for reconciliation to complete successfully
  contribution_->WaitForACReconcileCompleted();
  ASSERT_EQ(contribution_->GetACStatus(), ledger::Result::LEDGER_OK);

  // Make sure that balance is updated correctly
  contribution_->IsBalanceCorrect();

  // Check that summary table shows the appropriate contribution
  rewards_browsertest_util::WaitForElementToContain(
      contents(),
      "[color='contribute']",
      "-5.000BAT");
}

IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    MultipleRecurringOverBudgetAndPartialAutoContribution) {
  rewards_browsertest_helper::EnableRewards(browser());

  contribution_->TipViaCode(
      "duckduckgo.com",
      5.0,
      ledger::PublisherStatus::VERIFIED,
      0,
      true);

  contribution_->TipViaCode(
      "site1.com",
      10.0,
      ledger::PublisherStatus::VERIFIED,
      0,
      true);

  contribution_->TipViaCode(
      "site2.com",
      10.0,
      ledger::PublisherStatus::VERIFIED,
      0,
      true);

  contribution_->TipViaCode(
      "site3.com",
      10.0,
      ledger::PublisherStatus::VERIFIED,
      0,
      true);
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  const bool verified = true;
  rewards_browsertest_helper::VisitPublisher(
      browser(),
      rewards_browsertest_util::GetUrl(https_server_.get(), "duckduckgo.com"),
      verified);

  // Trigger contribution process
  rewards_service_->StartMonthlyContributionForTest();

  // Wait for reconciliation to complete
  contribution_->WaitForMultipleTipReconcileCompleted(3);
  ASSERT_EQ(contribution_->GetTipStatus(), ledger::Result::LEDGER_OK);

  // Wait for reconciliation to complete successfully
  contribution_->WaitForACReconcileCompleted();
  ASSERT_EQ(contribution_->GetACStatus(), ledger::Result::LEDGER_OK);

  // Make sure that balance is updated correctly
  contribution_->IsBalanceCorrect();

  // Check that summary table shows the appropriate contribution
  rewards_browsertest_util::WaitForElementToContain(
      contents(),
      "[color='contribute']",
      "-5.000BAT");
}

IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    SplitProcessorAutoContribution) {
  response_->SetVerifiedWallet(true);
  contribution_->SetUpUpholdWallet(50.0);

  rewards_browsertest_helper::EnableRewards(browser());
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  rewards_browsertest_helper::VisitPublisher(
      browser(),
      rewards_browsertest_util::GetUrl(https_server_.get(), "3zsistemi.si"),
      true);

  // 30 form unblinded and 20 from uphold
  rewards_service_->SetAutoContributionAmount(50.0);

  ledger::SKUOrderItemList items;
  auto item = ledger::SKUOrderItem::New();
  item->order_item_id = "ed193339-e58c-483c-8d61-7decd3c24827";
  item->order_id = "a38b211b-bf78-42c8-9479-b11e92e3a76c";
  item->quantity = 80;
  item->price = 0.25;
  item->description = "description";
  item->type = ledger::SKUOrderItemType::SINGLE_USE;
  items.push_back(std::move(item));

  auto order = ledger::SKUOrder::New();
  order->order_id = "a38b211b-bf78-42c8-9479-b11e92e3a76c";
  order->total_amount = 20;
  order->merchant_id = "";
  order->location = "brave.com";
  order->items = std::move(items);
  response_->SetSKUOrder(std::move(order));

  // Trigger contribution process
  rewards_service_->StartMonthlyContributionForTest();

  // Wait for reconciliation to complete successfully
  contribution_->WaitForMultipleACReconcileCompleted(2);
  auto statuses = contribution_->GetMultipleACStatus();
  ASSERT_EQ(statuses[0], ledger::Result::LEDGER_OK);
  ASSERT_EQ(statuses[1], ledger::Result::LEDGER_OK);

  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "[data-test-id='showMonthlyReport']");

  rewards_browsertest_util::WaitForElementToAppear(
      contents(),
      "#transactionTable");

  rewards_browsertest_util::WaitForElementToContain(
      contents(),
      "#transactionTable",
      "-30.000BAT");

  rewards_browsertest_util::WaitForElementToContain(
      contents(),
      "#transactionTable",
      "-20.000BAT");

  // Check that summary table shows the appropriate contribution
  rewards_browsertest_util::WaitForElementToContain(
      contents(),
      "[color=contribute]",
      "-50.000BAT");
}

IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    CheckIfReconcileWasReset) {
  rewards_browsertest_helper::EnableRewards(browser());
  uint64_t current_stamp = 0;

  base::RunLoop run_loop_first;
  rewards_service_->GetReconcileStamp(
      base::BindLambdaForTesting([&](uint64_t stamp) {
        current_stamp = stamp;
        run_loop_first.Quit();
      }));
  run_loop_first.Run();
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  rewards_browsertest_helper::VisitPublisher(
      browser(),
      rewards_browsertest_util::GetUrl(https_server_.get(), "duckduckgo.com"),
      true);

  contribution_->TipPublisher(
      rewards_browsertest_util::GetUrl(https_server_.get(), "duckduckgo.com"),
      rewards_browsertest_util::ContributionType::MonthlyTip,
      1);

  base::RunLoop run_loop_second;
  rewards_service_->GetReconcileStamp(
      base::BindLambdaForTesting([&](uint64_t stamp) {
        ASSERT_NE(current_stamp, stamp);
        run_loop_second.Quit();
      }));
  run_loop_second.Run();
}

IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    CheckIfReconcileWasResetACOff) {
  rewards_browsertest_helper::EnableRewards(browser());
  uint64_t current_stamp = 0;

  rewards_service_->SetAutoContributeEnabled(false);

  base::RunLoop run_loop_first;
  rewards_service_->GetReconcileStamp(
      base::BindLambdaForTesting([&](uint64_t stamp) {
        current_stamp = stamp;
        run_loop_first.Quit();
      }));
  run_loop_first.Run();
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());
  contribution_->TipPublisher(
      rewards_browsertest_util::GetUrl(https_server_.get(), "duckduckgo.com"),
      rewards_browsertest_util::ContributionType::MonthlyTip,
      1);

  base::RunLoop run_loop_second;
  rewards_service_->GetReconcileStamp(
      base::BindLambdaForTesting([&](uint64_t stamp) {
        ASSERT_NE(current_stamp, stamp);
        run_loop_second.Quit();
      }));
  run_loop_second.Run();
}

IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    SplitProcessOneTimeTip) {
  response_->SetVerifiedWallet(true);
  contribution_->SetUpUpholdWallet(50.0);
  rewards_browsertest_helper::EnableRewards(browser());
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  contribution_->TipPublisher(
      rewards_browsertest_util::GetUrl(
          https_server_.get(),
          "kjozwiakstaging.github.io"),
      rewards_browsertest_util::ContributionType::OneTimeTip,
      2,
      1);

  rewards_browsertest_util::ActivateTabAtIndex(browser(), 0);

  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "[data-test-id='showMonthlyReport']");

  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "[data-test-id='tab-oneTimeDonation']");

  rewards_browsertest_util::WaitForElementToEqual(
      contents(),
      "[data-test-id='activity-table-body'] tr:nth-of-type(1) "
      "td:nth-of-type(3)",
      "20.000BAT28.60 USD");

  rewards_browsertest_util::WaitForElementToEqual(
      contents(),
      "[data-test-id='activity-table-body'] tr:nth-of-type(2) "
      "td:nth-of-type(3)",
      "30.000BAT42.90 USD");
}

IN_PROC_BROWSER_TEST_F(
    RewardsContributionBrowserTest,
    PanelDefaultMonthlyTipChoices) {
  rewards_browsertest_helper::EnableRewards(browser());
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "3zsistemi.si");

  // Add a recurring tip of 10 BAT.
  contribution_->TipViaCode(
      "3zsistemi.si",
      10.0,
      ledger::PublisherStatus::VERIFIED,
      0,
      true);

  content::WebContents* popup =
      rewards_browsertest_helper::OpenRewardsPopup(browser());
  const auto tip_options = rewards_browsertest_util::GetRewardsPopupTipOptions(
      popup);
  ASSERT_EQ(tip_options, std::vector<double>({ 0, 1, 10, 100 }));
}

}  // namespace rewards_browsertest
