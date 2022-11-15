/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/app/brave_command_ids.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/common/rewards_util.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "url/gurl.h"

namespace brave_rewards {

class BraveRewardsOFACTest : public InProcessBrowserTest {
 public:
  BraveRewardsOFACTest() = default;
  ~BraveRewardsOFACTest() override = default;

  content::WebContents* web_contents() const {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::BrowserContext* browser_context() {
    return web_contents()->GetBrowserContext();
  }

  Profile* profile() { return browser()->profile(); }

  PrefService* prefs() { return user_prefs::UserPrefs::Get(browser_context()); }
};

// Verify that brave_rewards::IsSupported works correctly based on the locale.
IN_PROC_BROWSER_TEST_F(BraveRewardsOFACTest, IsBraveRewardsDisabled) {
  {
    const brave_l10n::test::ScopedDefaultLocale locale("en_CA");  // "Canada"
    EXPECT_FALSE(IsUnsupportedRegion());
    EXPECT_TRUE(brave_rewards::IsSupported(prefs()));
    EXPECT_TRUE(brave_rewards::IsSupported(
        prefs(), brave_rewards::IsSupportedOptions::kSkipRegionCheck));
    EXPECT_TRUE(brave_rewards::IsSupportedForProfile(profile()));
    EXPECT_TRUE(brave_rewards::IsSupportedForProfile(
        profile(), brave_rewards::IsSupportedOptions::kSkipRegionCheck));
  }
  {
    const brave_l10n::test::ScopedDefaultLocale locale("es_CU");  // "Cuba"
    EXPECT_TRUE(IsUnsupportedRegion());
    EXPECT_FALSE(brave_rewards::IsSupported(prefs()));
    EXPECT_TRUE(brave_rewards::IsSupported(
        prefs(), brave_rewards::IsSupportedOptions::kSkipRegionCheck));
    EXPECT_FALSE(brave_rewards::IsSupportedForProfile(profile()));
    EXPECT_TRUE(brave_rewards::IsSupportedForProfile(
        profile(), brave_rewards::IsSupportedOptions::kSkipRegionCheck));
  }
}

// Verify that Rewards and Ads services don't get created when in an OFAC
// sanctioned region.
IN_PROC_BROWSER_TEST_F(BraveRewardsOFACTest, GetRewardsAndAdsServices) {
  {
    const brave_l10n::test::ScopedDefaultLocale locale("en_CA");  // "Canada"
    EXPECT_NE(brave_rewards::RewardsServiceFactory::GetForProfile(profile()),
              nullptr);
    EXPECT_NE(brave_ads::AdsServiceFactory::GetForProfile(profile()), nullptr);
  }

  {
    const brave_l10n::test::ScopedDefaultLocale locale("es_CU");  // "Cuba"
    EXPECT_EQ(brave_rewards::RewardsServiceFactory::GetForProfile(profile()),
              nullptr);
    EXPECT_EQ(brave_ads::AdsServiceFactory::GetForProfile(profile()), nullptr);
  }
}

// Verify that Rewards menu item is enabled in the app menu even when in an OFAC
// sanctioned region.
IN_PROC_BROWSER_TEST_F(BraveRewardsOFACTest, AppMenuItemEnabled) {
  auto* command_controller = browser()->command_controller();
  {
    const brave_l10n::test::ScopedDefaultLocale locale("en_CA");  // "Canada"
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_REWARDS));
  }
  {
    const brave_l10n::test::ScopedDefaultLocale locale("es_CU");  // "Cuba"
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_REWARDS));
  }
}

// Verify that brave://rewards page is reachable even when  in an OFAC
// sanctioned region.
IN_PROC_BROWSER_TEST_F(BraveRewardsOFACTest, RewardsPagesAccess) {
  const GURL url("chrome://rewards");

  {
    const brave_l10n::test::ScopedDefaultLocale locale("en_CA");  // "Canada"
    auto* rfh = ui_test_utils::NavigateToURL(browser(), url);
    EXPECT_TRUE(rfh);
    EXPECT_FALSE(rfh->IsErrorDocument());
  }

  {
    const brave_l10n::test::ScopedDefaultLocale locale("es_CU");  // "Cuba"
    auto* rfh = ui_test_utils::NavigateToURL(browser(), url);
    EXPECT_TRUE(rfh);
    EXPECT_FALSE(rfh->IsErrorDocument());
  }
}

}  // namespace brave_rewards
