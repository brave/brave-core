/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser_command_controller.h"

#include <memory>
#include <optional>

#include "base/functional/callback_helpers.h"
#include "base/test/scoped_feature_list.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/skus/common/features.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/policy/configuration_policy_handler_list_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"
#include "components/sync/base/command_line_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/browser/utils.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/features.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

#if defined(TOOLKIT_VIEWS)
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_coordinator.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry_key.h"
#include "chrome/browser/ui/views/side_panel/side_panel_enums.h"
#include "chrome/browser/ui/views/side_panel/side_panel_test_utils.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#endif

class BraveBrowserCommandControllerTest : public InProcessBrowserTest {
 public:
  BraveBrowserCommandControllerTest() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    scoped_feature_list_.InitWithFeatures(
        {skus::features::kSkusFeature, brave_vpn::features::kBraveVPN}, {});
#endif
  }
  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    provider_.SetDefaultReturns(
        /*is_initialization_complete_return=*/true,
        /*is_first_policy_load_complete_return=*/true);
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);
  }

#if BUILDFLAG(ENABLE_AI_CHAT)
  void BlockAIChatByPolicy(bool value) {
    policy::PolicyMap policies;
    policies.Set(policy::key::kBraveAIChatEnabled,
                 policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_MACHINE,
                 policy::POLICY_SOURCE_PLATFORM, base::Value(!value), nullptr);
    provider_.UpdateChromePolicy(policies);
    EXPECT_EQ(ai_chat::IsAIChatEnabled(browser()->profile()->GetPrefs()),
              !value);
  }
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  void BlockVPNByPolicy(bool value) {
    policy::PolicyMap policies;
    policies.Set(policy::key::kBraveVPNDisabled, policy::POLICY_LEVEL_MANDATORY,
                 policy::POLICY_SCOPE_MACHINE, policy::POLICY_SOURCE_PLATFORM,
                 base::Value(value), nullptr);
    provider_.UpdateChromePolicy(policies);
    EXPECT_EQ(
        brave_vpn::IsBraveVPNDisabledByPolicy(browser()->profile()->GetPrefs()),
        value);
  }

  void SetPurchasedUserForBraveVPN(Browser* browser, bool purchased) {
    auto* service =
        brave_vpn::BraveVpnServiceFactory::GetForProfile(browser->profile());
    ASSERT_TRUE(!!service);
    auto target_state = purchased
                            ? brave_vpn::mojom::PurchasedState::PURCHASED
                            : brave_vpn::mojom::PurchasedState::NOT_PURCHASED;
    service->SetPurchasedState(skus::GetDefaultEnvironment(), target_state);
    // Call explicitely to update vpn commands status because mojo works in
    // async way.
    static_cast<chrome::BraveBrowserCommandController*>(
        browser->command_controller())
        ->OnPurchasedStateChanged(target_state, std::nullopt);
  }

  void CheckBraveVPNCommands(Browser* browser) {
    // Only IDC_BRAVE_VPN_MENU command is changed based on purchased state.
    auto* command_controller = browser->command_controller();
    SetPurchasedUserForBraveVPN(browser, false);
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_VPN_PANEL));
    EXPECT_TRUE(command_controller->IsCommandEnabled(
        IDC_TOGGLE_BRAVE_VPN_TOOLBAR_BUTTON));
    EXPECT_TRUE(
        command_controller->IsCommandEnabled(IDC_SEND_BRAVE_VPN_FEEDBACK));
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_ABOUT_BRAVE_VPN));
    EXPECT_TRUE(
        command_controller->IsCommandEnabled(IDC_MANAGE_BRAVE_VPN_PLAN));

    EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_BRAVE_VPN_MENU));
    EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_TOGGLE_BRAVE_VPN));

    SetPurchasedUserForBraveVPN(browser, true);
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_VPN_PANEL));
    EXPECT_TRUE(command_controller->IsCommandEnabled(
        IDC_TOGGLE_BRAVE_VPN_TOOLBAR_BUTTON));
    EXPECT_TRUE(
        command_controller->IsCommandEnabled(IDC_SEND_BRAVE_VPN_FEEDBACK));
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_ABOUT_BRAVE_VPN));
    EXPECT_TRUE(
        command_controller->IsCommandEnabled(IDC_MANAGE_BRAVE_VPN_PLAN));

    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_BRAVE_VPN_MENU));
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_TOGGLE_BRAVE_VPN));
  }

  void CheckBraveVPNCommandsDisabledByPolicy(Browser* browser) {
    auto* command_controller = browser->command_controller();
    SetPurchasedUserForBraveVPN(browser, false);
    EXPECT_FALSE(
        command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_VPN_PANEL));
    EXPECT_FALSE(command_controller->IsCommandEnabled(
        IDC_TOGGLE_BRAVE_VPN_TOOLBAR_BUTTON));
    EXPECT_FALSE(
        command_controller->IsCommandEnabled(IDC_SEND_BRAVE_VPN_FEEDBACK));
    EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_ABOUT_BRAVE_VPN));
    EXPECT_FALSE(
        command_controller->IsCommandEnabled(IDC_MANAGE_BRAVE_VPN_PLAN));

    EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_BRAVE_VPN_MENU));
    EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_TOGGLE_BRAVE_VPN));

    SetPurchasedUserForBraveVPN(browser, true);
    EXPECT_FALSE(
        command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_VPN_PANEL));
    EXPECT_FALSE(command_controller->IsCommandEnabled(
        IDC_TOGGLE_BRAVE_VPN_TOOLBAR_BUTTON));
    EXPECT_FALSE(
        command_controller->IsCommandEnabled(IDC_SEND_BRAVE_VPN_FEEDBACK));
    EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_ABOUT_BRAVE_VPN));
    EXPECT_FALSE(
        command_controller->IsCommandEnabled(IDC_MANAGE_BRAVE_VPN_PLAN));

    EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_BRAVE_VPN_MENU));
    EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_TOGGLE_BRAVE_VPN));
  }
#endif

 private:
  policy::MockConfigurationPolicyProvider provider_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Regular window
IN_PROC_BROWSER_TEST_F(BraveBrowserCommandControllerTest,
                       BraveCommandsEnableTest) {
  // Test normal browser's brave commands status.
  auto* command_controller = browser()->command_controller();
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_REWARDS));

#if BUILDFLAG(ENABLE_TOR)
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_NEW_TOR_CONNECTION_FOR_SITE));
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_NEW_OFFTHERECORD_WINDOW_TOR));
#else
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_NEW_TOR_CONNECTION_FOR_SITE));
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_NEW_OFFTHERECORD_WINDOW_TOR));
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  EXPECT_FALSE(
      brave_vpn::IsBraveVPNDisabledByPolicy(browser()->profile()->GetPrefs()));
  CheckBraveVPNCommands(browser());
  BlockVPNByPolicy(true);
  CheckBraveVPNCommandsDisabledByPolicy(browser());
  BlockVPNByPolicy(false);
  CheckBraveVPNCommands(browser());
#endif

  if (syncer::IsSyncAllowedByFlag()) {
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));
  } else {
    EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));
  }

  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WALLET));

  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_ADD_NEW_PROFILE));
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_OPEN_GUEST_PROFILE));
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER));

  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_TOGGLE_SIDEBAR));
}

// Create private browser and test its brave commands status.
IN_PROC_BROWSER_TEST_F(BraveBrowserCommandControllerTest,
                       BraveCommandsEnableTestPrivateWindow) {
  auto* private_browser = CreateIncognitoBrowser();
  auto* command_controller = private_browser->command_controller();
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_REWARDS));

#if BUILDFLAG(ENABLE_TOR)
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_NEW_TOR_CONNECTION_FOR_SITE));
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_NEW_OFFTHERECORD_WINDOW_TOR));
#endif

  if (syncer::IsSyncAllowedByFlag()) {
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));
  } else {
    EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));
  }

  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WALLET));
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_ADD_NEW_PROFILE));
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_OPEN_GUEST_PROFILE));
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER));
#if BUILDFLAG(ENABLE_AI_CHAT)
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_TOGGLE_AI_CHAT));
#endif
}

// Create guest browser and test its brave commands status.
IN_PROC_BROWSER_TEST_F(BraveBrowserCommandControllerTest,
                       BraveCommandsEnableTestGuestWindow) {
  ui_test_utils::BrowserChangeObserver browser_creation_observer(
      nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);
  profiles::SwitchToGuestProfile(base::DoNothing());

  Browser* guest_browser = browser_creation_observer.Wait();
  DCHECK(guest_browser);
  EXPECT_TRUE(guest_browser->profile()->IsGuestSession());
  auto* command_controller = guest_browser->command_controller();
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_REWARDS));

#if BUILDFLAG(ENABLE_TOR)
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_NEW_TOR_CONNECTION_FOR_SITE));
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_NEW_OFFTHERECORD_WINDOW_TOR));
#endif

  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));

  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WALLET));
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_ADD_NEW_PROFILE));
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_OPEN_GUEST_PROFILE));
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER));
#if BUILDFLAG(ENABLE_AI_CHAT)
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_TOGGLE_AI_CHAT));
#endif
}

// Launch tor window and check its command status.
#if BUILDFLAG(ENABLE_TOR)
IN_PROC_BROWSER_TEST_F(BraveBrowserCommandControllerTest,
                       BraveCommandsEnableTestPrivateTorWindow) {
  ui_test_utils::BrowserChangeObserver tor_browser_creation_observer(
      nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);
  brave::NewOffTheRecordWindowTor(browser());
  Browser* tor_browser = tor_browser_creation_observer.Wait();
  DCHECK(tor_browser);
  EXPECT_TRUE(tor_browser->profile()->IsTor());
  auto* command_controller = tor_browser->command_controller();
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_REWARDS));

  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_NEW_TOR_CONNECTION_FOR_SITE));
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_NEW_OFFTHERECORD_WINDOW_TOR));

  if (syncer::IsSyncAllowedByFlag()) {
    EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));
  } else {
    EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_SYNC));
  }

  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WALLET));
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_ADD_NEW_PROFILE));
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_OPEN_GUEST_PROFILE));
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER));
#if BUILDFLAG(ENABLE_AI_CHAT)
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_TOGGLE_AI_CHAT));
#endif

  // Check tor commands when tor is disabled.
  TorProfileServiceFactory::SetTorDisabled(true);
  command_controller = browser()->command_controller();
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_NEW_TOR_CONNECTION_FOR_SITE));
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_NEW_OFFTHERECORD_WINDOW_TOR));
}
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
IN_PROC_BROWSER_TEST_F(BraveBrowserCommandControllerTest,
                       ToggleAIChat_ControlledByPolicy) {
  auto* command_controller = browser()->command_controller();
  // Sanity check policy is enabled by default
  EXPECT_TRUE(ai_chat::IsAIChatEnabled(browser()->profile()->GetPrefs()));
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_TOGGLE_AI_CHAT));
  // When AI Chat is blocked by policy, the commands should not be available
  BlockAIChatByPolicy(true);
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_TOGGLE_AI_CHAT));
  // When AI Chat is unblocked by policy, the commands should become available
  BlockAIChatByPolicy(false);
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_TOGGLE_AI_CHAT));
}
#endif

IN_PROC_BROWSER_TEST_F(BraveBrowserCommandControllerTest,
                       BraveCommandsCloseTabsToLeft) {
  auto* command_controller = browser()->command_controller();

  // Browser starts with a single about:blank page. Shouldn't be able to close
  // tabs to the left because there's nothing to the left.
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_WINDOW_CLOSE_TABS_TO_LEFT));

  GURL t1 = GURL("https://example.com");
  chrome::AddTabAt(browser(), t1, 1, true);
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_WINDOW_CLOSE_TABS_TO_LEFT));
  EXPECT_EQ(2, browser()->tab_strip_model()->count());

  // Open a 3rd tab in the background, so t1 is still focused
  GURL t2 = GURL("https://foo.com");
  chrome::AddTabAt(browser(), t2, 2, false);
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_WINDOW_CLOSE_TABS_TO_LEFT));
  EXPECT_EQ(3, browser()->tab_strip_model()->count());

  command_controller->ExecuteCommand(IDC_WINDOW_CLOSE_TABS_TO_LEFT);
  EXPECT_EQ(2, browser()->tab_strip_model()->count());

  EXPECT_EQ(t1,
            browser()->tab_strip_model()->GetWebContentsAt(0)->GetVisibleURL());
  EXPECT_EQ(t2,
            browser()->tab_strip_model()->GetWebContentsAt(1)->GetVisibleURL());
}

IN_PROC_BROWSER_TEST_F(BraveBrowserCommandControllerTest,
                       BraveCommandsCloseUnpinnedTabs) {
  auto* tsm = browser()->tab_strip_model();
  auto* command_controller = browser()->command_controller();
  // Should start with one open tab which isn't pinned.
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_WINDOW_CLOSE_UNPINNED_TABS));

  tsm->SetTabPinned(0, true);
  EXPECT_FALSE(
      command_controller->IsCommandEnabled(IDC_WINDOW_CLOSE_UNPINNED_TABS));

  GURL unpinned("https://example.com");
  chrome::AddTabAt(browser(), unpinned, 1, false);
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_WINDOW_CLOSE_UNPINNED_TABS));

  GURL will_pin("https://will.pin");
  chrome::AddTabAt(browser(), will_pin, 2, true);
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_WINDOW_CLOSE_UNPINNED_TABS));

  tsm->SetTabPinned(2, true);
  EXPECT_TRUE(
      command_controller->IsCommandEnabled(IDC_WINDOW_CLOSE_UNPINNED_TABS));

  EXPECT_EQ(3, tsm->count());

  command_controller->ExecuteCommand(IDC_WINDOW_CLOSE_UNPINNED_TABS);
  EXPECT_EQ(2, tsm->count());

  for (int i = 0; i < tsm->count(); ++i) {
    EXPECT_TRUE(tsm->IsTabPinned(i));
  }

  EXPECT_EQ(GURL("about:blank"), tsm->GetWebContentsAt(0)->GetVisibleURL());
  EXPECT_EQ(will_pin, tsm->GetWebContentsAt(1)->GetVisibleURL());
}

IN_PROC_BROWSER_TEST_F(BraveBrowserCommandControllerTest,
                       BraveCommandsAddAllToNewGroup) {
  auto* command_controller = browser()->command_controller();
  auto* tsm = browser()->tab_strip_model();

  GURL url("https://example.com");
  chrome::AddTabAt(browser(), url, 1, false);
  chrome::AddTabAt(browser(), url, 2, false);
  chrome::AddTabAt(browser(), url, 3, false);
  chrome::AddTabAt(browser(), url, 4, false);

  EXPECT_EQ(5, tsm->count());
  command_controller->ExecuteCommand(IDC_WINDOW_ADD_ALL_TABS_TO_NEW_GROUP);
  EXPECT_EQ(5, tsm->count());

  // All tabs should have the same group.
  auto group = tsm->GetTabGroupForTab(0);
  EXPECT_TRUE(group.has_value());

  for (int i = 1; i < tsm->count(); ++i) {
    EXPECT_EQ(group, tsm->GetTabGroupForTab(i));
  }
}

#if BUILDFLAG(ENABLE_AI_CHAT) && defined(TOOLKIT_VIEWS)
IN_PROC_BROWSER_TEST_F(BraveBrowserCommandControllerTest,
                       BraveCommandsToggleAIChat) {
  SidePanelEntryKey ai_chat_key =
      SidePanelEntry::Key(SidePanelEntryId::kChatUI);
  SidePanelCoordinator* side_panel_coordinator =
      SidePanelUtil::GetSidePanelCoordinatorForBrowser(browser());
  auto side_panel_waiter = SidePanelWaiter(side_panel_coordinator);

  // initially no panel is showing
  EXPECT_FALSE(side_panel_coordinator->IsSidePanelEntryShowing(ai_chat_key));
  EXPECT_FALSE(side_panel_coordinator->IsSidePanelShowing());
  // after command, ai chat panel is showing
  browser()->command_controller()->ExecuteCommand(IDC_TOGGLE_AI_CHAT);
  EXPECT_TRUE(side_panel_coordinator->IsSidePanelShowing());
  EXPECT_TRUE(side_panel_coordinator->IsSidePanelEntryShowing(ai_chat_key));
  // after command again, no panel is showing
  browser()->command_controller()->ExecuteCommand(IDC_TOGGLE_AI_CHAT);
  side_panel_waiter.WaitForSidePanelClose();
  EXPECT_FALSE(side_panel_coordinator->IsSidePanelEntryShowing(ai_chat_key));
  EXPECT_FALSE(side_panel_coordinator->IsSidePanelShowing());

  // open a different side panel
  SidePanelEntryKey bookmarks_key =
      SidePanelEntry::Key(SidePanelEntryId::kBookmarks);
  side_panel_coordinator->Toggle(bookmarks_key,
                                 SidePanelOpenTrigger::kToolbarButton);
  // after command, ai chat panel is showing
  browser()->command_controller()->ExecuteCommand(IDC_TOGGLE_AI_CHAT);
  EXPECT_TRUE(side_panel_coordinator->IsSidePanelShowing());
  EXPECT_TRUE(side_panel_coordinator->IsSidePanelEntryShowing(ai_chat_key));
  // after command again, no panel is showing
  browser()->command_controller()->ExecuteCommand(IDC_TOGGLE_AI_CHAT);
  side_panel_waiter.WaitForSidePanelClose();
  EXPECT_FALSE(side_panel_coordinator->IsSidePanelEntryShowing(ai_chat_key));
  EXPECT_FALSE(side_panel_coordinator->IsSidePanelShowing());
}
#endif
