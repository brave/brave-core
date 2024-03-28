/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_helper.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_contribution.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_network_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_response.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

// npm run test -- brave_browser_tests --filter=RewardsNotificationBrowserTest.*

namespace brave_rewards {

using RewardsNotificationType =
    RewardsNotificationService::RewardsNotificationType;

class RewardsNotificationBrowserTest
    : public InProcessBrowserTest,
      public RewardsNotificationServiceObserver {
 public:
  RewardsNotificationBrowserTest() {
    contribution_ =
        std::make_unique<test_util::RewardsBrowserTestContribution>();
    response_ = std::make_unique<test_util::RewardsBrowserTestResponse>();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    context_helper_ =
        std::make_unique<test_util::RewardsBrowserTestContextHelper>(browser());

    // HTTP resolver
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->RegisterRequestHandler(
        base::BindRepeating(&test_util::HandleRequest));
    ASSERT_TRUE(https_server_->Start());

    // Rewards service
    brave::RegisterPathProvider();
    auto* profile = browser()->profile();
    rewards_service_ = static_cast<RewardsServiceImpl*>(
        RewardsServiceFactory::GetForProfile(profile));

    // Response mock
    base::ScopedAllowBlockingForTesting allow_blocking;
    response_->LoadMocks();
    rewards_service_->ForTestingSetTestResponseCallback(
        base::BindRepeating(
            &RewardsNotificationBrowserTest::GetTestResponse,
            base::Unretained(this)));
    rewards_service_->SetEngineEnvForTesting();

    // Other
    contribution_->Initialize(browser(), rewards_service_);
    rewards_notification_service_ = rewards_service_->GetNotificationService();
    rewards_notification_service_->AddObserver(this);

    test_util::SetOnboardingBypassed(browser());
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
      RewardsNotificationService* rewards_notification_service,
      const RewardsNotificationService::RewardsNotification&
          rewards_notification) override {
    last_added_notification_ = rewards_notification;
    const auto& notifications = rewards_service_->GetAllNotifications();
    for (const auto& notification : notifications) {
      switch (notification.second.type_) {
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
      RewardsNotificationService* rewards_notification_service,
      const RewardsNotificationService::RewardsNotification& notification)
      override {
    last_deleted_notification_ = notification;
    delete_notification_ = true;
    if (wait_for_delete_notification_loop_) {
      wait_for_delete_notification_loop_->Quit();
    }
  }

  void OnAllNotificationsDeleted(
      RewardsNotificationService* rewards_notification_service) override {}

  void OnGetNotification(
      RewardsNotificationService* rewards_notification_service,
      const RewardsNotificationService::RewardsNotification& notification)
      override {}

  void WaitForAddNotificationCallback() {
    if (add_notification_) {
      return;
    }

    wait_for_add_notification_loop_ = std::make_unique<base::RunLoop>();
    wait_for_add_notification_loop_->Run();
  }

  void WaitForDeleteNotificationCallback() {
    if (delete_notification_) {
      return;
    }

    wait_for_delete_notification_loop_ = std::make_unique<base::RunLoop>();
    wait_for_delete_notification_loop_->Run();
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

  raw_ptr<RewardsNotificationService> rewards_notification_service_ = nullptr;
  raw_ptr<RewardsServiceImpl> rewards_service_ = nullptr;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<test_util::RewardsBrowserTestContribution> contribution_;
  std::unique_ptr<test_util::RewardsBrowserTestResponse> response_;
  std::unique_ptr<test_util::RewardsBrowserTestContextHelper> context_helper_;

  RewardsNotificationService::RewardsNotification last_added_notification_;
  RewardsNotificationService::RewardsNotification last_deleted_notification_;

  bool add_notification_ = false;
  std::unique_ptr<base::RunLoop> wait_for_add_notification_loop_;

  bool delete_notification_ = false;
  std::unique_ptr<base::RunLoop> wait_for_delete_notification_loop_;
};

}  // namespace brave_rewards
