/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/extension_rewards_service_observer.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "brave/components/brave_rewards/browser/content_site.h"

using namespace brave_rewards;

class ExtensionRewardsServiceObserverBrowserTest
    : public InProcessBrowserTest,
      public ExtensionRewardsServiceObserver {
  public:

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    rewards_service_ = RewardsServiceFactory::GetForProfile(browser()->profile());
  }

  void TearDown() override {
    InProcessBrowserTest::TearDown();
  }

  void OnRecurringDonations(
      RewardsService* rewards_service,
      brave_rewards::ContentSiteList list) override {

    EXPECT_STREQ(list.front().id.c_str(), "brave.com");
    EXPECT_STREQ(std::to_string(list.front().weight).c_str(), "10");

    on_recurring_notifications_callback_was_called_ = true;
  }

  void WaitForOnRecurringDonationsCallback() {
    if (on_recurring_notifications_callback_was_called_) {
      return;
    }

    base::RunLoop run_loop;
    run_loop.Run();
  }

  RewardsService* rewards_service_;
  bool on_recurring_notifications_callback_was_called_ = false;
};

IN_PROC_BROWSER_TEST_F(ExtensionRewardsServiceObserverBrowserTest, SaveARecurringDonation) {
  rewards_service_->AddObserver(this);

  rewards_service_->AddRecurringPayment("brave.com", 10);
  WaitForOnRecurringDonationsCallback();

  rewards_service_->RemoveObserver(this);
}
