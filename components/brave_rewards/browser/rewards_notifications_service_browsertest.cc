/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_notifications_service.h"
#include "brave/components/brave_rewards/browser/rewards_notifications_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_notifications_service_observer.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"

using namespace brave_rewards;

class BraveRewardsNotificationsBrowserTest
    : public InProcessBrowserTest,
      public RewardsNotificationsServiceObserver {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    rewards_notifications_service_ =
        RewardsNotificationsServiceFactory::GetForProfile(browser()->profile());
  }

  void TearDown() override {
    InProcessBrowserTest::TearDown();
  }

  void OnNotificationAdded(
      RewardsNotificationsService* rewards_notifications_service,
      const RewardsNotificationsService::RewardsNotification& notification) override {
    EXPECT_TRUE(notification.args_.size() == 2);
    EXPECT_STREQ(notification.args_.at(0).c_str(), "foo");
    EXPECT_STREQ(notification.args_.at(1).c_str(), "bar");

    EXPECT_TRUE(notification.id_ != 0);
    EXPECT_TRUE(notification.timestamp_ != 0);

    notification_id_ = notification.id_;

    add_notification_callback_was_called_ = true;
  }

  void OnNotificationDeleted(
      RewardsNotificationsService* rewards_notifications_service,
      const RewardsNotificationsService::RewardsNotification& notification) override {
    EXPECT_TRUE(notification.id_ != 0);
    EXPECT_TRUE(notification.timestamp_ != 0);

    delete_notification_callback_was_called_ = true;
  }

  void OnAllNotificationsDeleted(
      RewardsNotificationsService* rewards_notifications_service) override {
  }

  void OnGetNotification(
      RewardsNotificationsService* rewards_notifications_service,
      const RewardsNotificationsService::RewardsNotification& notification) override {
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

  RewardsNotificationsService* rewards_notifications_service_;

  RewardsNotificationsService::RewardsNotificationID notification_id_ = 0;

  bool add_notification_callback_was_called_ = false;
  bool delete_notification_callback_was_called_ = false;
};

IN_PROC_BROWSER_TEST_F(BraveRewardsNotificationsBrowserTest, AddGrantNotification) {
  rewards_notifications_service_->AddObserver(this);

  RewardsNotificationsService::RewardsNotificationArgs args;
  args.push_back("foo");
  args.push_back("bar");
  
  rewards_notifications_service_->AddNotification(
      RewardsNotificationsService::REWARDS_NOTIFICATION_GRANT, args);
  WaitForAddNotificationCallback();

  rewards_notifications_service_->RemoveObserver(this);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsNotificationsBrowserTest, AddGrantNotificationAndDeleteIt) {
  rewards_notifications_service_->AddObserver(this);

  RewardsNotificationsService::RewardsNotificationArgs args;
  args.push_back("foo");
  args.push_back("bar");
  
  rewards_notifications_service_->AddNotification(
      RewardsNotificationsService::REWARDS_NOTIFICATION_GRANT, args);
  WaitForAddNotificationCallback();

  EXPECT_TRUE(notification_id_ != 0);

  rewards_notifications_service_->DeleteNotification(notification_id_);
  WaitForDeleteNotificationCallback();

  rewards_notifications_service_->RemoveObserver(this);
}
