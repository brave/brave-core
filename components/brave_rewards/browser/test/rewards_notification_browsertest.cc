/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_helper.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_contribution.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_network_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_promotion.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_response.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

// npm run test -- brave_browser_tests --filter=RewardsNotificationBrowserTest.*

namespace rewards_browsertest {

using RewardsNotificationType =
    brave_rewards::RewardsNotificationService::RewardsNotificationType;

class RewardsNotificationBrowserTest
    : public InProcessBrowserTest,
      public brave_rewards::RewardsNotificationServiceObserver {
 public:
  RewardsNotificationBrowserTest() {
    contribution_ = std::make_unique<RewardsBrowserTestContribution>();
    promotion_ = std::make_unique<RewardsBrowserTestPromotion>();
    response_ = std::make_unique<RewardsBrowserTestResponse>();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    context_helper_ =
        std::make_unique<RewardsBrowserTestContextHelper>(browser());

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
            &RewardsNotificationBrowserTest::GetTestResponse,
            base::Unretained(this)));
    rewards_service_->SetLedgerEnvForTesting();

    // Other
    promotion_->Initialize(browser(), rewards_service_);
    contribution_->Initialize(browser(), rewards_service_);
    rewards_notification_service_ = rewards_service_->GetNotificationService();
    rewards_notification_service_->AddObserver(this);

    rewards_browsertest_util::SetOnboardingBypassed(browser());
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
      base::flat_map<std::string, std::string>* headers) {
    response_->Get(
        url,
        method,
        response_status_code,
        response);
  }

  void OnNotificationAdded(
      brave_rewards::RewardsNotificationService* rewards_notification_service,
      const brave_rewards::RewardsNotificationService::RewardsNotification&
      notification) override {
    last_added_notification_ = notification;
    const auto& notifications = rewards_service_->GetAllNotifications();
    for (const auto& notification : notifications) {
      switch (notification.second.type_) {
        case RewardsNotificationType::REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS: {
          insufficient_notification_would_have_already_shown_ = true;
          if (wait_for_insufficient_notification_loop_) {
            wait_for_insufficient_notification_loop_->Quit();
          }
          break;
        }
        default: {
          add_notification_ = true;
          if (wait_for_add_notification_loop_) {
            wait_for_add_notification_loop_->Quit();
          }
          break;
        }
      }
    }
  }

  void OnNotificationDeleted(
      brave_rewards::RewardsNotificationService* rewards_notification_service,
      const brave_rewards::RewardsNotificationService::RewardsNotification&
      notification) override {
    last_deleted_notification_ = notification;
    delete_notification_ = true;
    if (wait_for_delete_notification_loop_) {
      wait_for_delete_notification_loop_->Quit();
    }
  }

  void OnAllNotificationsDeleted(
      brave_rewards::RewardsNotificationService* rewards_notification_service)
      override {
  }

  void OnGetNotification(
      brave_rewards::RewardsNotificationService* rewards_notification_service,
      const brave_rewards::RewardsNotificationService::RewardsNotification&
      notification) override {
  }

  void WaitForAddNotificationCallback() {
    if (add_notification_) {
      return;
    }

    wait_for_add_notification_loop_.reset(new base::RunLoop);
    wait_for_add_notification_loop_->Run();
  }

  void WaitForDeleteNotificationCallback() {
    if (delete_notification_) {
      return;
    }

    wait_for_delete_notification_loop_.reset(new base::RunLoop);
    wait_for_delete_notification_loop_->Run();
  }

  void WaitForInsufficientFundsNotification() {
    if (insufficient_notification_would_have_already_shown_) {
      return;
    }

    wait_for_insufficient_notification_loop_.reset(new base::RunLoop);
    wait_for_insufficient_notification_loop_->Run();
  }

  void CheckInsufficientFundsForTesting() {
    rewards_service_->MaybeShowNotificationAddFundsForTesting(
        base::BindOnce(
            &RewardsNotificationBrowserTest::
            ShowNotificationAddFundsForTesting,
            base::Unretained(this)));
  }

  /**
   * When using notification observer for insufficient funds, tests will fail
   * for sufficient funds because observer will never be called for
   * notification. Use this as callback to know when we come back with
   * sufficient funds to prevent inf loop
   * */
  void ShowNotificationAddFundsForTesting(bool sufficient) {
    if (!sufficient) {
      return;
    }

    insufficient_notification_would_have_already_shown_ = true;
    if (wait_for_insufficient_notification_loop_) {
      wait_for_insufficient_notification_loop_->Quit();
    }
  }

  bool IsShowingNotificationForType(const RewardsNotificationType type) {
    const auto& notifications = rewards_service_->GetAllNotifications();
    for (const auto& notification : notifications) {
      if (notification.second.type_ == type) {
        return true;
      }
    }

    return false;
  }

  raw_ptr<brave_rewards::RewardsNotificationService>
      rewards_notification_service_ = nullptr;
  raw_ptr<brave_rewards::RewardsServiceImpl> rewards_service_ = nullptr;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<RewardsBrowserTestContribution> contribution_;
  std::unique_ptr<RewardsBrowserTestPromotion> promotion_;
  std::unique_ptr<RewardsBrowserTestResponse> response_;
  std::unique_ptr<RewardsBrowserTestContextHelper> context_helper_;

  brave_rewards::RewardsNotificationService::RewardsNotification
    last_added_notification_;
  brave_rewards::RewardsNotificationService::RewardsNotification
    last_deleted_notification_;

  bool insufficient_notification_would_have_already_shown_ = false;
  std::unique_ptr<base::RunLoop> wait_for_insufficient_notification_loop_;

  bool add_notification_ = false;
  std::unique_ptr<base::RunLoop> wait_for_add_notification_loop_;

  bool delete_notification_ = false;
  std::unique_ptr<base::RunLoop> wait_for_delete_notification_loop_;
};

IN_PROC_BROWSER_TEST_F(
    RewardsNotificationBrowserTest,
    AddGrantNotification) {
  brave_rewards::RewardsNotificationService::RewardsNotificationArgs args;
  args.push_back("foo");
  args.push_back("bar");

  rewards_notification_service_->AddNotification(
      brave_rewards::RewardsNotificationService::REWARDS_NOTIFICATION_GRANT,
      args,
      "rewards_notification_grant");
  WaitForAddNotificationCallback();

  EXPECT_EQ(last_added_notification_.args_.size(), 2ul);
  EXPECT_STREQ(last_added_notification_.args_.at(0).c_str(), "foo");
  EXPECT_STREQ(last_added_notification_.args_.at(1).c_str(), "bar");

  EXPECT_STREQ(
      last_added_notification_.id_.c_str(),
      "rewards_notification_grant");
  EXPECT_NE(last_added_notification_.timestamp_, 0ul);
}

IN_PROC_BROWSER_TEST_F(
    RewardsNotificationBrowserTest,
    AddGrantNotificationAndDeleteIt) {
  brave_rewards::RewardsNotificationService::RewardsNotificationArgs args;
  args.push_back("foo");
  args.push_back("bar");

  rewards_notification_service_->AddNotification(
      brave_rewards::RewardsNotificationService::REWARDS_NOTIFICATION_GRANT,
      args,
      "rewards_notification_grant");
  WaitForAddNotificationCallback();

  EXPECT_STREQ(
      last_added_notification_.id_.c_str(),
      "rewards_notification_grant");

  rewards_notification_service_->DeleteNotification(
      last_added_notification_.id_);
  WaitForDeleteNotificationCallback();
  EXPECT_STREQ(
      last_deleted_notification_.id_.c_str(),
      "rewards_notification_grant");
  EXPECT_NE(last_deleted_notification_.timestamp_,  0ul);
}

IN_PROC_BROWSER_TEST_F(
    RewardsNotificationBrowserTest,
    AddGrantNotificationAndFakeItAndDeleteIt) {
  brave_rewards::RewardsNotificationService::RewardsNotificationArgs args;
  args.push_back("foo");
  args.push_back("bar");

  rewards_notification_service_->AddNotification(
      brave_rewards::RewardsNotificationService::REWARDS_NOTIFICATION_GRANT,
      args,
      "rewards_notification_grant");
  WaitForAddNotificationCallback();

  EXPECT_STREQ(
      last_added_notification_.id_.c_str(),
      "rewards_notification_grant");

  rewards_notification_service_->DeleteNotification("not_valid");
  WaitForDeleteNotificationCallback();
  EXPECT_TRUE(
      last_deleted_notification_.type_ ==
      brave_rewards::RewardsNotificationService::REWARDS_NOTIFICATION_INVALID);
}

IN_PROC_BROWSER_TEST_F(
    RewardsNotificationBrowserTest,
    InsufficientNotificationForZeroAmountZeroPublishers) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  CheckInsufficientFundsForTesting();
  WaitForInsufficientFundsNotification();
  const auto& notifications = rewards_service_->GetAllNotifications();

  if (notifications.empty()) {
    SUCCEED();
    return;
  }

  bool is_showing_notification = IsShowingNotificationForType(
      RewardsNotificationType::REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS);

  EXPECT_FALSE(is_showing_notification);
}

IN_PROC_BROWSER_TEST_F(
    RewardsNotificationBrowserTest,
    InsufficientNotificationForACNotEnoughFunds) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_service_->SetAutoContributeEnabled(true);
  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());
  // Visit publishers
  const bool verified = true;
  context_helper_->VisitPublisher(
      rewards_browsertest_util::GetUrl(https_server_.get(), "duckduckgo.com"),
      verified);
  context_helper_->VisitPublisher(
      rewards_browsertest_util::GetUrl(https_server_.get(), "bumpsmack.com"),
      verified);
  context_helper_->VisitPublisher(
      rewards_browsertest_util::GetUrl(https_server_.get(), "brave.com"),
      !verified,
      true);

  CheckInsufficientFundsForTesting();
  WaitForInsufficientFundsNotification();
  const auto& notifications = rewards_service_->GetAllNotifications();

  if (notifications.empty()) {
    SUCCEED();
    return;
  }

  bool is_showing_notification = IsShowingNotificationForType(
      RewardsNotificationType::REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS);

  EXPECT_FALSE(is_showing_notification);
}

IN_PROC_BROWSER_TEST_F(
    RewardsNotificationBrowserTest,
    InsufficientNotificationForInsufficientAmount) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_browsertest_util::CreateWallet(rewards_service_);
  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  contribution_->TipViaCode("duckduckgo.com", 20.0,
                            ledger::type::PublisherStatus::UPHOLD_VERIFIED, 0,
                            true);

  contribution_->TipViaCode(
      "brave.com",
      50.0,
      ledger::type::PublisherStatus::NOT_VERIFIED,
      0,
      true);

  CheckInsufficientFundsForTesting();
  WaitForInsufficientFundsNotification();
  const auto& notifications = rewards_service_->GetAllNotifications();

  if (notifications.empty()) {
    SUCCEED();
    return;
  }

  bool is_showing_notification = IsShowingNotificationForType(
      RewardsNotificationType::REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS);

  EXPECT_FALSE(is_showing_notification);
}

IN_PROC_BROWSER_TEST_F(
    RewardsNotificationBrowserTest,
    InsufficientNotificationForVerifiedInsufficientAmount) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_browsertest_util::CreateWallet(rewards_service_);
  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  contribution_->TipViaCode("duckduckgo.com", 50.0,
                            ledger::type::PublisherStatus::UPHOLD_VERIFIED, 0,
                            true);

  contribution_->TipViaCode(
      "brave.com",
      50.0,
      ledger::type::PublisherStatus::NOT_VERIFIED,
      0,
      true);

  CheckInsufficientFundsForTesting();
  WaitForInsufficientFundsNotification();
  const auto& notifications = rewards_service_->GetAllNotifications();

  if (notifications.empty()) {
    FAIL() << "Should see Insufficient Funds notification";
    return;
  }

  bool is_showing_notification = IsShowingNotificationForType(
      RewardsNotificationType::REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS);

  EXPECT_TRUE(is_showing_notification);
}

}  // namespace rewards_browsertest
