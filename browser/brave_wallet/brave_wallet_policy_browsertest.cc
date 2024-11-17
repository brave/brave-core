/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/strings/stringprintf.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/pref_names.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_window.h"
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
#include "extensions/common/constants.h"
#include "url/gurl.h"

namespace policy {

class BraveWalletPolicyTest : public InProcessBrowserTest,
                              public ::testing::WithParamInterface<bool> {
 public:
  BraveWalletPolicyTest() = default;
  ~BraveWalletPolicyTest() override = default;

  void SetUpInProcessBrowserTestFixture() override {
    EXPECT_CALL(provider_, IsInitializationComplete(testing::_))
        .WillRepeatedly(testing::Return(true));
    BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);
    PolicyMap policies;
    policies.Set(key::kBraveWalletDisabled, POLICY_LEVEL_MANDATORY,
                 POLICY_SCOPE_USER, POLICY_SOURCE_PLATFORM,
                 base::Value(IsBraveWalletDisabledTest()), nullptr);
    provider_.UpdateChromePolicy(policies);
  }

  bool IsBraveWalletDisabledTest() { return GetParam(); }

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

// Verify that brave_wallet::IsDisabledByPolicy works correctly based on the
// preference set by the policy.
IN_PROC_BROWSER_TEST_P(BraveWalletPolicyTest, IsBraveWalletDisabled) {
  EXPECT_TRUE(prefs()->FindPreference(brave_wallet::prefs::kDisabledByPolicy));
  if (IsBraveWalletDisabledTest()) {
    EXPECT_TRUE(prefs()->GetBoolean(brave_wallet::prefs::kDisabledByPolicy));
    EXPECT_FALSE(brave_wallet::IsAllowed(prefs()));
    EXPECT_FALSE(brave_wallet::IsAllowedForContext(profile()));
  } else {
    EXPECT_FALSE(prefs()->GetBoolean(brave_wallet::prefs::kDisabledByPolicy));
    EXPECT_TRUE(brave_wallet::IsAllowed(prefs()));
    EXPECT_TRUE(brave_wallet::IsAllowedForContext(profile()));

    auto* profile = browser()->profile();
    auto* incognito_profile = CreateIncognitoBrowser(profile)->profile();
    ui_test_utils::BrowserChangeObserver browser_creation_observer(
        nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);
    profiles::SwitchToGuestProfile();
    Browser* guest_browser = browser_creation_observer.Wait();
    DCHECK(guest_browser);
    auto* guest_profile = guest_browser->profile();
    ASSERT_TRUE(guest_profile->IsGuestSession());

    ui_test_utils::BrowserChangeObserver tor_browser_creation_observer(
        nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);
    brave::NewOffTheRecordWindowTor(browser());
    Browser* tor_browser = tor_browser_creation_observer.Wait();
    DCHECK(tor_browser);
    auto* tor_profile = tor_browser->profile();
    ASSERT_TRUE(tor_profile->IsTor());

    // By default the wallet should not be allowed for private, guest, or tor.
    EXPECT_FALSE(brave_wallet::IsAllowedForContext(incognito_profile));
    EXPECT_FALSE(brave_wallet::IsAllowedForContext(tor_profile));
    EXPECT_FALSE(brave_wallet::IsAllowedForContext(guest_profile));

    // Setting pref should allow it for incognito, but not for guest, or tor.
    prefs()->SetBoolean(kBraveWalletPrivateWindowsEnabled, true);
    EXPECT_TRUE(brave_wallet::IsAllowedForContext(incognito_profile));
    EXPECT_FALSE(brave_wallet::IsAllowedForContext(tor_profile));
    EXPECT_FALSE(brave_wallet::IsAllowedForContext(guest_profile));
  }
}

// Verify that Wallet service doesn't get created when Brave Wallet is
// disabled by policy.
IN_PROC_BROWSER_TEST_P(BraveWalletPolicyTest, GetWalletService) {
  if (IsBraveWalletDisabledTest()) {
    EXPECT_EQ(brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
                  profile()),
              nullptr);
  } else {
    EXPECT_NE(brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
                  profile()),
              nullptr);
  }
}

// Verify that Wallet menu item isn't enabled in the app menu when Brave
// Wallet is disabled by policy.
IN_PROC_BROWSER_TEST_P(BraveWalletPolicyTest, AppMenuItemDisabled) {
  auto* command_controller = browser()->command_controller();
  if (IsBraveWalletDisabledTest()) {
    EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WALLET));
  } else {
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WALLET));
  }
}

// Verify that brave://wallet page isn't reachable when Brave Wallet is
// disabled by policy.
IN_PROC_BROWSER_TEST_P(BraveWalletPolicyTest, WalletPageAccess) {
  const GURL url("chrome://wallet");
  auto* rfh = ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_TRUE(rfh);
  EXPECT_EQ(IsBraveWalletDisabledTest(), rfh->IsErrorDocument());
}

// Verify that the wallet item is not shown in the sidebar when Brave Wallet is
// disabled by policy.
IN_PROC_BROWSER_TEST_P(BraveWalletPolicyTest, WalletInSidebar) {
  BraveBrowser* brave_browser = static_cast<BraveBrowser*>(browser());
  sidebar::SidebarController* controller = brave_browser->sidebar_controller();
  sidebar::SidebarModel* model = controller->model();

  const auto items = model->GetAllSidebarItems();
  EXPECT_LT(0UL, items.size());

  const auto iter = base::ranges::find_if(items, [](const auto& i) {
    return (i.built_in_item_type ==
            sidebar::SidebarItem::BuiltInItemType::kWallet);
  });

  if (IsBraveWalletDisabledTest()) {
    EXPECT_TRUE(iter == items.end());
  } else {
    EXPECT_FALSE(iter == items.end());
  }
}

INSTANTIATE_TEST_SUITE_P(
    BraveWalletPolicyTest,
    BraveWalletPolicyTest,
    ::testing::Bool(),
    [](const testing::TestParamInfo<BraveWalletPolicyTest::ParamType>& info) {
      return absl::StrFormat("BraveWallet_%sByPolicy",
                             info.param ? "Disabled" : "NotDisabled");
    });

}  // namespace policy
