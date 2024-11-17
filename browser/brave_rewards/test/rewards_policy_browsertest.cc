/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/app/brave_command_ids.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"
#include "brave/browser/ui/views/brave_actions/brave_rewards_action_view.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/common/rewards_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/navigation_handle_observer.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/gurl.h"

namespace policy {

class BraveRewardsPolicyTest : public InProcessBrowserTest,
                               public ::testing::WithParamInterface<bool> {
 public:
  BraveRewardsPolicyTest() = default;
  ~BraveRewardsPolicyTest() override = default;

  void SetUpInProcessBrowserTestFixture() override {
    EXPECT_CALL(provider_, IsInitializationComplete(testing::_))
        .WillRepeatedly(testing::Return(true));
    BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);
    PolicyMap policies;
    policies.Set(key::kBraveRewardsDisabled, POLICY_LEVEL_MANDATORY,
                 POLICY_SCOPE_USER, POLICY_SOURCE_PLATFORM,
                 base::Value(IsBraveRewardsDisabledTest()), nullptr);
    provider_.UpdateChromePolicy(policies);
  }

  bool IsBraveRewardsDisabledTest() { return GetParam(); }

  content::WebContents* web_contents() const {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::BrowserContext* browser_context() {
    return web_contents()->GetBrowserContext();
  }

  Profile* profile() { return browser()->profile(); }

  PrefService* prefs() { return user_prefs::UserPrefs::Get(browser_context()); }

 private:
  MockConfigurationPolicyProvider provider_;
};

// Verify that brave_rewards::IsDisabledByPolicy works correctly based on the
// preference set by the policy.
IN_PROC_BROWSER_TEST_P(BraveRewardsPolicyTest, IsBraveRewardsDisabled) {
  EXPECT_TRUE(prefs()->FindPreference(brave_rewards::prefs::kDisabledByPolicy));
  if (IsBraveRewardsDisabledTest()) {
    EXPECT_TRUE(prefs()->GetBoolean(brave_rewards::prefs::kDisabledByPolicy));
    EXPECT_FALSE(brave_rewards::IsSupported(prefs()));
    EXPECT_FALSE(brave_rewards::IsSupportedForProfile(profile()));
  } else {
    EXPECT_FALSE(prefs()->GetBoolean(brave_rewards::prefs::kDisabledByPolicy));
    EXPECT_TRUE(brave_rewards::IsSupported(prefs()));
    EXPECT_TRUE(brave_rewards::IsSupportedForProfile(profile()));
  }
}

// Verify that Rewards and Ads services don't get created when Brave Rewards are
// disabled by policy.
IN_PROC_BROWSER_TEST_P(BraveRewardsPolicyTest, GetRewardsAndAdsServices) {
  if (IsBraveRewardsDisabledTest()) {
    EXPECT_EQ(brave_rewards::RewardsServiceFactory::GetForProfile(profile()),
              nullptr);
    EXPECT_EQ(brave_ads::AdsServiceFactory::GetForProfile(profile()), nullptr);
  } else {
    EXPECT_NE(brave_rewards::RewardsServiceFactory::GetForProfile(profile()),
              nullptr);
    EXPECT_NE(brave_ads::AdsServiceFactory::GetForProfile(profile()), nullptr);
  }
}

// Verify that Rewards menu item isn't enabled in the app menu when Brave
// Rewards are disabled by policy.
IN_PROC_BROWSER_TEST_P(BraveRewardsPolicyTest, AppMenuItemDisabled) {
  auto* command_controller = browser()->command_controller();
  if (IsBraveRewardsDisabledTest()) {
    EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_REWARDS));
  } else {
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_REWARDS));
  }
}

// Verify that brave://rewards and brave://rewards-internals pages aren't
// reachable when Brave Rewards are disabled by policy.
IN_PROC_BROWSER_TEST_P(BraveRewardsPolicyTest, RewardsPagesAccess) {
  for (const auto& url :
       {GURL("chrome://rewards"), GURL("chrome://rewards-internals")}) {
    SCOPED_TRACE(testing::Message() << "url=" << url);
    auto* rfh = ui_test_utils::NavigateToURL(browser(), url);
    EXPECT_TRUE(rfh);
    EXPECT_EQ(IsBraveRewardsDisabledTest(), rfh->IsErrorDocument());
  }
}

// Verify that Brave Rewards icon is not shown in the location bar when Brave
// Rewards are disabled by policy.
IN_PROC_BROWSER_TEST_P(BraveRewardsPolicyTest, RewardsIconIsHidden) {
  const auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  ASSERT_NE(browser_view, nullptr);
  const auto* brave_location_bar_view =
      static_cast<BraveLocationBarView*>(browser_view->GetLocationBarView());
  ASSERT_NE(brave_location_bar_view, nullptr);
  const auto* brave_actions = brave_location_bar_view->brave_actions_.get();
  ASSERT_NE(brave_actions, nullptr);
  EXPECT_TRUE(
      prefs()->GetBoolean(brave_rewards::prefs::kShowLocationBarButton));
  if (IsBraveRewardsDisabledTest()) {
    EXPECT_FALSE(brave_actions->rewards_action_btn_->GetVisible());
  } else {
    EXPECT_TRUE(brave_actions->rewards_action_btn_->GetVisible());
  }
}

INSTANTIATE_TEST_SUITE_P(
    BraveRewardsPolicyTest,
    BraveRewardsPolicyTest,
    ::testing::Bool(),
    [](const testing::TestParamInfo<BraveRewardsPolicyTest::ParamType>& info) {
      return absl::StrFormat("BraveRewards_%sByPolicy",
                             info.param ? "Disabled" : "NotDisabled");
    });

}  // namespace policy
