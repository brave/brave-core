/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"

// npm run test -- brave_browser_tests --filter=RewardsNotificationBrowserTest.*

class RewardsNotificationBrowserTest
    : public InProcessBrowserTest,
      public brave_rewards::RewardsNotificationServiceObserver {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    rewards_service_ =
        brave_rewards::RewardsServiceFactory::GetForProfile(
            browser()->profile());
    rewards_notification_service_ = rewards_service_->GetNotificationService();
  }

  void TearDown() override {
    InProcessBrowserTest::TearDown();
  }

  void OnNotificationAdded(
      brave_rewards::RewardsNotificationService* rewards_notification_service,
      const brave_rewards::RewardsNotificationService::RewardsNotification&
      notification) override {
    EXPECT_EQ(notification.args_.size(), 2ul);
    EXPECT_STREQ(notification.args_.at(0).c_str(), "foo");
    EXPECT_STREQ(notification.args_.at(1).c_str(), "bar");

    EXPECT_STREQ(notification.id_.c_str(), "rewards_notification_grant");
    EXPECT_NE(notification.timestamp_, 0ul);

    notification_id_ = notification.id_;

    add_notification_callback_was_called_ = true;
  }

  void OnNotificationDeleted(
      brave_rewards::RewardsNotificationService* rewards_notification_service,
      const brave_rewards::RewardsNotificationService::RewardsNotification&
      notification) override {
    if (notification.id_ == "not_valid") {
      EXPECT_TRUE(notification.type_ ==
        brave_rewards::RewardsNotificationService::
        REWARDS_NOTIFICATION_INVALID);
    } else {
      EXPECT_STREQ(notification.id_.c_str(), "rewards_notification_grant");
      EXPECT_NE(notification.timestamp_,  0ul);
    }

    delete_notification_callback_was_called_ = true;
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
    if (add_notification_callback_was_called_)
      return;
    base::RunLoop run_loop;
    run_loop.Run();
  }

  void WaitForDeleteNotificationCallback() {
    if (delete_notification_callback_was_called_)
      return;
    base::RunLoop run_loop;
    run_loop.Run();
  }

  brave_rewards::RewardsNotificationService* rewards_notification_service_;
  brave_rewards::RewardsService* rewards_service_;

  brave_rewards::RewardsNotificationService::RewardsNotificationID
  notification_id_;

  bool add_notification_callback_was_called_ = false;
  bool delete_notification_callback_was_called_ = false;
};

IN_PROC_BROWSER_TEST_F(
    RewardsNotificationBrowserTest,
    AddGrantNotification) {
  rewards_notification_service_->AddObserver(this);

  brave_rewards::RewardsNotificationService::RewardsNotificationArgs args;
  args.push_back("foo");
  args.push_back("bar");

  rewards_notification_service_->AddNotification(
      brave_rewards::RewardsNotificationService::REWARDS_NOTIFICATION_GRANT,
      args,
      "rewards_notification_grant");
  WaitForAddNotificationCallback();

  rewards_notification_service_->RemoveObserver(this);
}

IN_PROC_BROWSER_TEST_F(
    RewardsNotificationBrowserTest,
    AddGrantNotificationAndDeleteIt) {
  rewards_notification_service_->AddObserver(this);

  brave_rewards::RewardsNotificationService::RewardsNotificationArgs args;
  args.push_back("foo");
  args.push_back("bar");

  rewards_notification_service_->AddNotification(
      brave_rewards::RewardsNotificationService::REWARDS_NOTIFICATION_GRANT,
      args,
      "rewards_notification_grant");
  WaitForAddNotificationCallback();

  EXPECT_STREQ(notification_id_.c_str(), "rewards_notification_grant");

  rewards_notification_service_->DeleteNotification(notification_id_);
  WaitForDeleteNotificationCallback();

  rewards_notification_service_->RemoveObserver(this);
}

IN_PROC_BROWSER_TEST_F(
    RewardsNotificationBrowserTest,
    AddGrantNotificationAndFakeItAndDeleteIt) {
  rewards_notification_service_->AddObserver(this);

  brave_rewards::RewardsNotificationService::RewardsNotificationArgs args;
  args.push_back("foo");
  args.push_back("bar");

  rewards_notification_service_->AddNotification(
      brave_rewards::RewardsNotificationService::REWARDS_NOTIFICATION_GRANT,
      args,
      "rewards_notification_grant");
  WaitForAddNotificationCallback();

  EXPECT_STREQ(notification_id_.c_str(), "rewards_notification_grant");

  rewards_notification_service_->DeleteNotification("not_valid");
  WaitForDeleteNotificationCallback();

  rewards_notification_service_->RemoveObserver(this);
}
