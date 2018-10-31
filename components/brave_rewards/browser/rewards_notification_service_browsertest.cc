/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"

using namespace brave_rewards;

class BraveRewardsNotificationBrowserTest
    : public InProcessBrowserTest,
      public RewardsNotificationServiceObserver {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    rewards_service_ =
        RewardsServiceFactory::GetForProfile(browser()->profile());
    rewards_notification_service_ =
        rewards_service_->notification_service(browser()->profile());
  }

  void TearDown() override {
    InProcessBrowserTest::TearDown();
  }

  void OnNotificationAdded(
      RewardsNotificationService* rewards_notification_service,
      const RewardsNotificationService::RewardsNotification& notification) override {
    EXPECT_TRUE(notification.args_.size() == 2);
    EXPECT_STREQ(notification.args_.at(0).c_str(), "foo");
    EXPECT_STREQ(notification.args_.at(1).c_str(), "bar");

    EXPECT_STREQ(notification.id_.c_str(), "rewards_notification_grant");
    EXPECT_TRUE(notification.timestamp_ != 0);

    notification_id_ = notification.id_;

    add_notification_callback_was_called_ = true;
  }

  void OnNotificationDeleted(
      RewardsNotificationService* rewards_notification_service,
      const RewardsNotificationService::RewardsNotification& notification) override {
    EXPECT_STREQ(notification.id_.c_str(), "rewards_notification_grant");
    EXPECT_TRUE(notification.timestamp_ != 0);

    delete_notification_callback_was_called_ = true;
  }

  void OnAllNotificationsDeleted(
      RewardsNotificationService* rewards_notification_service) override {
  }

  void OnGetNotification(
      RewardsNotificationService* rewards_notification_service,
      const RewardsNotificationService::RewardsNotification& notification) override {
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

  RewardsNotificationService* rewards_notification_service_;
  RewardsService* rewards_service_;

  RewardsNotificationService::RewardsNotificationID notification_id_;

  bool add_notification_callback_was_called_ = false;
  bool delete_notification_callback_was_called_ = false;
};

IN_PROC_BROWSER_TEST_F(BraveRewardsNotificationBrowserTest, AddGrantNotification) {
  rewards_notification_service_->AddObserver(this);

  RewardsNotificationService::RewardsNotificationArgs args;
  args.push_back("foo");
  args.push_back("bar");

  rewards_notification_service_->AddNotification(
      RewardsNotificationService::REWARDS_NOTIFICATION_GRANT, args,
      "rewards_notification_grant");
  WaitForAddNotificationCallback();

  rewards_notification_service_->RemoveObserver(this);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsNotificationBrowserTest, AddGrantNotificationAndDeleteIt) {
  rewards_notification_service_->AddObserver(this);

  RewardsNotificationService::RewardsNotificationArgs args;
  args.push_back("foo");
  args.push_back("bar");

  rewards_notification_service_->AddNotification(
      RewardsNotificationService::REWARDS_NOTIFICATION_GRANT, args,
      "rewards_notification_grant");
  WaitForAddNotificationCallback();

  EXPECT_STREQ(notification_id_.c_str(), "rewards_notification_grant");

  rewards_notification_service_->DeleteNotification(notification_id_);
  WaitForDeleteNotificationCallback();

  rewards_notification_service_->RemoveObserver(this);
}
